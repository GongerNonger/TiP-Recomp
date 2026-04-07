#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cstring>

namespace TextureTools {

// DDS file header structures
#pragma pack(push, 1)
struct DDS_PIXELFORMAT {
    uint32_t dwSize = 32;
    uint32_t dwFlags = 0;
    uint32_t dwFourCC = 0;
    uint32_t dwRGBBitCount = 0;
    uint32_t dwRBitMask = 0;
    uint32_t dwGBitMask = 0;
    uint32_t dwBBitMask = 0;
    uint32_t dwABitMask = 0;
};

struct DDS_HEADER {
    uint32_t dwMagic = 0x20534444; // "DDS "
    uint32_t dwSize = 124;
    uint32_t dwFlags = 0x1 | 0x2 | 0x4 | 0x1000; // CAPS|HEIGHT|WIDTH|PIXELFORMAT
    uint32_t dwHeight = 0;
    uint32_t dwWidth = 0;
    uint32_t dwPitchOrLinearSize = 0;
    uint32_t dwDepth = 0;
    uint32_t dwMipMapCount = 0;
    uint32_t dwReserved1[11] = {};
    DDS_PIXELFORMAT ddspf;
    uint32_t dwCaps = 0x1000; // TEXTURE
    uint32_t dwCaps2 = 0;
    uint32_t dwCaps3 = 0;
    uint32_t dwCaps4 = 0;
    uint32_t dwReserved2 = 0;
};
#pragma pack(pop)

// Make FourCC from string
inline uint32_t makeFourCC(const char* cc) {
    return (uint32_t)cc[0] | ((uint32_t)cc[1] << 8) | ((uint32_t)cc[2] << 16) | ((uint32_t)cc[3] << 24);
}

// Create DDS header for a given format
inline DDS_HEADER makeDDSHeader(uint32_t width, uint32_t height, uint32_t format, uint32_t dataSize) {
    DDS_HEADER hdr;
    hdr.dwWidth = width;
    hdr.dwHeight = height;
    hdr.dwFlags |= 0x80000; // LINEARSIZE

    switch (format) {
    case 1: // DXT1
        hdr.ddspf.dwFlags = 0x4; // FOURCC
        hdr.ddspf.dwFourCC = makeFourCC("DXT1");
        hdr.dwPitchOrLinearSize = dataSize;
        break;
    case 2: // DXT3
        hdr.ddspf.dwFlags = 0x4;
        hdr.ddspf.dwFourCC = makeFourCC("DXT3");
        hdr.dwPitchOrLinearSize = dataSize;
        break;
    case 3: // DXT5
        hdr.ddspf.dwFlags = 0x4;
        hdr.ddspf.dwFourCC = makeFourCC("DXT5");
        hdr.dwPitchOrLinearSize = dataSize;
        break;
    case 4: // A8R8G8B8
        hdr.ddspf.dwFlags = 0x41; // RGB|ALPHAPIXELS
        hdr.ddspf.dwRGBBitCount = 32;
        hdr.ddspf.dwRBitMask = 0x00FF0000;
        hdr.ddspf.dwGBitMask = 0x0000FF00;
        hdr.ddspf.dwBBitMask = 0x000000FF;
        hdr.ddspf.dwABitMask = 0xFF000000;
        hdr.dwPitchOrLinearSize = width * 4;
        hdr.dwFlags |= 0x8; // PITCH instead of LINEARSIZE
        hdr.dwFlags &= ~0x80000;
        break;
    case 12: // DXN (BC5)
        hdr.ddspf.dwFlags = 0x4;
        hdr.ddspf.dwFourCC = makeFourCC("ATI2");
        hdr.dwPitchOrLinearSize = dataSize;
        break;
    default: // Unknown — write as raw with DXT5 header as fallback
        hdr.ddspf.dwFlags = 0x4;
        hdr.ddspf.dwFourCC = makeFourCC("DXT5");
        hdr.dwPitchOrLinearSize = dataSize;
        break;
    }

    return hdr;
}

// Captured texture info
struct CapturedTexture {
    uint32_t ppcAddr;       // dbTexture_s PPC address
    uint32_t format;        // texture format enum
    uint16_t width;
    uint16_t height;
    uint32_t imageDataAddr; // PPC address of pixel data
    uint32_t dataSize;      // size of pixel data
    std::string assetName;  // captured asset name (may be approximate)
};

// Global texture capture list
inline std::vector<CapturedTexture> g_CapturedTextures;
inline bool g_CaptureEnabled = false;

// Save a captured texture as DDS file
inline bool saveDDS(const std::string& path, const CapturedTexture& tex, uint8_t* membase) {
    DDS_HEADER hdr = makeDDSHeader(tex.width, tex.height, tex.format, tex.dataSize);

    std::ofstream file(path, std::ios::binary);
    if (!file) return false;

    file.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));

    // Read pixel data from PPC memory and write to file
    // Note: data may be in Xbox 360 tiled format
    if (tex.imageDataAddr > 0x40000000 && tex.dataSize > 0 && tex.dataSize < 16 * 1024 * 1024) {
        file.write(reinterpret_cast<const char*>(membase + tex.imageDataAddr), tex.dataSize);
    }

    file.close();
    return true;
}

// Dump all captured textures to a directory
inline int dumpAllTextures(const std::string& dir, uint8_t* membase) {
    namespace fs = std::filesystem;
    fs::create_directories(dir);

    int count = 0;
    for (size_t i = 0; i < g_CapturedTextures.size(); i++) {
        auto& tex = g_CapturedTextures[i];

        // Build filename from asset name or index
        std::string filename;
        if (!tex.assetName.empty() && tex.assetName.find("aid_") == 0) {
            filename = tex.assetName;
            // Remove aid_ prefix for cleaner names
            if (filename.substr(0, 4) == "aid_") filename = filename.substr(4);
        } else {
            char buf[64];
            snprintf(buf, 64, "texture_%04zu_%dx%d", i, tex.width, tex.height);
            filename = buf;
        }
        filename += ".dds";

        std::string path = dir + "/" + filename;
        if (saveDDS(path, tex, membase)) {
            count++;
        }
    }
    return count;
}

} // namespace TextureTools
