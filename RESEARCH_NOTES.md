# Research Notes — Piñata Vision & Spawn System

## Session Date: 2026-04-05/06

---

## Spawn Function Discovery

### What sub_824CB440 Actually Is
- **NOT a spawn function** — it's a post-spawn cutscene/notification trigger
- Called AFTER a piñata is already created to trigger the intro animation
- Calling it directly causes "Do you want to continue? This garden will be lost forever" dialog
- Takes: r3=383 (scene dispatch msg), r4=tagID, r5=parent, r6-r8=flags

### The Real Entity Creation: sub_82575AB8
- Located at PPC 0x82575AB8 in retip_recomp.30.cpp line 2183
- Takes: r3=gardenScene, r4=position, r5=gardenData, r7=speciesTagID, f1=height, f2=groundLevel
- Calls sub_82575E50 (type dispatch) → sub_82575578 (asset factory)
- Returns entity pointer in r3

### Full Spawn Chain
```
sub_82336440 (garden tick/update)
  → sub_82334638 (piñata spawn controller — validates eligibility)
    → sub_82575AB8 (entity creation wrapper)
      → sub_82575E50 (type dispatch by species)
        → sub_82575578 (asset factory — loads model)
    → sub_824CB440 (cutscene trigger — POST-spawn)
    → sub_824C9B38 (event registration — POST-spawn)
```

---

## Species ID System

### supportPinataTag_e Enum
- Starts at 0 (aRTrigger), animals at 3-168
- Animal_aaaaaa (2) and Animal_zzzzzz (169) are sentinel values
- "spare" entries are TiP-exclusive piñatas
- Total: ~166 animal slots, ~50 spare slots for TiP species

### TiP Species Mapping (from Piñata Vision barcode database)
```
ID 17  = zz3skunk      = Smelba
ID 19  = zz3armadillo  = Parmadillo
ID 25  = zz3crab       = Custacean
ID 29  = zz3dinosaur   = Choclodocus
ID 53  = zz3gorilla    = Sarsgorilla
ID 65  = zz3ladybird   = Sweetle
ID 85  = zz3owl        = Hootyfruity
ID 86  = zz3peacock    = Peckanmix
ID 101 = zz3rhino      = Limeoceros
ID 102 = zz3rhinopest  = Sour Limeoceros
ID 107 = zz3skunkpest  = Sour Smelba
ID 111 = zz3smallbird  = Tartridge
ID 121 = zz3tiger      = Tigermisu
ID 123 = zz3turtle     = Cherrapin
ID 130 = zz3walrus     = Walrusk
ID 132 = zz3warthog    = Hoghurt
```

### Species Validation
- `sub_825A0818` converts tagID to typeCode (0=Animal, 46=invalid)
- ALL animal IDs 3-168 return typeCode 0 (valid Animal)
- But NOT all have encyclopedia data — NULL entries crash when spawned
- Encyclopedia lookup `sub_825357B8` returns null for species without game data
- Valid species confirmed via live PPC context encyclopedia scan

### Crash-Causing IDs (NULL in encyclopedia scan)
46, 56, 57, 66-68, 70-72, 74, 76, 82, 98-99, 104-106, 115-118, 120, 122, 124-125, 128, 139, 141-145, 149, 151-154, 157-158, 161-168

---

## Choclodocus (zz3dinosaur)

### Entity IDs
- **29** = base body/torso (incomplete — spawns without bones)
- **208** = egg (**hatches into FULL Choclodocus!**)
- **1784** = Life Sweet (candy)

### Dino Bones (assembly parts)
- 720 = dinoredbone
- 721 = dinogreenbone
- 722 = dinobluebone
- 723 = dinoskull
- 724 = dinoribs
- 725 = dinospine

### Color System (from Piñata Vision barcodes)
```
Color 0 = Blue   (barcode: FD6198786BD25C9B)
Color 1 = Green  (barcode: 8683F3F160A87698)
Color 2 = Red    (barcode: E0206B2A1A0EFE80)
Color 3 = Elite Neon (barcode: BF619A786BD25C9B) — ultra-rare, only 25 people ever got this
```

### Color Application
- Colors are applied via SparseCallback (type 0x13, data 0-3)
- The sparse callback is a POST-creation color change on existing entity
- Garden controller stores color at offsets 4864-4866
- Global color byte at PPC 0x83DBECB5
- **UNSOLVED**: Writing to both locations doesn't affect hatched Choclodocus
- The hatching pipeline may read color from a different source
- The color change cards were marked "Unsupported" in the barcode database

### What We Tried (color)
1. ❌ Writing to global 0x83DBECB5 before spawn — entity doesn't read it
2. ❌ Message 260 (set variant) after spawn — body doesn't respond
3. ❌ Writing to garden controller offset 4864 — hatching still produces blue
4. ❌ Barcode sparse injection — sends msg 260 but no visual change
5. **Next: Hook into the actual barcode camera pipeline to inject raw barcode data**

---

## Dragonache

### Metallic Gold Texture
- Spawning Dragonache (ID 32) via sub_82575AB8 produces metallic gold texture
- This is UNINITIALIZED variant data — the raw base model material
- Nobody has ever seen this before — unique to the spawner mod
- Crating and retrieving resets to yellow (serialization validates variant data)
- Leaving/rejoining garden changes to black

### Color System
- Different from Choclodocus — terrain-based (egg hatching location)
- 6 colors: Gold (golden paving), Brown (dirt), Blue (water), Green (grass), White (snow), Red/Yellow (sand)
- Body parts (teeth, mane, ridges, wings, tail) randomized at hatch time
- 20,000+ unique combinations

---

## Piñata Vision Barcode System

### Barcode Format
- EAN-13 based with modifications
- Each row: 16 hex digits → 60 bits data + 4 bits checksum
- Multi-row barcodes separated by space

### Obfuscation (3 steps)
1. **Logical translation**: `tr/76543210EFABCD98/0-9A-F/` (per-nibble transform)
2. **XOR negation**: XOR with negate mask (indexed by check digit)
3. **Shuffle**: Bit reordering (indexed by check digit, 16 different tables)

### TLV Command Types
```
00000 = Start/End of data (4-bit sub-type)
00001 = PlaceTag ID (12-bit entity ID)
00010 = Trick Stick reskin (12-bit)
00011 = Start Date (12-bit)
00100 = End Date (12-bit)
00111 = Name (variable length, 5-bit chars)
01000 = Gamertag (30-bit hash)
01001 = Wildcard trait (2-bit)
01010 = Variant color (4-bit)
01011 = Size (3-bit)
10000 = SparseCallback (16-bit: upper 8 = type, lower 8 = data)
10001 = Timewarp (4-bit)
10010 = Weather type (3-bit)
10011 = Weather duration (8-bit)
10100 = Target ID for Sparse (12-bit)
10110 = Reuse previous entity (5-bit)
```

### Key Barcodes
```
F1706B7B6D69A38F = PlaceTag Choclodocus (ID 29)
96FEF696AB02C4A6 = PlaceTag Choclodocus Egg (ID 208)
BF619A786BD25C9B = Elite Neon reskin (Sparse type=19, data=3)
FD6198786BD25C9B = Blue color (Sparse type=19, data=0)
8683F3F160A87698 = Green color (Sparse type=19, data=1)
E0206B2A1A0EFE80 = Red color (Sparse type=19, data=2)
D6727936AF6BF6B4 = View in Journal
```

---

## Architecture Notes

### Hook System
- `gardenMainGetGardenScene` (0x824E1120) — overridden with PPC_WEAK_FUNC
- Original called first, then spawn/inject/scan logic runs
- Fires during game logic tick (not render) — correct context for spawning
- `fps_hook` (0x8229B0C8) — used for deferred variant messages

### PPC Memory Access
- `REX_DATA_REFERENCE_DECLARE` or direct: `membase + ppc_address`
- Big-endian: use `std::byteswap()` for multi-byte values
- `rex::Runtime::instance()->memory()->virtual_membase()` for membase

### Garden Global Table
- Base: PPC 0x83D27000
- 5 slots × 504 bytes each
- slot[4] = garden scene pointer
- slot[16] = state (1 = active)
- Garden controller accessible via entity system (not directly from slot)

---

## Next Steps

### Priority 1: Full Piñata Vision Pipeline Injection
- Hook XUsbcam functions to fake camera connection
- Inject raw barcode data into the camera result pipeline
- Let the game's own DigitalObjects controller process everything natively
- This would handle assembly, color, spawning, wildcards correctly

### Priority 2: Wildcard System
- Wildcard trait is TLV command type 01001 (2-bit value)
- Variant color is TLV command type 01010 (4-bit value)
- Both need to go through the PV pipeline, not message 260
- Romance/breeding pipeline sets wildcards differently

### Priority 3: Identify Remaining Species
- ~10 "try me!" spare entries still unnamed
- Spawn each and visually identify
- Or find localization string lookup function

### Priority 4: Elite Neon Choclodocus
- Egg hatching produces full body but always blue
- Color write to garden controller offset 4864 doesn't work
- Need to find where hatching ACTUALLY reads color
- May require hooking the hatch function itself
- Alternative: inject via full PV pipeline (Priority 1)

---

## Resources
- Barcode database: `barcodes.txt` (5293 entries, downloaded from pinatavision/pv-decoder)
- Perl decoder: `pv_decoder.pl` (by FeralKitty/Kathryn Jensen, v1.01 26FEB2013)
- pinataisland.info wiki — community reverse engineering
- TiP-Recomp GitHub: SolarCookies/TiP-Recomp
