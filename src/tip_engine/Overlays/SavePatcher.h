#pragma once
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <filesystem>

namespace SavePatcher {

struct TextureEntry {
    std::string fullString;   // e.g., "aid_texture_pinata_actor_animal_zz3dinosaur_elite_face"
    std::string species;      // e.g., "zz3dinosaur"
    std::string variant;      // e.g., "elite"
    std::string bodyPart;     // e.g., "face"
    size_t fileOffset;        // offset in save file
    size_t stringLen;         // length including null
    size_t paddingBytes;      // null padding after string
};

// Known color variants per special species
inline const std::map<std::string, std::vector<std::string>> KNOWN_VARIANTS = {
    {"zz3dinosaur", {"pink", "blue", "green", "red", "elite"}},
    {"dragon", {"dirt", "gold", "grass", "water", "snow", "sand"}},
};

// Known body parts
inline const std::vector<std::string> BODY_PARTS = {
    "face", "body", "tail", "mane", "wings", "teeth", "ridges",
    "fur", "horn", "head", "legs", "shell", "back", "belly", "skin"
};

inline bool isBodyPart(const std::string& s) {
    return std::find(BODY_PARTS.begin(), BODY_PARTS.end(), s) != BODY_PARTS.end();
}

// Find the most recent save directory
inline std::string findSaveDir() {
    std::string base = "C:/Users/Administrator/Documents/retip/B13EBABEBABEBABE/4D53085F/00000001";
    namespace fs = std::filesystem;
    std::string newest;
    std::filesystem::file_time_type newestTime{};

    for (auto& entry : fs::directory_iterator(base)) {
        if (entry.is_directory() && entry.path().filename().string().find("pinsav") == 0) {
            auto savePath = entry.path() / "save.txt";
            if (fs::exists(savePath)) {
                auto t = fs::last_write_time(savePath);
                if (newest.empty() || t > newestTime) {
                    newest = savePath.string();
                    newestTime = t;
                }
            }
        }
    }
    return newest;
}

// Scan a save file for all piñata texture variant strings
inline std::vector<TextureEntry> scanSave(const std::string& savePath) {
    std::vector<TextureEntry> results;

    std::ifstream file(savePath, std::ios::binary);
    if (!file) return results;

    std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    const std::string prefix = "aid_texture_pinata_actor_animal_";
    size_t pos = 0;

    while ((pos = data.find(prefix, pos)) != std::string::npos) {
        // Find end of string (null terminator)
        size_t end = data.find('\0', pos);
        if (end == std::string::npos) break;

        std::string fullStr = data.substr(pos, end - pos);

        // Count padding nulls after string
        size_t padCount = 0;
        for (size_t i = end; i < data.size() && data[i] == '\0'; i++) padCount++;

        // Parse the rest after prefix
        std::string rest = fullStr.substr(prefix.size());

        // Split by underscore
        std::vector<std::string> tokens;
        std::string token;
        for (char c : rest) {
            if (c == '_') { if (!token.empty()) { tokens.push_back(token); token.clear(); } }
            else token += c;
        }
        if (!token.empty()) tokens.push_back(token);

        // Find body part (search from end)
        TextureEntry entry;
        entry.fullString = fullStr;
        entry.fileOffset = pos;
        entry.stringLen = end - pos + 1; // including null
        entry.paddingBytes = padCount;

        for (int i = (int)tokens.size() - 1; i >= 0; i--) {
            if (isBodyPart(tokens[i])) {
                entry.bodyPart = tokens[i];
                // Check if token before bodyPart is a variant
                if (i >= 2) {
                    // Species might be multi-token (e.g., "zz3dinosaur")
                    // Check if tokens[i-1] is a known variant
                    bool foundVariant = false;
                    for (auto& [spec, vars] : KNOWN_VARIANTS) {
                        for (auto& v : vars) {
                            if (tokens[i-1] == v) {
                                entry.variant = v;
                                // Species is everything before variant
                                for (int j = 0; j < i-1; j++) {
                                    if (!entry.species.empty()) entry.species += "_";
                                    entry.species += tokens[j];
                                }
                                foundVariant = true;
                                break;
                            }
                        }
                        if (foundVariant) break;
                    }
                    if (!foundVariant) {
                        entry.variant = tokens[i-1];
                        for (int j = 0; j < i-1; j++) {
                            if (!entry.species.empty()) entry.species += "_";
                            entry.species += tokens[j];
                        }
                    }
                }
                break;
            }
        }

        if (!entry.species.empty() && !entry.bodyPart.empty()) {
            results.push_back(entry);
        }

        pos = end + 1;
    }

    return results;
}

// Patch a save file: replace one variant with another for a species
inline std::string patchSave(const std::string& savePath, const std::string& species,
                              const std::string& fromVariant, const std::string& toVariant) {
    std::ifstream file(savePath, std::ios::binary);
    if (!file) return "Cannot open save file";
    std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    int count = 0;
    std::string results;

    // Search for each body part texture string
    for (auto& bp : BODY_PARTS) {
        std::string oldStr = species + "_" + fromVariant + "_" + bp;
        std::string newStr = species + "_" + toVariant + "_" + bp;

        size_t pos = 0;
        while ((pos = data.find(oldStr, pos)) != std::string::npos) {
            // Check we have enough padding
            size_t oldEnd = pos + oldStr.size();
            int availableSpace = 0;
            for (size_t i = oldEnd; i < data.size() && data[i] == '\0'; i++) availableSpace++;

            int lenDiff = (int)newStr.size() - (int)oldStr.size();

            if (lenDiff > availableSpace) {
                return "Error: '" + toVariant + "' is too long (needs " +
                       std::to_string(lenDiff) + " extra bytes, only " +
                       std::to_string(availableSpace) + " available)";
            }

            // Replace the string, adjust null padding
            data.replace(pos, oldStr.size(), newStr);
            // Fix padding: remove excess nulls or add nulls
            if (lenDiff > 0) {
                // New string is longer — eat into padding
                data.erase(pos + newStr.size(), lenDiff);
                // Re-insert nulls at the end to keep file size
                for (int i = 0; i < lenDiff; i++) {
                    // Actually, just let the existing nulls be consumed
                    // The null terminator after the string handles it
                }
            } else if (lenDiff < 0) {
                // New string is shorter — add padding nulls
                data.insert(pos + newStr.size(), -lenDiff, '\0');
                // Remove same amount to keep file size
                data.erase(pos + newStr.size() + (-lenDiff) + availableSpace, -lenDiff);
            }

            results += bp + ": " + fromVariant + " -> " + toVariant + "\n";
            count++;
            pos += newStr.size();
        }
    }

    if (count == 0) {
        return "No '" + fromVariant + "' textures found for " + species;
    }

    // Backup original
    std::string backupPath = savePath + ".bak";
    std::ofstream backup(backupPath, std::ios::binary);
    if (!backup) return "Cannot create backup";

    // Re-read original for backup
    std::ifstream origFile(savePath, std::ios::binary);
    backup << origFile.rdbuf();
    origFile.close();
    backup.close();

    // Write patched file
    std::ofstream outFile(savePath, std::ios::binary);
    if (!outFile) return "Cannot write patched file";
    outFile.write(data.data(), data.size());
    outFile.close();

    return "Patched " + std::to_string(count) + " textures!\n" + results + "Backup at: " + backupPath;
}

} // namespace SavePatcher
