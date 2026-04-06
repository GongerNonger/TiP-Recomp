# Dragonache Variant System — Technical Documentation

## Overview
The Dragonache (tag 32, Animal_dragon) uses a **terrain-based color system** distinct from the Choclodocus. It has 6 colors determined by where the egg hatches, plus 5 randomized body parts (teeth, mane, wings, tail, ridges) with multiple variants each, producing **20,000+ unique combinations**.

---

## Color System

### 6 Terrain-Based Colors
| Index | Save Name | Terrain | Visual |
|-------|-----------|---------|--------|
| 0 | `_dirt` | Dirt/brown soil | Brown |
| 1 | `_gold` | Golden paving | Gold |
| 2 | `_grass` | Short/long grass | Green |
| 3 | `_water` | Water | Blue |
| 4 | `_snow` | Snow/ice | White |
| 5 | `_sand` | Sand | Red/Yellow |

### How Color Is Set
**Unlike the Choclodocus**, the Dragonache stores its color as a **string** at offset **+2952** within the species data structure (`[piñata+2416]+2952`), not a numeric index.

#### During Egg Hatching (retip_recomp.12.cpp line 71597):
```
if (tag == 32) {  // Dragonache
    if (eggData[4868] != 0) {
        // Piñata Vision preset: load body parts from eggData[4872-4877]
    } else {
        // Natural hatch:
        sub_825599D0(entity)           // Randomize 5 body parts
        terrainType = sub_821EBC08()   // Get terrain at egg position
        sub_82559D88(entity, terrainType)  // Set color from terrain
    }
} else if (tag == 29) {  // Choclodocus
    // Different path — uses sub_82559420 with numeric color index
}
```

### Key Color Functions

| PPC Address | Function | Purpose |
|-------------|----------|---------|
| `sub_82559D88` | Terrain → Color | Takes terrain byte, looks up color string, writes to offset 2952 |
| `sub_82559E98` | Apply Color | Reads color string from +2952, resolves texture, applies to model |
| `sub_82559E20` | Color → Index | Reverse lookup: compares color string against table, returns 0-10 |

### Terrain Lookup Table
At PPC address `0x84196E40`: 9 entries × 8 bytes each. Maps terrain type byte to color string pointer.

---

## Body Part System (5 Parts × Multiple Variants)

### Randomization Function: `sub_825599D0` (PPC 0x825599D0)
- Loops 5 times (body part indices 1-5)
- For each part, counts available variants from a 9-entry table at offset 24432
- Uses **Linear Congruential Generator** PRNG: `next = prev * 1664525 + 1013904223`
- Generates random float 0.0–1.0, maps to variant index
- Calls `sub_82559908` to apply each part

### Body Part Indices
| Index | Body Part |
|-------|-----------|
| 1 | Teeth |
| 2 | Mane |
| 3 | Wings |
| 4 | Tail |
| 5 | Ridges (back spines) |

### Single Part Setter: `sub_82559908` (PPC 0x82559908)
- r3 = piñata object
- r4 = body part index (1-5)
- r5 = variant value
- For variants 1-3: calls `sub_821E9EE0` to set model switch state (mesh visibility)
- For variant 0: clears the 2-bit field via bit masking

### All Parts Setter: `sub_82559AD8` (PPC 0x82559AD8)
- Wrapper that calls `sub_82559908` for indices 1-5 sequentially
- Parameters: r3=entity, r4=teeth, r5=mane, r6=wings, r7=tail, r8=ridges

### Body Part Storage in glModel_s
Stored as **2-bit values** packed into bytes 212-215 of glModel:
- **Byte 214, bits 4-5**: Teeth variant
- **Byte 214, bits 6-7**: Mane variant  
- **Byte 215, bits 0-1**: Wings/Tail variant

Game variables 9235, 9236, 9237 receive these values for script access.

---

## Piñata Vision Preset System

When a Dragonache is created via Piñata Vision card:
- `eggData[4868]` is set to non-zero (flag: "barcode preset")
- `eggData[4872-4877]` contain 6 bytes of preset body part variant values
- The hatching code reads these instead of randomizing
- Color is ALSO preset (not terrain-dependent when barcode sets it)

### Dragonache Barcodes
| Barcode | Purpose |
|---------|---------|
| `E07B74121E088AE0` | PlaceTag dragon (ID 32) |
| `F1706B687562A38F` | PlaceTag Egg dragon (ID 211) |
| `A5F8F4664F0E5792` | PlaceTag Sweet dragon (ID 1693) |

**No color variant barcodes exist for Dragonache** — unlike Choclodocus which has SparseCallback type 0x13 cards for Blue/Green/Red/Elite. Dragonache color is solely terrain-based.

---

## Spawning a Custom Dragonache

### Via Egg + Save Patching (PROVEN)
1. Spawn Dragonache egg (tag 208... actually tag 211 from barcodes)
2. Hatch on desired terrain for color
3. Save game, close game
4. Patch save: replace `_dirt` with `_gold`/`_grass`/`_water`/`_snow`/`_sand`
5. Reload — Dragonache has new color

### Via Direct Function Calls (RESEARCH)
To set color at runtime:
```cpp
// Set terrain-based color
sub_82559D88(entity, terrainType);  // terrainType = byte matching lookup table

// Or set all 5 body parts
sub_82559AD8(entity, teeth, mane, wings, tail, ridges);
```

### The Gold Metallic Dragonache
When spawned via our spawner (`sub_82575AB8`), the Dragonache gets:
- **No color string** at +2952 (uninitialized)
- **No body parts** set (switchStateArray all zeros)
- Result: raw base model material appears as metallic gold
- Crating/uncrating triggers serialization which resets to default (yellow/brown)

---

## Entity Data Structure

### Key Offsets from Piñata Object
| Offset | Purpose |
|--------|---------|
| +132 | Species tag ID (32 = Dragonache) |
| +596 | Pointer to glModel_s |
| +2416 | Pointer to species data structure |

### Species Data Offsets (from [piñata+2416])
| Offset | Purpose |
|--------|---------|
| +2952 | Color string ("dirt", "gold", "grass", "water", "snow", "sand") |
| +3088 | Texture path buffer 1 (136 bytes) |
| +3224 | Texture path buffer 2 (136 bytes) |
| +3360 | Texture path buffer 3 (136 bytes) |
| +3496 | Texture path buffer 4 (136 bytes) |
| +3632 | Texture path buffer 5 (136 bytes) |
| +3768 | Texture path buffer 6 (136 bytes) |

### glModel_s Offsets
| Offset | Purpose |
|--------|---------|
| +212-215 | Body part variant bitfields (2 bits each, packed) |
| +576 | Dragon special coloring flag (byte) |

### Egg Data Offsets (from garden controller / r22)
| Offset | Purpose |
|--------|---------|
| +4868 | Piñata Vision preset flag (non-zero = barcode preset) |
| +4872-4877 | Preset body part variant bytes (6 bytes) |

---

## Comparison: Dragonache vs Choclodocus

| Feature | Dragonache | Choclodocus |
|---------|-----------|-------------|
| Tag ID | 32 | 29 |
| Color system | String-based (`_dirt`, `_gold`, etc.) | Index-based (0-3) |
| Color source | Terrain at hatch | Bones used in assembly |
| # Colors | 6 | 4 (+1 Elite) |
| Body parts | 5 randomized (teeth, mane, wings, tail, ridges) | None (body determined by bones) |
| Color function | `sub_82559D88` (terrain→string) | `sub_82559420` (index→table lookup) |
| Apply function | `sub_82559E98` (string→texture) | `sub_825595E0` (channels→vertex data) |
| PV color cards | None | 4 (Blue, Green, Red, Elite Neon) |
| Save storage | `_dragon_{color}` texture string | `_dinosaur_{color}_{part}` texture strings |

---

## Implementation Plan: Dragonache Editor

### Phase 1: Color via Save Patching (EASY, already works)
- Use SavePatcher to replace `_dirt` with other variants
- Add Dragonache variants to the save patcher dropdown

### Phase 2: Body Part Control via Game Functions (MEDIUM)
1. After spawning, call `sub_82559AD8(entity, t, m, w, tail, r)` to set all 5 body parts
2. Call `sub_82559D88(entity, terrainType)` to set color
3. Both functions need the entity pointer from sub_82575AB8
4. Must be called AFTER entity is fully initialized (deferred)

### Phase 3: Dragonache Editor UI (from the spawn menu)
```
[Dragonache Editor]
Color: [dropdown: Dirt/Gold/Grass/Water/Snow/Sand]
Teeth: [slider 0-3]
Mane:  [slider 0-3]
Wings: [slider 0-3]
Tail:  [slider 0-3]
Ridges:[slider 0-3]
[Spawn Custom Dragonache]
```
