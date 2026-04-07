# Session Handoff — Complete State of TiP-Recomp Spawn Menu Mod

## Project Overview
A mod for Viva Piñata: Trouble in Paradise via the TiP-Recomp (SolarCookies) static recompilation project. Adds Piñata Vision spawn menu, barcode injection, texture tools, and species management.

**Repo:** https://github.com/GongerNonger/TiP-Recomp (fork of SolarCookies/TiP-Recomp)

---

## What Works (Shippable Features)

### Spawn Menu (F8)
- 90+ verified species with search and category filtering
- 16 TiP-exclusive piñatas correctly identified from Piñata Vision barcode database
- Amber Gem (719), Wishing Well (798), Dino Bones (720-725) in spawn list
- Species validation via `sub_825A0818` prevents crashes from invalid IDs
- Spawns via `sub_82575AB8` (the real entity creation function, NOT sub_824CB440 which is a cutscene trigger)
- Spawning hooks into `gardenMainGetGardenScene` for game-logic-safe execution

### Elite Neon Choclodocus (ACHIEVED)
- Save file texture patching: replace `_pink_`/`_blue_`/`_green_`/`_red_` with `_elite_` in save.txt
- Automated patch script (patch_elite_choclodocus.pl)
- Save Patcher UI in spawn menu (works when game is fully closed before patching)
- Full Choclodocus from egg hatching (tag 208)
- Choclodocus body only from tag 29

### Barcode Injection System
- C++ port of pv_decoder.pl (verified exact match with Perl decoder)
- Handles PlaceTag, SparseCallback, Variant, Wildcard, Reuse commands
- Preset buttons: Elite Neon, Dino colors, Dino Egg, Place Dino, Whirlm variants, Amber, Wishing Well
- Searchable wildcard barcode dropdown (268 entries from barcodes.txt)
- Decode preview shows what each barcode does before injection

### Other Features
- Trick triggering on mature piñatas (sub_82382328)
- Dino Color slider (writes to global 0x83DBECB5)
- Variant slider and Wildcard slider (experimental)
- Species ID scanner (dumps to species_scan.txt)
- File system hook (rex_fsOpenFile) for logging file access

---

## What Partially Works

### Texture Init Hook
- `rex_dbTextureInitTexture` hooked via PPC_WEAK_FUNC override
- Calls original FIRST (critical — calling after or probing memory crashes)
- Captures texture address, format, dimensions, imageDataStart, sizeOfOneFrame
- Asset name correlation via `rex_assetIdPrintf` hook (captures stale names)
- Asset lookup hook on `sub_821D1C10` (maps some texture addrs → names)
- **93+ textures captured without crashing**

### Texture Dump
- DDS export with correct headers (DXT1/DXT3/DXT5/DXN/A8R8G8B8)
- TextureTools.h: CapturedTexture struct, DDS header builder, dump functions
- "Start Texture Capture" → spawn → "Stop & Dump" workflow
- Files saved to C:/Users/Administrator/Downloads/texture_dump/

### Xbox 360 De-tiling
- Ported crunch algorithm from rexglue SDK source (TiledOffset2DRow/Column)
- **DXT1 128x128 textures de-tile correctly** (recognizable game graphics!)
- DXT3/DXT5 and 512x512+ textures still scrambled
- XenosTiling.h has the Xenia-based algorithm (also tested, same results)
- The issue: larger textures may need different pitch alignment or the Xenos bank/pipe interleaving

### Texture Swap
- Swap tool in UI (enter two indices, click Swap)
- Swaps imageDataStart pointers + resets currentFrameLoaded
- **Does NOT produce visible changes** — GPU has cached copy, PPC swap doesn't propagate

---

## What Doesn't Work

### Runtime Variant Colors for Regular Piñatas
- Message 260 (sub_825885B0) doesn't visually affect regular piñatas
- Direct curVariantColourIndex write crashes (NULL colourDisplacementTable)
- sub_82382328 triggers TRICKS, not color changes
- entity+3636 + state 1093 triggers sparkle particles but no actual color change (wrong state pointer — the strategy controller is NOT at entity+2564)
- Eat system color change requires full pipeline (item matching, recipes)
- Passing wildcard via r9 to sub_82575AB8 produces default piñatas (no effect)

### Amber Event
- Event 9113 trigger doesn't spawn amber (needs Wishing Well entity context)
- Amber Gem tag 719 added to spawn list but untested via direct spawn

---

## Key Technical Discoveries

### Entity Creation
- `sub_824CB440` = **cutscene/notification trigger** (NOT spawner!) — causes "garden will be lost" dialog
- `sub_82575AB8` = **real entity creation** function
- Entity + 0x110 → glModel_s (confirmed stable across multiple spawns)
- glModel_s + 0x138 → scenegraphInst_s region (but PPC offsets don't match C++ header)
- model ptr always 0x821DEC78 for Whirlm (static data in code region)

### Spawn Chain
```
sub_82575AB8 (entity creation)
  → sub_82575E50 (type dispatch)
    → sub_82575578 (asset factory)
  → sub_825885B0 (message 260, variant — called with r25 from stack[92])
```

### Choclodocus Color System
```
sub_82649E98 (SparseCallback dispatcher)
  → Type 0x13 handler at loc_8264A438
    → sub_82546C50 (find Choclodocus in garden)
    → sub_825597C0 (pre-color setup)
    → sub_82559420(entity, colorIndex) → apply color
```
Colors: 0=Blue, 1=Green, 2=Red, 3=Elite Neon
Save names: `_pink_`, `_blue_`, `_green_`, `_red_`, `_elite_` (face/body/tail)

### Dragonache System
- 6 terrain-based colors (dirt/gold/grass/water/snow/sand)
- Color stored as STRING at species_data+2952
- 5 body parts: teeth, mane, wings, tail, ridges (randomized via LCG PRNG)
- Functions: sub_82559D88 (terrain→color), sub_825599D0 (random parts), sub_82559AD8 (set all parts)

### Eat → Color Change Path (for regular piñatas)
```
sub_8243A6B8 (eat/consume)
  → sub_82559F98 (check variant trigger — returns new variant index)
  → entity+3636 = variant index
  → strategy state 1093 at strategy_controller+156 (NOT entity+2564!)
  → sub_82454A78 (sparkle particles)
  → sub_82454B18 (variant execution)
  → sub_8255A3A8 (texture swap on scenegraph)
```
The strategy controller is r24 in the eat function — a SEPARATE object from the entity. We don't have its pointer.

### Eat → Trick Path
```
sub_8243A6B8 (eat/consume)
  → sub_8255CC10 (check trick trigger — returns 1 or 2)
  → sub_82382328(speciesData, trickIndex, 1) — triggers trick animation
```

### Asset/Texture Loading
- Textures packed in `debug_pack.bin` (1GB archive), NOT individual files
- `rex_fsOpenFile` only intercepts .pkg fallback files, not per-texture loads
- Assets identified by 8-byte name hash, looked up in debug_hash.bin index
- Texture init hook captures textures but can't easily identify piñata-specific ones
- `sub_821D1C10` maps some asset IDs → data pointers (hooked for name correlation)

### Save File Format
- Profile (pinpro/profile.txt): journal entries like `animal_variant2_silver_worm` (NOT post office)
- Garden saves (pinsavN/save.txt): binary entity data, texture variant strings for Choclodocus/Dragonache
- Post office data: dense binary in profile, format not fully decoded
- Headers folder required for profile detection (missing = "select memory device" error)

### Piñata Vision Barcode Format
```
TLV commands (5-bit type):
00001 = PlaceTag (12-bit entity ID)
01001 = Wildcard (2-bit trait 0-3)
01010 = Variant (4-bit color 0-15)
10000 = SparseCallback (8-bit type + 8-bit data)
10110 = Reuse (5-bit)
```
Whirlm variants: values 3, 4, 5 (and 15=Black)
Whirlm wildcards: values 1, 2, 3

---

## Files Modified

### Source Files
| File | Purpose |
|------|---------|
| `src/tip_engine/hooks.cpp` | Main hooks: spawn, barcode injection, texture capture, variant system |
| `src/tip_engine/Overlays/SpawnMenu.cpp` | ImGui UI: spawn menu, barcode presets, texture tools, save patcher |
| `src/tip_engine/Overlays/SpawnMenu.h` | SpawnRequest struct, DeferredVariant, globals |
| `src/tip_engine/Overlays/BarcodeInjector.h` | PV barcode decoder (C++ port of pv_decoder.pl) |
| `src/tip_engine/Overlays/SavePatcher.h` | Save file texture variant scanner/patcher |
| `src/tip_engine/TextureTools.h` | DDS export, texture capture structs |
| `src/tip_engine/XenosTiling.h` | Xbox 360 Xenos GPU de-tiling (from Xenia) |
| `src/tip_engine/Types/VivaClassTypes.h` | Species ID list (90+ entries) |
| `src/retip_app.h` | SpawnMenuDialog registration + XUsbcam stubs |
| `CMakeLists.txt` | SpawnMenu.cpp added to RETIP_SOURCES |
| `retip_config.toml` | Named functions for spawn system |

### Documentation Files
| File | Purpose |
|------|---------|
| `SPAWN_MENU_README.md` | User guide |
| `RESEARCH_NOTES.md` | Session 1 technical findings |
| `TEXTURE_VARIANT_SYSTEM.md` | Complete texture/variant documentation |
| `TEXTURE_OVERRIDE_PLAN.md` | Texture override implementation plan |
| `WILDCARD_SYSTEM.md` | Wildcard system analysis |
| `DRAGONACHE_SYSTEM.md` | Dragonache variant system |
| `FUTURE_FEATURES.md` | Roadmap |
| `WILDCARD_HANDOFF.md` | Wildcard changes from parallel session |

---

## Build Instructions
```bash
NINJA_DIR="/c/Users/Administrator/AppData/Local/Microsoft/WinGet/Packages/Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe"
export PATH="/c/Program Files/CMake/bin:/c/Program Files/LLVM/bin:$NINJA_DIR:$PATH"
cd /c/Users/Administrator/Downloads/TiP-Recomp
cmake --preset win-amd64-relwithdebinfo  # configure (first time only)
cmake --build --preset win-amd64-relwithdebinfo  # build
cp out/build/win-amd64-relwithdebinfo/retip.exe /c/Users/Administrator/Downloads/reTiP/reTiP/Game/reTiP.exe  # deploy
```

## Key Paths
- Game: `C:\Users\Administrator\Downloads\reTiP\reTiP\Game\`
- Saves: `C:\Users\Administrator\Documents\retip\B13EBABEBABEBABE\4D53085F\00000001\`
- Backup: `C:\Users\Administrator\Documents\retip_save_backup\`
- Barcode DB: `C:\Users\Administrator\Downloads\barcodes.txt`
- PV Decoder: `C:\Users\Administrator\Downloads\pv_decoder.pl`
- Texture dumps: `C:\Users\Administrator\Downloads\texture_dump\`
- SDK: `C:\Users\Administrator\Downloads\rexglue-sdk\`

---

## Priority Next Steps

### 1. Fix Wildcard Spawning (Highest Priority)
The r9 parameter approach doesn't work. Best remaining approaches:
- **Hook the egg hatching pipeline** — find PPC equivalent of PC port's `sub_795FF0` (wildcard probability multiplier). Override to return 10000 for guaranteed wildcards.
- **Find the strategy controller pointer** — the color change needs `strategy_controller+156 = 1093`, but we need the controller's address (it's the first parameter to `sub_8243A6B8`).

### 2. Texture Override System
- The texture init hook works and captures data
- Swapping PPC-side pointers doesn't affect GPU (GPU has cached copy)
- Need to either: invalidate GPU texture cache, or hook at the D3D12 upload level
- The rexglue SDK's `TextureCache::LoadTextureDataFromResidentMemoryImpl` handles GPU upload
- Or: hook the shared memory system to mark pages dirty after swap

### 3. De-tiling Refinement
- DXT1 128x128 works via crunch algorithm from rexglue source
- Larger textures and DXT3/DXT5 still scrambled
- May need to read pitch from the GPU's fetch constant (d3dHeader in dbTexture_s)
- Or hook at GPU upload point where data is already de-tiled
