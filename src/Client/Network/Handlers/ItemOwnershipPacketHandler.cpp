#include "ItemOwnershipPacketHandler.h"
#include <cstring>
#include <algorithm>

namespace Client::Network::Handlers
{
    EterBase::PacketResult<void> ProcessItemOwnership(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketItemOwnership))
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketItemOwnership*>(buffer.data());

        EterBase::EntityId vid(packet->dwVID);

        // Bezpieczne zakonczenie ciagu znakow null
        char safeName[Client::Network::Protocol::CHARACTER_NAME_MAX_LEN + 1];
        std::copy_n(packet->szName, Client::Network::Protocol::CHARACTER_NAME_MAX_LEN + 1, safeName);
        safeName[Client::Network::Protocol::CHARACTER_NAME_MAX_LEN] = '\0';
        
        std::string ownerName(safeName);

        ItemOwnershipEvent event(vid, std::move(ownerName));
        Client::Core::EventBus::GetInstance().Publish(event);

        return {};
    }
}
