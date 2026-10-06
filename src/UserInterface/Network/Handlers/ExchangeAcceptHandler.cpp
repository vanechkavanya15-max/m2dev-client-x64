#include "../../StdAfx.h"
/**
 * @file ExchangeAcceptHandler.cpp
 * @brief Implementation of the ExchangeAcceptHandler for modern C++23 architecture.
 */

#include "ExchangeAcceptHandler.h"
#include "../../PythonExchange.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/LogModern.h"

/**
 * @brief Processes an incoming exchange accept packet.
 * 
 * Extracts the ready state and updates the internal C++ state in CPythonExchange.
 * Then publishes an event to EventBus to notify the UI without directly calling Python.
 * 
 * @param packet The incoming GC exchange packet.
 * @return EterBase::PacketResult<void> indicating success.
 */
EterBase::PacketResult<void> ExchangeAcceptHandler::HandleAccept(const TPacketGCExchange& packet)
{
    CPythonExchange& exchange = CPythonExchange::Instance();
    uint8_t isAccepted = static_cast<uint8_t>(packet.arg1);

    if (packet.is_me)
    {
        exchange.SetAcceptToSelf(isAccepted);
    }
    else
    {
        exchange.SetAcceptToTarget(isAccepted);
    }

    EterBase::ModernLogger::Info("Exchange accept state changed: is_me={}, accepted={}", 
        packet.is_me, isAccepted);

    UserInterface::Core::EventBus::GetInstance().Publish(
        ExchangeAcceptEvent(packet.is_me != 0, isAccepted != 0)
    );
    
    return {};
}
