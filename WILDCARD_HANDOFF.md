# Wildcard Implementation Handoff

## What Was Changed

### hooks.cpp — Two edits to pass wildcard values during entity creation

**1. Barcode injection path (~line 953):**
Changed from:
```cpp
ctx.r9.u64 = barcodeVariant;
```
To:
```cpp
ctx.r9.u64 = barcodeWildcard > 0 ? barcodeWildcard : barcodeVariant;
```
This makes barcode injection use the wildcard value (from CMD_WILDCARD) when present, falling back to variant.

**2. Manual spawn path (~line 1316):**
Changed from:
```cpp
ctx.r9.u64 = 0; // variant applied post-creation via eat system, not at spawn
```
To:
```cpp
ctx.r9.u64 = wildcardTrait;
```
This passes the wildcard slider value via r9 during entity creation.

### SpawnMenu.cpp — Searchable wildcard barcode dropdown

- Added `#include <algorithm>` and `#include <cctype>`
- Added `WildcardEntry` struct and `g_WildcardDB[]` array with 268 wildcard barcodes parsed from barcodes.txt
- Replaced individual wildcard preset buttons with a searchable dropdown:
  - Text filter input ("Search species...")
  - Scrollable child window (120px) showing filtered results
  - Clicking an entry loads the barcode hex into the inject field

## What Doesn't Work

**Passing wildcard trait via r9 to sub_82575AB8 does NOT produce wildcard piñatas.** Tested with the wildcard slider set to 1, 2, and 3 — all four spawned Whirlms were default appearance. r9 alone is not enough.

## Build Note

The last build compiled successfully (`cmake --build --preset win-amd64-relwithdebinfo`) but **crashed on launch**. The crash may be related to the new code or a dependency issue. The backup exe was restored from `C:\Users\Administrator\Desktop\reTiP_Game_Backup\reTiP.exe`.

## What To Try Next

From WILDCARD_SYSTEM.md, these approaches remain:

### 1. Hook the egg hatching pipeline (most promising)
- A wildcard probability multiplier function is called during egg hatching
- PC port equivalent: `sub_795FF0` (called from `Pinata_HatchEggAndSpawnOffspring_4dedd0`)
- The cheat menu overrides this to return 10000 (guaranteed wildcard)
- Need to find the PPC equivalent function address
- Hooking this would make bred piñatas hatch as wildcards when the slider is set

### 2. Set curVariantColourIndex after entity fully initializes
- Location: scenegraphInst_s offset +321 (0x141)
- Previous attempts crashed because colourDisplacementTable is NULL for regular piñatas
- Might work if done late enough in the initialization (deferred by N frames?)

### 3. Barcode injection with game's original PV handler
- The wildcard barcodes work on real Xbox 360 hardware via the camera
- The game's original PV processing code knows how to create wildcards from barcodes
- Instead of reimplementing the spawn, could we call the game's actual barcode processing function?
- Look for the function that processes scanned PV card data end-to-end

### 4. Save file modification
- Patch curVariantColourIndex byte in serialized entity data in save.txt
- SavePatcher.h already has infrastructure for scanning/patching save files
- This worked for Choclodocus Elite Neon texture variants

## Key Files
- `src/tip_engine/hooks.cpp` — spawn logic and barcode injection
- `src/tip_engine/Overlays/SpawnMenu.cpp` — UI with wildcard dropdown
- `src/tip_engine/Overlays/SpawnMenu.h` — SpawnRequest struct (has wildcardTrait field)
- `src/tip_engine/Overlays/BarcodeInjector.h` — PV barcode decoder (CMD_WILDCARD = type 9)
- `WILDCARD_SYSTEM.md` — full technical documentation
- `C:/Users/Administrator/Downloads/barcodes.txt` — 271 wildcard barcodes database
