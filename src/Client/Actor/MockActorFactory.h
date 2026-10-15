#pragma once

#include <string>
#include <string_view>
#include <cstdint>

#include "../Core/Result.h"
#include "../World/ActorRegistry.h"

namespace Client::Actor {

/**
 * @class MockActorFactory
 * @brief Zero-Conflict Headless Actor Factory dla testow jednostkowych (C++23).
 *
 * Umozliwia generowanie struktur 'Client::World::ActorRecord' 
 * calkowicie w pamieci RAM (bez UI i obslugi DirectX).
 */
class MockActorFactory {
public:
    enum class ActorType : uint8_t {
        Player = 0,
        Monster = 1,
        NPC = 2
    };

    /**
     * @brief Tworzy aktora z wymaganymi atrybutami, walidujac dane wejsciowe.
     */
    [[nodiscard]] static Client::Core::Result<Client::World::ActorRecord, Client::Core::EntityError> CreateActor(
        Client::World::EntityVid vid,
        ActorType type,
        uint32_t race,
        float x,
        float y,
        float z,
        float rotation,
        std::string_view name,
        uint32_t guildId,
        uint8_t empire) noexcept
    {
        if (vid.value() == 0) {
            return std::unexpected(Client::Core::EntityError::NotFound); // Using NotFound as Invalid is not defined, mapping 0 to an error
        }
        
        Client::World::ActorRecord record{
            vid,
            race,
            static_cast<uint8_t>(type),
            x,
            y,
            z,
            rotation,
            std::string(name),
            guildId,
            empire,
            false // isDead
        };
        
        return record;
    }

    [[nodiscard]] static Client::Core::Result<Client::World::ActorRecord, Client::Core::EntityError> CreatePlayer(
        Client::World::EntityVid vid,
        std::string_view name,
        float x = 0.0f,
        float y = 0.0f,
        uint8_t empire = 1) noexcept
    {
        return CreateActor(vid, ActorType::Player, 0, x, y, 0.0f, 0.0f, name, 0, empire);
    }

    [[nodiscard]] static Client::Core::Result<Client::World::ActorRecord, Client::Core::EntityError> CreateMonster(
        Client::World::EntityVid vid,
        uint32_t race,
        float x = 0.0f,
        float y = 0.0f) noexcept
    {
        return CreateActor(vid, ActorType::Monster, race, x, y, 0.0f, 0.0f, "Monster", 0, 0);
    }

    [[nodiscard]] static Client::Core::Result<Client::World::ActorRecord, Client::Core::EntityError> CreateNPC(
        Client::World::EntityVid vid,
        uint32_t race,
        std::string_view name,
        float x = 0.0f,
        float y = 0.0f) noexcept
    {
        return CreateActor(vid, ActorType::NPC, race, x, y, 0.0f, 0.0f, name, 0, 0);
    }
};

} // namespace Client::Actor
