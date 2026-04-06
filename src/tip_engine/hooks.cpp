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

  // Process deferred variant application
  if (g_DeferredVariant.framesRemaining > 0) {
      g_DeferredVariant.framesRemaining--;
      if (g_DeferredVariant.framesRemaining == 0 && g_DeferredVariant.entity != 0) {
          Log("Applying variant " + std::to_string(g_DeferredVariant.variantIndex)
              + " to entity " + std::to_string(g_DeferredVariant.entity), 3);
          rex::GuestToHostFunction<void>(sub_825885B0,
              g_DeferredVariant.entity,
              static_cast<uint32_t>(g_DeferredVariant.variantIndex));
          g_DeferredVariant.entity = 0;
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

// The REAL entity creation function (not the cutscene trigger)
PPC_EXTERN_IMPORT(sub_82575AB8);
// Encyclopedia species info lookup
PPC_EXTERN_IMPORT(sub_825357B8);
// Species tag ID validator - returns type code (0-44 = valid, 46 = invalid)
PPC_EXTERN_IMPORT(sub_825A0818);
// Message dispatch: sends message 260 (set variant/wildcard color)
PPC_EXTERN_IMPORT(sub_825885B0);
// Event dispatch (used for Amber/Wishing Well event 9113)
PPC_EXTERN_IMPORT(sub_8258ADC8);

// Override gardenMainGetGardenScene — call original, then spawn if pending
PPC_EXTERN_IMPORT(__imp__rex_gardenMainGetGardenScene_824E1120);

extern "C" PPC_FUNC(rex_gardenMainGetGardenScene_824E1120) {
    // Call the original implementation
    __imp__rex_gardenMainGetGardenScene_824E1120(ctx, base);

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

                // If result is non-null, try to read data from it
                std::string info = "NULL";
                if (result != 0) {
                    // Try reading a few offsets to find name-like data
                    info = "PTR=0x";
                    char hex[16]; snprintf(hex, 16, "%08X", result); info += hex;

                    // Try offset 16 (what encyclopedia uses)
                    uint32_t v16 = std::byteswap(*(uint32_t*)(mb + result + 16));
                    snprintf(hex, 16, "%08X", v16); info += " [16]=0x"; info += hex;

                    // Try reading a string at various offsets from the struct
                    for (int off : {24, 28, 32, 36, 40, 44, 48, 52}) {
                        uint32_t ptr = std::byteswap(*(uint32_t*)(mb + result + off));
                        if (ptr > 0x82000000 && ptr < 0x90000000) {
                            const char* s = (const char*)(mb + ptr);
                            if (s[0] >= 0x20 && s[0] <= 0x7E && s[1] >= 0x20 && s[1] <= 0x7E) {
                                char buf[80];
                                snprintf(buf, 80, " [%d]=\"%.40s\"", off, s);
                                info += buf;
                            }
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

    // Check for pending spawn request
    if (!g_SpawnRequest.pending) return;

    uint32_t gardenScene = ctx.r3.u32;
    if (gardenScene == 0) return; // Not in a garden

    uint32_t tagID = g_SpawnRequest.tagID;
    int variantIndex = g_SpawnRequest.variantIndex;
    g_SpawnRequest.pending = false;

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
    ctx.r9.u64 = 0;
    ctx.f1.f64 = 1.0;
    ctx.f2.f64 = 0.0;

    sub_82575AB8(ctx, base);

    uint32_t spawnedEntity = ctx.r3.u32;
    g_LastSpawnedEntity = spawnedEntity;

    if (spawnedEntity != 0) {
        Log("Spawned entity: " + std::to_string(spawnedEntity), 3);

        // Defer variant application by a few frames to let entity fully initialize
        if (variantIndex >= 0) {
            g_DeferredVariant.entity = spawnedEntity;
            g_DeferredVariant.variantIndex = variantIndex;
            g_DeferredVariant.framesRemaining = 5; // wait 5 frames
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