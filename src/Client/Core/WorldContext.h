#pragma once

#include <cstdint>
#include "StrongTypes.h"

namespace Client::Core {

/**
 * @brief WorldContext reprezentuje stan świata i gracza bez globalnych singletonów.
 */
struct WorldContext {
    EntityVid localPlayerVid{0};
    MapCoords localPlayerCoords{};
    float localPlayerRotation{0.0f};
    uint32_t currentMapIndex{0};
    uint32_t currentChannel{1};
    uint32_t currentHp{0};
    uint32_t maxHp{0};
    uint32_t currentSp{0};
    uint32_t maxSp{0};
    int64_t currentGold{0};
    int64_t currentCheque{0};
    int64_t currentGaya{0};
    bool isDead{false};

    void Reset() noexcept {
        localPlayerVid = EntityVid(0);
        localPlayerCoords = MapCoords{};
        localPlayerRotation = 0.0f;
        currentMapIndex = 0;
        currentChannel = 1;
        currentHp = 0;
        maxHp = 0;
        currentSp = 0;
        maxSp = 0;
        currentGold = 0;
        currentCheque = 0;
        currentGaya = 0;
        isDead = false;
    }
};

} // namespace Client::Core
