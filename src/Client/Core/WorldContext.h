#pragma once

#include <cstdint>
#include "StrongTypes.h"

namespace Client::Core {

struct WorldContext {
    uint32_t currentSp{0};
    uint32_t maxSp{0};
    int64_t currentGold{0};
     int64_t currentCheque{0};
     int64_t currentGaya{0};
    bool isDead{false};
    
    MapCoords localPlayerCoords;
    float localPlayerRotation{0.0f};

    void Reset() noexcept {
        currentSp = 0;
        maxSp = 0;
        currentGold = 0;
         currentCheque = 0;
         currentGaya = 0;
        isDead = false;
        localPlayerCoords = MapCoords{};
        localPlayerRotation = 0.0f;
    }
};

} // namespace Client::Core
