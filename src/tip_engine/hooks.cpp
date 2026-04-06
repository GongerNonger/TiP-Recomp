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
#include "Overlays/SpawnMenu.h"
#include "Overlays/BarcodeInjector.h"

#include "rex_macros.h"
#include <fstream>
#include "tip_engine/Types/CommonTypes.h"

// Forward declaration for spawn system
void spawnTick_hook();

// Message dispatch for variant/wildcard (used by deferred variant system)
PPC_EXTERN_IMPORT(sub_825885B0);

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
REXCVAR_DEFINE_BOOL(DisableMainDraw, false, "_Trouble in Paradise", "Disables the Main Draw Pass");
REXCVAR_DEFINE_BOOL(DisableUIDraw, false, "_Trouble in Paradise", "Disables the UI Draw Pass");

REXCVAR_DEFINE_BOOL(UseAspectRatioFromConfig, false, "_Trouble in Paradise", "Use Aspect Ratio from config");
REXCVAR_DEFINE_DOUBLE(AspectRatio, 1.7777778f, "_Trouble in Paradise", "Aspect Ratio");


REX_PPC_EXTERN_IMPORT(camMainGetPos_821F07E0);

float * camMainGetPos_821F07E0_Hook(float *result){
  /*
  if(REXCVAR_GET(Freecam)) {
    result[0] = to_byteswapped_float(static_cast<float>(REXCVAR_GET(Freecam_X)));
    result[1] = to_byteswapped_float(static_cast<float>(REXCVAR_GET(Freecam_Y)));
    result[2] = to_byteswapped_float(static_cast<float>(REXCVAR_GET(Freecam_Z)));

    //.data:82C34E98 Me_36.virtCam
    uintptr_t virtCam = reinterpret_cast<uintptr_t>(0x100000000ull + 0x82C34E98);
    *(float *)(virtCam + 8) = result[0];
    *(float *)(virtCam + 12) = result[1];
    *(float *)(virtCam + 16) = result[2];
    DebugLogFloat("Freecam X", static_cast<float>(REXCVAR_GET(Freecam_X)));
    DebugLogFloat("Freecam Y", static_cast<float>(REXCVAR_GET(Freecam_Y)));
    DebugLogFloat("Freecam Z", static_cast<float>(REXCVAR_GET(Freecam_Z)));

    if((float *)(virtCam + 8) != nullptr) {
        DebugLogFloat("CamMainGetPos X", *(float *)(virtCam + 8));
    }
    if((float *)(virtCam + 12) != nullptr) {
        DebugLogFloat("CamMainGetPos Y", *(float *)(virtCam + 12));
    }
    if((float *)(virtCam + 16) != nullptr) {
        DebugLogFloat("CamMainGetPos Z", *(float *)(virtCam + 16));
    }

    return result;
  }else{
    //.data:82C34E98 Me_36.virtCam
    uintptr_t virtCam = reinterpret_cast<uintptr_t>(0x100000000ull + 0x82C34E98);
    //*(float *)(virtCam + 8) = result[0];
    //*(float *)(virtCam + 12) = result[1];
    //*(float *)(virtCam + 16) = result[2];
    DebugLogFloat("Freecam X", static_cast<float>(REXCVAR_GET(Freecam_X)));
    DebugLogFloat("Freecam Y", static_cast<float>(REXCVAR_GET(Freecam_Y)));
    DebugLogFloat("Freecam Z", static_cast<float>(REXCVAR_GET(Freecam_Z)));
    if((float *)(virtCam + 8) != nullptr) {
        DebugLogFloat("CamMainGetPos X", *(float *)(virtCam + 8));
    }
    if((float *)(virtCam + 12) != nullptr) {
        DebugLogFloat("CamMainGetPos Y", *(float *)(virtCam + 12));
    }
    if((float *)(virtCam + 16) != nullptr) {
        DebugLogFloat("CamMainGetPos Z", *(float *)(virtCam + 16));
    }
    return rex ::GuestToHostFunction<float *>(__imp__rex_camMainGetPos_821F07E0, result);
  }
    */
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


auto frameTime=std::chrono::system_clock::now();
int frame = 0;
double fpsHistory[10] = {};
int fpsHistoryIndex = 0;
int fpsHistoryCount = 0;

void fps_hook() {
  // Process any pending spawn requests each frame
  spawnTick_hook();

  // Process deferred texture dump — scan entity after it's fully initialized
  if (g_DeferredDump.framesRemaining > 0) {
      g_DeferredDump.framesRemaining--;
      if (g_DeferredDump.framesRemaining == 0 && g_DeferredDump.entity != 0) {
          uint8_t* membase = rex::Runtime::instance()->memory()->virtual_membase();
          uint32_t entityAddr = g_DeferredDump.entity;
          std::ofstream dump("C:/Users/Administrator/Downloads/entity_dump.txt");
          if (dump.is_open()) {
              char buf[512];
              snprintf(buf, 512, "Entity at PPC 0x%08X (tagID=%d) — DEFERRED SCAN (30 frames)\n\n", entityAddr, g_DeferredDump.tagID);
              dump << buf;

              // Read glModel pointer at entity+0x110 (known fixed offset)
              uint32_t glModelAddr = std::byteswap(*(uint32_t*)(membase + entityAddr + 0x110));
              snprintf(buf, 512, "Entity+0x110 = 0x%08X (glModel candidate)\n", glModelAddr);
              dump << buf;

              auto isValidPtr = [](uint32_t p) -> bool {
                  return (p >= 0x40000000 && p < 0x50000000) || (p >= 0x80000000 && p < 0x8E000000);
              };

              if (isValidPtr(glModelAddr)) {
                  uint32_t modelPtr = std::byteswap(*(uint32_t*)(membase + glModelAddr));
                  snprintf(buf, 512, "model ptr: 0x%08X (valid=%d)\n\n", modelPtr, isValidPtr(modelPtr));
                  dump << buf;

                  // Dump raw sgInst region to find real offsets
                  // sgInst starts at glModel+0x138
                  uint32_t sgBase = glModelAddr + 0x138;
                  snprintf(buf, 512, "=== sgInst raw dump at glModel+0x138 = PPC 0x%08X ===\n", sgBase);
                  dump << buf;

                  // Dump 512 bytes of sgInst looking for texture table and color indices
                  for (int row = 0; row < 512; row += 16) {
                      snprintf(buf, 512, "sgInst+%04X: ", row);
                      dump << buf;
                      for (int col = 0; col < 16; col++) {
                          snprintf(buf, 512, "%02X ", *(membase + sgBase + row + col));
                          dump << buf;
                      }
                      dump << " | ";
                      // Also show as big-endian u32 values
                      for (int col = 0; col < 16; col += 4) {
                          uint32_t val = std::byteswap(*(uint32_t*)(membase + sgBase + row + col));
                          if (isValidPtr(val)) {
                              snprintf(buf, 512, "[PTR 0x%08X] ", val);
                          } else if (val < 100) {
                              snprintf(buf, 512, "[int %d] ", val);
                          } else {
                              snprintf(buf, 512, "[0x%08X] ", val);
                          }
                          dump << buf;
                      }
                      dump << std::endl;
                  }

                  // Search for texture names by following ALL valid pointers from sgInst
                  dump << "\n=== Following all pointers looking for texture names ===" << std::endl;
                  for (int off = 0; off < 512; off += 4) {
                      uint32_t ptr = std::byteswap(*(uint32_t*)(membase + sgBase + off));
                      if (!isValidPtr(ptr)) continue;

                      // Check if ptr points to something with readable strings
                      // Follow one level of indirection too
                      for (int subOff = 0; subOff < 64; subOff += 4) {
                          uint32_t subPtr = std::byteswap(*(uint32_t*)(membase + ptr + subOff));
                          if (!isValidPtr(subPtr)) continue;

                          const char* s = (const char*)(membase + subPtr);
                          // Check if it looks like a texture name
                          if (strncmp(s, "aid_", 4) == 0 || strncmp(s, "texture", 7) == 0 ||
                              strncmp(s, "pinata", 6) == 0 || strncmp(s, "animal", 6) == 0) {
                              snprintf(buf, 512, "  sgInst+0x%03X -> 0x%08X +0x%02X -> 0x%08X = \"%.80s\"\n",
                                  off, ptr, subOff, subPtr, s);
                              dump << buf;
                          }
                      }
                  }

                  // === Option C: Search dbScenegraph for texture name pointers ===
                  // SAFE: only dereference data-range pointers (0x82-0x8E), never heap
                  uint32_t sgDataPtr = std::byteswap(*(uint32_t*)(membase + sgBase));
                  dump << "\n=== Scanning dbScenegraph at 0x" << std::hex << sgDataPtr << std::dec << " ===" << std::endl;
                  if (sgDataPtr >= 0x40000000 && sgDataPtr < 0x50000000) {
                      int found = 0;
                      // Scan dbScenegraph (heap object) for data-range string pointers only
                      for (int off = 0; off < 512 && found < 20; off += 4) {
                          uint32_t val = std::byteswap(*(uint32_t*)(membase + sgDataPtr + off));
                          if (val >= 0x82000000 && val < 0x8E000000) {
                              const char* s = (const char*)(membase + val);
                              if (s[0] >= 0x20 && s[0] <= 0x7E && s[1] >= 0x20 && strnlen(s, 80) > 3) {
                                  snprintf(buf, 512, "  sg+0x%03X -> 0x%08X = \"%.70s\"\n", off, val, s);
                                  dump << buf;
                                  found++;
                              }
                          }
                      }
                      if (found == 0) dump << "  No strings found in dbScenegraph\n";
                  }
              } else {
                  snprintf(buf, 512, "Entity+0x110 = 0x%08X — NOT a valid pointer\n", glModelAddr);
                  dump << buf;
              }
              dump.close();
          }
          g_DeferredDump.entity = 0;
      }
  }

  frame++;
  auto Time = std::chrono::system_clock::now();
  std::chrono::duration<double, std::milli> delta = Time - frameTime;
  frameTime = Time;
  double fpsfromMS = 1000 / delta.count();
  if (frame >= 2) {
    frame = 0;
    fpsHistory[fpsHistoryIndex] = fpsfromMS;
    fpsHistoryIndex = (fpsHistoryIndex + 1) % 10;
    if (fpsHistoryCount < 10) fpsHistoryCount++;

    double sum = 0.0;
    for (int i = 0; i < fpsHistoryCount; i++) sum += fpsHistory[i];
    fpsCount = sum / fpsHistoryCount;
  }

  showfps = REXCVAR_GET(show_fps_overlay);

  /*
  lightMainWorkspace_s* workspace = reinterpret_cast<lightMainWorkspace_s*>(0x100000000ull + 0x82C3C010);
  workspace->dirLight.col = {
      to_byteswapped_float(static_cast<float>((REXCVAR_GET(directionalColor) >> 24) & 0xFF) / 255.0f),
      to_byteswapped_float(static_cast<float>((REXCVAR_GET(directionalColor) >> 16) & 0xFF) / 255.0f),
      to_byteswapped_float(static_cast<float>((REXCVAR_GET(directionalColor) >> 8) & 0xFF) / 255.0f),
      to_byteswapped_float(static_cast<float>((REXCVAR_GET(directionalColor)) & 0xFF) / 255.0f)
  };
   workspace->ambientCol = {
      to_byteswapped_float(static_cast<float>((REXCVAR_GET(ambientColor) >> 24) & 0xFF) / 255.0f),
      to_byteswapped_float(static_cast<float>((REXCVAR_GET(ambientColor) >> 16) & 0xFF) / 255.0f),
      to_byteswapped_float(static_cast<float>((REXCVAR_GET(ambientColor) >> 8) & 0xFF) / 255.0f),
      to_byteswapped_float(static_cast<float>((REXCVAR_GET(ambientColor)) & 0xFF) / 255.0f)
  };
   workspace->modelAmbientCol = {
      to_byteswapped_float(static_cast<float>((REXCVAR_GET(ambientModelColor) >> 24) & 0xFF) / 255.0f),
      to_byteswapped_float(static_cast<float>((REXCVAR_GET(ambientModelColor) >> 16) & 0xFF) / 255.0f),
      to_byteswapped_float(static_cast<float>((REXCVAR_GET(ambientModelColor) >> 8) & 0xFF) / 255.0f),
      to_byteswapped_float(static_cast<float>((REXCVAR_GET(ambientModelColor)) & 0xFF) / 255.0f)
  };

  workspace->fogCol = {
      to_byteswapped_float(static_cast<float>((REXCVAR_GET(fogColor) >> 24) & 0xFF) / 255.0f),
      to_byteswapped_float(static_cast<float>((REXCVAR_GET(fogColor) >> 16) & 0xFF) / 255.0f),
      to_byteswapped_float(static_cast<float>((REXCVAR_GET(fogColor) >> 8) & 0xFF) / 255.0f),
      to_byteswapped_float(static_cast<float>((REXCVAR_GET(fogColor)) & 0xFF) / 255.0f)
  };
  workspace->fogOpacity = to_byteswapped_float(static_cast<float>(REXCVAR_GET(fogOpacity)));
  workspace->blueShiftScalar = to_byteswapped_float(static_cast<float>(REXCVAR_GET(blueShiftScalar)));
  workspace->cubeFogEnabled = REXCVAR_GET(cubeFogEnabled) ? 1 : 0;
  */
}

bool PresentParams_hook(PPCRegister& r11) {
  //r11.u32 is a * to a _D3DPRESENT_PARAMETERS_ struct
  if(r11.u32 == 0) {
    return false;
  }
  _D3DPRESENT_PARAMETERS_* params = reinterpret_cast<_D3DPRESENT_PARAMETERS_*>(0x100000000ull + r11.u32);

  
  auto bs = [](uint32_t v) { return std::byteswap(v); };
  auto bsi = [](int v) { return static_cast<int>(std::byteswap(static_cast<uint32_t>(v))); };

  params->FullScreen_RefreshRateInHz = bs(164);
  params->PresentationInterval = bs(0); // D3DPRESENT_INTERVAL_ONE
  if(REXCVAR_GET(lock_fps)) {
    params->PresentationInterval = bs(2); // D3DPRESENT_INTERVAL_TWO
  }

  //params->BackBufferHeight = bs(1080);
  //params->BackBufferWidth = bs(1920);
  return false;
}

void PresentParams2_hook(PPCRegister& r3){
   // Guest memory is big-endian (PPC), byte-swap each 32-bit field for host (x86)
  auto bs = [](uint32_t v) { return std::byteswap(v); };
  auto bsi = [](int v) { return static_cast<int>(std::byteswap(static_cast<uint32_t>(v))); };

  //r3 is a * to video parameters struct
  if(r3.u32 == 0) {
    return;
  }
  videoParams_s* params = reinterpret_cast<videoParams_s*>(0x100000000ull + r3.u32);
  params->resolutionType = bs(2);
  //params->width = bs(1920);
  //params->height = bs(1080);
  params->refreshRateHZ = bs(164);
  params->presentInterval = bs(0); // D3DPRESENT_INTERVAL_ONE
  if(REXCVAR_GET(lock_fps)) {
    params->presentInterval = bs(2); // D3DPRESENT_INTERVAL_TWO
  }
  params->presentImmediately = bs(1); // TRUE
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

// ============================================================
// Pinata Vision Spawn System
// ============================================================
//
// sub_824CB440 is the PPC equivalent of the PC port's SpawnObject_93F220.
// It's a thin wrapper around the core dispatcher sub_824CB760.
//
// Calling convention (from generated recomp analysis):
//   r3 = 383          (scene dispatch message ID, constant)
//   r4 = tagID        (supportPinataTag_e value = g_PinataIDs.ID)
//   r5 = parentPtr    (entity/scene pointer, 0 for free spawn)
//   r6 = 0            (flags)
//   r7 = 0            (flags)
//   r8 = 1            (spawn mode: 1 = normal)
//
// sub_824CB6F8 is the store variant (sets store flag, used by shops).
// ============================================================

// The REAL entity creation function (unmodified — variant injection during creation breaks AI)
PPC_EXTERN_IMPORT(sub_82575AB8);

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

// DISABLED — texture init hook causes crash
extern "C" void rex_dbTextureInitTexture_DISABLED(PPCContext& ctx, uint8_t* base) {
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
// Encyclopedia species info lookup
PPC_EXTERN_IMPORT(sub_825357B8);
// Species tag ID validator - returns type code (0-44 = valid, 46 = invalid)
PPC_EXTERN_IMPORT(sub_825A0818);
// Message dispatch: sends message 260 (set variant/wildcard color)
PPC_EXTERN_IMPORT(sub_825885B0);
// Choclodocus color system (from SparseCallback type 0x13 handler)
PPC_EXTERN_IMPORT(sub_82546C50);  // Find piñata in garden by tag ID
PPC_EXTERN_IMPORT(sub_82559420);  // Apply dino color (r3=entity, r4=color 0-3)
PPC_EXTERN_IMPORT(sub_825597C0);  // Pre-color-change setup
PPC_EXTERN_IMPORT(rex_gardenMainGetGarden_824E10D8); // Get garden data

// Runtime variant change system (from eat→color change pipeline)
PPC_EXTERN_IMPORT(sub_82382328);  // Set variant change target (speciesData, variantIdx, flag)
PPC_EXTERN_IMPORT(sub_8242BE00);  // Reload textures for new variant
// Event dispatch (used for Amber/Wishing Well event 9113)
PPC_EXTERN_IMPORT(sub_8258ADC8);

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
                    Log("PV PlaceTag: ID " + std::to_string(cmd.value) +
                        " variant=" + std::to_string(barcodeVariant) +
                        " wildcard=" + std::to_string(barcodeWildcard), 3);
                    ctx = saveInj;
                    ctx.r3.u64 = cmd.value;
                    sub_825A0818(ctx, base);
                    if (ctx.r3.u32 > 44) {
                        Log("PV PlaceTag: invalid species " + std::to_string(cmd.value), 5);
                        ctx = saveInj;
                        break;
                    }
                    ctx = saveInj;
                    ctx.r3.u64 = gs;
                    ctx.r4.u64 = 0;
                    ctx.r5.u64 = 0;
                    ctx.r6.u64 = 0;
                    ctx.r7.u64 = cmd.value;
                    ctx.r8.u64 = 0;
                    // Pass variant value via r9 at creation time!
                    ctx.r9.u64 = barcodeVariant;
                    ctx.f1.f64 = 1.0;
                    ctx.f2.f64 = 0.0;
                    sub_82575AB8(ctx, base);
                    g_LastSpawnedEntity = ctx.r3.u32;
                    if (g_LastSpawnedEntity != 0)
                        Log("PV PlaceTag: entity " + std::to_string(g_LastSpawnedEntity), 3);
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
                    if (g_LastSpawnedEntity != 0) {
                        ctx = saveInj;
                        ctx.r3.u64 = g_LastSpawnedEntity;
                        ctx.r4.u64 = cmd.value;
                        sub_825885B0(ctx, base);
                    }
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
            // COLOR CHANGE: write variant index to entity+3636
            // then set strategy state to 1093 at entity+2564
            // This triggers the game's sparkle animation + texture swap
            PPC_STORE_U32(entityAddr + 3636, std::byteswap(static_cast<uint32_t>(varIdx)));
            PPC_STORE_U32(entityAddr + 2564, std::byteswap(static_cast<uint32_t>(1093)));

            {
                std::ofstream dbg("C:/Users/Administrator/Downloads/variant_debug.txt");
                char buf[256];
                snprintf(buf, 256, "COLOR CHANGE: entity=0x%08X variant=%d at +3636, state=1093 at +2564\n",
                    entityAddr, varIdx);
                dbg << buf;
                dbg.close();
            }
            Log("Color variant " + std::to_string(varIdx) + " triggered!", 3);
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

    // Call sub_82575AB8 - the REAL entity creation function
    ctx.r3.u64 = gardenScene;
    ctx.r4.u64 = 0;
    ctx.r5.u64 = 0;
    ctx.r6.u64 = 0;
    ctx.r7.u64 = tagID;
    ctx.r8.u64 = 0;
    ctx.r9.u64 = 0; // variant applied post-creation via eat system, not at spawn
    ctx.f1.f64 = 1.0;
    ctx.f2.f64 = 0.0;

    sub_82575AB8(ctx, base);

    uint32_t spawnedEntity = ctx.r3.u32;
    g_LastSpawnedEntity = spawnedEntity;

    if (spawnedEntity != 0) {
        Log("Spawned entity: " + std::to_string(spawnedEntity), 3);

        // Don't apply variant at spawn — entity AI breaks
        // Variant must be applied after piñata matures (via "Apply Variant" button)

        // DISABLED: All scanning disabled to test stability
        // g_DeferredDump.entity = spawnedEntity;
        // g_DeferredDump.tagID = tagID;
        // g_DeferredDump.framesRemaining = 60;

        // DISABLED: Immediate scan was causing crashes
        if (false) {
            uint8_t* membase = rex::Runtime::instance()->memory()->virtual_membase();
            std::ofstream dump("C:/Users/Administrator/Downloads/entity_dump.txt");
            if (dump.is_open()) {
                dump << "Entity at PPC 0x" << std::hex << spawnedEntity << " (tagID=" << std::dec << tagID << ")" << std::endl;
                dump << "Variant setting: " << variantIndex << std::endl;
                dump << std::endl;

                // Search strategy: The entity contains a pointer to glModel_s somewhere.
                // glModel_s has sgInst embedded at offset 0x138 (312).
                // sgInst starts with a pointer to dbScenegraph_s.
                // curVariantColourIndex is at sgInst+0x141 (321).
                // So from glModel_s start: offset 0x138+0x141 = 0x279 (633).

                // Scan entity memory for pointers, follow each one,
                // check if it looks like a glModel_s by verifying:
                //   [ptr+0] = valid pointer (model)
                //   [ptr+0x138] = valid pointer (sgInst.sg -> dbScenegraph)
                //   [ptr+0x138+0x141] = small byte (curVariantColourIndex)

                dump << "=== Searching for glModel_s in entity memory ===" << std::endl;
                bool found = false;

                // PPC address ranges: heap 0x40000000-0x50000000, code/data 0x82000000-0x8E000000
                auto isValidPtr = [](uint32_t p) -> bool {
                    return (p >= 0x40000000 && p < 0x50000000) ||
                           (p >= 0x80000000 && p < 0x8E000000);
                };

                for (int entityOff = 0; entityOff < 2048; entityOff += 4) {
                    uint32_t candidate = std::byteswap(*(uint32_t*)(membase + spawnedEntity + entityOff));
                    if (!isValidPtr(candidate)) continue;

                    // Check if candidate looks like glModel_s
                    uint32_t modelPtr = std::byteswap(*(uint32_t*)(membase + candidate));
                    if (!isValidPtr(modelPtr)) continue;

                    // sgInst.sg at [candidate+0x138] should be valid pointer
                    uint32_t sgPtr = std::byteswap(*(uint32_t*)(membase + candidate + 0x138));
                    if (!isValidPtr(sgPtr)) continue;

                    // textureTable at sgInst+276 = [candidate+0x138+0x114]
                    uint32_t texTable = std::byteswap(*(uint32_t*)(membase + candidate + 0x138 + 276));

                    // Read the color indices
                    uint8_t defaultIdx = *(membase + candidate + 0x138 + 319);
                    uint8_t specialIdx = *(membase + candidate + 0x138 + 320);
                    uint8_t variantIdx = *(membase + candidate + 0x138 + 321);

                    // All three indices should be small (< 50)
                    if (defaultIdx < 50 && specialIdx < 50 && variantIdx < 50) {
                        char buf[512];
                        snprintf(buf, 512,
                            "FOUND glModel_s candidate!\n"
                            "  Entity+0x%04X -> glModel at PPC 0x%08X\n"
                            "  model ptr: 0x%08X\n"
                            "  sgInst.sg: 0x%08X\n"
                            "  textureTable: 0x%08X\n"
                            "  defaultColourIndex: %d\n"
                            "  specialAbilityColourIndex: %d\n"
                            "  curVariantColourIndex: %d\n"
                            "  === ADDRESS OF curVariantColourIndex: PPC 0x%08X ===\n",
                            entityOff, candidate,
                            modelPtr, sgPtr, texTable,
                            defaultIdx, specialIdx, variantIdx,
                            candidate + 0x138 + 321);
                        dump << buf;
                        found = true;

                        // Dump switchStateArray (body parts) at glModel+204
                        dump << "  switchStateArray: ";
                        for (int i = 0; i < 16; i++) {
                            snprintf(buf, 512, "%02X ", *(membase + candidate + 204 + i));
                            dump << buf;
                        }
                        dump << std::endl;

                        // Read colourDisplacementTable info
                        uint32_t cdTable = std::byteswap(*(uint32_t*)(membase + candidate + 0x138 + 296));
                        int cdSize = (int)std::byteswap(*(uint32_t*)(membase + candidate + 0x138 + 300));
                        snprintf(buf, 512, "  colourDisplacementTable: 0x%08X (size=%d)\n", cdTable, cdSize);
                        dump << buf;

                        // === PHASE 3: Dump texture table ===
                        if (texTable != 0) {
                            int texSize = (int)std::byteswap(*(uint32_t*)(membase + candidate + 0x138 + 280));
                            snprintf(buf, 512, "\n  === TEXTURE TABLE at 0x%08X (size=%d) ===\n", texTable, texSize);
                            dump << buf;

                            // Each entry is scenegraphInstTextureTable_s:
                            // PPC: {name_ptr(4), currentTexture_ptr(4), originalTexture_ptr(4)} = 12 bytes
                            for (int ti = 0; ti < texSize && ti < 32; ti++) {
                                uint32_t entryAddr = texTable + ti * 12;
                                uint32_t namePtr = std::byteswap(*(uint32_t*)(membase + entryAddr));
                                uint32_t curTexPtr = std::byteswap(*(uint32_t*)(membase + entryAddr + 4));
                                uint32_t origTexPtr = std::byteswap(*(uint32_t*)(membase + entryAddr + 8));

                                char texName[128] = "(null)";
                                if (namePtr > 0x40000000 && namePtr < 0x8E000000) {
                                    const char* n = (const char*)(membase + namePtr);
                                    strncpy(texName, n, 127);
                                }

                                snprintf(buf, 512, "  [%2d] name=0x%08X cur=0x%08X orig=0x%08X \"%s\"\n",
                                    ti, namePtr, curTexPtr, origTexPtr, texName);
                                dump << buf;
                            }
                        } else {
                            dump << "  textureTable is NULL\n";
                        }

                        // Log the address but DON'T write yet — just discover
                        if (variantIndex >= 0) {
                            uint32_t varAddr = candidate + 0x138 + 321;
                            snprintf(buf, 512, "  curVariantColourIndex address: PPC 0x%08X (SCAN ONLY - no write)\n", varAddr);
                            dump << buf;
                        }
                    }
                }

                if (!found) {
                    dump << "No glModel_s found in first 4096 bytes of entity" << std::endl;
                    dump << std::endl << "=== Raw entity dump (first 512 bytes) ===" << std::endl;
                    for (int row = 0; row < 512; row += 16) {
                        char line[200];
                        snprintf(line, 200, "+%04X: ", row);
                        dump << line;
                        for (int col = 0; col < 16; col++) {
                            snprintf(line, 200, "%02X ", *(membase + spawnedEntity + row + col));
                            dump << line;
                        }
                        dump << std::endl;
                    }
                }

                dump.close();
                Log("Entity analysis saved to entity_dump.txt", 3);
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

PPC_EXTERN_IMPORT(sub_825A0878);
// Asset ID builder - takes (r3=tagID, r4=outputBuffer) and produces asset path string
PPC_EXTERN_IMPORT(sub_8254C018);
// Encyclopedia species info lookup - returns species data ptr or 0 if invalid
PPC_EXTERN_IMPORT(sub_825357B8);

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