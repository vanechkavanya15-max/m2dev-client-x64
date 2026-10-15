#include "../../StdAfx.h"
#include "CharDispatcher_Spawn.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/LogModern.h"
#include <string_view>
#include <cstring>
#include <algorithm>

namespace Network::Dispatchers
{
    EterBase::PacketResult<void> ProcessCharacterAdd(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCCharacterAdd))
        {
            EterBase::ModernLogger::Error("ProcessCharacterAdd: Buffer underflow (expected at least {} bytes, got {})", 
                sizeof(TPacketGCCharacterAdd), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCCharacterAdd*>(buffer.data());

        CharacterSpawnEvent event{
            .vid = EterBase::EntityId{packet->dwVID},
            .angle = packet->angle,
            .x = packet->x,
            .y = packet->y,
            .z = packet->z,
            .type = packet->bType,
            .raceNum = packet->wRaceNum,
            .movingSpeed = packet->bMovingSpeed,
            .attackSpeed = packet->bAttackSpeed,
            .stateFlag = packet->bStateFlag,
            .affectFlag = {packet->dwAffectFlag[0], packet->dwAffectFlag[1]}
        };

        EterBase::ModernLogger::Debug("ProcessCharacterAdd: Spawned VID={}, Race={}", packet->dwVID, packet->wRaceNum);

        UserInterface::Core::EventBus::GetInstance().Publish(event);

        return {};
    }

    EterBase::PacketResult<void> ProcessCharacterAdd2(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCCharacterAdd2))
        {
            EterBase::ModernLogger::Error("ProcessCharacterAdd2: Buffer underflow (expected at least {} bytes, got {})", 
                sizeof(TPacketGCCharacterAdd2), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCCharacterAdd2*>(buffer.data());

        std::string_view nameView(packet->name, strnlen(packet->name, sizeof(packet->name)));

        CharacterSpawnEvent2 event{
            .vid = EterBase::EntityId{packet->dwVID},
            .name = std::string(nameView),
            .angle = packet->angle,
            .x = packet->x,
            .y = packet->y,
            .z = packet->z,
            .type = packet->bType,
            .raceNum = packet->wRaceNum,
            .parts = {},
            .movingSpeed = packet->bMovingSpeed,
            .attackSpeed = packet->bAttackSpeed,
            .stateFlag = packet->bStateFlag,
            .affectFlag = {packet->dwAffectFlag[0], packet->dwAffectFlag[1]},
            .empire = packet->bEmpire,
            .guildId = EterBase::GuildId{packet->dwGuild},
            .alignment = packet->sAlignment,
            .pkMode = packet->bPKMode,
            .mountVnum = EterBase::ItemVnum{packet->dwMountVnum}
        };

        std::copy(std::begin(packet->awPart), std::end(packet->awPart), event.parts.begin());

        EterBase::ModernLogger::Debug("ProcessCharacterAdd2: Spawned VID={}, Name={}, Race={}", packet->dwVID, event.name, packet->wRaceNum);

        UserInterface::Core::EventBus::GetInstance().Publish(event);

        return {};
    }

} // namespace Network::Dispatchers
