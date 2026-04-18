#include "tip_engine/hooks.h"

#include <cstdint>
#include <cmath>
#include <cstring>
#include <rex/ui/imgui_dialog.h>
#include "Overlays/Fps.h"
#include "imgui.h"

#include <rex/cvar.h>
#include "tip_engine/rex_macros.h"
#include <rex/graphics/flags.h>
#include <rex/ppc/types.h>
#include <rex/system/kernel_state.h>
#include "tip_engine/Log.h"
#include "tip_engine/D3DTypes.h"
#include "Overlays/DebugInfo.h"

#include "rex_macros.h"
#include <fstream>
#include <mutex>
#include "tip_engine/Types/CommonTypes.h"
#include "Overlays/SpawnMenu.h"
#include "Overlays/BarcodeInjector.h"
#include "TextureTools.h"

inline float to_byteswapped_float(float f) {
    uint32_t i = std::byteswap(*reinterpret_cast<uint32_t*>(&f));
    return *reinterpret_cast<float*>(&i);
}

inline double to_byteswapped_double(double d) {
    uint64_t i = std::byteswap(*reinterpret_cast<uint64_t*>(&d));
    return *reinterpret_cast<double*>(&i);
}

REXCVAR_DEFINE_BOOL(show_fps_overlay, false, "_Trouble in Paradise", "Show FPS overlay");
REXCVAR_DEFINE_BOOL(rgb_cursor, false, "_Trouble in Paradise", "Enables the Gursor");
REXCVAR_DEFINE_BOOL(lock_fps, false, "_Trouble in Paradise", "Lock to 30 FPS");
REXCVAR_DEFINE_BOOL(show_fps, false, "_Trouble in Paradise", "Show FPS Overlay");
REXCVAR_DEFINE_BOOL(DisableMainDraw, false, "_Trouble in Paradise", "Disables the Main Draw Pass");
REXCVAR_DEFINE_BOOL(DisableUIDraw, false, "_Trouble in Paradise", "Disables the UI Draw Pass");

REXCVAR_DEFINE_BOOL(UseAspectRatioFromConfig, false, "_Trouble in Paradise", "Use Aspect Ratio from config");
REXCVAR_DEFINE_DOUBLE(AspectRatio, 1.7777778f, "_Trouble in Paradise", "Aspect Ratio");

REXCVAR_DEFINE_BOOL(SkipIntros, false, "_Trouble in Paradise", "Skip Intro Videos");

// ============================================================
// Piñata Vision Spawn System — PPC function imports
// ============================================================
PPC_EXTERN_IMPORT(sub_825885B0);   // Message dispatch 260 (variant/wildcard)
PPC_EXTERN_IMPORT(sub_82383538);   // Wildcard visual applicator (speciesData, traitValue)
PPC_EXTERN_IMPORT(sub_8258ACE8);   // Wildcard event registration (r3=eventType, r4=speciesTag, r5=data)
PPC_EXTERN_IMPORT(sub_8258AD48);   // Wildcard status setter (r3=eventType, r4=speciesTag, r5=flag)
PPC_EXTERN_IMPORT(sub_8258AC58);   // Game variable setter (r3=varID, r4=value)
PPC_EXTERN_IMPORT(sub_82575AB8);   // Entity creation dispatcher (r3=scene, r7=tagID)
PPC_EXTERN_IMPORT(sub_825A0818);   // Validate species tag (r3=tagID → r3=typeCode)
PPC_EXTERN_IMPORT(sub_825357B8);   // Encyclopedia species info lookup
PPC_EXTERN_IMPORT(sub_82546C50);   // Find piñata in garden by tag ID
PPC_EXTERN_IMPORT(sub_82559420);   // Apply dino color (r3=entity, r4=color 0-3)
PPC_EXTERN_IMPORT(sub_825597C0);   // Pre-color-change setup
PPC_EXTERN_IMPORT(rex_gardenMainGetGarden_824E10D8); // Get garden data
PPC_EXTERN_IMPORT(sub_82382328);   // Set variant change target (speciesData, variantIdx, flag)
PPC_EXTERN_IMPORT(sub_8242BE00);   // Reload textures for new variant
PPC_EXTERN_IMPORT(sub_8258ADC8);   // Game event trigger
PPC_EXTERN_IMPORT(sub_824145E8);   // Apply wildcard trait (breeding hook)
PPC_EXTERN_IMPORT(sub_82559AD8);   // Dragonache: set all 5 body parts (entity, teeth, mane, wings, tail, ridges)
PPC_EXTERN_IMPORT(sub_82559D88);   // Dragonache: set color from terrain type (entity, terrainType)
PPC_EXTERN_IMPORT(sub_82559908);   // Dragonache: set single body part (entity, partIndex 1-5, variant)
PPC_EXTERN_IMPORT(sub_82559E98);   // Dragonache: apply color (reads string from speciesData+2952, resolves texture)
PPC_EXTERN_IMPORT(sub_825A0878);   // Convert typeCode back to tagID
PPC_EXTERN_IMPORT(sub_8254C018);   // Asset ID builder (r3=tagID, r4=outputBuffer)
// Cursor functions — declared as regular PPC functions (not hooked, just called)
extern "C" void rex_cursorMainGetWorkspace_821F6840(PPCContext& ctx, uint8_t* base);
extern "C" void rex_cursorMainGetCursor_822CC598(PPCContext& ctx, uint8_t* base);

//REXCVAR_DEFINE_BOOL(Freecam, false, "_Trouble in Paradise", "Enables the Freecam");
//REXCVAR_DEFINE_DOUBLE(Freecam_X, 0.0f, "_Trouble in Paradise", "Freecam X Position");
//REXCVAR_DEFINE_DOUBLE(Freecam_Y, 0.0f, "_Trouble in Paradise", "Freecam Y Position");
//REXCVAR_DEFINE_DOUBLE(Freecam_Z, 0.0f, "_Trouble in Paradise", "Freecam Z Position");

//REXCVAR_DEFINE_BOOL(Custom_Viewport, false, "_Trouble in Paradise", "Use custom viewport");
//REXCVAR_DEFINE_INT32(Viewport_X, 0, "_Trouble in Paradise", "Custom viewport X");
//REXCVAR_DEFINE_INT32(Viewport_Y, 0, "_Trouble in Paradise", "Custom viewport Y");
//REXCVAR_DEFINE_INT32(Viewport_Width, 1920, "_Trouble in Paradise", "Custom viewport width");
//REXCVAR_DEFINE_INT32(Viewport_Height, 1080, "_Trouble in Paradise", "Custom viewport height");


//REXCVAR_DEFINE_COLOR(ambientColor, 0x000000FF, "_Trouble in Paradise", "Controls the ambient color of the scene");
//REXCVAR_DEFINE_COLOR(ambientModelColor, 0x000000FF, "_Trouble in Paradise", "Controls the ambient color of the models in the scene");
//REXCVAR_DEFINE_COLOR(directionalColor, 0xFFFFFFFF, "_Trouble in Paradise", "Controls the color of the directional light in the scene");
//REXCVAR_DEFINE_COLOR(fogColor, 0x000000FF, "_Trouble in Paradise", "Controls the color of the fog in the scene");
//REXCVAR_DEFINE_DOUBLE(fogOpacity, 1.0f, "_Trouble in Paradise", "Controls the opacity of the fog in the scene");
//REXCVAR_DEFINE_DOUBLE(blueShiftScalar, 0.0f, "_Trouble in Paradise", "Controls the intensity of the blue shift effect in the scene");
//REXCVAR_DEFINE_BOOL(cubeFogEnabled, false, "_Trouble in Paradise", "Enables cube fog in the scene");

//REXCVAR_DEFINE_INT32(maxCPU, 60, "_Trouble in Paradise", "Limits the cpu FPS to the specified value (0 for unlimited)");
//REXCVAR_DEFINE_INT32(maxGPU, 60, "_Trouble in Paradise", "Limits the gpu FPS to the specified value (0 for unlimited)");


/*
REX_PPC_EXTERN_IMPORT(camMainGetPos_821F07E0);

float * camMainGetPos_821F07E0_Hook(float *result){
  if(REXCVAR_GET(Freecam)) {
    result[0] = to_byteswapped_float(static_cast<float>(REXCVAR_GET(Freecam_X)));
    result[1] = to_byteswapped_float(static_cast<float>(REXCVAR_GET(Freecam_Y)));
    result[2] = to_byteswapped_float(static_cast<float>(REXCVAR_GET(Freecam_Z)));

    //.data:82C34E98 Me_36.virtCam
    camVirt_s* virtCam = reinterpret_cast<camVirt_s*>(0x100000000ull + 0x82C34E98);
    virtCam->pos.x = result[0];
    virtCam->pos.y = result[1];
    virtCam->pos.z = result[2];

    camMainWorkspace_s* workspace = reinterpret_cast<camMainWorkspace_s*>(0x100000000ull + 0x82C34D48);

    if(REXCVAR_GET(Custom_Viewport)) {
      workspace->FullScreenViewport.X = std::byteswap(static_cast<uint32_t>(REXCVAR_GET(Viewport_X)));
      workspace->FullScreenViewport.Y = std::byteswap(static_cast<uint32_t>(REXCVAR_GET(Viewport_Y)));
      workspace->FullScreenViewport.Width = std::byteswap(static_cast<uint32_t>(REXCVAR_GET(Viewport_Width)));
      workspace->FullScreenViewport.Height = std::byteswap(static_cast<uint32_t>(REXCVAR_GET(Viewport_Height)));
      workspace->outputViewportIsDirty = 1;
    }

    //DebugLogInt32("CamPtr", uint32_t(workspace->virtCam));
    //camVirt_s* virtCam1 = reinterpret_cast<camVirt_s*>(0x100000000ull + workspace->virtCam);
    //DebugLogFloat("CamPosX", to_byteswapped_float(virtCam1->pos.x));
    //DebugLogFloat("CamPosY", to_byteswapped_float(virtCam1->pos.y));
    //DebugLogFloat("CamPosZ", to_byteswapped_float(virtCam1->pos.z));


    return result;
  }else{
    //.data:82C34E98 Me_36.virtCam
    //camVirt_s* virtCam = reinterpret_cast<camVirt_s*>(0x100000000ull + 0x82C34E98);
    //*(float *)(virtCam + 8) = result[0];
    //*(float *)(virtCam + 12) = result[1];
    //*(float *)(virtCam + 16) = result[2];
    return rex ::GuestToHostFunction<float *>(__imp__rex_camMainGetPos_821F07E0, result);
  }
  return rex ::GuestToHostFunction<float *>(__imp__rex_camMainGetPos_821F07E0, result);
}
REX_PPC_HOOK(camMainGetPos_821F07E0);
*/

REX_PPC_EXTERN_IMPORT(camMainGetAspectRatio_821F0730);
float camMainGetAspectRatio_821F0730_Hook() {
  float aspectRatio;
  //if(REXCVAR_GET(UseAspectRatioFromConfig)) {
  //  aspectRatio = static_cast<float>(REXCVAR_GET(AspectRatio));
  //}else{
    aspectRatio = rex::GuestToHostFunction<float>(__imp__rex_camMainGetAspectRatio_821F0730);
  //}
  return aspectRatio;
} REX_PPC_HOOK(camMainGetAspectRatio_821F0730);

//supportFrustumConstructClippingFrustum_8228BE08
// NOTE: Upstream aspect ratio hooks (supportFrustumConstructClippingFrustum, camMainPostDrawLetterbox,
// mlMtxPerspective, meUpdateOutputViewport) are disabled on this branch because the generated code
// was built with SDK 0.7.2 which doesn't have these function stubs. They'll work once we regenerate
// with the newer rexglue CLI. The AspectRatio_hook (midasm) still works for basic AR override.

/*
REX_PPC_EXTERN_IMPORT(supportFrustumConstructClippingFrustum_8228BE08);
void supportFrustumConstructClippingFrustum_8228BE08_Hook(double a1,double a2,double a3,double a4,int a5,int a6,int a7,int a8,float *a9,float *a10) {
    float customAR = static_cast<float>(REXCVAR_GET(AspectRatio));
    if(REXCVAR_GET(UseAspectRatioFromConfig)) {
      a2 = customAR;
    }
    rex::GuestToHostFunction<void>(__imp__rex_supportFrustumConstructClippingFrustum_8228BE08, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
}
REX_PPC_HOOK(supportFrustumConstructClippingFrustum_8228BE08);


REX_PPC_EXTERN_IMPORT(camMainPostDrawLetterbox_821F0910);
int camMainPostDrawLetterbox_821F0910_Hook(){
  return 1;
}
REX_PPC_HOOK(camMainPostDrawLetterbox_821F0910);

REX_PPC_EXTERN_IMPORT(mlMtxPerspective_82247408);
void mlMtxPerspective_82247408_Hook(float* outMtx, float fov, float aspectRatio, float nearZ, float farZ) {
    float customAR = aspectRatio;
    if(REXCVAR_GET(UseAspectRatioFromConfig)) {
      customAR = static_cast<float>(REXCVAR_GET(AspectRatio));
    }
    rex::GuestToHostFunction<void>(__imp__rex_mlMtxPerspective_82247408, outMtx, fov, customAR, nearZ, farZ);
}
REX_PPC_HOOK(mlMtxPerspective_82247408);


REX_PPC_EXTERN_IMPORT(meUpdateOutputViewport_821F0F30);
int meUpdateOutputViewport_821F0F30_Hook() {
    int result = rex::GuestToHostFunction<int>(__imp__rex_meUpdateOutputViewport_821F0F30);
    if(REXCVAR_GET(UseAspectRatioFromConfig)) {
        float customAR = static_cast<float>(REXCVAR_GET(AspectRatio));
        float* aspectRatioPtr = reinterpret_cast<float*>(0x100000000ull + 0x82C34F00);
        *aspectRatioPtr = to_byteswapped_float(customAR);
    }
    return result;
}
REX_PPC_HOOK(meUpdateOutputViewport_821F0F30);
*/

void AspectRatio_hook(PPCRegister& r3) {
  if(REXCVAR_GET(UseAspectRatioFromConfig)) {
    //r3.u32 + 0x399C | *(float *)&r3+0x399C = AspectRatio;
    float customAR = static_cast<float>(REXCVAR_GET(AspectRatio));
    float* aspectRatioPtr = reinterpret_cast<float*>(0x100000000ull + r3.u32 + 0x399C);
    *aspectRatioPtr = to_byteswapped_float(customAR);
  }
}

void CPU_fps_hook() {
  fpsManager.showFPS = REXCVAR_GET(show_fps);
  auto fpshook = fpsManager.GetCreateCounter("CPU");
  fpshook->Tick();

  // --- Piñata Vision Spawn System per-frame processing ---

  // Toggle spawn menu with F2
  if (ImGui::IsKeyPressed(ImGuiKey_F2)) {
      g_SpawnMenuOpen = !g_SpawnMenuOpen;
  }

  // (cursor target reading removed — crashes in midasm hook context)

  // Process deferred texture dump (entity needs time to fully initialize)
  if (g_DeferredDump.framesRemaining > 0) {
      g_DeferredDump.framesRemaining--;
      if (g_DeferredDump.framesRemaining == 0 && g_DeferredDump.entity != 0) {
          Log("Deferred entity dump triggered for 0x" + std::to_string(g_DeferredDump.entity), 3);
          g_DeferredDump.entity = 0;
      }
  }

  // Process deferred variant application (wait N frames for entity init)
  if (g_DeferredVariant.framesRemaining > 0) {
      g_DeferredVariant.framesRemaining--;
      if (g_DeferredVariant.framesRemaining == 0 && g_DeferredVariant.entity != 0) {
          g_DeferredVariantChange.entity = g_DeferredVariant.entity;
          g_DeferredVariantChange.variantIndex = g_DeferredVariant.variantIndex;
          g_DeferredVariantChange.isTrick = false;
          g_DeferredVariantChange.pending = true;
          g_DeferredVariant.entity = 0;
          Log("Deferred variant queued for entity", 3);
      }
  }
}

void GPU_fps_hook() {
  auto fpshook = fpsManager.GetCreateCounter("GPU");
  fpshook->Tick();
}

/*
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
*/

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

/* 12001 */
struct gardenBudgetUnit_sl
{
  unsigned int virtualMemory;
  unsigned int physicalMemory;
  unsigned int dualShadowBuffering;
  unsigned int cubeShadowBuffering;
  unsigned int regularShadowBuffering;
  unsigned int diggableSurfacePreDraw;
  unsigned int mainPassOpaque;
  unsigned int mainPassTransparent;
};

/* 12002 */
struct gardenBudgetClassUnit_sl
{
  unsigned int classLimit[45];
};


void tagUnitsBudget_hook() {
  // Set the budget for each unit class to 9999 (0x270F in hex)
  uint32_t& limit = *reinterpret_cast<uint32_t*>(0x100000000 + 0x83A5A8EC + 56);
  uint32_t& limit2 = *reinterpret_cast<uint32_t*>(0x100000000 + 0x83A5A8EC + 56 + 4);
  //83A5A8A8

  limit = std::byteswap(999999);
  limit2 = std::byteswap(999999);

  for (int i = 0; i < 2; i++) {
    //*reinterpret_cast<uint32_t*>(classLimitPtr + i * 4) = std::byteswap(0);
  }
  DebugLogInt32("BudgetHook", limit);


}

void tagClassUnitsBudget_hook(PPCRegister& r3) {
  //r3.u32 = the ptr to the gardenBudgetClassUnit_sl struct
  if(r3.u32 == 0) {
    return;
  }
  uint32_t* budgetPtr = reinterpret_cast<uint32_t*>(0x100000000 + r3.u32);
  for (int i = 0; i < 2260; i++) {
    budgetPtr[i] = std::byteswap(999);
  }
}

bool meUpdateOccupancyLevels_hook(PPCRegister& fp0){
  uint32_t& limit1 = *reinterpret_cast<uint32_t*>(0x100000000 + 0x83A5A8A8 + 64);
  uint32_t& limit2 = *reinterpret_cast<uint32_t*>(0x100000000 + 0x83A5A8A8 + 96);
  uint32_t& limit3 = *reinterpret_cast<uint32_t*>(0x100000000 + 0x83A5A8A8 + 132);
  uint32_t& limit4 = *reinterpret_cast<uint32_t*>(0x100000000 + 0x83A5A8A8 + 100);
  
  limit1 = std::byteswap(999999);
  limit2 = std::byteswap(999999);
  limit3 = std::byteswap(999999);
  limit4 = std::byteswap(999999);

  gardenBudgetUnit_sl* limits1 = reinterpret_cast<gardenBudgetUnit_sl*>(0x100000000 + 0x83A5A8A8 + 0);
  limits1->virtualMemory = std::byteswap(1282527612);
  limits1->physicalMemory = std::byteswap(1282527612);
  gardenBudgetUnit_sl* limits2 = reinterpret_cast<gardenBudgetUnit_sl*>(0x100000000 + 0x83A5A8A8 + 32);
  limits2->virtualMemory = std::byteswap(1282527612);
  limits2->physicalMemory = std::byteswap(1282527612);
  gardenBudgetUnit_sl* limits3 = reinterpret_cast<gardenBudgetUnit_sl*>(0x100000000 + 0x83A5A8A8 + 64);
  gardenBudgetUnit_sl* limits4 = reinterpret_cast<gardenBudgetUnit_sl*>(0x100000000 + 0x83A5A8A8 + 128);
  limits4->virtualMemory = std::byteswap(1282527612);
  limits4->physicalMemory = std::byteswap(1282527612);
  uint32_t& limit7 = *reinterpret_cast<uint32_t*>(0x100000000ull + 0x83A5A8A8ull + 28028);
  limit7 = 0x0000803F;
  return true;
}

bool skip_entityAvatarPinataSeedBigBrotherSaysYes_hook() {
  return true; // Always branch to loc_824DDA84
}

void one_hook(){
#ifndef __linux__
  REXCVAR_SET(d3d12_readback_resolve, true);
#endif
}

void two_hook(){
#ifndef __linux__
  REXCVAR_SET(d3d12_readback_resolve, false);
#endif
}

bool skipFirstDraw_hook(){
  return REXCVAR_GET(DisableMainDraw);
}

bool skipSecondDraw_hook(){
  return REXCVAR_GET(DisableUIDraw);
}

void skipRenderState0_hook(PPCRegister& r10){
  return;
  /*
  if(REXCVAR_GET(SkipShadowPass_One)){
    r10.u32 = 0; // Set shadow count to 0 to skip shadow pass
  }else{
    r10.u32 = 1;
  }
    */
}

void skipRenderState1_hook(PPCRegister& r9){
  return;
  /*
  if(REXCVAR_GET(SkipShadowPass_Two)){
    r9.u32 = 0; // Set shadow count to 0 to skip shadow pass
  }else{
    r9.u32 = 1;
  }
    */
}

void skipRenderState2_hook(PPCRegister& r9){
  return;
  /*
  if(REXCVAR_GET(SkipShadowPass_Three)){
    r9.u32 = 0; // Set shadow count to 0 to skip shadow pass
  }else{
    r9.u32 = 1;
  }
    */
}

void skipRenderState3_hook(PPCRegister& r9){
  return;
  /*
  if(REXCVAR_GET(SkipOpaquePass)){
    r9.u32 = 0; // Set shadow count to 0 to skip shadow pass
  }else{
    r9.u32 = 1;
  }
    */
}

void skipRenderState4_hook(PPCRegister& r9){
  return;
  /*
  if(REXCVAR_GET(SkipAlphaPass)){
    r9.u32 = 0; // Set shadow count to 0 to skip shadow pass
  }else{
    r9.u32 = 1;
  }
    */
}

void skipRenderState5_hook(PPCRegister& r9){
  return;
  /*
  if(REXCVAR_GET(SkipPostProcessPass)){
    r9.u32 = 0; // Set shadow count to 0 to skip shadow pass
  }else{
    r9.u32 = 1;
  }
    */
}

bool skiplighting_hook() {
  return false; // Always branch to loc_824DDA84
}

bool skiplightingTwo_hook() {
  return false; // Always branch to loc_824DDA84
}

bool SkipIntroVideos_hook() {
  return REXCVAR_GET(SkipIntros);
}

bool SkipIntroVideosTwo_hook() {
  return REXCVAR_GET(SkipIntros);
}

// Compatibility stubs for old midasm hooks (main's generated code still references these)
void fps_hook() {
  CPU_fps_hook(); // Delegate to new split hook
}
bool PresentParams_hook(PPCRegister& r31) {
  return false; // No longer needed — upstream removed this hook
}
void PresentParams2_hook(PPCRegister& r3) {
  // No longer needed — upstream removed this hook
}

// ============================================================
// Piñata Vision Spawn System — Texture & Asset Hooks
// ============================================================


// === Phase 3: Texture init hook ===
PPC_EXTERN_IMPORT(__imp__rex_dbTextureInitTexture);
std::ofstream g_TextureLog;
bool g_TextureLogging = false;

// === Phase 3: File system hook for asset override ===
PPC_EXTERN_IMPORT(__imp__rex_fsOpenFile);
std::ofstream g_FileLog;
bool g_FileLogging = false;

extern "C" PPC_FUNC(rex_fsOpenFile) {
    // r3 = file struct, r4 = filename ptr (PPC string)
    uint32_t filenamePtr = ctx.r4.u32;
    uint8_t* mb = rex::Runtime::instance()->memory()->virtual_membase();

    if (g_FileLogging && g_FileLog.is_open() && filenamePtr > 0x40000000) {
        const char* filename = (const char*)(mb + filenamePtr);
        if (strnlen(filename, 200) > 3 && strnlen(filename, 200) < 200) {
            g_FileLog << filename << "\n";
            g_FileLog.flush();
        }
    }

    // Call original
    __imp__rex_fsOpenFile(ctx, base);
}

// === PHASE 3: Fixed Texture Init Hook ===
// Previous crash was from memory probing. Fix: call original FIRST, then read only valid fields.
// This is the same hook SolarCookies uses on the PC port for texture packs.

// Asset name registry: captures names from rex_assetIdPrintf
PPC_EXTERN_IMPORT(__imp__rex_assetIdPrintf);
static std::string g_LastAssetName;
static std::mutex g_AssetNameMutex;

extern "C" PPC_FUNC(rex_assetIdPrintf) {
    // Save r3 (output buffer address) before calling original
    uint32_t outputBuf = ctx.r3.u32;

    // Call original
    __imp__rex_assetIdPrintf(ctx, base);

    // Read the formatted string from the output buffer
    if (outputBuf >= 0x40000000 && outputBuf < 0x8E000000) {
        uint8_t* mb = rex::Runtime::instance()->memory()->virtual_membase();
        const char* name = (const char*)(mb + outputBuf);
        size_t len = strnlen(name, 100);
        if (len > 10 && len < 90) {
            if (strncmp(name, "aid_texture", 11) == 0) {
                std::lock_guard<std::mutex> lock(g_AssetNameMutex);
                g_LastAssetName = name;
            }
        }
    }
}

// Asset lookup hook: maps asset ID pointers to their resolved data pointers
// sub_821D1C10 takes an asset ID (r3) and returns the data pointer
// We capture the input asset ID name and map it to the returned pointer
PPC_EXTERN_IMPORT(__imp__sub_821D1C10);
static std::unordered_map<uint32_t, std::string> g_TextureAddrToName;

extern "C" PPC_FUNC(sub_821D1C10) {
    // r3 = asset ID struct pointer — the name string is at offset 0
    uint32_t assetIdAddr = ctx.r3.u32;
    std::string name;

    if (assetIdAddr >= 0x40000000 && assetIdAddr < 0x8E000000) {
        uint8_t* mb = rex::Runtime::instance()->memory()->virtual_membase();
        const char* s = (const char*)(mb + assetIdAddr);
        if (s[0] == 'a' && s[1] == 'i' && s[2] == 'd' && s[3] == '_') {
            name = std::string(s, strnlen(s, 90));
        }
    }

    // Call original
    __imp__sub_821D1C10(ctx, base);

    // Map the returned pointer to the asset name
    if (!name.empty() && ctx.r3.u32 != 0) {
        std::lock_guard<std::mutex> lock(g_AssetNameMutex);
        g_TextureAddrToName[ctx.r3.u32] = name;
    }
}

// Texture init hook — safe version (calls original first)
extern "C" PPC_FUNC(rex_dbTextureInitTexture) {
    uint32_t texAddr = ctx.r3.u32;

    // Name comes from g_TextureAddrToName map (populated by sub_821D1C10 hook)
    // or falls back to g_LastAssetName from rex_assetIdPrintf

    // Call original FIRST — let it fully initialize the texture
    __imp__rex_dbTextureInitTexture(ctx, base);

    // Now the dbTexture_s is fully valid. Read ONLY known fields.
    if ((g_TextureLogging || TextureTools::g_CaptureEnabled) && texAddr >= 0x40000000) {
        uint8_t* mb = rex::Runtime::instance()->memory()->virtual_membase();

        // Read dbTexture_s fields at known offsets
        uint32_t format = std::byteswap(*(uint32_t*)(mb + texAddr + 0));
        uint16_t width = std::byteswap(*(uint16_t*)(mb + texAddr + 8));
        uint16_t height = std::byteswap(*(uint16_t*)(mb + texAddr + 10));
        uint32_t imgData = std::byteswap(*(uint32_t*)(mb + texAddr + 20));
        uint32_t imgSize = std::byteswap(*(uint32_t*)(mb + texAddr + 24));

        // Get the asset name: first check the address map, then fall back to last captured
        std::string assetName;
        {
            std::lock_guard<std::mutex> lock(g_AssetNameMutex);
            auto it = g_TextureAddrToName.find(texAddr);
            if (it != g_TextureAddrToName.end()) {
                assetName = it->second;
            } else {
                assetName = g_LastAssetName;
            }
        }

        static const char* fmtNames[] = {
            "UNK", "DXT1", "DXT3", "DXT5", "A8R8G8B8", "X8R8G8B8",
            "LIN_A8R8G8B8", "LIN_X8R8G8B8", "L8", "A8L8", "R5G6B5",
            "A4R4G4B4", "DXN", "DXT3A", "G8R8"
        };
        const char* fmtName = (format < 15) ? fmtNames[format] : "???";

        // Log to file if logging enabled
        if (g_TextureLogging && g_TextureLog.is_open()) {
            char buf[512];
            snprintf(buf, 512, "TEX 0x%08X %4dx%-4d %-10s %8d bytes  data=0x%08X  %s\n",
                texAddr, width, height, fmtName, imgSize, imgData, assetName.c_str());
            g_TextureLog << buf;
            g_TextureLog.flush();
        }

        // Capture texture for dump/override if capture enabled
        if (TextureTools::g_CaptureEnabled && imgData > 0 && imgSize > 0) {
            TextureTools::CapturedTexture cap;
            cap.ppcAddr = texAddr;
            cap.format = format;
            cap.width = width;
            cap.height = height;
            cap.imageDataAddr = imgData;
            cap.dataSize = imgSize;
            cap.assetName = assetName;
            TextureTools::g_CapturedTextures.push_back(cap);
        }
    }
}

// Keep old disabled version for reference
extern "C" void rex_dbTextureInitTexture_OLD_DISABLED(PPCContext& ctx, uint8_t* base) {
    uint32_t texAddr = ctx.r3.u32;

    // Log texture init if logging is enabled
    if (g_TextureLogging && g_TextureLog.is_open()) {
        uint8_t* mb = rex::Runtime::instance()->memory()->virtual_membase();
        char buf[512];

        // Dump the first 64 bytes of the dbTexture_s struct to understand its layout
        snprintf(buf, 512, "TextureInit: PPC 0x%08X | ", texAddr);
        g_TextureLog << buf;

        // Safe read: only read forward from the texture address
        if (texAddr >= 0x40000000 && texAddr < 0x8E000000) {
            // Dump first 32 bytes as hex
            for (int i = 0; i < 32; i++) {
                snprintf(buf, 512, "%02X", *(mb + texAddr + i));
                g_TextureLog << buf;
                if (i % 4 == 3) g_TextureLog << " ";
            }
            g_TextureLog << " | ";

            // Check each 4-byte value as potential pointer to string
            for (int off = 0; off < 64; off += 4) {
                uint32_t val = std::byteswap(*(uint32_t*)(mb + texAddr + off));
                if (val >= 0x82000000 && val < 0x8E000000) {
                    const char* s = (const char*)(mb + val);
                    if (s[0] >= 'a' && s[0] <= 'z' && s[1] >= 'a' && strnlen(s, 80) > 3) {
                        snprintf(buf, 512, "[+%d=\"%.60s\"] ", off, s);
                        g_TextureLog << buf;
                    }
                }
            }
        }
        g_TextureLog << "\n";
        g_TextureLog.flush();
    }

    // Call original
    __imp__rex_dbTextureInitTexture(ctx, base);
}
// (PPC_EXTERN_IMPORTs for spawn system declared at top of file)

void wildcardHatchCheck(PPCContext& ctx, uint8_t* base) {
    static const uint32_t traitValues[] = {10, 20, 21};

    // Log every call
    {
        std::ofstream hlog("C:/Users/Administrator/Downloads/wildcard_hook_log.txt", std::ios::app);
        hlog << "wildcardHatchCheck fired! r3=0x" << std::hex << ctx.r3.u32
             << " r4=" << std::dec << ctx.r4.u32
             << " ForceWildcard=" << g_ForceWildcard
             << " AutoThird=" << g_AutoThirdWildcard
             << " has=[" << g_HasTrait[0] << "," << g_HasTrait[1] << "," << g_HasTrait[2] << "]"
             << " trait=" << g_ForcedWildcardTrait << std::endl;
        hlog.close();
    }

    if (g_AutoThirdWildcard) {
        // Count how many traits the player has
        int haveCount = 0;
        int missingIdx = -1;
        for (int i = 0; i < 3; i++) {
            if (g_HasTrait[i]) haveCount++;
            else missingIdx = i;
        }

        if (haveCount >= 2 && missingIdx >= 0) {
            // Two traits present — force the missing third
            uint32_t missingTrait = traitValues[missingIdx];
            Log("WILDCARD AUTO-THIRD: Forcing trait " + std::to_string(missingIdx + 1) +
                " (value=" + std::to_string(missingTrait) + ") — the missing one!", 3);
            ctx.r4.u64 = missingTrait;
            sub_824145E8(ctx, base);
            g_HasTrait[missingIdx] = true; // now we have all three
            return;
        }
        // Less than 2 traits — fall through to normal force wildcard
    }

    if (g_ForceWildcard) {
        uint32_t traitVal = traitValues[g_ForcedWildcardTrait % 3];
        Log("WILDCARD: Forcing trait " + std::to_string(g_ForcedWildcardTrait) +
            " (value=" + std::to_string(traitVal) + ") on hatch!", 3);
        ctx.r4.u64 = traitVal;
        sub_824145E8(ctx, base);

        // Track this trait for Auto Third
        g_HasTrait[g_ForcedWildcardTrait % 3] = true;
    }
}

static bool g_PVControllerInitAttempted = false;

// Override gardenMainGetGardenScene — call original, then spawn if pending
PPC_EXTERN_IMPORT(__imp__rex_gardenMainGetGardenScene_824E1120);

extern "C" PPC_FUNC(rex_gardenMainGetGardenScene_824E1120) {
    // Call the original implementation
    __imp__rex_gardenMainGetGardenScene_824E1120(ctx, base);

    // Process barcode injection if pending
    if (g_BarcodeInject.pending) {
        g_BarcodeInject.pending = false;
        uint32_t gs = ctx.r3.u32;
        if (gs == 0) { Log("Barcode inject: not in garden!", 5); }
        else {
            auto decoded = PVDecode::decode(g_BarcodeInject.hexString);
            PPCContext saveInj = ctx;

            // Log scan header (append mode — one file per session)
            {
                std::ofstream plog("C:/Users/Administrator/Downloads/placetag_log.txt", std::ios::app);
                plog << std::endl << "========== BARCODE SCAN ==========" << std::endl;
                plog << "hex: " << g_BarcodeInject.hexString << std::endl;
                plog << "decoded: " << PVDecode::formatCommands(decoded) << std::endl;
                plog.close();
            }

            // Pre-scan commands to find variant/wildcard values BEFORE PlaceTag
            uint32_t barcodeVariant = 0;
            uint32_t barcodeWildcard = 0;
            for (auto& pre : decoded.commands) {
                if (pre.type == PVDecode::CMD_VARIANT) barcodeVariant = pre.value;
                if (pre.type == PVDecode::CMD_WILDCARD) barcodeWildcard = pre.value;
            }

            for (auto& cmd : decoded.commands) {
                switch (cmd.type) {
                case PVDecode::CMD_PLACE_TAG: {
                    // Log everything to file for debugging
                    {
                        std::ofstream plog("C:/Users/Administrator/Downloads/placetag_log.txt", std::ios::app);
                        plog << std::endl << "=== PlaceTag ===" << std::endl;
                        plog << "tagID: " << cmd.value << std::endl;
                        plog << "barcodeVariant: " << barcodeVariant << std::endl;
                        plog << "barcodeWildcard: " << barcodeWildcard << std::endl;
                        plog << "gardenScene: 0x" << std::hex << gs << std::endl;
                        plog << "All commands: " << PVDecode::formatCommands(decoded) << std::endl;

                        // Validate species
                        ctx = saveInj;
                        ctx.r3.u64 = cmd.value;
                        sub_825A0818(ctx, base);
                        uint32_t typeCode = ctx.r3.u32;
                        plog << "sub_825A0818 typeCode: " << std::dec << typeCode << std::endl;
                        plog.flush();

                        // Accessory item IDs (> 1660) — TEMPORARILY SKIPPED.
                        // Previous experiment called sub_82575AB8 with r3=0 which crashed.
                        // The real accessory spawn path needs more research. For now, log
                        // and skip so compound spawns like Master Chief still land the base
                        // pinata and subsequent Variant/Name commands.
                        if (cmd.value > 1660) {
                            plog << "Accessory PlaceTag: item_id=" << cmd.value << " — SKIPPED (spawn path not yet implemented)" << std::endl;
                            plog.close();
                            Log("PV PlaceTag: accessory " + std::to_string(cmd.value) + " skipped (not implemented)", 3);
                            ctx = saveInj;
                            break;
                        }

                        if (typeCode > 44) {
                            plog << "INVALID species — skipping spawn" << std::endl;
                            plog.close();
                            Log("PV PlaceTag: invalid species " + std::to_string(cmd.value), 5);
                            ctx = saveInj;
                            break;
                        }

                        plog << "Calling sub_82575AB8 with r7=" << cmd.value << " r9=0..." << std::endl;
                        plog.flush();
                        plog.close();
                    }

                    ctx = saveInj;
                    ctx.r3.u64 = gs;
                    ctx.r4.u64 = 0;
                    ctx.r5.u64 = 0;
                    ctx.r6.u64 = 0;
                    ctx.r7.u64 = cmd.value;
                    ctx.r8.u64 = 0;
                    ctx.r9.u64 = 0;
                    ctx.f1.f64 = 1.0;
                    ctx.f2.f64 = 0.0;
                    sub_82575AB8(ctx, base);
                    g_LastSpawnedEntity = ctx.r3.u32;
                    if (g_LastSpawnedEntity != 0)
                        Log("PV PlaceTag: entity 0x" + std::to_string(g_LastSpawnedEntity), 3);
                    else
                        Log("PV PlaceTag: spawn failed", 5);
                    break;
                }
                case PVDecode::CMD_SPARSE: {
                    Log("PV Sparse: type=" + std::to_string(cmd.sparseType) +
                        " data=" + std::to_string(cmd.sparseData), 3);
                    if (cmd.sparseType == 0x13) {
                        // Dino color change — replicate EXACT game code from loc_8264A438
                        uint8_t colorVal = static_cast<uint8_t>(cmd.sparseData);
                        if (colorVal <= 3) {
                            uint8_t* membase = rex::Runtime::instance()->memory()->virtual_membase();

                            // Step 1: Get garden (exact replica of game code)
                            ctx = saveInj;
                            rex_gardenMainGetGarden_824E10D8(ctx, base);
                            uint32_t gardenPtr = ctx.r3.u32;

                            if (gardenPtr != 0) {
                                // lwz r3,0(r3) — dereference garden to get data
                                uint32_t gardenData = std::byteswap(*(uint32_t*)(membase + gardenPtr));

                                // Step 2: Find Choclodocus — replicate exact game lis/addi math
                                // lis r8,-32241 → r8.s64 = -2112946176; addi r5,r8,-18592
                                // lis r10,-32242 → r10.s64 = -2113011712; addi r9,r10,-24436
                                // lis r11,-32234 → r11.s64 = -2112487424; lfs f1,r11-14556
                                int64_t r8val = -2112946176LL;
                                int64_t r10val = -2113011712LL;
                                int64_t r11val = -2112487424LL;
                                uint32_t r5addr = static_cast<uint32_t>(r8val + (-18592));
                                uint32_t r9addr = static_cast<uint32_t>(r10val + (-24436));
                                uint32_t f1addr = static_cast<uint32_t>(r11val + (-14556));

                                ctx = saveInj;
                                ctx.r3.u64 = gardenData;
                                ctx.r4.u64 = 0;
                                ctx.r5.u64 = r5addr;
                                ctx.r6.u64 = 29;          // Choclodocus tag
                                ctx.r8.u64 = 0;
                                ctx.r9.u64 = r9addr;
                                // Load f1 float from PPC memory
                                uint32_t f1raw = std::byteswap(*(uint32_t*)(membase + f1addr));
                                float f1val;
                                memcpy(&f1val, &f1raw, 4);
                                ctx.f1.f64 = double(f1val);

                                sub_82546C50(ctx, base);
                                uint32_t dinoEntity = ctx.r3.u32;

                                // Write debug to file
                                {
                                    std::ofstream dbg("C:/Users/Administrator/Downloads/dino_color_debug.txt");
                                    if (dbg.is_open()) {
                                        char buf[512];
                                        snprintf(buf, 512, "gardenPtr=0x%08X gardenData=0x%08X dinoEntity=0x%08X color=%d\nr5=0x%08X r9=0x%08X f1addr=0x%08X f1val=%f",
                                            gardenPtr, gardenData, dinoEntity, colorVal,
                                            r5addr, r9addr, f1addr, f1val);
                                        dbg << buf << std::endl;
                                        dbg.close();
                                    }
                                }

                                if (dinoEntity != 0) {
                                    // Step 3: Pre-color setup
                                    ctx = saveInj;
                                    ctx.r3.u64 = dinoEntity;
                                    sub_825597C0(ctx, base);

                                    // Step 4: Apply color!
                                    ctx = saveInj;
                                    ctx.r3.u64 = dinoEntity;
                                    ctx.r4.u64 = colorVal;
                                    sub_82559420(ctx, base);

                                    Log("DINO COLOR APPLIED! color=" + std::to_string(colorVal), 3);
                                } else {
                                    Log("PV Sparse: no Choclodocus found in garden!", 5);
                                }
                            } else {
                                Log("PV Sparse: no garden!", 5);
                            }
                        }
                    }
                    if (g_LastSpawnedEntity != 0) {
                        ctx = saveInj;
                        ctx.r3.u64 = g_LastSpawnedEntity;
                        ctx.r4.u64 = cmd.sparseData;
                        sub_825885B0(ctx, base);
                        Log("PV Sparse: msg 260 sent", 3);
                    } else {
                        Log("PV Sparse: no entity!", 5);
                    }
                    break;
                }
                case PVDecode::CMD_VARIANT: {
                    Log("PV Variant: " + std::to_string(cmd.value), 3);
                    if (g_LastSpawnedEntity != 0) {
                        ctx = saveInj;
                        ctx.r3.u64 = g_LastSpawnedEntity;
                        ctx.r4.u64 = cmd.value;
                        sub_825885B0(ctx, base);
                    }
                    break;
                }
                case PVDecode::CMD_WILDCARD: {
                    Log("PV Wildcard: trait=" + std::to_string(cmd.value), 3);
                    if (g_LastSpawnedEntity != 0 && cmd.value > 0) {
                        // Full wildcard apply — replicates egg hatch sequence
                        uint32_t speciesData = PPC_LOAD_U32(g_LastSpawnedEntity + 2416);
                        if (speciesData != 0) {
                            static const uint32_t wildcardTraitValues[] = { 10, 20, 21 };
                            uint32_t traitVal = wildcardTraitValues[(cmd.value - 1) % 3];

                            // Register wildcard event + status
                            uint32_t pvTag = 0; // get tag from last PlaceTag
                            for (auto& c2 : decoded.commands) {
                                if (c2.type == PVDecode::CMD_PLACE_TAG) pvTag = c2.value;
                            }
                            ctx = saveInj;
                            ctx.r3.u64 = 34; ctx.r4.u64 = pvTag; ctx.r5.u64 = 0;
                            sub_8258ACE8(ctx, base);
                            uint32_t evRes = ctx.r3.u32;
                            ctx = saveInj;
                            ctx.r3.u64 = 34; ctx.r4.u64 = pvTag; ctx.r5.u64 = 1;
                            sub_8258AD48(ctx, base);
                            if (evRes == 0) {
                                ctx = saveInj;
                                ctx.r3.u64 = 9214; ctx.r4.u64 = 1;
                                sub_8258AC58(ctx, base);
                            }

                            // Apply visual
                            ctx = saveInj;
                            ctx.r3.u64 = speciesData;
                            ctx.r4.u64 = traitVal;
                            sub_82383538(ctx, base);
                            Log("PV Wildcard: Full apply value " + std::to_string(traitVal) + "!", 3);
                        } else {
                            Log("PV Wildcard: No species data!", 5);
                        }
                    }
#if 0 // Old diagnostic dump — no longer needed
                        uint8_t* mb = rex::Runtime::instance()->memory()->virtual_membase();
                        std::ofstream wlog("C:/Users/Administrator/Downloads/wildcard_inject_log.txt");
                        wlog << "=== Wildcard Inject ===" << std::endl;
                        wlog << "Entity: 0x" << std::hex << g_LastSpawnedEntity << std::endl;
                        wlog << "Wildcard trait: " << std::dec << cmd.value << std::endl;
                        wlog << "Barcode variant: " << barcodeVariant << std::endl;
                        wlog << "Barcode wildcard: " << barcodeWildcard << std::endl;

                        // Dump entity fields around the variant/wildcard area
                        wlog << std::endl << "=== Entity Memory (offsets 440-500) ===" << std::endl;
                        for (int i = 440; i <= 500; i += 4) {
                            uint32_t val = std::byteswap(*(uint32_t*)(mb + g_LastSpawnedEntity + i));
                            wlog << "[+" << std::dec << i << "] = 0x" << std::hex << val << std::endl;
                        }

                        // Dump glModel + scenegraphInst area
                        uint32_t glModel = std::byteswap(*(uint32_t*)(mb + g_LastSpawnedEntity + 0x110));
                        wlog << std::endl << "=== glModel ===" << std::endl;
                        wlog << "entity+0x110 → glModel = 0x" << std::hex << glModel << std::endl;
                        if (glModel != 0 && glModel > 0x40000000) {
                            // scenegraphInst_s at glModel+312
                            uint32_t sgBase = glModel + 312;
                            wlog << "sgInst base = 0x" << sgBase << std::endl;
                            // colourDisplacementTable at sgInst+296
                            uint32_t cdTable = std::byteswap(*(uint32_t*)(mb + sgBase + 296));
                            uint32_t cdSize = std::byteswap(*(uint32_t*)(mb + sgBase + 300));
                            wlog << "colourDisplacementTable = 0x" << cdTable << " (size=" << std::dec << cdSize << ")" << std::endl;
                            // curVariantColourIndex at sgInst+321
                            uint8_t curVariant = mb[sgBase + 321];
                            uint8_t defaultColour = mb[sgBase + 319];
                            uint8_t specialColour = mb[sgBase + 320];
                            wlog << "defaultColourIndex = " << (int)defaultColour << std::endl;
                            wlog << "specialAbilityColourIndex = " << (int)specialColour << std::endl;
                            wlog << "curVariantColourIndex = " << (int)curVariant << std::endl;

                            // textureTable at sgInst+276
                            uint32_t texTable = std::byteswap(*(uint32_t*)(mb + sgBase + 276));
                            uint32_t texSize = std::byteswap(*(uint32_t*)(mb + sgBase + 280));
                            wlog << "textureTable = 0x" << std::hex << texTable << " (size=" << std::dec << texSize << ")" << std::endl;
                        }

                        // Global wildcard/variant object pointers
                        uint32_t variantObjPtr = std::byteswap(*(uint32_t*)(mb + 0x820E8FEC));
                        uint32_t wildcardObjPtr = std::byteswap(*(uint32_t*)(mb + 0x820E9008));
                        wlog << std::endl << "=== Global Pointers ===" << std::endl;
                        wlog << "[0x820E8FEC] variantObj = 0x" << std::hex << variantObjPtr << std::endl;
                        wlog << "[0x820E9008] wildcardObj = 0x" << std::hex << wildcardObjPtr << std::endl;

                        // Current entity+468/472 values
                        wlog << "entity+452 = 0x" << std::byteswap(*(uint32_t*)(mb + g_LastSpawnedEntity + 452)) << std::endl;
                        wlog << "entity+456 = 0x" << std::byteswap(*(uint32_t*)(mb + g_LastSpawnedEntity + 456)) << std::endl;
                        wlog << "entity+460 = 0x" << std::byteswap(*(uint32_t*)(mb + g_LastSpawnedEntity + 460)) << std::endl;
                        wlog << "entity+464 = 0x" << std::byteswap(*(uint32_t*)(mb + g_LastSpawnedEntity + 464)) << std::endl;
                        wlog << "entity+468 = 0x" << std::byteswap(*(uint32_t*)(mb + g_LastSpawnedEntity + 468)) << std::endl;
                        wlog << "entity+472 = 0x" << std::byteswap(*(uint32_t*)(mb + g_LastSpawnedEntity + 472)) << std::endl;

                        // DON'T write yet — just log. Previous attempts crashed.
                        // We need this dump to understand the entity structure first.
                        wlog << std::endl << "=== NOT writing to entity (crash prevention) ===" << std::endl;
                        wlog << "Would write: entity+468=" << std::hex << variantObjPtr << std::endl;
                        wlog << "Would write: entity+472=" << wildcardObjPtr << std::endl;
                        wlog.close();
#endif
                    break;
                }
                case PVDecode::CMD_TARGET_ID:
                    Log("PV TargetID: " + std::to_string(cmd.value), 3);
                    break;
                case PVDecode::CMD_REUSE:
                    Log("PV Reuse: entity " + std::to_string(g_LastSpawnedEntity), 3);
                    break;
                default: break;
                }
            }

            ctx = saveInj;
            ctx.r3.u32 = gs;
            Log("Barcode injection complete!", 3);
        }
    }

    // Process species scan if pending (needs live PPC context)
    if (g_ScanPending) {
        g_ScanPending = false;
        PPCContext scanCtx = ctx;

        std::string outPath = "C:/Users/Administrator/Downloads/species_scan.txt";
        std::ofstream scanFile(outPath);
        if (scanFile.is_open()) {
            scanFile << "=== Species ID Scan (Live Context) ===" << std::endl;
            uint8_t* mb = rex::Runtime::instance()->memory()->virtual_membase();

            for (uint32_t tagID = 3; tagID <= 168; tagID++) {
                ctx = scanCtx; // restore clean context each iteration
                ctx.r3.u64 = tagID;
                sub_825357B8(ctx, base);
                uint32_t result = ctx.r3.u32;

                const char* enumNames168[] = {
                    "","","","ant","beetle","badger","bat","bear","beaver","bee",
                    "blackbutterfly","bluebottle","bluebutterfly","boomslang",
                    "brownbutterfly","bushbaby","buzzard","spare90",
                    "canary","spare89","cat","chameleon","chicken","camel",
                    "cow","spare87","crocodile","crow","deer","spare86",
                    "dog","spare85","dragon","dragonfly","duck","spare84",
                    "eagle","spare83","elephant","firefly","firesalamander","spare81",
                    "flyingpig","fox","frog","gecko","gerbil","spare80",
                    "spare79","spare78","spare76","spare75","goose","spare74",
                    "grasssnake","greenbutterfly","spare73","spare72",
                    "hedgehog","hippo","horse","hyena","hydra",
                    "lemming","lemmingpest","spare67","spare66","spare65","spare64",
                    "lion","spare63","spare62","spare61","mandrill","spare60",
                    "mole","spare59","monkey","moose","moth","mouse","newt",
                    "spare57","orangebutterfly","ostrich","spare55","spare54",
                    "parrot","polarbear","penguin","pig","pigeon","poisonfrog",
                    "pinkbutterfly","pony","purplebutterfly","rabbit","raccoon",
                    "spare50","spare49","redbutterfly","spare48","spare47",
                    "robin","spare45","spare44","salamander","spare43",
                    "sheep","scorpion","scorpionpest","spare40",
                    "sparrow","spider","squirrel","spare39","spare38","spare37","spare36",
                    "swan","spare35","spare34","spare33","spare32","spare31","spare30",
                    "unicorn","batpest","spare29","vulture","spare27",
                    "whitebutterfly","spare26","wolf","yeti","worm","yak",
                    "yellowbutterfly","zebra","slugpest","spare23","spare22",
                    "spare21","spare20","spare19","spare18",
                    "crowpest","raccoonpest","crocodilepest","spare17",
                    "molepest","spare16","spare15","spare14","spare13",
                    "wolfpest","mandrillpest","spare12","spare11",
                    "snail","snailpest","graysquirrel","spare3","spare4",
                    "spare5","spare6","spare7","spare8","spare9"
                };
                const char* eName = (tagID < 169) ? enumNames168[tagID] : "???";

                // If result is non-null, dump struct contents to find name data
                std::string info = "NULL";
                if (result != 0) {
                    info = "VALID";
                    char hex[16];

                    // Dump first 128 bytes of the struct looking for strings and pointers
                    for (int off = 0; off < 128; off += 4) {
                        uint32_t val = std::byteswap(*(uint32_t*)(mb + result + off));
                        // Check if it's a pointer to readable string
                        if (val > 0x82000000 && val < 0xA0000000) {
                            const char* s = (const char*)(mb + val);
                            bool isStr = true;
                            int strLen = 0;
                            for (int i = 0; i < 40 && s[i]; i++) {
                                if (s[i] < 0x20 || s[i] > 0x7E) { isStr = false; break; }
                                strLen++;
                            }
                            if (isStr && strLen >= 3) {
                                char buf[100];
                                snprintf(buf, 100, " [%d]=\"%.50s\"", off, s);
                                info += buf;
                            }
                        }
                        // Also check if the raw bytes look like inline ASCII
                        const char* raw = (const char*)(mb + result + off);
                        if (raw[0] >= 'A' && raw[0] <= 'z' && raw[1] >= 'a' && raw[1] <= 'z'
                            && raw[2] >= 'a' && raw[2] <= 'z') {
                            char buf[60];
                            snprintf(buf, 60, " @%d=\"%.16s\"", off, raw);
                            info += buf;
                        }
                    }
                }

                scanFile << tagID << " | " << eName << " | " << info << std::endl;
            }
            scanFile << "=== Scan complete ===" << std::endl;
            scanFile.close();
        }

        ctx = scanCtx; // restore context
        ctx.r3.u32 = ctx.r3.u32; // keep gardenScene
        Log("Species scan saved!", 5);
    }

    // Find entity by tag in garden (requested by SpawnMenu)
    if (g_FindEntity.pending) {
        g_FindEntity.pending = false;
        uint32_t gs = ctx.r3.u32;
        if (gs != 0) {
            // Get garden data
            PPCContext findCtx = ctx;
            rex_gardenMainGetGarden_824E10D8(findCtx, base);
            uint32_t gardenPtr = findCtx.r3.u32;

            if (gardenPtr != 0) {
                uint8_t* membase = rex::Runtime::instance()->memory()->virtual_membase();
                uint32_t gardenData = std::byteswap(*(uint32_t*)(membase + gardenPtr));

                // Call sub_82546C50 to find entity by tag
                findCtx = ctx;
                findCtx.r3.u64 = gardenData;
                findCtx.r4.u64 = 0;
                findCtx.r5.u64 = 0;
                findCtx.r6.u64 = g_FindEntity.tagID;
                sub_82546C50(findCtx, base);
                uint32_t foundEntity = findCtx.r3.u32;

                if (foundEntity != 0 && foundEntity > 0x40000000) {
                    g_LastSpawnedEntity = foundEntity;
                    Log("Found entity 0x" + std::to_string(foundEntity) + " for tag " + std::to_string(g_FindEntity.tagID), 3);
                } else {
                    Log("No entity found for tag " + std::to_string(g_FindEntity.tagID), 5);
                }
            }
        }
        ctx.r3.u32 = gs; // restore
    }

    // Process deferred Dragonache customization
    if (g_DeferredDragonache.pending && g_DeferredDragonache.framesRemaining > 0) {
        g_DeferredDragonache.framesRemaining--;
        Log("Dragonache deferred: " + std::to_string(g_DeferredDragonache.framesRemaining) +
            " frames left, entity=0x" + std::to_string(g_DeferredDragonache.entity), 3);
        if (g_DeferredDragonache.framesRemaining == 0) {
            g_DeferredDragonache.pending = false;
            uint32_t entity = g_DeferredDragonache.entity;
            if (entity != 0) {
                uint8_t* membase = rex::Runtime::instance()->memory()->virtual_membase();
                PPCContext dCtx = ctx;

                // Check if this is an egg (tag 211) or adult (tag 32)
                // For eggs: write preset data into eggData so the game applies
                // body parts naturally at hatch time
                // For adults: call body part functions directly

                // Try to read species data to check if egg
                // Apply body parts directly via game functions
                // (Egg preset injection needs r22 base which is the garden controller,
                // not the entity — that requires more research. For now, spawn adult
                // or hatch egg naturally and apply parts after.)
                if (g_DeferredDragonache.color >= 0) {
                    dCtx = ctx;
                    dCtx.r3.u64 = entity;
                    dCtx.r4.u64 = static_cast<uint32_t>(g_DeferredDragonache.color);
                    sub_82559D88(dCtx, base);
                    dCtx = ctx;
                    dCtx.r3.u64 = entity;
                    sub_82559E98(dCtx, base);
                    Log("Dragonache color applied", 3);
                }

                auto setPart = [&](int partIdx, int val) {
                    if (val >= 0) {
                        dCtx = ctx;
                        dCtx.r3.u64 = entity;
                        dCtx.r4.u64 = static_cast<uint32_t>(partIdx);
                        dCtx.r5.u64 = static_cast<uint32_t>(val);
                        sub_82559908(dCtx, base);
                    }
                };
                setPart(1, g_DeferredDragonache.teeth);
                setPart(2, g_DeferredDragonache.mane);
                setPart(3, g_DeferredDragonache.wings);
                setPart(4, g_DeferredDragonache.tail);
                setPart(5, g_DeferredDragonache.ridges);
                Log("Dragonache body parts applied!", 3);
            }
        }
    }

    // Process deferred variant/trick change
    if (g_DeferredVariantChange.pending && g_DeferredVariantChange.entity != 0) {
        g_DeferredVariantChange.pending = false;
        uint32_t entityAddr = g_DeferredVariantChange.entity;
        int varIdx = g_DeferredVariantChange.variantIndex;
        bool isTrick = g_DeferredVariantChange.isTrick;
        uint8_t* mb = rex::Runtime::instance()->memory()->virtual_membase();

        if (isTrick) {
            // TRICK: use sub_82382328(speciesData, trickIndex, 1)
            uint32_t speciesData = std::byteswap(*(uint32_t*)(mb + entityAddr + 2416));
            if (speciesData > 0x40000000 && speciesData < 0x8E000000) {
                PPCContext varCtx = ctx;
                varCtx.r3.u64 = speciesData;
                varCtx.r4.u64 = static_cast<uint32_t>(varIdx);
                varCtx.r5.u64 = 1;
                sub_82382328(varCtx, base);
                Log("Trick " + std::to_string(varIdx) + " applied!", 3);
            }
        } else {
            // COLOR CHANGE: try message 260 directly on the mature entity
            // PV barcode values for Whirlm: 3=Variant3, 4=Variant2, 5=Variant1, 15=Black
            PPCContext varCtx = ctx;
            varCtx.r3.u64 = entityAddr;
            varCtx.r4.u64 = static_cast<uint32_t>(varIdx);
            sub_825885B0(varCtx, base);

            {
                std::ofstream dbg("C:/Users/Administrator/Downloads/variant_debug.txt");
                char buf[256];
                snprintf(buf, 256, "MSG 260: entity=0x%08X variant=%d\n", entityAddr, varIdx);
                dbg << buf;
                dbg.close();
            }
            Log("Message 260 sent: variant=" + std::to_string(varIdx), 3);
        }
    }

    // Check for pending spawn request
    if (!g_SpawnRequest.pending) return;

    uint32_t gardenScene = ctx.r3.u32;
    if (gardenScene == 0) return; // Not in a garden

    uint32_t tagID = g_SpawnRequest.tagID;
    int variantIndex = g_SpawnRequest.variantIndex;
    int wildcardTrait = g_SpawnRequest.wildcardTrait;
    int dinoColor = g_SpawnRequest.dinoColor;
    g_SpawnRequest.pending = false;

    // Write dinosaur color to BOTH the global AND the garden controller
    if (dinoColor >= 0) {
        uint8_t* membase = rex::Runtime::instance()->memory()->virtual_membase();

        // Write to global
        *(membase + 0x83DBECB5) = static_cast<uint8_t>(dinoColor);

        // Find the garden controller by scanning the garden slot table
        // Table at PPC 0x83D27000, 5 slots of 504 bytes each
        // slot[16] = state (1=active), slot[4] = scene pointer
        uint32_t tableBase = 0x83D27000;
        for (int i = 0; i < 5; i++) {
            uint32_t slotAddr = tableBase + i * 504;
            uint32_t state = std::byteswap(*(uint32_t*)(membase + slotAddr + 16));
            if (state == 1) {
                uint32_t scenePtr = std::byteswap(*(uint32_t*)(membase + slotAddr + 4));
                // The scene has the world at [18944], and the controller has scene at [248]
                // Search nearby slot fields for the controller
                for (int off = 0; off < 504; off += 4) {
                    uint32_t ptr = std::byteswap(*(uint32_t*)(membase + slotAddr + off));
                    if (ptr > 0x40000000 && ptr < 0xA0000000 && off != 4) {
                        // Check if this pointer's [248] equals the scene
                        uint32_t testScene = std::byteswap(*(uint32_t*)(membase + ptr + 248));
                        if (testScene == scenePtr) {
                            // Found the garden controller!
                            *(membase + ptr + 4864) = static_cast<uint8_t>(dinoColor);
                            *(membase + ptr + 4865) = static_cast<uint8_t>(dinoColor);
                            *(membase + ptr + 4866) = static_cast<uint8_t>(dinoColor);
                            Log("Garden controller at 0x" + std::to_string(ptr) +
                                " - set dino color bytes to " + std::to_string(dinoColor), 3);
                            break;
                        }
                    }
                }
                break;
            }
        }
    }

    // Save the full context
    PPCContext saveCtx = ctx;

    // Handle special events (like Amber)
    if (tagID == SPECIAL_AMBER_EVENT) {
        Log("Triggering Amber event (9113)", 3);
        ctx.r3.u64 = 9113;
        ctx.r4.u64 = 1;
        sub_8258ADC8(ctx, base);
        ctx = saveCtx;
        ctx.r3.u32 = gardenScene;
        Log("Amber event triggered!", 3);
        return;
    }

    Log("Spawning tag " + std::to_string(tagID) + " (scene=" + std::to_string(gardenScene) + ")", 3);

    // Validate the tag ID before spawning
    ctx.r3.u64 = tagID;
    sub_825A0818(ctx, base);
    uint32_t typeCode = ctx.r3.u32;
    ctx = saveCtx; // restore context after validation call
    saveCtx = ctx; // re-save clean context

    if (typeCode > 44) {
        Log("Invalid species tag " + std::to_string(tagID) + " (type=" + std::to_string(typeCode) + ") - not a valid pinata!", 5);
        ctx.r3.u32 = gardenScene;
        return;
    }

    Log("Valid species (type=" + std::to_string(typeCode) + "), spawning...", 3);

    // Species tag → Egg tag mapping (from PV barcode database)
    static const std::pair<uint32_t, uint32_t> speciesEggMap[] = {
        {3,184},{5,186},{6,187},{7,188},{8,189},{9,190},{11,192},{13,194},
        {15,195},{16,196},{17,288},{18,198},{19,185},{20,200},{21,201},
        {22,202},{23,197},{24,319},{25,204},{26,205},{27,206},{28,207},
        {29,208},{30,209},{32,211},{33,212},{34,213},{36,215},{38,217},
        {39,218},{40,219},{42,221},{43,222},{44,223},{45,230},{52,231},
        {53,232},{54,233},{58,237},{59,238},{60,239},{61,241},{62,240},
        {63,247},{65,246},{69,248},{73,252},{75,254},{77,256},{78,257},
        {79,258},{80,259},{81,260},{84,263},{85,264},{86,268},{87,266},
        {88,272},{89,267},{90,269},{91,270},{92,271},{94,273},{96,275},
        {97,276},{100,279},{101,281},{103,280},{108,287},{109,286},
        {111,289},{112,291},{113,292},{114,293},{119,298},{121,299},
        {123,305},{126,306},{129,310},{130,309},{131,311},{132,313},
        {133,312},{134,318},{135,314},{136,315},{138,317},{140,284},
    };

    // If wildcard requested as egg, spawn the egg instead — hatching goes through proper pipeline
    uint32_t spawnTag = tagID;
    bool spawnedAsEgg = false;
    if (wildcardTrait > 0 && g_SpawnRequest.wildcardAsEgg) {
        uint32_t eggTag = 0;
        for (auto& [species, egg] : speciesEggMap) {
            if (species == tagID) { eggTag = egg; break; }
        }
        if (eggTag != 0) {
            spawnTag = eggTag;
            spawnedAsEgg = true;
            Log("WILDCARD: Spawning egg " + std::to_string(eggTag) + " (enable Force Wildcard on Hatch!)", 3);
        } else {
            Log("WILDCARD: No egg for tag " + std::to_string(tagID) + " — spawning adult", 3);
        }
    }

    // Call sub_82575AB8 - the REAL entity creation function
    ctx.r3.u64 = gardenScene;
    ctx.r4.u64 = 0;
    ctx.r5.u64 = 0;
    ctx.r6.u64 = 0;
    ctx.r7.u64 = spawnTag;
    ctx.r8.u64 = 0;
    ctx.r9.u64 = 0;
    ctx.f1.f64 = 1.0;
    ctx.f2.f64 = 0.0;

    sub_82575AB8(ctx, base);

    uint32_t spawnedEntity = ctx.r3.u32;
    g_LastSpawnedEntity = spawnedEntity;

    // Set entity on pending Dragonache customization
    if (g_DeferredDragonache.pending && spawnedEntity != 0) {
        g_DeferredDragonache.entity = spawnedEntity;
    }

    if (spawnedEntity != 0) {
        Log("Spawned entity: " + std::to_string(spawnedEntity), 3);

        // Apply wildcard — replicates the full egg hatch wildcard sequence
        // This sets the visual model but may not fully register as wildcard in game systems
        // For full wildcard recognition, use the breeding "Force Wildcard on Hatch" method
        if (wildcardTrait > 0) {
            uint32_t speciesData = PPC_LOAD_U32(spawnedEntity + 2416);
            if (speciesData != 0) {
                static const uint32_t wildcardTraitValues[] = { 10, 20, 21 };
                uint32_t traitVal = wildcardTraitValues[(wildcardTrait - 1) % 3];

                // Register wildcard event + status
                PPCContext wcCtx = saveCtx;
                wcCtx.r3.u64 = 34; wcCtx.r4.u64 = tagID; wcCtx.r5.u64 = 0;
                sub_8258ACE8(wcCtx, base);
                uint32_t eventResult = wcCtx.r3.u32;
                wcCtx = saveCtx;
                wcCtx.r3.u64 = 34; wcCtx.r4.u64 = tagID; wcCtx.r5.u64 = 1;
                sub_8258AD48(wcCtx, base);
                if (eventResult == 0) {
                    wcCtx = saveCtx;
                    wcCtx.r3.u64 = 9214; wcCtx.r4.u64 = 1;
                    sub_8258AC58(wcCtx, base);
                }

                // Apply wildcard visual
                wcCtx = saveCtx;
                wcCtx.r3.u64 = speciesData;
                wcCtx.r4.u64 = traitVal;
                sub_82383538(wcCtx, base);

                Log("WILDCARD: Applied trait " + std::to_string(wildcardTrait) +
                    " (value=" + std::to_string(traitVal) + ")", 3);
            } else {
                Log("WILDCARD: No species data on entity!", 5);
            }
        }


        // Defer variant application by a few frames to let entity fully initialize
        if (variantIndex >= 0) {
            g_DeferredVariant.entity = spawnedEntity;
            g_DeferredVariant.variantIndex = variantIndex;
            g_DeferredVariant.framesRemaining = 5;
            Log("Variant " + std::to_string(variantIndex) + " queued (deferred)", 3);
        }
    } else {
        Log("Spawn returned 0 - entity may not have been created", 5);
    }

    // Restore full context
    ctx = saveCtx;
    ctx.r3.u32 = gardenScene;
}

// Species name table pointer (PPC address) and format string
// sub_82575578 uses: r31 = 0x82BA0000 + 15392 (name table), typeCode*4 indexes into it
// sub_825A0878 converts typeCode back to tagID

// (PPC_EXTERN_IMPORTs for these declared at top of file)

void scanSpeciesIDs() {
    // Map tag IDs to their enum names from supportPinataTag_e
    static const char* enumNames[] = {
        /*0*/"aRTrigger", "aNull_0", "Animal_aaaaaa",
        /*3*/"ant", "beetle", "badger", "bat", "bear", "beaver", "bee",
        /*10*/"blackbutterfly", "bluebottle", "bluebutterfly", "boomslang",
        /*14*/"brownbutterfly", "bushbaby", "buzzard", "spare90",
        /*18*/"canary", "spare89", "cat", "chameleon", "chicken", "camel",
        /*24*/"cow", "spare87", "crocodile", "crow", "deer", "spare86",
        /*30*/"dog", "spare85", "dragon", "dragonfly", "duck", "spare84",
        /*36*/"eagle", "spare83", "elephant", "firefly", "firesalamander", "spare81",
        /*42*/"flyingpig", "fox", "frog", "gecko", "gerbil", "spare80",
        /*48*/"spare79", "spare78", "spare76", "spare75", "goose", "spare74",
        /*54*/"grasssnake", "greenbutterfly", "spare73", "spare72",
        /*58*/"hedgehog", "hippo", "horse", "hyena", "hydra",
        /*63*/"lemming", "lemmingpest", "spare67", "spare66", "spare65", "spare64",
        /*69*/"lion", "spare63", "spare62", "spare61", "mandrill", "spare60",
        /*75*/"mole", "spare59", "monkey", "moose", "moth", "mouse", "newt",
        /*82*/"spare57", "orangebutterfly", "ostrich", "spare55", "spare54",
        /*87*/"parrot", "polarbear", "penguin", "pig", "pigeon", "poisonfrog",
        /*93*/"pinkbutterfly", "pony", "purplebutterfly", "rabbit", "raccoon",
        /*98*/"spare50", "spare49", "redbutterfly", "spare48", "spare47",
        /*103*/"robin", "spare45", "spare44", "salamander", "spare43",
        /*108*/"sheep", "scorpion", "scorpionpest", "spare40",
        /*112*/"sparrow", "spider", "squirrel", "spare39", "spare38", "spare37", "spare36",
        /*119*/"swan", "spare35", "spare34", "spare33", "spare32", "spare31", "spare30",
        /*126*/"unicorn", "batpest", "spare29", "vulture", "spare27",
        /*131*/"whitebutterfly", "spare26", "wolf", "yeti", "worm", "yak",
        /*137*/"yellowbutterfly", "zebra", "slugpest", "spare23", "spare22",
        /*142*/"spare21", "spare20", "spare19", "spare18",
        /*146*/"crowpest", "raccoonpest", "crocodilepest", "spare17",
        /*150*/"molepest", "spare16", "spare15", "spare14", "spare13",
        /*155*/"wolfpest", "mandrillpest", "spare12", "spare11",
        /*159*/"snail", "snailpest", "graysquirrel", "spare3-9_start",
        /*163*/"spare4", "spare5", "spare6", "spare7", "spare8", "spare9"
    };

    std::string outPath = "C:/Users/Administrator/Downloads/species_scan.txt";
    std::ofstream outFile(outPath);
    if (!outFile.is_open()) return;

    outFile << "=== Species ID Scan (Encyclopedia Method) ===" << std::endl;
    outFile << "ID | EnumName | Encyclopedia | NameData | CurrentListName" << std::endl;
    outFile << "---+----------+-------------+----------+----------------" << std::endl;

    uint8_t* membase = rex::Runtime::instance()->memory()->virtual_membase();

    for (uint32_t tagID = 3; tagID <= 170; tagID++) {
        const char* enumName = (tagID < sizeof(enumNames)/sizeof(enumNames[0]))
            ? enumNames[tagID] : "???";

        // Call sub_825357B8 - the encyclopedia's species info lookup
        uint32_t speciesInfo = rex::GuestToHostFunction<uint32_t>(sub_825357B8, tagID);

        // Try to read name data from the species info struct
        char nameData[256] = "N/A";
        if (speciesInfo != 0) {
            // Try reading strings/pointers from various offsets of the species info struct
            // The struct has useful data at offsets 0, 4, 8, 12, 16, 20, etc.
            std::string offsets = "";
            for (int off = 0; off <= 32; off += 4) {
                uint32_t val = std::byteswap(*(uint32_t*)(membase + speciesInfo + off));
                offsets += "+" + std::to_string(off) + "=0x" + std::to_string(val) + " ";
            }
            // Try offset 16 (used by encyclopedia to pass to sub_82537CF0)
            uint32_t nameRef = std::byteswap(*(uint32_t*)(membase + speciesInfo + 16));
            if (nameRef > 0x82000000 && nameRef < 0x90000000) {
                // Might be a pointer to a string or another struct
                const char* maybeStr = (const char*)(membase + nameRef);
                // Check if it looks like printable ASCII
                bool isStr = true;
                for (int i = 0; i < 4; i++) {
                    if (maybeStr[i] < 0x20 || maybeStr[i] > 0x7E) { isStr = false; break; }
                }
                if (isStr) {
                    snprintf(nameData, 255, "str@16=\"%.64s\"", maybeStr);
                } else {
                    // Try reading as a pointer chain: nameRef -> string
                    uint32_t namePtr2 = std::byteswap(*(uint32_t*)(membase + nameRef));
                    if (namePtr2 > 0x82000000 && namePtr2 < 0x90000000) {
                        const char* maybeStr2 = (const char*)(membase + namePtr2);
                        bool isStr2 = true;
                        for (int i = 0; i < 4; i++) {
                            if (maybeStr2[i] < 0x20 || maybeStr2[i] > 0x7E) { isStr2 = false; break; }
                        }
                        if (isStr2) {
                            snprintf(nameData, 255, "str@16->=\"%.64s\"", maybeStr2);
                        } else {
                            snprintf(nameData, 255, "ptr@16=0x%08X->0x%08X", nameRef, namePtr2);
                        }
                    } else {
                        snprintf(nameData, 255, "val@16=0x%08X", nameRef);
                    }
                }
            } else {
                snprintf(nameData, 255, "val@16=0x%08X", nameRef);
            }
        }

        // Find current list entry
        const char* listName = "(not in list)";
        for (auto& entry : g_PinataIDs) {
            if (entry.ID == tagID) { listName = entry.Name; break; }
        }

        outFile << tagID << " | " << enumName << " | "
                << (speciesInfo != 0 ? "VALID" : "INVALID") << " | "
                << nameData << " | " << listName << std::endl;
    }

    // Also list all TiP exclusive species we need to identify
    outFile << std::endl << "=== TiP Exclusive Species (need mapping) ===" << std::endl;
    outFile << "Known TiP species: Bispotti, Camello, Cherrapin, Chocstrich, Choclodocus," << std::endl;
    outFile << "  Custacean, Flapyak, Geckie, Hoghurt, Hootyfruity, Limeoceros," << std::endl;
    outFile << "  Moojoo, Parmadillo, Peckanmix, Pengum, Pieena, Polollybear," << std::endl;
    outFile << "  Robean, S'morepion, Sarsgorilla, Smelba, Sweetle, Tartridge," << std::endl;
    outFile << "  Tigermisu, Vulchurro, Walrusk, Lemmoning, Jeli, Flapjak" << std::endl;
    outFile << std::endl;
    outFile << "Spare slots that are TiP piñatas:" << std::endl;
    for (uint32_t tagID = 3; tagID <= 170; tagID++) {
        if (tagID < sizeof(enumNames)/sizeof(enumNames[0])) {
            if (strstr(enumNames[tagID], "spare") != nullptr) {
                outFile << "  ID " << tagID << " = " << enumNames[tagID] << std::endl;
            }
        }
    }

    outFile << std::endl << "=== Scan complete ===" << std::endl;
    outFile.close();
    Log("Species scan saved to " + outPath, 5);
}

void spawnTick_hook() {
    // No-op: spawning happens via gardenMainGetGardenScene hook
}
