#include "../../StdAfx.h"
/**
 * @file InterDispatcher_ExchangeItem.cpp
 * @brief Dispatcher for Exchange Item events (ADD/DEL) in modern C++23.
 */

#include "../../Packet.h"
#include "../../PythonExchange.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Domain/ExchangeSessionModel.h"

#include <span>

namespace Network::Dispatchers {

/**
 * @brief Handles incoming GC exchange item modification packets (ADD/DEL).
 * 
 * Safely extracts the data, updates the `CPythonExchange` state,
 * and publishes `ExchangeItemUpdatedEvent` via `EventBus` to decouple from GUI.
 * 
 * @param buffer Network packet byte span containing `TPacketGCExchange`.
 * @return EterBase::PacketResult<void> Success or packet parsing error.
 */
EterBase::PacketResult<void> DispatchExchangeItem(std::span<const uint8_t> buffer) {
    if (buffer.size() < sizeof(TPacketGCExchange)) {
        EterBase::ModernLogger::Error("DispatchExchangeItem: Niewystarczajaca dlugosc bufora pakietu ({} < {}).", 
            buffer.size(), sizeof(TPacketGCExchange));
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCExchange*>(buffer.data());
    auto& exchange = CPythonExchange::Instance();
    bool isMe = (packet->is_me != 0);

    if (packet->subheader == ExchangeSub::GC::ITEM_ADD) {
        EterBase::ItemSlot slotIndex(packet->arg2.cell);
        EterBase::ItemVnum itemVnum(packet->arg1);
        uint8_t itemCount = static_cast<uint8_t>(packet->arg3);

        if (isMe) {
            exchange.SetItemToSelf(static_cast<uint8_t>(slotIndex.value()), itemVnum.value(), itemCount);
            for (int i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i) {
                exchange.SetItemMetinSocketToSelf(static_cast<uint8_t>(slotIndex.value()), i, packet->alValues[i]);
            }
            for (int j = 0; j < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++j) {
                exchange.SetItemAttributeToSelf(static_cast<uint8_t>(slotIndex.value()), j, packet->aAttr[j].bType, packet->aAttr[j].sValue);
            }
        } else {
            exchange.SetItemToTarget(static_cast<uint8_t>(slotIndex.value()), itemVnum.value(), itemCount);
            for (int i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i) {
                exchange.SetItemMetinSocketToTarget(static_cast<uint8_t>(slotIndex.value()), i, packet->alValues[i]);
            }
            for (int j = 0; j < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++j) {
                exchange.SetItemAttributeToTarget(static_cast<uint8_t>(slotIndex.value()), j, packet->aAttr[j].bType, packet->aAttr[j].sValue);
            }
        }

        EterBase::ModernLogger::Info("DispatchExchangeItem (ADD): Zaktualizowano okno wymiany (slot: {}, isMe: {}).", 
            slotIndex.value(), isMe);

        UserInterface::Core::EventBus::GetInstance().Publish(
            UserInterface::Domain::ExchangeItemUpdatedEvent{isMe, slotIndex}
        );
        return {};
    }
    else if (packet->subheader == ExchangeSub::GC::ITEM_DEL) {
        EterBase::ItemSlot slotIndex(static_cast<uint16_t>(packet->arg1));

        if (isMe) {
            exchange.DelItemOfSelf(static_cast<uint8_t>(slotIndex.value()));
        } else {
            exchange.DelItemOfTarget(static_cast<uint8_t>(slotIndex.value()));
        }

        EterBase::ModernLogger::Info("DispatchExchangeItem (DEL): Usunieto przedmiot z okna wymiany (slot: {}, isMe: {}).", 
            slotIndex.value(), isMe);

        UserInterface::Core::EventBus::GetInstance().Publish(
            UserInterface::Domain::ExchangeItemUpdatedEvent{isMe, slotIndex}
        );
        return {};
    }

    EterBase::ModernLogger::Error("DispatchExchangeItem: Nieobslugiwany pod-kod dla przedmiotow wymiany ({}).", packet->subheader);
    return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
}

} // namespace Network::Dispatchers
