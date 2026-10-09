#include "ItemGroundAddPacketHandler.h"
 #include "../../../EterBase/Result.h"
 #include "../../../EterBase/StrongTypes.h"
 #include "Client/Core/EventBus.h"
 
namespace Client::Network
 {
    EterBase::PacketResult<void> ItemGroundAddPacketHandler::Handle(std::span<const uint8_t> payload)
     {
        if (payload.size() < sizeof(PacketItemGroundAdd))
         {
             return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
         }
 
        const auto* packet = reinterpret_cast<const PacketItemGroundAdd*>(payload.data());
 
         // Dekodowanie silnie typowanych identyfikatorów
         EterBase::EntityId dropVid(packet->dwVID);
         EterBase::ItemVnum itemVnum(packet->dwVnum);
 
        // Dekodowanie wspolrzednych i konwersja
         float localX = static_cast<float>(packet->lX);
         float localY = static_cast<float>((packet->lY > 10) ? (packet->lY * 100) : packet->lY);
         float localZ = static_cast<float>(packet->lZ);
