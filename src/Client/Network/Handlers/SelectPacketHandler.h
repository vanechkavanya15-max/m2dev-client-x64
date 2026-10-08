#pragma once

#include <span>
#include <cstdint>
#include <string>
#include "../../../EterBase/PacketResult.h"
#include "../../../EterBase/StrongTypes.h"
#include "Client/Core/EventBus.h"

namespace Client::Network::Handlers {

struct CharacterSlotData {
    uint32_t id;
    std::string name;
    uint8_t job;
    uint8_t level;
    uint32_t playMinutes;
    uint8_t str;
    uint8_t ht;
    uint8_t dex;
    uint8_t iq;
    uint16_t mainPart;
    uint16_t hairPart;
    uint8_t skillGroup;
    bool changeName;
    uint8_t slotIndex;
    uint32_t guildId;
    std::string guildName;
};

struct CharacterSlotUpdatedEvent : public Client::Core::IEvent {
    CharacterSlotData slotData;

    explicit CharacterSlotUpdatedEvent(const CharacterSlotData& data)
        : slotData(data) {}
};

struct CharacterSlotDeletedEvent : public Client::Core::IEvent {
    uint8_t slotIndex;

    explicit CharacterSlotDeletedEvent(uint8_t index)
        : slotIndex(index) {}
};

class SelectPacketHandler {
public:
    static EterBase::PacketResult<void> HandleLoginSuccess4(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleCreateSuccess(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleDeleteSuccess(std::span<const uint8_t> payload);
};

} // namespace Client::Network::Handlers
