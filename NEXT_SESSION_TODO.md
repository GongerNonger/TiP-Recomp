# Next Session TODO — Wildcard & Texture Override

## Immediate Priority: Wildcard Piñatas

### Approach 1: Direct sub_82559420 on mature entity (Quick Test)
The color apply function works for Choclodocus. Test if it works for regular piñatas:
1. Add back `g_LastSpawnedEntity` tracking (was removed in parallel session)
2. Add "Apply Color" button that calls:
   ```cpp
   PPC_EXTERN_IMPORT(sub_825597C0);  // pre-color setup
   PPC_EXTERN_IMPORT(sub_82559420);  // apply color
   
   // In gardenMainGetGardenScene hook:
   ctx.r3.u64 = lastSpawnedEntity;
   sub_825597C0(ctx, base);  // clear existing
   ctx.r3.u64 = lastSpawnedEntity;
   ctx.r4.u64 = variantIndex;  // try 3, 4, 5 for Whirlm
   sub_82559420(ctx, base);  // apply
   ```
3. Spawn Whirlm, wait for maturity, click Apply Color with variant 3/4/5

### Approach 2: Save Patching (Proven for Elite Neon)
Find where curVariantColourIndex is serialized in pinsavN/save.txt:
1. Spawn two Whirlms — one default, one attempted variant
2. Save game, close game
3. Diff the save files to find the variant byte
4. Patch the byte to 1/2/3 and reload
5. The save serialization system uses `rex_serialiseStreamRW_*` functions

### Approach 3: Feed barcode into DigitalObjects controller
The real PV system processes barcodes through sub_8264FFB0 (DigitalObjects controller).
To call it, we need:
1. The controller entity pointer (it's a scene entity in the garden)
2. A properly formatted barcode result structure
3. Call the controller's tick function which processes pending results

The controller searches for entities with sub_82546C50 and creates them through vtable dispatch.

## Texture Override Priority

### Fix GPU texture swap
PPC-side pointer swap doesn't affect GPU. Need to:
1. Mark shared memory pages dirty after swap (trigger GPU re-upload)
2. OR hook at D3D12 TextureCache level (host-side, not PPC)
3. OR use the `d3dHeader` GPU fetch constant to invalidate

### De-tiling for larger textures
DXT1 128x128 works. For 512x512 and DXT3/DXT5:
- Read pitch from GPU fetch constant (d3dHeader at dbTexture_s+4)
- Or try pitch = next_power_of_2(block_width)
- The crunch algorithm from rexglue SDK source IS correct for small textures

## Current Code State
- Last build compiles clean but crashed on launch (from parallel session changes)
- Backup exe at C:\Users\Administrator\Desktop\reTiP_Game_Backup\reTiP.exe
- The wildcard barcode dropdown (268 entries) was added in parallel session
- r9 wildcard parameter tested and confirmed NOT working
