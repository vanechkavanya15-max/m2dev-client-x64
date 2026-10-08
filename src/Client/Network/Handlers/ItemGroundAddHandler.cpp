#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "ItemGroundAddHandler.h"

// Systemy są odseparowane przez EventBus, Zero-Conflict
#include "../../../UserInterface/Core/EventBus.h"

namespace Client::Network::Handlers
{
    EterBase::PacketResult<void> ProcessItemGroundAdd(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketItemGroundAdd))
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketItemGroundAdd*>(buffer.data());

        // Dekodowanie silnie typowanych identyfikatorów
        EterBase::EntityId dropVid(packet->dwVID);
        EterBase::ItemVnum itemVnum(packet->dwVnum);

        // Dekodowanie wspolrzednych i konwersja (legacy scale z ItemDispatcher_GroundAdd)
        // Global to Local translation / scaling to real coordinates handled by the system
        // listening to this event. We just pass domain structure.
        float localX = static_cast<float>(packet->lX);
        float localY = static_cast<float>((packet->lY > 10) ? (packet->lY * 100) : packet->lY);
        float localZ = static_cast<float>(packet->lZ);

        Client::World::MapCoords coords{localX, localY, localZ};

        // Publikacja zdarzenia z pusta etykieta wlasnosci
        ItemGroundAddEvent event(dropVid, itemVnum, coords, "");
        UserInterface::Core::EventBus::GetInstance().Publish(event);

        return {};
    }
}
