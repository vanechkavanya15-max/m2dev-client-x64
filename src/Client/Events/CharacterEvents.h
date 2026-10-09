#pragma once

#include "EterBase/EventBus.h"
#include "Client/Core/StrongTypes.h"
#include <optional>

namespace Client::Events {

/**
 * @brief Zdarzenie domenowe powolania bytu do zycia na scenie (Spawn).
 * W 100% zaimplementowane przy uzyciu nowozytnych standardow C++23.
 */
struct CharacterSpawnEvent : public EterBase::IEvent {
    Client::Core::EntityVid vid;
    Client::Core::RaceVnum raceVnum;
    Client::Core::MapCoords initialCoords;
    
    constexpr CharacterSpawnEvent() noexcept = default;
    
    constexpr CharacterSpawnEvent(Client::Core::EntityVid vid, Client::Core::RaceVnum raceVnum, Client::Core::MapCoords coords) noexcept
        : vid(vid), raceVnum(raceVnum), initialCoords(coords) {}
        
    constexpr bool operator==(const CharacterSpawnEvent& other) const noexcept {
        return vid == other.vid && raceVnum == other.raceVnum && initialCoords == other.initialCoords;
    }
};

/**
 * @brief Zdarzenie domenowe znikania/usuwania bytu ze sceny (Despawn).
 */
struct CharacterDespawnEvent : public EterBase::IEvent {
    Client::Core::EntityVid vid;
    
    constexpr CharacterDespawnEvent() noexcept = default;
    
    constexpr explicit CharacterDespawnEvent(Client::Core::EntityVid vid) noexcept
        : vid(vid) {}
        
    constexpr bool operator==(const CharacterDespawnEvent& other) const noexcept {
        return vid == other.vid;
    }
};

/**
 * @brief Zdarzenie domenowe przemieszczania sie bytu po mapie.
 * Z opcjonalnymi wspolrzednymi docelowymi.
 */
struct CharacterMoveEvent : public EterBase::IEvent {
    Client::Core::EntityVid vid;
    Client::Core::MapCoords currentCoords;
    std::optional<Client::Core::MapCoords> destinationCoords;
    
    constexpr CharacterMoveEvent() noexcept = default;
    
    constexpr CharacterMoveEvent(Client::Core::EntityVid vid, Client::Core::MapCoords current, std::optional<Client::Core::MapCoords> destination = std::nullopt) noexcept
        : vid(vid), currentCoords(current), destinationCoords(destination) {}
        
    constexpr bool operator==(const CharacterMoveEvent& other) const noexcept {
        return vid == other.vid && currentCoords == other.currentCoords && destinationCoords == other.destinationCoords;
    }
};

} // namespace Client::Events

namespace Client::Core {
    using CharacterSpawnEvent = ::Client::Events::CharacterSpawnEvent;
    using CharacterDespawnEvent = ::Client::Events::CharacterDespawnEvent;
    using CharacterMoveEvent = ::Client::Events::CharacterMoveEvent;
}
