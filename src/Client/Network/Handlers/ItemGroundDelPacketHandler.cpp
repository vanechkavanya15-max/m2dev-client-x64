#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "ItemGroundDelPacketHandler.h"

// Systemy sa odseparowane przez EventBus, Zero-Conflict
#include "Client/Core/EventBus.h"

namespace Client::Network::Handlers
{
    EterBase::PacketResult<void> ProcessItemGroundDel(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketItemGroundDel))
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketItemGroundDel*>(buffer.data());

        // Dekodowanie silnie typowanego identyfikatora
        EterBase::EntityId dropVid(packet->dwVID);

        // Publikacja zdarzenia
        ItemGroundDelEvent event(dropVid);
        Client::Core::EventBus::GetInstance().Publish(event);

        return {};
    }
}
