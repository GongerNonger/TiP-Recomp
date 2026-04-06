#pragma once
#include <rex/ui/imgui_dialog.h>
#include "imgui.h"
#include "tip_engine/Types/VivaClassTypes.h"
#include <cstring>

// Special tag IDs for non-standard spawns
constexpr uint32_t SPECIAL_AMBER_EVENT = 60000;

// Global spawn request state - checked by the spawn hook each tick
struct SpawnRequest {
    bool pending = false;
    uint32_t tagID = 0;
    int variantIndex = -1; // -1 = default, 0+ = specific color variant (wildcard)
    int dinoColor = -1;    // -1 = default, 0=Blue, 1=Green, 2=Red, 3=Elite Neon
};

inline SpawnRequest g_SpawnRequest;
inline bool g_SpawnMenuOpen = false;
inline uint32_t g_LastSpawnedEntity = 0; // PPC address of last spawned entity
inline bool g_ScanPending = false;

// Barcode injection request
struct BarcodeInjectRequest {
    bool pending = false;
    std::string hexString;
};
inline BarcodeInjectRequest g_BarcodeInject;

// Deferred variant application (needs to wait a frame for entity to initialize)
struct DeferredVariant {
    uint32_t entity = 0;
    int variantIndex = -1;
    int framesRemaining = 0;
};

// Deferred texture dump (entity needs time to fully initialize)
struct DeferredTextureDump {
    uint32_t entity = 0;
    uint32_t tagID = 0;
    int framesRemaining = 0;
};
inline DeferredTextureDump g_DeferredDump;
inline DeferredVariant g_DeferredVariant;

class SpawnMenuDialog : public rex::ui::ImGuiDialog {
public:
    explicit SpawnMenuDialog(rex::ui::ImGuiDrawer* drawer)
        : rex::ui::ImGuiDialog(drawer) {}

    void OnDraw(ImGuiIO& io) override;

private:
    char searchBuffer[128] = {};
    int selectedIndex = -1;
    int categoryFilter = 0; // 0=All, 1=Animals, 2=Eggs, 3=Seeds, 4=Homes, 5=Props, 6=Trees/Plants, 7=Other
    bool scanRequested = false;
    bool scanComplete = false;

    static int GetCategory(const PinataIDs& item) {
        uint32_t id = item.ID;
        // Animals: 3-160
        if (id >= 3 && id <= 160) return 1;
        // Crates: 171-173
        if (id >= 171 && id <= 173) return 7;
        // Eggs: 184-320
        if (id >= 184 && id <= 320) return 2;
        // Fruits/Gems/Produce: 332-358
        if (id >= 332 && id <= 358) return 7;
        // NPCs: 361-382
        if (id >= 361 && id <= 382) return 7;
        // Seeds: 399-456
        if (id >= 399 && id <= 456) return 3;
        // Homes: 470-613
        if (id >= 470 && id <= 613) return 4;
        // Surfaces/Materials: 618-662
        if (id >= 618 && id <= 662) return 7;
        // Props/Decorations: 665-770
        if (id >= 665 && id <= 770) return 5;
        // Trees/Plants: 877-964
        if (id >= 877 && id <= 964) return 6;
        // Shovel parts: 977-1004
        if (id >= 977 && id <= 1004) return 7;
        // Life Candy: 1501-1581
        if (id >= 1501 && id <= 1581) return 7;
        // Accessories: 1660+
        if (id >= 1660) return 7;
        // Sites: 1306-1449
        if (id >= 1306 && id <= 1449) return 4;
        // Flowerheads: 1454-1470
        if (id >= 1454 && id <= 1470) return 6;
        // Vegetables: 1488-1495
        if (id >= 1488 && id <= 1495) return 6;
        // Candy: 1218-1238
        if (id >= 1218 && id <= 1238) return 7;
        // Paving: 1283-1292
        if (id >= 1283 && id <= 1292) return 5;
        // Fertilizer: 1600-1608
        if (id >= 1600 && id <= 1608) return 7;
        return 7; // Other
    }

    static const char* GetCategoryName(int cat) {
        switch (cat) {
            case 0: return "All";
            case 1: return "Animals";
            case 2: return "Eggs";
            case 3: return "Seeds";
            case 4: return "Homes/Sites";
            case 5: return "Props/Decor";
            case 6: return "Trees/Plants";
            case 7: return "Other";
            default: return "Unknown";
        }
    }

    static bool MatchesSearch(const char* name, const char* search) {
        if (search[0] == '\0') return true;
        // Case-insensitive substring search
        const char* n = name;
        size_t slen = strlen(search);
        while (*n) {
            bool match = true;
            for (size_t i = 0; i < slen && n[i]; i++) {
                char a = n[i];
                char b = search[i];
                if (a >= 'A' && a <= 'Z') a += 32;
                if (b >= 'A' && b <= 'Z') b += 32;
                if (a != b) { match = false; break; }
            }
            if (match) return true;
            n++;
        }
        return false;
    }
};
