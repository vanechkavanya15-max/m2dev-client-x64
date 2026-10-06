#include "../../StdAfx.h"
#include "ExchangeItemDelHandler.h"
#include "../../Packet.h"
#include "../../PythonExchange.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

namespace Network::Handlers
{

/**
 * @brief Przetwarza pakiet usuniecia przedmiotu z okna wymiany (C++20).
 * 
 * @param buffer Bufor bajtow zawierajacy strukture TPacketGCExchange.
 * @return Zwraca sukces lub EterBase::PacketError w przypadku bledu (np. zbyt maly bufor).
 */
EterBase::PacketResult<void> HandleExchangeItemDel(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(TPacketGCExchange))
    {
        EterBase::ModernLogger::Log(EterBase::LogLevel::Warning, "HandleExchangeItemDel: BufferUnderflow, expected {}, got {}", sizeof(TPacketGCExchange), buffer.size());
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCExchange*>(buffer.data());

    // Strict types z C++23 (standard 2026)
    EterBase::ItemSlot slotIndex(packet->arg1);
    bool isMe = packet->is_me;

    auto& exchange = CPythonExchange::Instance();

    if (isMe)
    {
        exchange.DelItemOfSelf(static_cast<uint8_t>(slotIndex.value()));
    }
    else
    {
        exchange.DelItemOfTarget(static_cast<uint8_t>(slotIndex.value()));
    }

    // Publikacja eventu oznaczajacego zmiane stanu
    ExchangeItemDelEvent eventObj(static_cast<uint8_t>(slotIndex.value()), isMe);
    UserInterface::Core::EventBus::GetInstance().Publish(eventObj);

    EterBase::ModernLogger::Log(EterBase::LogLevel::Info, "HandleExchangeItemDel: Successfully removed item at slot {}, isMe: {}", slotIndex.value(), isMe);

    return {};
}

} // namespace Network::Handlers
