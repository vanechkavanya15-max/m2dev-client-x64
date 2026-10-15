#include "ActorAddPacketHandler.h"
#include <cstring>

namespace Client::Network::Handlers {

ActorAddPacketHandler::ActorAddPacketHandler(Client::World::ActorRegistry& registry) noexcept
    : m_registry(registry)
{
}

EterBase::PacketResult<void> ActorAddPacketHandler::Handle(std::span<const uint8_t> payload)
{
    if (payload.size() < sizeof(TPacketGCCharacterAdd)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCCharacterAdd packet;
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCCharacterAdd));

    Client::World::ActorRecord record;
    record.vid = EterBase::EntityId(packet.id);
    record.race = packet.raceNum;
    record.type = packet.type;
    record.x = static_cast<float>(packet.x);
    record.y = static_cast<float>(packet.y);
    record.z = static_cast<float>(packet.z);
    record.rotation = packet.angle;
    // Pozostale pola sa domyslnie inicjalizowane (isDead = false, itp.) wewnatrz ActorRecord.
    record.isDead = false;
    record.guildId = 0;
    record.empire = 0;
    // name is empty string by default

    if (!m_registry.RegisterActor(record)) {
        // Zgodnie z wytycznymi mozna zwrocic blad, jezeli aktor juz istnieje lub dane sa bledne
        return std::unexpected(EterBase::PacketError::MalformedPayload);
    }

    return {};
}

uint16_t ActorAddPacketHandler::GetExpectedSize() const
{
    return sizeof(TPacketGCCharacterAdd);
}

bool ActorAddPacketHandler::IsDynamicSize() const
{
    return false;
}

} // namespace Client::Network::Handlers
