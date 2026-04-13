#pragma once

#include <string>
#include <vector>
#include <cstring>
#include "imgui.h"
#include "tip_engine/Overlays/Fps.h"
#include <rex/ppc/context.h>

inline FPSManager fpsManager;

// Wildcard breeding hook — called from patched retip_recomp.19.cpp
void wildcardHatchCheck(PPCContext& ctx, uint8_t* base);