# Texture & Variant System — Technical Documentation

## Overview
Viva Piñata: TiP has multiple systems for piñata visual customization:
1. **Color Displacement** — shader-based color tinting (Choclodocus, Dragonache)
2. **Wildcard Variants** — rare breeding variants with unique visual traits
3. **Medal Tiers** — silver/gold/gold2/gold3/gold4 quality levels
4. **Texture Assets** — per-species texture files loaded from game data

---

## 1. Texture Asset Naming Convention

### Format
```
aid_texture_pinata_actor_animal_{species}_{variant}_{bodyPart}
```

### Known Species Names (internal → display)
```
worm = Whirlm               ant = Raisant
dragon = Dragonache          bee = Buzzlegum
zz3dinosaur = Choclodocus    fox = Pretztail
zz3tiger = Tigermisu         cat = Kittyfloss
lion = Roario                wolf = Mallowolf
```

### Choclodocus Texture Variants
```
_pink_face/body/tail    = Default (appears blue-ish)
_blue_face/body/tail    = Blue variant
_green_face/body/tail   = Green variant
_red_face/body/tail     = Red variant
_elite_face/body/tail   = Elite Neon (black + neon green zigzag) — ultra-rare
```

### Dragonache Texture Variants (terrain-based)
```
_dirt     = Brown (hatched on dirt/soil)
_gold     = Gold (hatched on golden paving)
_grass    = Green (hatched on short/long grass)
_water    = Blue (hatched in water)
_snow     = White (hatched on snow/ice)
_sand     = Red/Yellow (hatched on sand)
```

---

## 2. Save File Texture Storage

### Garden Saves (pinsavN/save.txt)
- Only **special piñatas** (Choclodocus, Dragonache) store texture variant names
- Regular piñatas load default textures based on species tag ID
- Texture strings: `aid_texture_pinata_actor_animal_{species}_{variant}_{part}\0` with null padding
- Patching: replace variant portion (e.g., `_pink_` → `_elite_`) keeping file size constant

### Profile (pinpro/profile.txt)
- Post office entries use format: `animal_resident_{medal}_{species}`
- Medal tiers: silver, gold, gold2, gold3, gold4, arctic, desert
- Medal is NOT a color variant — it's the piñata's quality/award level
- Post office data is serialized after the name string (variable length)

---

## 3. Runtime Texture System

### Structure Chain
```
Entity (PPC pointer from sub_82575AB8)
  → entity + some offset → glModel_s
    → glModel_s + 0x138 (312) → scenegraphInst_s sgInst
      → sgInst + 0x141 (321) → curVariantColourIndex (byte)
      → sgInst + 0x114 (276) → textureTable (ptr)
      → sgInst + 0x128 (296) → colourDisplacementTable (ptr)
```

### scenegraphInst_s Key Fields (PPC offsets)
| Offset | Size | Field |
|--------|------|-------|
| +0 | 4 | sg (dbScenegraph_s pointer) |
| +16 | 208 | blendShapeWeights[52] |
| +256 | 4 | solidColour (RGBA) |
| +260 | 4 | ambientOverride (RGBA) |
| +272 | 4 | textureBlendVal (float) |
| +276 | 4 | textureTable (ptr to scenegraphInstTextureTable_s array) |
| +280 | 4 | textureTableSize (int) |
| +296 | 4 | colourDisplacementTable (ptr) |
| +300 | 4 | colourDisplacementTableSize (int) |
| +319 | 1 | defaultColourIndex |
| +320 | 1 | specialAbilityColourIndex |
| **+321** | **1** | **curVariantColourIndex** |

### glModel_s Key Fields (PPC offsets)
| Offset | Size | Field |
|--------|------|-------|
| +0 | 4 | model (dbModel_s pointer) |
| +4 | 4 | modelExtras |
| +204 | 16 | switchStateArray (body part variants) |
| +220 | 16 | switchHitsStateArray |
| +236 | 16 | textureBlend (mlV4) |
| **+312** | var | **sgInst (scenegraphInst_s — embedded, not pointer)** |

### scenegraphInstTextureTable_s (per texture entry)
```c
struct scenegraphInstTextureTable_s {
    char* name;              // Texture name string
    dbTexture_s* currentTexture;  // Currently applied texture
    dbTexture_s* originalTexture; // Original/backup texture
};
```
Swapping `currentTexture` pointer changes the rendered texture without re-uploading.

### scenegraphInstColourDisplacement_s (per color variant)
```c
struct scenegraphInstColourDisplacement_s {
    char name[128];                    // Color variant name
    int doesTargetRequireBaseFurGain;  // Fur shader flag
    float blend;                       // Color blend amount
};
```

---

## 4. Color Displacement System (Choclodocus)

### Call Chain
```
sub_82649E98 (SparseCallback dispatcher at PPC 0x82649E98)
  → Type 0x13 handler at loc_8264A438
    → rex_gardenMainGetGarden_824E10D8() → garden data
    → sub_82546C50(gardenData, 0, r5, tag=29, 0, r9) → find Choclodocus entity
    → sub_825597C0(entity) → pre-color setup
    → sub_82559420(entity, colorIndex) → APPLY COLOR
      → sub_825595E0(entity, colorNames[0-5]) → write color channels
        → sub_82559548(entity, colorName) → per-channel color writer
```

### Color Indices
| Index | Choclodocus | Dragonache |
|-------|------------|------------|
| 0 | Blue | Gold (golden paving) |
| 1 | Green | Brown (dirt) |
| 2 | Red | Green (grass) |
| 3 | Elite Neon | Blue (water) |
| 4 | — | White (snow) |
| 5 | — | Red/Yellow (sand) |

### Color Apply Function: sub_82559420 (PPC 0x82559420)
- r3 = entity pointer (glModel_s)
- r4 = color variant index (0-3 for dino, 0-5 for dragon)
- Loads visual data at entity+2416
- Uses color index shifted left by 4 as table offset
- Calls sub_825595E0 with 6 color channel pointers at offsets:
  3088, 3224, 3360, 3496, 3632, 3768 from visual data base

---

## 5. Wildcard System

### Piñata Vision Barcode Commands
| TLV Type | Bits | Name | Description |
|----------|------|------|-------------|
| 01001 | 2 | Wildcard | Wildcard trait (0-3) |
| 01010 | 4 | Variant | Color variant (0-15) |
| 10000 | 16 | Sparse | Sparse callback (type:8, data:8) |

### How Wildcards Work
- Created during romance (breeding 7th piñata of same species)
- Each species has 3 wildcard variants with unique visual traits
- Wildcard piñatas have 10x base monetary value
- The `curVariantColourIndex` on `scenegraphInst_s` likely controls which variant
- Setting this byte to non-zero values should activate wildcard visuals

### Status: Partially Working
- Writing `curVariantColourIndex` directly crashes during entity creation
- Deferred write (10 frames later) also crashes
- Message 260 (sub_825885B0) doesn't visually affect regular piñatas
- The variant index may need to be set through the game's own initialization pipeline

---

## 6. Texture Loading Pipeline

```
SPAWN EVENT (sub_82575AB8)
  ↓
ENTITY FACTORY (sub_82575578)
  ↓ Load model asset via rex_assetIdPrintf → sub_82252108
  ↓
rex_dbTextureInitTexture (PPC 0x821FC308)
  ↓ For each texture unit in model:
  ├─ sub_827F9320 (type 0: standard texture init)
  ├─ sub_827F9400 (type 2: compressed/volume)
  └─ sub_827F94C8 (finalize texture, GPU upload)
  ↓
SCENEGRAPH INSTANCE CREATION
  ├─ textureTable[n].originalTexture = loaded texture ptr
  ├─ textureTable[n].currentTexture = same (can be swapped)
  ├─ colourDisplacementTable populated from model data
  └─ curVariantColourIndex = 0 (default)
```

### Hook Points for Custom Textures
1. **`rex_dbTextureInitTexture` (0x821FC308)** — intercept during load
2. **`textureTable[n].currentTexture` pointer swap** — after entity load (simpler)
3. **`colourDisplacementTable` modification** — add custom color entries

---

## 7. Medal/Award Tiers in Post Office

| Tier | Internal Name | Species Examples |
|------|--------------|-----------------|
| silver | animal_resident_silver | Whirlm, butterflies, sparrow |
| gold | animal_resident_gold | Raisant, Sherbat, Barkbark |
| gold2 | animal_resident_gold2 | Sweetooth, Doenut, Horstachio |
| gold3 | animal_resident_gold3 | Bonboon, Elephanilla, Chewnicorn |
| gold4 | animal_resident_gold4 | Dragonache, Roario, Choclodocus, Tigermisu |
| arctic | animal_resident_arctic | Arctic region variants |
| desert | animal_resident_desert | Desert region variants |

---

## 8. Proven Techniques

### Save File Texture Patching (works for Choclodocus/Dragonache)
1. Save game, close game fully
2. Open `pinsavN_*/save.txt` in binary editor
3. Find `aid_texture_pinata_actor_animal_{species}_{color}_{part}`
4. Replace color string (pad with nulls if shorter, must not exceed available padding)
5. Relaunch game, load garden

### Automated via patch_elite_choclodocus.pl
```
perl patch_elite_choclodocus.pl path/to/save.txt
```
Changes any Choclodocus color to Elite Neon. Creates .bak backup.

---

## 9. Open Questions

1. **Where is glModel_s relative to entity pointer?** Entity+0x110 found in one dump but not confirmed stable
2. **Does writing curVariantColourIndex actually change visuals?** Crashes during/after creation suggest timing issue
3. **What textures exist per species?** Game asset database needs enumeration
4. **Can we load custom .dds textures at runtime?** Hook rex_dbTextureInitTexture and provide override path
5. **Xbox 360 texture tiling** — do custom textures need to be tiled/swizzled?
