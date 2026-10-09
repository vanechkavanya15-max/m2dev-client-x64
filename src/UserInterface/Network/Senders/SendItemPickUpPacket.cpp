#include "StdAfx.h"
#include "Client/Network/Protocol/BeaviumProtocol.h"
#include "../../../EterLib/NetStream.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

#include <cstdint>
#include <span>
#include <cstring>
#include "../../Packet.h"

namespace Core {
    /**
     * @brief Event triggered when an item pickup packet is successfully sent.
     */
    struct ItemPickUpEvent : public UserInterface::Core::IEvent {
        uint32_t dropVid{0};

        ItemPickUpEvent() = default;
        explicit ItemPickUpEvent(uint32_t vid) : dropVid(vid) {}
    };
}

namespace {
#pragma pack(push, 1)
    /**
     * @brief Proxy structure representing the item pickup packet sent to the server.
     * 
     * Ensures strict 1-byte alignment to match network protocol specifications.
     */
    struct ProxyPacketCGItemPickUp
    {
        uint8_t  header; ///< Packet header identifier.
        uint32_t vid;    ///< Virtual ID of the item to pick up.
    };
    static_assert(sizeof(ProxyPacketCGItemPickUp) == 5, "ProxyPacketCGItemPickUp must be 5 bytes");
#pragma pack(pop)
}

namespace Network
{
    /**
     * @brief Handler responsible for sending item pickup packets securely and correctly aligned.
     */
    class SendItemPickUpHandler
    {
    public:
        /**
         * @brief Sends an item pickup request packet to the server for a specific item on the ground.
         * 
         * @param dropVid The strong-typed EntityId of the item to pick up.
         * @param networkStream Pointer to the network stream used to send the payload.
         * 
         * @return A PacketResult indicating success or a specific PacketError on failure.
         */
        static EterBase::PacketResult<void> SendItemPickUp(EterBase::EntityId dropVid, CNetworkStream* networkStream)
        {
            if (!networkStream)
            {
                EterBase::ModernLogger::Error("Failed to send item pickup packet: networkStream is null.");
                return EterBase::MakeError(EterBase::PacketError::SessionClosed);
            }

            if (dropVid.value() == 0)
            {
                EterBase::ModernLogger::Error("Failed to send item pickup packet: dropVid is invalid (0).");
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            ProxyPacketCGItemPickUp pickUpPacket;
            std::memset(&pickUpPacket, 0, sizeof(pickUpPacket));
            pickUpPacket.header = static_cast<uint8_t>(::CG::ITEM_PICKUP);
            pickUpPacket.vid = dropVid.value();

            std::span<const uint8_t> pickUpBuffer(reinterpret_cast<const uint8_t*>(&pickUpPacket), sizeof(pickUpPacket));
            
            if (!networkStream->Send(static_cast<int>(pickUpBuffer.size()), pickUpBuffer.data()))
            {
                EterBase::ModernLogger::Error("Failed to send item pickup packet: Send() returned false.");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            // Emit event using Core::EventBus::Instance() as required
            UserInterface::Core::EventBus::Instance().Publish(Core::ItemPickUpEvent(dropVid.value()));

            EterBase::ModernLogger::Info("Item pickup packet sent successfully for VID: {}", dropVid.value());

            return {};
        }
    };
}
