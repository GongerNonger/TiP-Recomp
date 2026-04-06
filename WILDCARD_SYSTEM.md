# Wildcard Piñata System — Technical Documentation

## Overview
Wildcards are rare cosmetic variants of standard piñatas. Each species has 3 wildcard variants (values 1-3, with 0 = no wildcard). They are visually distinct with unique traits (fangs, mane, bushy tail, etc.) and have **10x monetary value**. They can be obtained through breeding or Piñata Vision cards.

---

## How Wildcards Are Created

### Breeding (Normal Gameplay)
1. Romance (breed) the same species multiple times
2. On the **7th romance**, the romance mini-game becomes harder
3. If the player collects **all hearts before time runs out**, the offspring becomes a wildcard
4. The wildcard trait (1, 2, or 3) determines which visual variant
5. To get all 3 traits: breed naturally (trait 1), romance two wildcards with flashing hearts (trait 2), trading/PV cards (trait 3)

### Internal Mechanism
- A **wildcard probability multiplier** function is called during egg hatching
- PC port equivalent: `sub_795FF0` (called from `Pinata_HatchEggAndSpawnOffspring_4dedd0`)
- Returns a multiplier that scales with breed count
- The cheat menu overrides this to return 10000 (guaranteed wildcard)
- The species definition has "WildcardParams" that define available traits

### Piñata Vision Cards
- **263 wildcard barcodes** exist across all species
- Each species typically has 3 Wildcard cards (one per trait)
- Barcode TLV type: `01001` (CMD_WILDCARD), 2-bit payload (0-3)
- Example: Whirlm Wildcard1, Whirlm Wildcard3, Whirlm "2nd wildcard"

---

## Visual Rendering

### Key Field: `curVariantColourIndex`
- Location: `scenegraphInst_s` offset **+321** (0x141)
- Type: `unsigned char`
- Values: 0 = default, 1-3 = wildcard variants
- Adjacent fields:
  - `+319`: `defaultColourIndex` (byte)
  - `+320`: `specialAbilityColourIndex` (byte)
  - `+321`: **`curVariantColourIndex`** (byte)

### From Entity to curVariantColourIndex
```
Entity pointer (from sub_82575AB8)
  → Entity + 0x110 → glModel_s pointer
    → glModel_s + 0x138 (312) → scenegraphInst_s (embedded)
      → scenegraphInst_s + 0x141 (321) → curVariantColourIndex
Total: glModel + 0x279 (633)
```

### Color Application Function: `sub_82559420` (PPC 0x82559420)
- r3 = entity (glModel_s pointer)
- r4 = color variant index
- Uses color index * 16 as offset into a lookup table
- Calls `sub_825595E0` with 6 color channel pointers
- The color channels modify vertex/material data on the entity

### Message 260 System: `sub_825885B0` (PPC 0x825885B0)
- r3 = target entity
- r4 = variant value to set
- Constructs message ID 260 and dispatches through entity's vtable
- Entity's message handler calls into the variant system

---

## Piñata Vision Barcode Commands

### Wildcard Command (Type 9)
```
TLV: 01001 (2 bits payload)
Values: 0 = none, 1-3 = wildcard trait
Decoded: CMD_WILDCARD
```

### Variant Command (Type 10)
```
TLV: 01010 (4 bits payload)
Values: 0-15 = color variant
Decoded: CMD_VARIANT
```

### SparseCallback Command (Type 16)
```
TLV: 10000 (16 bits payload)
Upper 8 bits = callback type
Lower 8 bits = callback data

Known callback types:
  0x00-0x06 = Cases 0-6 (general operations)
  0x03 = Type 3 handler
  0x10 = Float-based parameter
  0x11 = Species-indexed operations (up to 45 species)
  0x13 = Choclodocus/Dragonache color change
  0xF0 = Garden data modification
```

---

## Save File Storage

### Profile (pinpro/profile.txt)
- Post office entries: `animal_resident_{medal}_{species}`
- Medal tiers: silver, gold, gold2, gold3, gold4, arctic, desert
- Medal tiers are NOT wildcard indicators — they're quality/award levels
- Wildcard status stored separately in serialized entity data

### Garden Saves (pinsavN/save.txt)
- Wildcard status stored as part of the entity's serialized scenegraph data
- The `curVariantColourIndex` byte is serialized with the entity
- Special piñatas (Choclodocus, Dragonache) also store texture variant name strings

---

## Current Implementation Status

### What Works
- Barcode decoder handles Wildcard (type 9) and Variant (type 10) commands
- Both dispatch through `sub_825885B0` (message 260)
- **Choclodocus color changes work** via the SparseCallback type 0x13 handler
- Save file texture patching works for Choclodocus Elite Neon

### What Doesn't Work Yet
- **Message 260 doesn't visually affect regular piñatas**
  - Regular piñatas have NULL `colourDisplacementTable`
  - The color displacement system only works for special piñatas
- **Direct `curVariantColourIndex` writes crash**
  - Writing during entity creation: crashes immediately
  - Writing after creation (deferred): still crashes
  - Likely because NULL colourDisplacementTable causes index-out-of-bounds
- **Wildcards require initialization during entity creation**
  - The variant must be set DURING the spawn/hatch pipeline
  - Post-spawn application doesn't work for regular species

### Possible Approaches for Regular Piñata Wildcards

1. **Hook the egg hatching pipeline**
   - Force the wildcard probability to 100%
   - PC equivalent: override `sub_795FF0` to return 10000
   - Need to find the PPC equivalent function

2. **Set variant during entity creation (sub_82575AB8)**
   - The `r9` parameter to sub_82575AB8 is stored at stack[84]
   - This is passed as `r25` to sub_825885B0 (message 260) DURING creation
   - If we pass a non-zero r9, the entity might be created as a wildcard
   - Currently r9=0 in our spawner

3. **File system texture override**
   - Hook `rex_fsOpenFile` to redirect texture file reads
   - Replace default species texture with a wildcard variant texture
   - Works regardless of entity state

4. **Save file modification**
   - Identify where `curVariantColourIndex` is in the serialized save data
   - Patch the byte to 1, 2, or 3
   - Reload the save — entity reconstructs with wildcard variant

---

## Key Functions

| PPC Address | Function | Purpose |
|-------------|----------|---------|
| 0x82559420 | sub_82559420 | Apply color variant to entity |
| 0x825595E0 | sub_825595E0 | Color displacement applicator (6 channels) |
| 0x82559548 | sub_82559548 | Per-channel color writer |
| 0x825885B0 | sub_825885B0 | Message 260 sender (set variant) |
| 0x82588610 | sub_82588610 | Message 259 sender (query variant) |
| 0x82649E98 | sub_82649E98 | SparseCallback dispatcher |
| 0x8264A438 | loc_8264A438 | Type 0x13 handler (dino color) |
| 0x82575AB8 | sub_82575AB8 | Entity creation (r9 may control variant) |
| 0x82546C50 | sub_82546C50 | Find piñata in garden by tag ID |

---

## Barcode Database Statistics
- **263** wildcard barcodes across all species
- **374** variant barcodes across all species
- Each species typically has 3 Wildcard + 3 Variant cards
- `actorStrategyId_PinataPestRevertVariant` (0x488) confirms pests can revert wildcards
