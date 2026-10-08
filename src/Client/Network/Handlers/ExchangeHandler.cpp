#include "ExchangeHandler.h"
#include "UserInterface/Packets/Packet_Exchange.h"
#include "UserInterface/Packet.h"

namespace Client::Network::Handlers {

EterBase::PacketResult<void> ExchangeHandler::HandleExchangePacket(
    const std::span<const uint8_t>& payload,
    Client::Gameplay::PlayerExchange* exchangeState,
    EterBase::EntityId selfId,
    EterBase::EntityId targetId)
{
    if (payload.size() < sizeof(TPacketGCExchange)) {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCExchange*>(payload.data());
    
    // Determine the acting player based on isInitiator flag.
    // In GC packets, isInitiator generally indicates if the action targets our client (self) or the other.
    EterBase::EntityId actingPlayer = packet->isInitiator ? selfId : targetId;

    switch (packet->subheader) {
        case ExchangeSub::GC::START: {
            // Usually handled by router to create PlayerExchange
            break;
        }
        case ExchangeSub::GC::ITEM_ADD: {
            if (!exchangeState) return EterBase::MakeError(EterBase::PacketError::SessionClosed);
            
            EterBase::ItemSlot slot{packet->itemPos.cell};
            EterBase::ItemVnum vnum{packet->value1};
            
            auto result = exchangeState->AddItem(actingPlayer, slot, vnum);
            if (!result) {
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }
            break;
        }
        case ExchangeSub::GC::ELK_ADD: {
            if (!exchangeState) return EterBase::MakeError(EterBase::PacketError::SessionClosed);
            
            Client::Gameplay::Gold amount{packet->value1};
            auto result = exchangeState->AddGold(actingPlayer, amount);
            if (!result) {
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }
            break;
        }
        case ExchangeSub::GC::ACCEPT: {
            if (!exchangeState) return EterBase::MakeError(EterBase::PacketError::SessionClosed);
            
            // GC packet uses packet->value1 to indicate accepted or just the subheader itself.
            // Looking at legacy code: HandleAccept uses packet->arg1. Here arg1 is value1.
            uint8_t isAccepted = static_cast<uint8_t>(packet->value1);
            if (isAccepted) {
                // Assuming Lock happens before Accept. Legacy just calls SetAcceptToSelf.
                // In modern TradeDomain, we need to Lock then Accept.
                auto lockRes = exchangeState->Lock(actingPlayer);
                if (!lockRes) {
                    // Ignoring lock errors if already locked
                }
                auto result = exchangeState->Accept(actingPlayer);
                if (!result) {
                    return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
                }
            }
            break;
        }
        case ExchangeSub::GC::END: {
            if (!exchangeState) return EterBase::MakeError(EterBase::PacketError::SessionClosed);
            exchangeState->Cancel(); // END implies closure or cancel. In legacy, End() closes window.
            break;
        }
        case ExchangeSub::GC::ITEM_DEL: {
            // Not directly supported in TradeDomain AddItem/AddGold/Lock/Accept/Cancel. 
            // In TradeDomain, we can't delete item. We ignore or it's out of scope of TradeDomain API for now.
            break;
        }
        case ExchangeSub::GC::ALREADY:
        case ExchangeSub::GC::LESS_ELK:
            // Error handling, maybe cancel
            if (exchangeState) exchangeState->Cancel();
            break;
        default:
            return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
    }
    
    return {};
}

} // namespace Client::Network::Handlers
