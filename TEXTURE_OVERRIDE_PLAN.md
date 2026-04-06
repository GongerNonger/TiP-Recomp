# Texture Override System — Implementation Plan

## Research Findings Summary

### What the VP1 PC Port Does (proven approach)
- SolarCookies' own cheat menu has `AssetHooks.cpp` with VDAT asset dump/replace
- The Nexus Mods "Texture Packer" dumps textures as .tga, loads replacements from `mods/`
- Both hook the texture init function (equivalent to our `rex_dbTextureInitTexture`)
- Assets identified by tag name strings: `aid_texture_pinata_actor_animal_{species}_{variant}_{part}`

### What the ReXGlue SDK Provides
- **`HostPathDevice`** in `rex/filesystem/devices/host_path_device.h`
  - Maps a host disk folder to a VFS mount point
  - Constructor: `HostPathDevice(mount_path, host_path, read_only)`
  - Can be registered before the game data device for override priority
- **VFS architecture** (Xenia-derived): `RegisterDevice()`, `ResolvePath()`, symbolic links
- **No built-in texture override** — must be implemented

### How Other Recomps Do It
- **UnleashedRecomp**: VFS overlay (CPKREDIR-style filesystem redirection)
- **Zelda64Recomp**: RT64 hash-based texture replacement with DDS/PNG support
- Both use "check mods folder first, fall back to game data" pattern

### SolarCookies' Config Note
Line 75 of retip_config.toml:
```
0x821FC308 = {name = "rex_dbTextureInitTexture"} # this is what i hook on the pc port for texturepacks
```

---

## Three Implementation Approaches

### Approach A: VFS Overlay Device (Cleanest, SDK-Native)

**How it works:**
1. Create a `mods/` directory alongside the game
2. Register a `HostPathDevice` pointing to `mods/` in the VFS
3. The game's asset pipeline checks `mods/` first, falls back to game data
4. Drop replacement files in `mods/` with matching names

**Implementation:**
```cpp
// In retip_app.h OnPostSetup() or similar
auto* runtime = rex::Runtime::instance();
auto modsDevice = std::make_unique<rex::filesystem::HostPathDevice>(
    "game:", "mods/", true /* read_only */);
runtime->file_system()->RegisterDevice(std::move(modsDevice));
```

**Pros:** Clean, uses SDK infrastructure, no PPC hooks needed
**Cons:** Need to understand VFS path format, may require assets in game's exact format
**Risk:** The game may not load individual texture files but rather package files

### Approach B: File System Hook (Already Started)

**How it works:**
1. Our `rex_fsOpenFile` hook intercepts every file open
2. Check if a replacement file exists in `mods/` directory
3. If yes, modify the PPC filename string to point to the mod file
4. The game loads the replacement transparently

**Implementation:**
```cpp
extern "C" PPC_FUNC(rex_fsOpenFile) {
    // Read filename from PPC memory
    const char* filename = (const char*)(membase + ctx.r4.u32);
    
    // Check for override in mods/ directory
    std::string modPath = "mods/" + extractBaseName(filename);
    if (fileExists(modPath)) {
        // Write mod path to PPC memory and redirect
        writePPCString(ctx.r4.u32, modPath);
    }
    
    __imp__rex_fsOpenFile(ctx, base);
}
```

**Pros:** Already have the hook, mirrors UnleashedRecomp approach
**Cons:** Need to understand file path format, textures may be in packages
**Risk:** If textures are embedded in package files, individual file replacement won't work

### Approach C: Texture Init Hook (Most Powerful, PC Port Proven)

**How it works:**
1. Fix the crash in `rex_dbTextureInitTexture` hook
2. After the original function populates `dbTexture_s`, check for replacement
3. Load replacement DDS/TGA from disk
4. Swap `imageDataStart` pointer in the `dbTexture_s` struct

**Implementation:**
```cpp
extern "C" PPC_FUNC(rex_dbTextureInitTexture) {
    // Call original first
    __imp__rex_dbTextureInitTexture(ctx, base);
    
    // After init, check for texture replacement
    // The dbTexture_s at ctx.r3 now has valid data
    // Read the texture's asset tag name from the calling context
    // If a replacement exists, swap imageDataStart
}
```

**Why it crashed before:** Our previous hook read memory at negative offsets from the texture address, hitting unmapped memory. Fix: only read AFTER the original function completes, and only access known-valid fields.

**The crash fix:**
- Don't probe memory for texture names during init
- Instead, maintain a map of texture addresses → names populated from `rex_assetIdPrintf` calls
- Or: log texture init calls WITHOUT reading any extra memory (just count them)

**Pros:** SolarCookies uses this exact approach on the PC port, most powerful
**Cons:** Need to handle Xbox 360 texture tiling/swizzling, GPU memory management
**Risk:** Texture format compatibility (DDS must match original format/dimensions)

---

## Recommended Path

### Step 1: Test the file log (5 minutes)
Enable the `rex_fsOpenFile` file logger, spawn a piñata, check `file_log.txt`.
This tells us whether textures are loaded as individual files or from packages.

### Step 2a: If individual files → Approach B (file redirect)
- Simplest implementation
- Just redirect file paths to `mods/` directory
- Drop replacement textures in matching structure

### Step 2b: If package files → Approach C (texture init hook)
- Fix the crash (remove memory probing, just call original + post-process)
- Build a texture name registry from `rex_assetIdPrintf` calls
- After texture init, check registry and swap `imageDataStart`

### Step 3: Texture dumper tool
- Add a "Dump All Textures" button that saves every loaded texture to disk
- Users can edit these and drop them back in `mods/`
- Like the VP1 Texture Packer but for TiP

---

## Key Data

### dbTexture_s Struct (PPC layout)
| Offset | Type | Field |
|--------|------|-------|
| +0 | u32 | format (enum) |
| +4 | ptr | d3dHeader (GPU descriptor) |
| +8 | u16 | width |
| +10 | u16 | height |
| +12 | u8 | type (0=2D, 2=volume) |
| +17 | u8 | numFrames |
| +18 | u8 | currentFrameLoaded |
| +19 | u8 | requiredFrame |
| +20 | ptr | imageDataStart |
| +24 | u32 | sizeOfOneFrame |

### Texture Formats
| Value | Format | Block Size |
|-------|--------|-----------|
| 1 | DXT1 (BC1) | 8 bytes/4x4 |
| 2 | DXT3 (BC2) | 16 bytes/4x4 |
| 3 | DXT5 (BC3) | 16 bytes/4x4 |
| 4 | A8R8G8B8 | 4 bytes/pixel |
| 12 | DXN (BC5) | 16 bytes/4x4 |

### Xbox 360 Texture Tiling
- Xbox 360 uses tiled texture formats (bit 0x100 in D3DFMT)
- De-tiling required for custom textures
- RareView tool (NCDyson/RareView) has de-tiling code for Rare Xbox 360 formats
- The recomp framework handles de-tiling at the GPU abstraction layer

---

## Resources
- [VP1 Texture Packer (Nexus)](https://www.nexusmods.com/vivapinata/mods/10)
- [VP1 Cheat Menu AssetHooks](VivaPinata-Cheat-Menu/VivaPinata_Injector/Hooks/AssetLoading/AssetHooks.cpp)
- [RareView (Xbox 360 texture extractor)](https://github.com/NCDyson/RareView)
- [RT64 Texture Pack docs](https://github.com/rt64/rt64/blob/main/TEXTURE-PACKS.md)
- [ReXGlue HostPathDevice](rexglue-sdk/include/rex/filesystem/devices/host_path_device.h)
