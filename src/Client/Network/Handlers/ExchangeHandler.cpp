#include "StdAfx.h"
#include "ExchangeHandler.h"
#include "../Protocol/Protocol.h"

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
    
    // Determine the acting player based on is_me flag.
    EterBase::EntityId actingPlayer = packet->is_me ? selfId : targetId;

    switch (packet->subheader) {
        case ExchangeSub::GC::START: {
            break;
        }
        case ExchangeSub::GC::ITEM_ADD: {
            if (!exchangeState) return EterBase::MakeError(EterBase::PacketError::SessionClosed);
            
            EterBase::ItemSlot slot{packet->arg2.cell};
            EterBase::ItemVnum vnum{packet->arg1};
            
            auto result = exchangeState->AddItem(actingPlayer, slot, vnum);
            if (!result) {
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }
            break;
        }
        case ExchangeSub::GC::ELK_ADD: {
            if (!exchangeState) return EterBase::MakeError(EterBase::PacketError::SessionClosed);
            
            Client::Gameplay::Gold amount{packet->arg1};
            auto result = exchangeState->AddGold(actingPlayer, amount);
            if (!result) {
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }
            break;
        }
        case ExchangeSub::GC::ACCEPT: {
            if (!exchangeState) return EterBase::MakeError(EterBase::PacketError::SessionClosed);
            
            uint8_t isAccepted = static_cast<uint8_t>(packet->arg1);
            if (isAccepted) {
                auto lockRes = exchangeState->Lock(actingPlayer);
                (void)lockRes;
                auto result = exchangeState->Accept(actingPlayer);
                if (!result) {
                    return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
                }
            }
            break;
        }
        case ExchangeSub::GC::END: {
            if (!exchangeState) return EterBase::MakeError(EterBase::PacketError::SessionClosed);
            (void)exchangeState->Cancel();
            break;
        }
        case ExchangeSub::GC::ITEM_DEL: {
            break;
        }
        case ExchangeSub::GC::ALREADY:
        case ExchangeSub::GC::LESS_ELK:
            if (exchangeState) (void)exchangeState->Cancel();
            break;
        default:
            return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
    }
    
    return {};
}

} // namespace Client::Network::Handlers
