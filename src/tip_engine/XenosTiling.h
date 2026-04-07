#pragma once
#include <cstdint>

// Xbox 360 Xenos GPU texture tiling/untiling
// Ported from Xenia: src/xenia/gpu/texture_address.h
// Original: Copyright 2024 Xenia Developers (BSD license)

namespace XenosTiling {

// Macro tile constants
constexpr uint32_t kMacroTileWidthLog2 = 5;       // 32 texels wide
constexpr uint32_t kMacroTileHeight2DLog2 = 5;    // 32 texels tall (2D)
constexpr uint32_t kMacroTileHeight3DLog2 = 3;    // 8 texels tall (3D)
constexpr uint32_t kMacroTileDepthLog2 = 2;       // 4 texels deep (3D)

// Combine outer/inner bytes with bank, pipe, and y LSB
template <typename Address>
inline Address TiledCombine(const Address outer_inner_bytes, const uint32_t bank,
                            const uint32_t pipe, const uint32_t y_lsb) {
    return Address((y_lsb << 4) | (pipe << 6) | (bank << 11)) |
           (outer_inner_bytes & 0b1111) |
           (((outer_inner_bytes >> 4) & 0b1) << 5) |
           (((outer_inner_bytes >> 5) & 0b111) << 8) |
           (outer_inner_bytes >> 8 << 12);
}

// Calculate tiled offset for a 2D texture at block position (x, y)
// pitch_aligned: width aligned to macro tile boundary (in blocks for DXT)
// bytes_per_block_log2: log2 of block size (3 for DXT1/8 bytes, 4 for DXT5/16 bytes)
inline int32_t Tiled2D(const int32_t x, const int32_t y,
                       const uint32_t pitch_aligned,
                       const unsigned int bytes_per_block_log2) {
    const int32_t outer_blocks =
        ((y >> kMacroTileHeight2DLog2) *
             int32_t(pitch_aligned >> kMacroTileWidthLog2) +
         (x >> kMacroTileWidthLog2))
        << 6;
    const int32_t inner_blocks = (((y >> 1) & 0b111) << 3) | (x & 0b111);
    const int32_t outer_inner_bytes = (outer_blocks | inner_blocks)
                                      << bytes_per_block_log2;
    const uint32_t bank = (y >> 4) & 0b1;
    const uint32_t pipe = ((x >> 3) & 0b11) ^ (((y >> 3) & 0b1) << 1);
    return TiledCombine(outer_inner_bytes, bank, pipe, y & 1);
}

// Untile a 2D texture from Xbox 360 tiled format to linear
// For DXT formats: x,y are in BLOCK coordinates (4x4 pixel blocks)
// bytes_per_block: 8 for DXT1, 16 for DXT3/DXT5/DXN
inline void Untile2D(const uint8_t* src, uint8_t* dst,
                     uint32_t width_blocks, uint32_t height_blocks,
                     uint32_t bytes_per_block) {
    // Align pitch to 32-block macro tile boundary
    uint32_t pitch_aligned = (width_blocks + 31) & ~31;

    unsigned int bpp_log2 = 0;
    if (bytes_per_block == 8) bpp_log2 = 3;       // DXT1
    else if (bytes_per_block == 16) bpp_log2 = 4;  // DXT3/5/DXN
    else if (bytes_per_block == 4) bpp_log2 = 2;   // A8R8G8B8
    else if (bytes_per_block == 2) bpp_log2 = 1;   // R5G6B5
    else if (bytes_per_block == 1) bpp_log2 = 0;   // L8

    uint32_t linear_stride = width_blocks * bytes_per_block;

    for (uint32_t y = 0; y < height_blocks; y++) {
        for (uint32_t x = 0; x < width_blocks; x++) {
            int32_t tiled_offset = Tiled2D(x, y, pitch_aligned, bpp_log2);
            uint32_t linear_offset = y * linear_stride + x * bytes_per_block;

            if (tiled_offset >= 0) {
                memcpy(dst + linear_offset, src + tiled_offset, bytes_per_block);
            }
        }
    }
}

// Convenience: untile a DXT1 texture (4 bpp, 8 bytes per 4x4 block)
inline void UntileDXT1(const uint8_t* src, uint8_t* dst, uint32_t width, uint32_t height) {
    Untile2D(src, dst, (width + 3) / 4, (height + 3) / 4, 8);
}

// Convenience: untile a DXT3/DXT5 texture (8 bpp, 16 bytes per 4x4 block)
inline void UntileDXT5(const uint8_t* src, uint8_t* dst, uint32_t width, uint32_t height) {
    Untile2D(src, dst, (width + 3) / 4, (height + 3) / 4, 16);
}

// Convenience: untile a DXN (BC5) texture (16 bytes per 4x4 block)
inline void UntileDXN(const uint8_t* src, uint8_t* dst, uint32_t width, uint32_t height) {
    Untile2D(src, dst, (width + 3) / 4, (height + 3) / 4, 16);
}

// Convenience: untile an A8R8G8B8 texture (4 bytes per pixel)
inline void UntileA8R8G8B8(const uint8_t* src, uint8_t* dst, uint32_t width, uint32_t height) {
    Untile2D(src, dst, width, height, 4);
}

} // namespace XenosTiling
