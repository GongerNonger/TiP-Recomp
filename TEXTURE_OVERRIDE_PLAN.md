# Texture Override System — Implementation Plan (Updated)

## Critical Discovery: Asset Architecture

**Textures are packed in `debug_pack.bin` (1GB archive), NOT individual files.**

The game's asset loading has two tiers:
1. **Primary**: `debug_pack.bin` — binary-searched by 8-byte asset hash via `rex_assetPackedDebug`
2. **Fallback**: Individual `.pkg` files in `packages/` — only if not in debug pack

This means **`rex_fsOpenFile` hook CANNOT intercept per-texture loads**. Textures are read
at byte offsets within the archive, not opened as separate files.

### Game Asset Directory
```
assets/Beta/
  packed/
    db_index.bin      (380KB — asset index)
    debug_hash.bin    (591KB — hash lookup table)
    debug_pack.bin    (1GB — ALL game assets including textures)
  packages/           (~1000 .pkg files — fallback)
  bundles/            (language files)
  movie/              (video files)
  xwavebank/          (audio files)
```

### Asset Identification
- Assets identified by **name hash** (8 bytes), NOT file path
- Asset name format: `aid_texture_pinata_actor_animal_{species}_{variant}_{part}`
- Name hashed via `rex_assetIdPrintf` → looked up in `debug_hash.bin` → data read from `debug_pack.bin`

---

## Recommended Approach: Fix Texture Init Hook (Approach C)

This is **confirmed as the PC port's approach** by SolarCookies' own code and config comment:
```toml
0x821FC308 = {name = "rex_dbTextureInitTexture"} # this is what i hook on the pc port for texturepacks
```

### Why Our Previous Hook Crashed
The crash was from **memory probing** in the hook body — reading at negative offsets from the
texture address and following invalid pointers. The hook mechanism itself (PPC_WEAK_FUNC override)
works correctly. Fix: remove all memory probing, only read known-valid fields AFTER the original
function completes.

### Implementation Plan

#### Step 1: Minimal Safe Hook (just log texture addresses)
```cpp
extern "C" PPC_FUNC(rex_dbTextureInitTexture) {
    __imp__rex_dbTextureInitTexture(ctx, base);  // Call original FIRST
    
    // After init, the dbTexture_s at the address is fully valid
    // Log ONLY the address and format — no memory probing
    if (g_TextureLogging) {
        g_TextureLog << "0x" << std::hex << texAddr << std::endl;
    }
}
```

#### Step 2: Build Asset Name Registry
Hook `rex_assetIdPrintf` (PPC 0x821D17F0) to capture every asset name as it's built.
Store in a map: hash → name string. Then when `rex_dbTextureInitTexture` is called,
we can look up the texture's name from the calling context.

#### Step 3: Texture Dump Tool
For each loaded texture, read the `dbTexture_s` struct:
- format, width, height at known offsets
- imageDataStart pointer → raw pixel data
- sizeOfOneFrame → data size
Save as DDS files to `mods/dump/` with the asset name as filename.

#### Step 4: Texture Override
For each texture being initialized:
1. Check if `mods/textures/{asset_name}.dds` exists on disk
2. If yes, read the DDS file
3. Allocate PPC memory for the new texture data
4. After original init completes, swap `imageDataStart` pointer
5. May need to trigger GPU re-upload (set `currentFrameLoaded = 0`)

### Xbox 360 Texture Tiling
- Xbox 360 uses tiled texture format (bit 0x100 in D3DFMT)
- The recomp's GPU abstraction layer handles de-tiling during rendering
- Custom textures should be in LINEAR format (the recomp will handle it)
- OR: match the original format exactly (tiled DXT1/DXT5)
- RareView tool (NCDyson/RareView) has de-tiling code as reference

---

## Alternative: Asset Name Registry Approach

Instead of hooking texture init, hook the **asset loader** to intercept at a higher level:

### Hook rex_assetPackedDebug
Make it return 0 (not found) for specific asset hashes → forces fallback to .pkg file loading →
then provide custom .pkg files in the packages/ directory.

**Pros:** Works at the asset level, not texture level
**Cons:** Need to create valid .pkg format files, need hash → name mapping

### Hook sub_821D05A0 (Section Loader Orchestrator)  
Register a custom loader that checks `mods/` first, falls through to original loaders.

**Pros:** Architecturally clean
**Cons:** Complex function pointer table manipulation

---

## dbTexture_s Struct (from texture init hook captures)
```
+0:  format (u32) — 1=DXT1, 3=DXT5, 4=A8R8G8B8
+4:  d3dHeader (ptr) — Xbox 360 GPU fetch constant  
+8:  width (u16) + height (u16)
+12: type (u8) — 0=2D, 2=volume
+17: numFrames (u8)
+18: currentFrameLoaded (u8) — set to 0 to force re-upload
+19: requiredFrame (u8)
+20: imageDataStart (ptr) — THE swap point for texture replacement
+24: sizeOfOneFrame (u32) — size of pixel data
```

## rex_fsOpenFile Details (for reference, NOT for texture override)
```
Parameters:
  r3 = output struct (receives file handle pointer)
  r4 = VFS path string (e.g., "game:\Beta\packages\123.pkg")
  r5 = access flags (0x80000000 = read)
  r6 = creation disposition (3 = OPEN_EXISTING)
Returns: r3 = 0 (success) or error code

Path resolution: game: → \Device\Harddisk0\Partition1 → {game_data_root}\
```

## Resources
- [VP1 Texture Packer (Nexus)](https://www.nexusmods.com/vivapinata/mods/10) — proven texture dump/replace for VP1 PC
- [VP1 Cheat Menu AssetHooks](VivaPinata-Cheat-Menu/Hooks/AssetLoading/AssetHooks.cpp) — VDAT asset system
- [RareView](https://github.com/NCDyson/RareView) — Xbox 360 Rare texture extractor with de-tiling
- [ReXGlue HostPathDevice](rexglue-sdk/include/rex/filesystem/devices/host_path_device.h) — VFS overlay support
- [RT64 Texture Packs](https://github.com/rt64/rt64/blob/main/TEXTURE-PACKS.md) — hash-based replacement reference
