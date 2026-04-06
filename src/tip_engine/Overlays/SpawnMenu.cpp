#include "SpawnMenu.h"
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

    ImGui::SetNextWindowSize(ImVec2(420.0f, 520.0f), ImGuiCond_FirstUseEver);

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
        g_SpawnRequest.pending = true;
        Log("Spawn requested: " + std::string(g_PinataIDs[selectedIndex].Name)
            + (variantIndex >= 0 ? " (variant " + std::to_string(variantIndex) + ")" : ""), 3);
    }

    if (!canSpawn) ImGui::EndDisabled();

    // Scan species button - discovers all valid tag IDs and their names
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
