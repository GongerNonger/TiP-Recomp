#include "SpawnMenu.h"
#include "BarcodeInjector.h"
#include "SavePatcher.h"
#include <tip_engine/hooks.h>
#include "tip_engine/rex_macros.h"
#include "tip_engine/Log.h"
#include <cstdio>

void SpawnMenuDialog::OnDraw(ImGuiIO& io) {
    // Toggle menu with F8
    if (ImGui::IsKeyPressed(ImGuiKey_F8, false)) {
        g_SpawnMenuOpen = !g_SpawnMenuOpen;
    }

    if (!g_SpawnMenuOpen) return;

    ImGui::SetNextWindowSize(ImVec2(480.0f, 750.0f), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Pinata Vision Spawner", &g_SpawnMenuOpen, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    // Status message
    if (g_SpawnRequest.pending) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Spawn pending...");
    }

    ImGui::Separator();

    // Search bar
    ImGui::Text("Search:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##search", searchBuffer, sizeof(searchBuffer));

    // Category filter
    ImGui::Text("Category:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo("##category", GetCategoryName(categoryFilter))) {
        for (int i = 0; i <= 7; i++) {
            if (ImGui::Selectable(GetCategoryName(i), categoryFilter == i)) {
                categoryFilter = i;
                selectedIndex = -1;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    // Variant/Wildcard control
    static int variantIndex = -1;
    ImGui::Text("Variant:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderInt("##variant", &variantIndex, -1, 20, variantIndex == -1 ? "Default" : "%d");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Set color variant. -1=Default, 0+=Wildcard variants.\nTry different values to discover hidden colors!");
    }

    // Dino Color (Choclodocus/Dragonache special color)
    static int dinoColor = -1;
    static const char* dinoColorNames[] = {"Default", "Blue", "Green", "Red", "Elite Neon"};
    ImGui::Text("Dino Color:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderInt("##dinocolor", &dinoColor, -1, 3,
        (dinoColor >= 0 && dinoColor <= 3) ? dinoColorNames[dinoColor + 1] : dinoColorNames[0]);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Choclodocus/Dragonache color.\n-1=Default, 0=Blue, 1=Green, 2=Red, 3=Elite Neon");
    }

    ImGui::Separator();

    // Item list
    float listHeight = ImGui::GetContentRegionAvail().y - 40.0f;
    if (ImGui::BeginChild("##itemlist", ImVec2(0, listHeight), true)) {
        int visibleIdx = 0;
        for (size_t i = 0; i < g_PinataIDs.size(); i++) {
            const auto& item = g_PinataIDs[i];

            // Category filter
            if (categoryFilter != 0 && GetCategory(item) != categoryFilter)
                continue;

            // Search filter
            if (!MatchesSearch(item.Name, searchBuffer))
                continue;

            // Color by category
            int cat = GetCategory(item);
            ImVec4 color;
            switch (cat) {
                case 1: color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); break; // Animals - red
                case 2: color = ImVec4(1.0f, 0.8f, 0.4f, 1.0f); break; // Eggs - orange
                case 3: color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); break; // Seeds - green
                case 4: color = ImVec4(0.4f, 0.6f, 1.0f, 1.0f); break; // Homes - blue
                case 5: color = ImVec4(0.8f, 0.4f, 1.0f, 1.0f); break; // Props - purple
                case 6: color = ImVec4(0.4f, 1.0f, 0.8f, 1.0f); break; // Trees - teal
                default: color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); break; // Other - gray
            }

            char label[256];
            snprintf(label, sizeof(label), "[%u] %s", item.ID, item.Name);

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            bool selected = (selectedIndex == static_cast<int>(i));
            if (ImGui::Selectable(label, selected)) {
                selectedIndex = static_cast<int>(i);
            }
            ImGui::PopStyleColor();

            visibleIdx++;
        }
    }
    ImGui::EndChild();

    // Spawn button
    bool canSpawn = (selectedIndex >= 0 && selectedIndex < static_cast<int>(g_PinataIDs.size())
                     && !g_SpawnRequest.pending);

    if (!canSpawn) ImGui::BeginDisabled();

    if (ImGui::Button("Spawn Selected", ImVec2(-1, 0))) {
        g_SpawnRequest.tagID = g_PinataIDs[selectedIndex].ID;
        g_SpawnRequest.variantIndex = variantIndex;
        g_SpawnRequest.dinoColor = dinoColor;
        g_SpawnRequest.pending = true;
        Log("Spawn requested: " + std::string(g_PinataIDs[selectedIndex].Name)
            + (variantIndex >= 0 ? " (variant " + std::to_string(variantIndex) + ")" : ""), 3);
    }

    if (!canSpawn) ImGui::EndDisabled();

    // === Piñata Vision Barcode Injection ===
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Pinata Vision Barcode Inject:");

    static char barcodeHex[64] = "";

    // Preset buttons for key barcodes (no manual typing needed!)
    if (ImGui::Button("Elite Neon")) { strcpy(barcodeHex, "BF619A786BD25C9B"); }
    ImGui::SameLine();
    if (ImGui::Button("Dino Blue")) { strcpy(barcodeHex, "FD6198786BD25C9B"); }
    ImGui::SameLine();
    if (ImGui::Button("Dino Green")) { strcpy(barcodeHex, "8683F3F160A87698"); }
    ImGui::SameLine();
    if (ImGui::Button("Dino Red")) { strcpy(barcodeHex, "E0206B2A1A0EFE80"); }

    if (ImGui::Button("Dino Egg")) { strcpy(barcodeHex, "96FEF696AB02C4A6"); }
    ImGui::SameLine();
    if (ImGui::Button("Place Dino")) { strcpy(barcodeHex, "F1706B7B6D69A38F"); }
    ImGui::SameLine();
    if (ImGui::Button("Dino Journal")) { strcpy(barcodeHex, "D6727936AF6BF6B4"); }

    ImGui::SetNextItemWidth(-80);
    ImGui::InputText("##barcode", barcodeHex, sizeof(barcodeHex));
    ImGui::SameLine();
    bool canInject = !g_BarcodeInject.pending && !g_SpawnRequest.pending;
    if (!canInject) ImGui::BeginDisabled();
    if (ImGui::Button("Inject", ImVec2(-1, 0))) {
        std::string hex = barcodeHex;
        if (hex.size() >= 16) {
            // Decode and show what it is
            auto decoded = PVDecode::decode(hex);
            if (decoded.valid) {
                Log("Barcode: " + PVDecode::formatCommands(decoded), 3);
                g_BarcodeInject.hexString = hex;
                g_BarcodeInject.pending = true;
            } else {
                Log("Decode error: " + decoded.error, 5);
            }
        } else {
            Log("Enter a 16+ character hex barcode string", 5);
        }
    }
    if (!canInject) ImGui::EndDisabled();
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Paste a PV barcode hex from barcodes.txt\nExamples:\nBF619A786BD25C9B = Elite Neon Choclodocus\nF1706B7B6D69A38F = Place Choclodocus\n96FEF696AB02C4A6 = Choclodocus Egg");
    }

    // Show decoded preview if there's text in the input
    if (strlen(barcodeHex) >= 16) {
        auto preview = PVDecode::decode(std::string(barcodeHex));
        if (preview.valid) {
            ImGui::TextWrapped("Decoded: %s", PVDecode::formatCommands(preview).c_str());
        }
    }

    // === Save Texture Patcher ===
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Save Texture Patcher:");

    static std::vector<SavePatcher::TextureEntry> scanResults;
    static std::string patchStatus;
    static int selectedVariant = 0;

    // Known variants for the dropdown
    static const char* dinoVariants[] = {"pink", "blue", "green", "red", "elite"};
    static const char* dragonVariants[] = {"dirt", "gold", "grass", "water", "snow", "sand"};

    if (ImGui::Button("Scan Current Save", ImVec2(-1, 0))) {
        auto savePath = SavePatcher::findSaveDir();
        if (!savePath.empty()) {
            scanResults = SavePatcher::scanSave(savePath);
            patchStatus = "Found " + std::to_string(scanResults.size()) + " variant textures in save";
        } else {
            patchStatus = "No save found!";
        }
    }

    if (!scanResults.empty()) {
        // Group by species
        std::map<std::string, std::string> speciesCurrentVariant;
        for (auto& e : scanResults) {
            speciesCurrentVariant[e.species] = e.variant;
        }

        for (auto& [species, currentVar] : speciesCurrentVariant) {
            ImGui::Text("%s: current = %s", species.c_str(), currentVar.c_str());

            const char** variants = nullptr;
            int varCount = 0;

            if (species.find("dinosaur") != std::string::npos) {
                variants = dinoVariants; varCount = 5;
            } else if (species.find("dragon") != std::string::npos) {
                variants = dragonVariants; varCount = 6;
            }

            if (variants) {
                std::string comboId = "##var_" + species;
                ImGui::SameLine();
                ImGui::SetNextItemWidth(80);
                ImGui::Combo(comboId.c_str(), &selectedVariant, variants, varCount);
                ImGui::SameLine();
                std::string btnId = "Patch##" + species;
                if (ImGui::Button(btnId.c_str())) {
                    auto savePath = SavePatcher::findSaveDir();
                    if (!savePath.empty()) {
                        patchStatus = SavePatcher::patchSave(savePath, species, currentVar, variants[selectedVariant]);
                        // Re-scan
                        scanResults = SavePatcher::scanSave(savePath);
                    }
                }
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1,1,0,1), "(close game first!)");
            }
        }
    }

    if (!patchStatus.empty()) {
        ImGui::TextWrapped("%s", patchStatus.c_str());
    }

    ImGui::Separator();

    // File access logging toggle
    extern std::ofstream g_FileLog;
    extern bool g_FileLogging;
    if (ImGui::Button(g_FileLogging ? "Stop File Log" : "Start File Log", ImVec2(-1, 0))) {
        g_FileLogging = !g_FileLogging;
        if (g_FileLogging) {
            g_FileLog.open("C:/Users/Administrator/Downloads/file_log.txt", std::ios::trunc);
            Log("File logging started", 3);
        } else {
            g_FileLog.close();
            Log("File logging stopped — check file_log.txt", 3);
        }
    }

    // Scan species button
    if (ImGui::Button("Scan All Species IDs (check log)", ImVec2(-1, 0))) {
        scanRequested = true;
    }

    ImGui::End();

    // Queue scan to run from the game logic hook (needs live PPC context)
    if (scanRequested) {
        scanRequested = false;
        ::g_ScanPending = true;
    }
}
