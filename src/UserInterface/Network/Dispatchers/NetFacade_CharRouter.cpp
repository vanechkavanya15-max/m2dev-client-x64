#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/Result.h"
#include "../../Core/EventBus.h"

#include <cstring>
#include <string>
#include <vector>
#include <span>

namespace UserInterface::Network::Dispatchers {

namespace {

    struct CharacterAddEvent : public Core::IEvent {
        EterBase::EntityId entityId;
        std::string name;
        float angle;
        int32_t x;
        int32_t y;
        int32_t z;
        uint8_t type;
        uint16_t raceNum;
        uint16_t parts[CHR_EQUIPPART_NUM]{};
        uint8_t movingSpeed;
        uint8_t attackSpeed;
        uint8_t stateFlag;
        uint32_t affectFlag[2]{};
        uint8_t empire;
        EterBase::GuildId guildId;
        int16_t alignment;
        uint8_t pkMode;
        EterBase::ItemVnum mountVnum;
    };

    struct CharacterUpdateEvent : public Core::IEvent {
        EterBase::EntityId entityId;
        uint16_t parts[CHR_EQUIPPART_NUM]{};
        uint8_t movingSpeed;
        uint8_t attackSpeed;
        uint8_t stateFlag;
        uint32_t affectFlag[2]{};
        EterBase::GuildId guildId;
        int16_t alignment;
        uint8_t pkMode;
        EterBase::ItemVnum mountVnum;
    };

    struct CharacterDeleteEvent : public Core::IEvent {
        EterBase::EntityId entityId;
    };

    struct CharacterMoveEvent : public Core::IEvent {
        EterBase::EntityId entityId;
        uint8_t func;
        uint8_t arg;
        uint8_t rot;
        int32_t x;
        int32_t y;
        uint32_t time;
        uint32_t duration;
    };

    struct SyncPositionElement {
        EterBase::EntityId entityId;
        int32_t x;
        int32_t y;
    };

    struct CharacterSyncPositionEvent : public Core::IEvent {
        std::vector<SyncPositionElement> positions;
    };

} // namespace anonymous

class NetFacadeCharRouter {
public:
    static EterBase::PacketResult<void> HandleCharacterAdd(std::span<const uint8_t> payload) {
        if (payload.size() < sizeof(TPacketGCCharacterAdd)) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCCharacterAdd packet;
        std::memcpy(&packet, payload.data(), sizeof(packet));

        CharacterAddEvent event;
        event.entityId = EterBase::EntityId(packet.dwVID);
        // packet_add_char doesn't have name field, but TPacketGCCharacterAdd2 has.
        // We will leave name empty for TPacketGCCharacterAdd
        event.angle = packet.angle;
        event.x = packet.x;
        event.y = packet.y;
        event.z = packet.z;
        event.type = packet.bType;
        event.raceNum = packet.wRaceNum;
        // awPart not in TPacketGCCharacterAdd
        event.movingSpeed = packet.bMovingSpeed;
        event.attackSpeed = packet.bAttackSpeed;
        event.stateFlag = packet.bStateFlag;
        event.affectFlag[0] = packet.dwAffectFlag[0];
        event.affectFlag[1] = packet.dwAffectFlag[1];
        
        EterBase::ModernLogger::Debug("NetFacadeCharRouter::HandleCharacterAdd - entityId: {}", packet.dwVID);
        Core::EventBus::GetInstance().Publish(event);

        return {};
    }

    static EterBase::PacketResult<void> HandleCharacterAdd2(std::span<const uint8_t> payload) {
        if (payload.size() < sizeof(TPacketGCCharacterAdd2)) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCCharacterAdd2 packet;
        std::memcpy(&packet, payload.data(), sizeof(packet));

        CharacterAddEvent event;
        event.entityId = EterBase::EntityId(packet.dwVID);
        
        size_t nameLen = strnlen(packet.name, sizeof(packet.name));
        event.name = std::string(packet.name, nameLen);

        event.angle = packet.angle;
        event.x = packet.x;
        event.y = packet.y;
        event.z = packet.z;
        event.type = packet.bType;
        event.raceNum = packet.wRaceNum;
        
        for (size_t i = 0; i < CHR_EQUIPPART_NUM; ++i) {
            event.parts[i] = packet.awPart[i];
        }

        event.movingSpeed = packet.bMovingSpeed;
        event.attackSpeed = packet.bAttackSpeed;
        event.stateFlag = packet.bStateFlag;
        event.affectFlag[0] = packet.dwAffectFlag[0];
        event.affectFlag[1] = packet.dwAffectFlag[1];
        event.empire = packet.bEmpire;
        event.guildId = EterBase::GuildId(packet.dwGuild);
        event.alignment = packet.sAlignment;
        event.pkMode = packet.bPKMode;
        event.mountVnum = EterBase::ItemVnum(packet.dwMountVnum);

        EterBase::ModernLogger::Debug("NetFacadeCharRouter::HandleCharacterAdd2 - entityId: {}, name: {}", packet.dwVID, event.name);
        Core::EventBus::GetInstance().Publish(event);

        return {};
    }

    static EterBase::PacketResult<void> HandleCharacterUpdate(std::span<const uint8_t> payload) {
        if (payload.size() < sizeof(TPacketGCCharacterUpdate)) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCCharacterUpdate packet;
        std::memcpy(&packet, payload.data(), sizeof(packet));

        CharacterUpdateEvent event;
        event.entityId = EterBase::EntityId(packet.dwVID);
        
        for (size_t i = 0; i < CHR_EQUIPPART_NUM; ++i) {
            event.parts[i] = packet.awPart[i];
        }

        event.movingSpeed = packet.bMovingSpeed;
        event.attackSpeed = packet.bAttackSpeed;
        event.stateFlag = packet.bStateFlag;
        event.affectFlag[0] = packet.dwAffectFlag[0];
        event.affectFlag[1] = packet.dwAffectFlag[1];
        event.guildId = EterBase::GuildId(packet.dwGuildID);
        event.alignment = packet.sAlignment;
        event.pkMode = packet.bPKMode;
        event.mountVnum = EterBase::ItemVnum(packet.dwMountVnum);

        EterBase::ModernLogger::Debug("NetFacadeCharRouter::HandleCharacterUpdate - entityId: {}", packet.dwVID);
        Core::EventBus::GetInstance().Publish(event);

        return {};
    }

    static EterBase::PacketResult<void> HandleCharacterUpdate2(std::span<const uint8_t> payload) {
        if (payload.size() < sizeof(TPacketGCCharacterUpdate2)) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCCharacterUpdate2 packet;
        std::memcpy(&packet, payload.data(), sizeof(packet));

        CharacterUpdateEvent event;
        event.entityId = EterBase::EntityId(packet.dwVID);
        
        for (size_t i = 0; i < CHR_EQUIPPART_NUM; ++i) {
            event.parts[i] = packet.awPart[i];
        }

        event.movingSpeed = packet.bMovingSpeed;
        event.attackSpeed = packet.bAttackSpeed;
        event.stateFlag = packet.bStateFlag;
        event.affectFlag[0] = packet.dwAffectFlag[0];
        event.affectFlag[1] = packet.dwAffectFlag[1];
        event.guildId = EterBase::GuildId(packet.dwGuildID);
        event.alignment = packet.sAlignment;
        event.pkMode = packet.bPKMode;
        event.mountVnum = EterBase::ItemVnum(packet.dwMountVnum);

        EterBase::ModernLogger::Debug("NetFacadeCharRouter::HandleCharacterUpdate2 - entityId: {}", packet.dwVID);
        Core::EventBus::GetInstance().Publish(event);

        return {};
    }

    static EterBase::PacketResult<void> HandleCharacterDelete(std::span<const uint8_t> payload) {
        if (payload.size() < sizeof(TPacketGCCharacterDelete)) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCCharacterDelete packet;
        std::memcpy(&packet, payload.data(), sizeof(packet));

        CharacterDeleteEvent event;
        event.entityId = EterBase::EntityId(packet.dwVID);

        EterBase::ModernLogger::Debug("NetFacadeCharRouter::HandleCharacterDelete - entityId: {}", packet.dwVID);
        Core::EventBus::GetInstance().Publish(event);

        return {};
    }

    static EterBase::PacketResult<void> HandleCharacterMove(std::span<const uint8_t> payload) {
        if (payload.size() < sizeof(TPacketGCMove)) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCMove packet;
        std::memcpy(&packet, payload.data(), sizeof(packet));

        CharacterMoveEvent event;
        event.entityId = EterBase::EntityId(packet.dwVID);
        event.func = packet.bFunc;
        event.arg = packet.bArg;
        event.rot = packet.bRot;
        event.x = packet.lX;
        event.y = packet.lY;
        event.time = packet.dwTime;
        event.duration = packet.dwDuration;

        EterBase::ModernLogger::Debug("NetFacadeCharRouter::HandleCharacterMove - entityId: {}, x: {}, y: {}", packet.dwVID, packet.lX, packet.lY);
        Core::EventBus::GetInstance().Publish(event);

        return {};
    }

    static EterBase::PacketResult<void> HandleCharacterSyncPosition(std::span<const uint8_t> payload) {
        if (payload.size() < sizeof(TPacketGCSyncPosition)) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCSyncPosition packet;
        std::memcpy(&packet, payload.data(), sizeof(packet));

        size_t remainingBytes = payload.size() - sizeof(TPacketGCSyncPosition);
        if (remainingBytes % sizeof(TPacketGCSyncPositionElement) != 0) {
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        size_t elementCount = remainingBytes / sizeof(TPacketGCSyncPositionElement);
        
        CharacterSyncPositionEvent event;
        event.positions.reserve(elementCount);

        const uint8_t* elementsData = payload.data() + sizeof(TPacketGCSyncPosition);
        
        for (size_t i = 0; i < elementCount; ++i) {
            TPacketGCSyncPositionElement element;
            std::memcpy(&element, elementsData + (i * sizeof(TPacketGCSyncPositionElement)), sizeof(element));
            
            SyncPositionElement evElement;
            evElement.entityId = EterBase::EntityId(element.dwVID);
            evElement.x = element.lX;
            evElement.y = element.lY;
            event.positions.push_back(evElement);
        }

        EterBase::ModernLogger::Debug("NetFacadeCharRouter::HandleCharacterSyncPosition - count: {}", elementCount);
        Core::EventBus::GetInstance().Publish(event);

        return {};
    }
};

} // namespace UserInterface::Network::Dispatchers
