# Hook Architecture — Pinata Vision Spawn System

This document describes the reverse-engineered hooking system used to add spawn menu, barcode injection, wildcard breeding, and texture tools to TiP-Recomp. Every address and offset is derived from static analysis of the Xbox 360 PPC binary.

## Overview

The mod adds five major systems, all implemented as hooks into the game's existing PPC function call chain:

| System | Entry Point | PPC Address | Method |
|--------|-------------|-------------|--------|
| Spawn Menu + Barcode Injection | `gardenMainGetGardenScene` | `0x824E1120` | Function override (PPC_FUNC) |
| Texture Capture & Dump | `dbTextureInitTexture` | `0x821FC308` | Function override (PPC_FUNC) |
| Asset Name Registry | `assetIdPrintf` | `0x821D17F0` | Function override (PPC_FUNC) |
| Wildcard Breeding Override | `sub_824144F8` | `0x824144F8` | Generated code patch |
| Spawn Menu UI | ImGui overlay | N/A | ImGuiDialog subclass |

None of these hooks modify the original game logic — they call the original function first (`__imp__`), then execute additional code.

---

## 1. Spawn System — gardenMainGetGardenScene Hook

**Hooked function:** `rex_gardenMainGetGardenScene_824E1120` at PPC `0x824E1120`

This function is called every frame by the game's main loop to get the active garden scene pointer. Our hook overrides it, calls the original, then checks for pending spawn/barcode/scan requests.

### Execution Flow

```
Game main loop
  -> gardenMainGetGardenScene_824E1120()
     -> [original function] returns garden scene pointer in r3
     -> Check g_BarcodeInject.pending → barcode injection pipeline
     -> Check g_ScanPending → species ID scanner
     -> Check g_DeferredVariantChange.pending → deferred variant apply
     -> Check g_SpawnRequest.pending → spawn menu entity creation
```

### Barcode Injection Pipeline

When `g_BarcodeInject.pending` is true:

1. **Decode** — `PVDecode::decode(hexString)` parses the TLV barcode format
2. **Pre-scan** — Extract Variant and Wildcard values before processing PlaceTags
3. **Command loop** — Process each decoded command:

| Command | Handler |
|---------|---------|
| `CMD_PLACE_TAG` | Validate via `sub_825A0818`, spawn via `sub_82575AB8` |
| `CMD_SPARSE` | Type 0x13 = dino color via `sub_82559420`; others via `sub_825885B0` (msg 260) |
| `CMD_VARIANT` | Deferred to `g_DeferredVariantChange` (entity needs init time) |
| `CMD_WILDCARD` | Full apply: event registration + visual via `sub_82383538` |
| `CMD_NAME` | Decoded but not yet applied (name-setter function TBD) |

### Entity Creation Call

```
sub_82575AB8(ctx, base)
  r3 = gardenScene pointer (from gardenMainGetGardenScene)
  r4 = 0
  r5 = 0
  r6 = 0
  r7 = species tag ID (from PlaceTag command)
  r8 = 0
  r9 = 0 (variant — NOT used at creation, breaks AI)
  f1 = 1.0 (scale)
  f2 = 0.0
  
  Returns: r3 = entity PPC address (0 on failure)
```

### Species Validation

```
sub_825A0818(tagID)
  r3 = tag ID
  Returns: r3 = type code
    0-44 = valid pinata species
    46+ = invalid / non-species item
```

### Why Variant is Deferred

Research (commit f3a504a) found that passing variant data at entity creation time causes the entity's AI state machine to break — the pinata spawns but won't move, eat, or interact. The fix is to defer variant application by 5 frames using `g_DeferredVariant`, allowing the entity's strategy/AI subsystems to fully initialize first.

---

## 2. Barcode Decoder — BarcodeInjector.h

**Source:** Ported from `pv_decoder.pl` by FeralKitty (Kathryn Jensen)

### Decode Pipeline

```
Hex string (e.g. "BF619A786BD25C9B")
  -> hexToBits(): 4 bits per hex char
  -> unobfuscateRow(): for each 15-hex-char row:
     1. Logical translate (tr/76543210EFABCD98/0-9A-F/)
     2. XOR with negate mask (indexed by check digit)
     3. Unshuffle using 60-position shuffle table
  -> TLV parse: read 5-bit type codes, dispatch handlers
```

### TLV Command Types

| Type Code | Name | Bit Width | Description |
|-----------|------|-----------|-------------|
| 1 | START/END | 1 | Barcode boundary marker |
| 2 | PLACE_TAG | 12 | Species/item ID to spawn |
| 7 | NAME | variable (5-bit chars) | Custom pinata name |
| 9 | WILDCARD | 2 | Wildcard trait (1-3) |
| 10 | VARIANT | 4 | Color variant (0-15) |
| 16 | SPARSE | 16 | Event callback (type:8 + data:8) |

### Shuffle Tables

16 tables of 60 positions each (lines 45-62 of BarcodeInjector.h). Each table permutes the 60 data bits of a barcode row. The check digit (last hex char) selects which table and negate mask to use.

---

## 3. Wildcard Breeding Override

**Hooked function:** `sub_824144F8` (wildcard probability check during egg hatch)

When `g_ForceWildcard` is true, the hook intercepts the breeding pipeline at the point where the game decides whether an egg hatches as a wildcard. Instead of using the random probability, it forces the wildcard trait.

### Call Chain

```
Egg hatch sequence
  -> sub_824144F8 (probability check)
     -> [our hook] if g_ForceWildcard:
        -> sub_824145E8(ctx, base) with r4 = g_ForcedWildcardTrait
           (trait writer — sets entity variant fields)
        -> Caller then calls sub_824146C0 (visual applicator)
           -> sub_82414190 (final setup)
```

### Wildcard Trait Values

| Trait | Value | Visual |
|-------|-------|--------|
| 1 | 10 | Body pattern A |
| 2 | 20 | Body pattern B |
| 3 | 21 | Body pattern C |

This chain is safe — it doesn't access `colourDisplacementTable` (NULL for regular species' model assets) and doesn't break entity AI.

---

## 4. Texture Capture System

### Asset Name Registry

**Hooked function:** `rex_assetIdPrintf` at `0x821D17F0`

Captures formatted asset name strings (e.g. `"aid_animal_whirlm_body"`) as they're processed by the asset system. Stores the last-seen name in `g_LastAssetName`.

**Hooked function:** `sub_821D1C10` at `0x821D1C10`

Maps asset ID pointers to data pointers. When an asset's data pointer is resolved, this hook records the mapping in `g_TextureAddrToName` so that when `dbTextureInitTexture` fires later, we can look up which asset the texture belongs to.

### Texture Init Hook

**Hooked function:** `rex_dbTextureInitTexture` at `0x821FC308`

Fires when the game initializes a texture in GPU memory. Our hook:

1. Calls the original function
2. If capture is enabled, reads the `dbTexture_s` struct:
   - `+0`: format (0=DXT1, 1=DXT3, 2=DXT5, ...)
   - `+8-10`: width, height (uint16, big-endian)
   - `+20`: imageDataAddr (uint32, big-endian)
   - `+24`: dataSize (uint32, big-endian)
3. Looks up the asset name from `g_TextureAddrToName`
4. Stores the captured texture in `TextureTools::g_CapturedTextures`

---

## 5. Key PPC Functions Reference

| Address | Name | Purpose | Arguments | Returns |
|---------|------|---------|-----------|---------|
| `0x82575AB8` | Entity creation | Spawns entity in garden | r3=scene, r7=tagID, f1=scale | r3=entity addr |
| `0x825A0818` | Species validator | Checks if tag is valid species | r3=tagID | r3=typeCode (0-44 valid) |
| `0x825885B0` | Message 260 dispatch | Sends variant/wildcard message | r3=entity, r4=value | — |
| `0x82383538` | Wildcard visual | Applies wildcard body trait | r3=speciesData, r4=traitVal | — |
| `0x8258ACE8` | Event registration | Registers wildcard event | r3=eventType, r4=tagID, r5=data | r3=result |
| `0x8258AD48` | Status setter | Sets wildcard status flag | r3=eventType, r4=tagID, r5=flag | — |
| `0x82546C50` | Find entity by tag | Searches garden for species | r3=gardenData, r6=tagID | r3=entity |
| `0x82559420` | Dino color apply | Sets Choclodocus color | r3=entity, r4=color(0-3) | — |
| `0x82382328` | Variant target | Sets variant change on speciesData | r3=speciesData, r4=idx, r5=flag | — |
| `0x824E1120` | Get garden scene | Returns active garden context | — | r3=gardenScene |
| `0x824E10D8` | Get garden | Returns garden data pointer | — | r3=gardenPtr |
| `0x825357B8` | Encyclopedia lookup | Gets species info struct | r3=tagID | r3=speciesInfoPtr |

---

## 6. Entity Memory Layout

```
Entity (PPC address)
  +0x110  -> glModel_s pointer (heap)
  +2416   -> speciesData pointer
  +3792   -> display mode (4 = accessory attach mode)
  +5276   -> attachment state

glModel_s (heap, ~700 bytes)
  +0      -> dbModel_s pointer (model definition)
  +312    -> sgInst (scenegraph instance, embedded)
    +0    -> dbScenegraph_s pointer (static data)
    +276  -> textureTable
    +296  -> colourDisplacementTable (NULL for regular species)
    +319-321 -> color indices

dbTexture_s
  +0      -> format enum (0=DXT1, 1=DXT3, 2=DXT5, ...)
  +8-10   -> width, height (uint16 big-endian)
  +20     -> imageDataAddr (uint32 big-endian)
  +24     -> dataSize (uint32 big-endian)
```

---

## 7. Diagnostic Log Files

| File | Purpose |
|------|---------|
| `placetag_log.txt` | Full barcode decode trace per scan (append mode) |
| `wildcard_hook_log.txt` | Breeding override trace |
| `variant_debug.txt` | Message 260 dispatch log |
| `dino_color_debug.txt` | Choclodocus color application |
| `species_scan.txt` | Encyclopedia tag ID dump |
| `entity_dump.txt` | Deferred entity structure scan |
| `model_dump.txt` | glModel/scenegraph analysis |
| `texture_log.txt` | Texture init events |

---

## 8. File Map

| File | Purpose |
|------|---------|
| `src/tip_engine/hooks.cpp` | All hook implementations |
| `src/tip_engine/hooks.h` | Hook declarations, FPSManager global |
| `src/tip_engine/Overlays/SpawnMenu.cpp` | ImGui spawn menu UI |
| `src/tip_engine/Overlays/SpawnMenu.h` | UI types: SpawnRequest, DeferredVariant, BarcodeInjectRequest |
| `src/tip_engine/Overlays/BarcodeInjector.h` | TLV barcode decoder (ported from pv_decoder.pl) |
| `src/tip_engine/TextureTools.h` | Texture capture/dump/swap utilities |
| `config/retip_hooked.toml` | Hooked function declarations |
| `config/retip_midasm.toml` | Midasm (mid-instruction) hook points |
