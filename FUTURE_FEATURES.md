# Future Features & Research Notes

## Priority 1: Custom Piñata Variants / Texture System

### What We Know
- Each piñata species has a `colourDisplacementTable` on its `scenegraphInst_s`
- `curVariantColourIndex` (byte) selects which color variant is active
- Color variants are applied via `sub_825595E0` which takes 6 color channel pointers
- The channels are at offsets 3088, 3224, 3360, 3496, 3632, 3768 from visual data base
- Wildcards are piñatas with non-default `curVariantColourIndex` values
- Each species has 3 wildcard variants in the base game

### Texture Naming Convention (from save files)
```
aid_texture_pinata_actor_animal_{species}_{variant}_{part}
```
Examples:
- `aid_texture_pinata_actor_animal_zz3dinosaur_elite_face`
- `aid_texture_pinata_actor_animal_zz3dinosaur_green_body`
- `aid_texture_pinata_actor_animal_worm_face` (default, no variant)

### Save File Color Variants (Choclodocus)
- `_pink_` = default hatched (appears blue in game)
- `_blue_` = blue variant
- `_green_` = green variant
- `_red_` = red variant
- `_elite_` = Elite Neon (black + neon green zigzag)

### How Custom Variants Could Work
1. **Save patching approach** (proven): Replace texture name strings in save.txt
   - Limited to textures that already exist in the game files
   - Works for any color variant the game has assets for

2. **Runtime texture injection**: Load custom textures at runtime
   - Hook `rex_dbTextureInitTexture` (0x821FC308, already named in config)
   - Intercept texture loading and substitute custom textures
   - Would allow completely new color schemes / patterns
   - Could create custom wildcard variants for any species

3. **Color displacement modification**: Modify the color channels directly
   - Write to the 6 color channel slots on a live entity
   - Could create color combinations the game doesn't have textures for
   - Simpler than full texture replacement

### Research Needed
- Map the `colourDisplacementTable` structure fully
- Find which texture assets exist for each species (what variants are in the game files)
- Understand the texture loading pipeline (`rex_dbTextureInitTexture`)
- Test if custom .dds/.png textures can be loaded at runtime via the asset system

---

## Priority 2: Per-Body-Part Choclodocus Colors

### The Problem
The Choclodocus save stores separate texture names for face, body, and tail.
But the rendering engine uses a single color index that applies uniformly.
Setting different colors per part in the save results in only the face color being used.

### What Would Be Needed
- Modify the color displacement shader to support per-mesh-region colors
- Change `sub_825595E0` to accept different values per body part
- Split the scenegraph's color channels by mesh region
- This is deep engine/shader work, not a save patch

### Difficulty: Very High
This requires modifying the rendering pipeline itself.

---

## Priority 3: Wildcard System for Regular Piñatas

### What We Know
- Piñata Vision barcode TLV type `01001` = Wildcard trait (2-bit value: 0-3)
- Piñata Vision barcode TLV type `01010` = Variant color (4-bit value: 0-15)
- Wildcards are created during romance (breeding 7th piñata of same species)
- Each species has 3 wildcard variants with unique visual traits
- Wildcard value is 10x base monetary value
- `curVariantColourIndex` on `scenegraphInst_s` likely controls this

### Approach
- The barcode injection system already decodes Wildcard and Variant commands
- Need to find where these are applied during the PV pipeline
- Similar to Choclodocus color: find the handler for barcode type `01001`
- Or: save-patch the variant index like we did with dino textures

### Save File Research Needed
- Find where wildcard status is stored in save.txt
- Compare a normal piñata's save data vs a wildcard piñata's
- The variant name in saves (e.g., `animal_resident_gold4`) may encode wildcard status

---

## Priority 4: Full Piñata Vision Camera Pipeline

### Goal
Hook XUsbcam functions to fake camera input, letting the game process 
barcodes natively through the DigitalObjects controller.

### Why
- Would handle ALL barcode types correctly (spawn, color, wildcard, accessories, tricks)
- Game's own assembly/initialization pipeline runs properly
- No need to reverse engineer each handler individually

### Approach
- Replace `XUsbcamGetState` → return 2 (connected)
- Replace `XUsbcamCreate` → return 0 (success)
- Feed pre-decoded barcode data through the result callbacks
- The DigitalObjects controller would process everything natively

### Difficulty: High
The camera pipeline uses async APCs, ring buffers, and complex state machines.

---

## Completed Features (Session 1-2)

### Spawn Menu (F8)
- 90+ verified species with search and categories
- 16 TiP-exclusive piñatas correctly identified
- Species validation prevents crashes
- Dino Color slider and Variant slider

### Barcode Injection
- Full C++ port of pv_decoder.pl (verified exact match)
- Preset buttons for Choclodocus cards
- Handles PlaceTag, SparseCallback, Variant, Wildcard, Reuse commands

### Elite Neon Choclodocus
- **ACHIEVED** via save file texture patching
- Method: Replace `_pink_`/`_blue_`/`_green_`/`_red_` with `_elite_` in save.txt
- Perl patch script created for sharing
- First Elite Neon on PC, first custom-colored Choclodocus ever

### Choclodocus Full Body
- Egg (tag 208) hatches into complete dinosaur
- Body-only (tag 29) spawns just the torso
- Dino bones identified: 720-725

### Species Discovery
- Complete TiP species mapping from Piñata Vision barcode database
- Encyclopedia-based validation via live PPC context scanning
- Metallic gold Dragonache (uninitialized variant artifact)

### Save Manipulation
- Texture name patching for color variants
- Post office entity analysis
- Profile/header structure documented
