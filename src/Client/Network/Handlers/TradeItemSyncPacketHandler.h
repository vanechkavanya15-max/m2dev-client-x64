#pragma once

#include <span>
#include <cstdint>
#include <expected>
#include <functional>

#include "EterBase/Result.h"
#include "UserInterface/Core/EventBus.h"

namespace Client::Network::Handlers {

/**
 * @struct TradeItemSyncEvent
 * @brief Zdarzenie dodania/synchronizacji przedmiotu na siatce wymiany.
 */
struct TradeItemSyncEvent : public UserInterface::Core::IEvent
{
    uint8_t slotIndex;
    uint32_t itemVnum;
    uint32_t itemCount;
    bool isMe;

    constexpr TradeItemSyncEvent(uint8_t slot, uint32_t vnum, uint32_t count, bool me) noexcept
        : slotIndex(slot), itemVnum(vnum), itemCount(count), isMe(me) {}
};

using EventPublisher = std::function<void(const TradeItemSyncEvent&)>;

/**
 * @class TradeItemSyncPacketHandler
 * @brief Przetwarza pakiety TPacketGCExchange typu ExchangeSub::GC::ITEM_ADD.
 *        Wysyla zdarzenie TradeItemSyncEvent do podanego publishera (Dependency Injection).
 */
class TradeItemSyncPacketHandler {
public:
    static EterBase::PacketResult<void> HandlePacket(std::span<const uint8_t> buffer, EventPublisher publisher);
};

} // namespace Client::Network::Handlers
