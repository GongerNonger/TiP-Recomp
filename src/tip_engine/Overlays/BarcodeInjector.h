#pragma once
#include <string>
#include <vector>
#include <cstdint>

// Piñata Vision barcode decoder - ported from pv_decoder.pl by FeralKitty/Kathryn Jensen
namespace PVDecode {

// TLV command types
enum CommandType {
    CMD_START_END = 0,    // 00000
    CMD_PLACE_TAG = 1,    // 00001 - 12-bit entity ID
    CMD_TRICK_STICK = 2,  // 00010 - 12-bit
    CMD_START_DATE = 3,   // 00011 - 12-bit
    CMD_END_DATE = 4,     // 00100 - 12-bit
    CMD_NAME = 7,         // 00111 - variable length
    CMD_GAMERTAG = 8,     // 01000 - 30-bit
    CMD_WILDCARD = 9,     // 01001 - 2-bit wildcard trait
    CMD_VARIANT = 10,     // 01010 - 4-bit variant color
    CMD_SIZE = 11,        // 01011 - 3-bit
    CMD_SPARSE = 16,      // 10000 - 16-bit sparse callback
    CMD_TIMEWARP = 17,    // 10001 - 4-bit
    CMD_WEATHER = 18,     // 10010 - 3-bit
    CMD_DURATION = 19,    // 10011 - 8-bit
    CMD_TARGET_ID = 20,   // 10100 - 12-bit target for sparse
    CMD_REUSE = 22,       // 10110 - 5-bit reuse
};

struct Command {
    CommandType type;
    uint32_t value;        // Decoded value
    uint16_t sparseType;   // For sparse: upper 8 bits
    uint16_t sparseData;   // For sparse: lower 8 bits
    std::string name;      // For name commands
};

struct DecodedBarcode {
    std::vector<Command> commands;
    bool valid = false;
    std::string error;
};

// Shuffle tables (from pv_decoder.pl)
static const int shuffle[16][60] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59 },
    { 0, 30, 1, 31, 2, 32, 3, 33, 4, 34, 5, 35, 6, 36, 7, 37, 8, 38, 9, 39, 10, 40, 11, 41, 12, 42, 13, 43, 14, 44, 15, 45, 16, 46, 17, 47, 18, 48, 19, 49, 20, 50, 21, 51, 22, 52, 23, 53, 24, 54, 25, 55, 26, 56, 27, 57, 28, 58, 29, 59 },
    { 0, 20, 40, 1, 21, 41, 2, 22, 42, 3, 23, 43, 4, 24, 44, 5, 25, 45, 6, 26, 46, 7, 27, 47, 8, 28, 48, 9, 29, 49, 10, 30, 50, 11, 31, 51, 12, 32, 52, 13, 33, 53, 14, 34, 54, 15, 35, 55, 16, 36, 56, 17, 37, 57, 18, 38, 58, 19, 39, 59 },
    { 0, 12, 24, 36, 48, 1, 13, 25, 37, 49, 2, 14, 26, 38, 50, 3, 15, 27, 39, 51, 4, 16, 28, 40, 52, 5, 17, 29, 41, 53, 6, 18, 30, 42, 54, 7, 19, 31, 43, 55, 8, 20, 32, 44, 56, 9, 21, 33, 45, 57, 10, 22, 34, 46, 58, 11, 23, 35, 47, 59 },
    { 0, 9, 18, 27, 36, 44, 52, 1, 10, 19, 28, 37, 45, 53, 2, 11, 20, 29, 38, 46, 54, 3, 12, 21, 30, 39, 47, 55, 4, 13, 22, 31, 40, 48, 56, 5, 14, 23, 32, 41, 49, 57, 6, 15, 24, 33, 42, 50, 58, 7, 16, 25, 34, 43, 51, 59, 8, 17, 26, 35 },
    { 0, 7, 14, 21, 28, 35, 42, 48, 54, 1, 8, 15, 22, 29, 36, 43, 49, 55, 2, 9, 16, 23, 30, 37, 44, 50, 56, 3, 10, 17, 24, 31, 38, 45, 51, 57, 4, 11, 18, 25, 32, 39, 46, 52, 58, 5, 12, 19, 26, 33, 40, 47, 53, 59, 6, 13, 20, 27, 34, 41 },
    { 0, 6, 12, 18, 24, 30, 35, 40, 45, 50, 55, 1, 7, 13, 19, 25, 31, 36, 41, 46, 51, 56, 2, 8, 14, 20, 26, 32, 37, 42, 47, 52, 57, 3, 9, 15, 21, 27, 33, 38, 43, 48, 53, 58, 4, 10, 16, 22, 28, 34, 39, 44, 49, 54, 59, 5, 11, 17, 23, 29 },
    { 0, 5, 10, 15, 20, 25, 30, 35, 40, 44, 48, 52, 56, 1, 6, 11, 16, 21, 26, 31, 36, 41, 45, 49, 53, 57, 2, 7, 12, 17, 22, 27, 32, 37, 42, 46, 50, 54, 58, 3, 8, 13, 18, 23, 28, 33, 38, 43, 47, 51, 55, 59, 4, 9, 14, 19, 24, 29, 34, 39 },
    { 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 39, 42, 45, 48, 51, 54, 57, 1, 5, 9, 13, 17, 21, 25, 29, 33, 37, 40, 43, 46, 49, 52, 55, 58, 2, 6, 10, 14, 18, 22, 26, 30, 34, 38, 41, 44, 47, 50, 53, 56, 59, 3, 7, 11, 15, 19, 23, 27, 31, 35 },
    { 0, 4, 8, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 1, 5, 9, 13, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46, 49, 52, 55, 58, 2, 6, 10, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44, 47, 50, 53, 56, 59, 3, 7, 11 },
    { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 59, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 57 },
    { 0, 3, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 1, 4, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 2, 5 },
    { 0, 3, 6, 9, 12, 15, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 1, 4, 7, 10, 13, 16, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 2, 5, 8, 11, 14, 17 },
    { 0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 44, 46, 48, 50, 52, 54, 56, 58, 1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 45, 47, 49, 51, 53, 55, 57, 59, 2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41 },
    { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41 },
    { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45 },
};

// Negate masks (from pv_decoder.pl)
static const char* negateMasks[16] = {
    "878F0B496878F0B0", "9607CF2B59607CF0", "A59E03CD2A59E030", "B416C7AF1B416C70",
    "C3AD1A41EC3AD1B0", "D225DE23DD225DF0", "E1BC12C5AE1BC130", "F034D6A79F034D70",
    "F0B496878F0B4970", "E13C52E5BE13C530", "A59E03CD2A59E030", "B416C7AF1B416C70",
    "C32D5A61FC32D5B0", "D2A59E03CD2A59F0", "878F0B496878F0B0", "9607CF2B59607CF0",
};

// Hex char to 4-bit value
inline uint8_t hexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

// Convert hex string to bit array
inline std::vector<bool> hexToBits(const std::string& hex) {
    std::vector<bool> bits;
    for (char c : hex) {
        uint8_t v = hexVal(c);
        bits.push_back((v >> 3) & 1);
        bits.push_back((v >> 2) & 1);
        bits.push_back((v >> 1) & 1);
        bits.push_back(v & 1);
    }
    return bits;
}

// Read N bits from position as uint32
inline uint32_t readBits(const std::vector<bool>& bits, int& offset, int count) {
    uint32_t val = 0;
    for (int i = 0; i < count && (offset + i) < (int)bits.size(); i++) {
        val = (val << 1) | (bits[offset + i] ? 1 : 0);
    }
    offset += count;
    return val;
}

// Unobfuscate one 16-hex-digit row → 60 bits of decoded data
inline std::vector<bool> unobfuscateRow(const std::string& rowHex) {
    if (rowHex.size() != 16) return {};

    // Get check digit (last hex digit) as index
    int index = hexVal(rowHex[15]);

    // Step 1: Logical translation (reverse the tr/76543210EFABCD98/0-9A-F/ transform)
    std::string translated = rowHex;
    for (auto& c : translated) {
        // Original: 0→7, 1→6, 2→5, 3→4, 4→3, 5→2, 6→1, 7→0, 8→E, 9→F, A→A, B→B, C→C, D→D, E→9, F→8
        // Reverse:  0←7, 1←6, 2←5, 3←4, 4←3, 5←2, 6←1, 7←0, 8←F, 9←E, A←A, B←B, C←C, D←D, E←8, F←9
        static const char revTable[] = "76543210FEDCBA98";
        int v = hexVal(c);
        c = "0123456789ABCDEF"[(int)hexVal(revTable[v])];
    }

    // Step 2: XOR with negate mask
    std::string maskHex = negateMasks[index];
    uint8_t rowBytes[8], maskBytes[8];
    for (int i = 0; i < 8; i++) {
        rowBytes[i] = (hexVal(translated[i*2]) << 4) | hexVal(translated[i*2+1]);
        maskBytes[i] = (hexVal(maskHex[i*2]) << 4) | hexVal(maskHex[i*2+1]);
        rowBytes[i] ^= maskBytes[i];
    }

    // Convert to 64 bits
    std::vector<bool> allBits;
    for (int i = 0; i < 8; i++) {
        for (int b = 7; b >= 0; b--) {
            allBits.push_back((rowBytes[i] >> b) & 1);
        }
    }

    // Step 3: Unshuffle first 60 bits
    std::vector<bool> decoded(60, false);
    for (int i = 0; i < 60; i++) {
        decoded[shuffle[index][i]] = allBits[i];
    }

    return decoded;
}

// Decode a full barcode hex string (one or two 16-char rows separated by space)
inline DecodedBarcode decode(const std::string& barcodeHex) {
    DecodedBarcode result;

    // Split by space for multi-row barcodes
    std::vector<std::string> rows;
    std::string current;
    for (char c : barcodeHex) {
        if (c == ' ' && !current.empty()) {
            rows.push_back(current);
            current.clear();
        } else if (c != ' ') {
            current += c;
        }
    }
    if (!current.empty()) rows.push_back(current);

    // Unobfuscate all rows into one bit stream
    std::vector<bool> bits;
    for (auto& row : rows) {
        if (row.size() != 16) {
            result.error = "Invalid row length: " + row;
            return result;
        }
        auto rowBits = unobfuscateRow(row);
        bits.insert(bits.end(), rowBits.begin(), rowBits.end());
    }

    // Parse TLV commands
    int offset = 0;
    bool endOfData = false;

    while (!endOfData && offset + 5 <= (int)bits.size()) {
        uint32_t type = readBits(bits, offset, 5);
        Command cmd;

        switch (type) {
        case 0: { // 00000 Start/End
            uint32_t sub = readBits(bits, offset, 4);
            if (sub == 0) { endOfData = true; cmd.type = CMD_START_END; cmd.value = 0; }
            else { cmd.type = CMD_START_END; cmd.value = 1; }
            break;
        }
        case 1: { // PlaceTag ID
            cmd.type = CMD_PLACE_TAG;
            cmd.value = readBits(bits, offset, 12);
            break;
        }
        case 9: { // Wildcard
            cmd.type = CMD_WILDCARD;
            cmd.value = readBits(bits, offset, 2);
            break;
        }
        case 10: { // Variant
            cmd.type = CMD_VARIANT;
            cmd.value = readBits(bits, offset, 4);
            break;
        }
        case 16: { // SparseCallback
            cmd.type = CMD_SPARSE;
            uint32_t sparse = readBits(bits, offset, 16);
            cmd.value = sparse;
            cmd.sparseType = (sparse >> 8) & 0xFF;
            cmd.sparseData = sparse & 0xFF;
            break;
        }
        case 20: { // Target ID
            cmd.type = CMD_TARGET_ID;
            cmd.value = readBits(bits, offset, 12);
            break;
        }
        case 22: { // Reuse
            cmd.type = CMD_REUSE;
            cmd.value = readBits(bits, offset, 5);
            break;
        }
        default: {
            // Skip unknown types with estimated sizes
            int sizes[] = {4,12,12,12,12,0,0,0,30,2,4,3,0,0,0,0,16,4,3,8,12,0,5};
            int skip = (type < 23) ? sizes[type] : 4;
            readBits(bits, offset, skip);
            cmd.type = (CommandType)type;
            cmd.value = 0;
            break;
        }
        }

        result.commands.push_back(cmd);
    }

    result.valid = true;
    return result;
}

// Format decoded commands as human-readable string
inline std::string formatCommands(const DecodedBarcode& bc) {
    std::string out;
    for (auto& cmd : bc.commands) {
        switch (cmd.type) {
        case CMD_START_END: out += cmd.value ? "[Start] " : "[End] "; break;
        case CMD_PLACE_TAG: out += "PlaceTag(" + std::to_string(cmd.value) + ") "; break;
        case CMD_WILDCARD: out += "Wildcard(" + std::to_string(cmd.value) + ") "; break;
        case CMD_VARIANT: out += "Variant(" + std::to_string(cmd.value) + ") "; break;
        case CMD_SPARSE: out += "Sparse(type=" + std::to_string(cmd.sparseType) + ",data=" + std::to_string(cmd.sparseData) + ") "; break;
        case CMD_TARGET_ID: out += "TargetID(" + std::to_string(cmd.value) + ") "; break;
        case CMD_REUSE: out += "Reuse(" + std::to_string(cmd.value) + ") "; break;
        default: out += "Cmd" + std::to_string((int)cmd.type) + "(" + std::to_string(cmd.value) + ") "; break;
        }
    }
    return out;
}

} // namespace PVDecode
