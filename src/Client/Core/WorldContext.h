#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include "StrongTypes.h"

namespace Client::Core {

struct WorldEntity {
    uint32_t vid{0};
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
    bool isHostile{false};
};

struct WorldContext {
    uint32_t currentHp{0};
    uint32_t maxHp{0};
    uint32_t currentSp{0};
    uint32_t maxSp{0};
    uint64_t currentExp{0};
    int64_t currentGold{0};
    int64_t currentCheque{0};
    int64_t currentGaya{0};
    uint32_t currentMapIndex{0};
    uint32_t currentChannel{1};
    bool isDead{false};
    
    EntityVid localPlayerVid{0};
    MapCoords localPlayerCoords;
    float localPlayerRotation{0.0f};
    float posX{0.0f};
    float posY{0.0f};
    float posZ{0.0f};

    struct InventoryItem {
        uint16_t slot{0};
        uint32_t vnum{0};
        uint32_t count{0};
    };
    std::vector<InventoryItem> inventory;

    std::vector<WorldEntity> entities;

    std::array<int64_t, 255> points{};

    [[nodiscard]] int64_t GetPoint(uint32_t pointType) const noexcept {
        if (pointType >= points.size()) return 0;
        return points[pointType];
    }

    void SetPoint(uint32_t pointType, int64_t value) noexcept {
        if (pointType >= points.size()) return;
        points[pointType] = value;
        switch (pointType) {
            case 5:  // POINT_HP
                currentHp = static_cast<uint32_t>(value);
                break;
            case 6:  // POINT_MAX_HP
                maxHp = static_cast<uint32_t>(value);
                break;
            case 7:  // POINT_SP
                currentSp = static_cast<uint32_t>(value);
                break;
            case 8:  // POINT_MAX_SP
                maxSp = static_cast<uint32_t>(value);
                break;
            case 3:  // POINT_EXP
                currentExp = static_cast<uint64_t>(value);
                break;
            case 11: // POINT_GOLD
                currentGold = value;
                break;
            default:
                break;
        }
    }

    void Reset() noexcept {
        currentHp = 0;
        maxHp = 0;
        currentSp = 0;
        maxSp = 0;
        currentExp = 0;
        currentGold = 0;
        currentCheque = 0;
        currentGaya = 0;
        currentMapIndex = 0;
        currentChannel = 1;
        isDead = false;
        localPlayerVid = EntityVid{0};
        localPlayerCoords = MapCoords{};
        localPlayerRotation = 0.0f;
        posX = 0.0f;
        posY = 0.0f;
        posZ = 0.0f;
        inventory.clear();
        entities.clear();
        points.fill(0);
    }
};

} // namespace Client::Core
