#include "tip_engine/hooks.h"

#include <cstdint>
#include <cmath>
#include <cstring>
#include <rex/ui/imgui_dialog.h>
#include "imgui.h"

#include <rex/cvar.h>
#include "tip_engine/rex_macros.h"
#include <rex/graphics/flags.h>
#include <rex/ppc/types.h>
#include <rex/system/kernel_state.h>
#include "tip_engine/Log.h"
#include "tip_engine/D3DTypes.h"
#include "Overlays/DebugInfo.h"
#include "Webcam.h"

#include "rex_macros.h"
#include <fstream>
#include <chrono>
#include <thread>
#include "tip_engine/Types/CommonTypes.h"

inline float to_byteswapped_float(float f) {
    uint32_t i = std::byteswap(*reinterpret_cast<uint32_t*>(&f));
    return *reinterpret_cast<float*>(&i);
}

inline double to_byteswapped_double(double d) {
    uint64_t i = std::byteswap(*reinterpret_cast<uint64_t*>(&d));
    return *reinterpret_cast<double*>(&i);
}

REXCVAR_DEFINE_BOOL(rgb_cursor, false, "_Trouble in Paradise", "Enables the Gursor");
REXCVAR_DEFINE_BOOL(lock_fps, false, "_Trouble in Paradise", "Lock to 30 FPS");
REXCVAR_DEFINE_BOOL(DisableMainDraw, false, "_Trouble in Paradise", "Disables the Main Draw Pass");
REXCVAR_DEFINE_BOOL(DisableUIDraw, false, "_Trouble in Paradise", "Disables the UI Draw Pass");

REXCVAR_DEFINE_BOOL(UseAspectRatioFromConfig, false, "_Trouble in Paradise", "Use Aspect Ratio from config");
REXCVAR_DEFINE_DOUBLE(AspectRatio, 1.7777778f, "_Trouble in Paradise", "Aspect Ratio");


REXCVAR_DEFINE_COLOR(ambientColor, 0x000000FF, "_Trouble in Paradise", "Controls the ambient color of the scene");
REXCVAR_DEFINE_COLOR(ambientModelColor, 0x000000FF, "_Trouble in Paradise", "Controls the ambient color of the models in the scene");
REXCVAR_DEFINE_COLOR(directionalColor, 0xFFFFFFFF, "_Trouble in Paradise", "Controls the color of the directional light in the scene");
REXCVAR_DEFINE_COLOR(fogColor, 0x000000FF, "_Trouble in Paradise", "Controls the color of the fog in the scene");
REXCVAR_DEFINE_DOUBLE(fogOpacity, 1.0f, "_Trouble in Paradise", "Controls the opacity of the fog in the scene");
REXCVAR_DEFINE_DOUBLE(blueShiftScalar, 0.0f, "_Trouble in Paradise", "Controls the intensity of the blue shift effect in the scene");
REXCVAR_DEFINE_BOOL(cubeFogEnabled, false, "_Trouble in Paradise", "Enables cube fog in the scene");

REXCVAR_DEFINE_INT32(maxCPU, 60, "_Trouble in Paradise", "Limits the cpu FPS to the specified value (0 for unlimited)");
REXCVAR_DEFINE_INT32(maxGPU, 60, "_Trouble in Paradise", "Limits the gpu FPS to the specified value (0 for unlimited)");



REX_PPC_EXTERN_IMPORT(camMainGetPos_821F07E0);

float * camMainGetPos_821F07E0_Hook(float *result){
  return rex ::GuestToHostFunction<float *>(__imp__rex_camMainGetPos_821F07E0, result);
}

REX_PPC_HOOK(camMainGetPos_821F07E0);

//double rex_camMainGetAspectRatio_821F0730()
REX_PPC_EXTERN_IMPORT(camMainGetAspectRatio_821F0730);

float camMainGetAspectRatio_821F0730_Hook() {
  if(REXCVAR_GET(UseAspectRatioFromConfig)) {
    float aspectRatio = static_cast<float>(REXCVAR_GET(AspectRatio));
    DebugLogFloat("Aspect Ratio", aspectRatio);
    return aspectRatio;
  }
  float aspectRatio = rex::GuestToHostFunction<float>(__imp__rex_camMainGetAspectRatio_821F0730);
  DebugLogFloat("Aspect Ratio", static_cast<float>(aspectRatio));
  return aspectRatio;
}

REX_PPC_HOOK(camMainGetAspectRatio_821F0730);


void CPU_fps_hook() {
  auto fpshook = fpsManager.GetCreateCounter("CPU");
  fpshook->Tick();
}

void GPU_fps_hook() {
  auto fpshook = fpsManager.GetCreateCounter("GPU");
  fpshook->Tick();
}

PPC_EXTERN_IMPORT(__imp__rex_appMainTickPreDraw_821C91C0);
PPC_EXTERN_IMPORT(__imp__rex_appMainDraw_821C8E78);
void MainLoop_hook() {
  using clock = std::chrono::steady_clock;

  auto last_cpu_tick = clock::now();
  auto last_gpu_draw = clock::now();
  auto cpu_accumulator = clock::duration::zero();

  int Run = 1;
  while (Run) {
    auto now = clock::now();

    // CPU tick rate from cvar (0 = unlimited)
    int32_t cpuLimit = REXCVAR_GET(maxCPU);
    if (cpuLimit > 0) {
      auto cpu_interval = std::chrono::duration_cast<clock::duration>(
          std::chrono::duration<double>(1.0 / cpuLimit));
      cpu_accumulator += now - last_cpu_tick;
      last_cpu_tick = now;

      while (cpu_accumulator >= cpu_interval) {
        Run = rex ::GuestToHostFunction<int>(__imp__rex_appMainTickPreDraw_821C91C0);
        TriggerReadCallback();
        cpu_accumulator -= cpu_interval;
        if (!Run) break;
      }
    } else {
      Run = rex ::GuestToHostFunction<int>(__imp__rex_appMainTickPreDraw_821C91C0);
      TriggerReadCallback();
    }

    // GPU draw rate from cvar (0 = unlimited)
    if (Run) {
      int32_t gpuLimit = REXCVAR_GET(maxGPU);
      if (gpuLimit > 0) {
        auto gpu_interval = std::chrono::duration_cast<clock::duration>(
            std::chrono::duration<double>(1.0 / gpuLimit));
        auto gpu_elapsed = now - last_gpu_draw;
        if (gpu_elapsed >= gpu_interval) {
          rex ::GuestToHostFunction<void>(__imp__rex_appMainDraw_821C8E78);
          last_gpu_draw = now;
        }
      } else {
        rex ::GuestToHostFunction<void>(__imp__rex_appMainDraw_821C8E78);
      }
    }

    if (cpuLimit > 0 || REXCVAR_GET(maxGPU) > 0) {
      std::this_thread::yield();
    }
  }
}

void vsync_hook(PPCRegister& r10) {
  if(!REXCVAR_GET(lock_fps)) {
    r10.u32 = 0; // Force vsync off
  }
}

bool Space1_hook() {
  return true; // Always branch to loc_824DC830
}

bool Space2_hook() {
  return true; // Always branch to loc_824DD5C0
}

void Space3_hook(PPCRegister& r3) {
  r3.u32 = 1; // Set r3 to 1
}

void Space4_hook(PPCRegister& r6) {
  r6.u32 = 1; // Set r6 to 1
}

void Space5_hook(PPCRegister& r3) {
  r3.u32 = 0; // Set r3 to 0
}

bool Space6_hook() {
  return true; // Always branch to loc_824DDA84
}

bool Space7_hook() {
  return true; // Always branch to loc_824DDA84
}

uint32_t HI(const std::string& hexColor) {
    if (hexColor.size() != 9 || hexColor[0] != '#') {
        return 0xFFFFFFFF; // Default to white if invalid format
    }
    uint32_t r = std::stoul(hexColor.substr(1, 2), nullptr, 16);
    uint32_t g = std::stoul(hexColor.substr(3, 2), nullptr, 16);
    uint32_t b = std::stoul(hexColor.substr(5, 2), nullptr, 16);
    uint32_t a = std::stoul(hexColor.substr(7, 2), nullptr, 16);
    return (r << 24) | (g << 16) | (b << 8) | a;
}

// Classify a host-endian RGBA color as red or yellow
static bool isRedColor(uint32_t rgba) {
    uint8_t r = (rgba >> 24) & 0xFF;
    uint8_t g = (rgba >> 16) & 0xFF;
    uint8_t b = (rgba >>  8) & 0xFF;
    return r > 180 && g < 80 && b < 80;
}

static bool isYellowColor(uint32_t rgba) {
    uint8_t r = (rgba >> 24) & 0xFF;
    uint8_t g = (rgba >> 16) & 0xFF;
    uint8_t b = (rgba >>  8) & 0xFF;
    return r > 180 && g > 180 && b < 80;
}

// Triangle tracking arrays (8 red, 8 yellow)
static constexpr int MAX_TRIANGLES = 8;
static uint32_t redPtrs[MAX_TRIANGLES] = {};
static uint32_t yellowPtrs[MAX_TRIANGLES] = {};
static uint32_t redLastKnown[MAX_TRIANGLES] = {};   // value at offset 0 when captured
static uint32_t yellowLastKnown[MAX_TRIANGLES] = {};
static int redCount = 0;
static int yellowCount = 0;

void CursorColor_hook(PPCRegister& r31, PPCRegister& r27)
{
  if(!REXCVAR_GET(rgb_cursor)) {
    return;
  }

    // Resolve color pointer (guest memory is big-endian)
    uint32_t* colorPtr = reinterpret_cast<uint32_t*>(0x100000000ull + r31.u32 + 24);
    uint32_t rawColor = std::byteswap(*colorPtr); // host-endian RGBA

    // Read a stable field (offset 0) for sanity checking — we never modify this
    uint32_t baseValue = *reinterpret_cast<uint32_t*>(0x100000000ull + r31.u32);

    // Sanity check: if any stored entry's base value changed, memory was reallocated
    bool invalidated = false;
    for (int i = 0; i < redCount && !invalidated; i++) {
        uint32_t cur = *reinterpret_cast<uint32_t*>(0x100000000ull + redPtrs[i]);
        if (cur != redLastKnown[i]) invalidated = true;
    }
    for (int i = 0; i < yellowCount && !invalidated; i++) {
        uint32_t cur = *reinterpret_cast<uint32_t*>(0x100000000ull + yellowPtrs[i]);
        if (cur != yellowLastKnown[i]) invalidated = true;
    }
    if (invalidated) {
        redCount = 0;
        yellowCount = 0;
        memset(redPtrs, 0, sizeof(redPtrs));
        memset(yellowPtrs, 0, sizeof(yellowPtrs));
        memset(redLastKnown, 0, sizeof(redLastKnown));
        memset(yellowLastKnown, 0, sizeof(yellowLastKnown));
    }

    // Check if this pointer is already tracked
    bool isTracked = false;
    for (int i = 0; i < redCount; i++) {
        if (redPtrs[i] == r31.u32) { isTracked = true; break; }
    }
    if (!isTracked) {
        for (int i = 0; i < yellowCount; i++) {
            if (yellowPtrs[i] == r31.u32) { isTracked = true; break; }
        }
    }

    // If not tracked yet, classify by original color and add
    if (!isTracked) {
        if (isRedColor(rawColor) && redCount < MAX_TRIANGLES) {
            redPtrs[redCount] = r31.u32;
            redLastKnown[redCount] = baseValue;
            redCount++;
            isTracked = true;
        } else if (isYellowColor(rawColor) && yellowCount < MAX_TRIANGLES) {
            yellowPtrs[yellowCount] = r31.u32;
            yellowLastKnown[yellowCount] = baseValue;
            yellowCount++;
            isTracked = true;
        }
    }

    // Only apply rainbow to tracked (red/yellow) triangles
    if (!isTracked) return;

    // Determine if this is a red triangle (for hue offset)
    bool isRed = false;
    for (int i = 0; i < redCount; i++) {
        if (redPtrs[i] == r31.u32) { isRed = true; break; }
    }

    // Time-based hue cycling: full rainbow every 3 seconds
    auto now = std::chrono::steady_clock::now();
    static auto start = now;
    double elapsed = std::chrono::duration<double>(now - start).count();
    double hueD = fmod(elapsed * 120.0 + (isRed ? 180.0 : 0.0), 360.0); // red offset by 180°
    float hue = static_cast<float>(hueD);

    // HSV to RGB with full saturation and value
    float h = hue / 60.0f;
    int sector = static_cast<int>(h) % 6;
    float f = h - static_cast<int>(h);
    float q = 1.0f - f;

    float r, g, b;
    switch (sector) {
        case 0: r = 1; g = f; b = 0; break;
        case 1: r = q; g = 1; b = 0; break;
        case 2: r = 0; g = 1; b = f; break;
        case 3: r = 0; g = q; b = 1; break;
        case 4: r = f; g = 0; b = 1; break;
        default: r = 1; g = 0; b = q; break;
    }

    uint32_t ri = static_cast<uint32_t>(r * 255.0f);
    uint32_t gi = static_cast<uint32_t>(g * 255.0f);
    uint32_t bi = static_cast<uint32_t>(b * 255.0f);
    uint32_t rgba = (ri << 24) | (gi << 16) | (bi << 8) | 0xFF;

    // Apply cycling color, byte-swapped for PPC
    *colorPtr = std::byteswap(rgba);
}

bool skip_entityAvatarPinataSeedBigBrotherSaysYes_hook() {
  return true; // Always branch to loc_824DDA84
}

void one_hook(){
  REXCVAR_SET(d3d12_readback_resolve, true);
}

void two_hook(){
  REXCVAR_SET(d3d12_readback_resolve, false);
}

bool skipFirstDraw_hook(){
  return REXCVAR_GET(DisableMainDraw);
}

bool skipSecondDraw_hook(){
  return REXCVAR_GET(DisableUIDraw);
}

bool skiplighting_hook() {
  return false; // Always branch to loc_824DDA84
}

bool skiplightingTwo_hook() {
  return false; // Always branch to loc_824DDA84
}