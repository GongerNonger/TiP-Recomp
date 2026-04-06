# Piñata Vision Spawn Menu Mod for TiP-Recomp

## Overview
This mod adds a Piñata Vision spawn menu (F8) to Viva Piñata: Trouble in Paradise via the TiP-Recomp project. It includes a species spawner, barcode injection system, and species scanner.

## Features

### Spawn Menu (F8)
- Search and category filtering (Animals, Eggs, Seeds, Homes, Props, Trees)
- Color-coded species list with 90+ verified entries
- 16 TiP-exclusive piñatas correctly identified
- Variant/Wildcard slider (experimental)
- Dino Color slider (0=Blue, 1=Green, 2=Red, 3=Elite Neon)
- Species ID scanner (dumps to species_scan.txt)

### Barcode Injection System
- C++ port of pv_decoder.pl barcode deobfuscation
- Preset buttons for Choclodocus cards (Elite Neon, Blue, Green, Red, Egg, Place, Journal)
- Manual hex input with real-time decode preview
- Handles PlaceTag, SparseCallback, Variant, Wildcard, Reuse commands

## How to Use

### Spawning Piñatas
1. Press **F8** to open the spawn menu
2. Search or browse for a species
3. Click **Spawn Selected**
4. Walk around the garden to trigger the spawn

### Barcode Injection
1. Press **F8** to open the spawn menu
2. Scroll to the "Pinata Vision Barcode Inject" section
3. Click a preset button OR type a 16-character hex barcode
4. Check the decode preview at the bottom
5. Click **Inject**
6. Walk around the garden to trigger

## Key Technical Details

### Entity Creation
- Uses `sub_82575AB8` (the real entity factory), NOT `sub_824CB440` (which is a cutscene trigger)
- Spawns happen via a hook on `gardenMainGetGardenScene` (runs during game logic tick)
- Species validation via `sub_825A0818` prevents crashes from invalid IDs

### Choclodocus
- Tag ID 29 = body/torso only (incomplete)
- Tag ID 208 = egg (hatches into FULL Choclodocus)
- Dino bones: 720=red, 721=green, 722=blue, 723=skull, 724=ribs, 725=spine
- Internal name: `zz3dinosaur`

### Dragonache
- Tag ID 32 = spawns with metallic gold texture (uninitialized variant data)
- Color normally set by terrain during egg hatching (gold paving, grass, water, snow, sand, dirt)
- 20,000+ possible combinations (6 colors × random body parts)

## Files Modified
- `src/tip_engine/Overlays/SpawnMenu.h` — Spawn request structs, deferred variant system
- `src/tip_engine/Overlays/SpawnMenu.cpp` — ImGui spawn menu UI
- `src/tip_engine/Overlays/BarcodeInjector.h` — PV barcode decoder (ported from Perl)
- `src/tip_engine/hooks.cpp` — Spawn hook, barcode injection handler, species scanner
- `src/tip_engine/Types/VivaClassTypes.h` — Species ID list
- `src/retip_app.h` — SpawnMenuDialog registration
- `CMakeLists.txt` — SpawnMenu.cpp added to build
- `retip_config.toml` — Named function entries

## Known Issues
- Choclodocus body (ID 29) spawns as torso only — use egg (ID 208) for full body
- Dino color (Elite Neon) not yet working — hatching ignores color writes
- Dragonache spawns with metallic gold (uninitialized) — resets to yellow if crated
- Variant/Wildcard slider doesn't visually affect piñatas yet
- Some TiP spare entries still unidentified (~10 remaining)
