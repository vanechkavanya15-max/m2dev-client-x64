#include "../../StdAfx.h"
/**
 * @file ExchangeMoneyHandler.cpp
 * @brief Implementation of the modern C++23 ExchangeMoneyHandler.
 */

#include "ExchangeMoneyHandler.h"
#include "../../PythonExchange.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/StrongTypes.h"
#include <optional>

namespace UserInterface::Network::Handlers {

/**
 * @brief Processes an incoming exchange money packet.
 * @param packet The incoming GC exchange packet containing the elk amount.
 * @return EterBase::PacketResult<void> representing success or error.
 */
EterBase::PacketResult<void> ExchangeMoneyHandler::HandlePacket(const TPacketGCExchange& packet)
{
    if (packet.subheader != ExchangeSub::GC::ELK_ADD) {
        EterBase::ModernLogger::Error("ExchangeMoneyHandler::HandlePacket: Invalid subheader {}, expected ELK_ADD", packet.subheader);
        return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
    }

    auto& exchange = CPythonExchange::Instance();

    // Validates if exchange window is currently trading
    std::optional<std::reference_wrapper<CPythonExchange>> optExchange(exchange);

    auto result = optExchange
        .and_then([](auto exRef) -> std::optional<std::reference_wrapper<CPythonExchange>> {
            if (!exRef.get().isTrading()) {
                EterBase::ModernLogger::Warn("ExchangeMoneyHandler: Received ELK_ADD while not trading.");
                return std::nullopt;
            }
            return exRef;
        })
        .transform([&packet](auto exRef) {
            uint32_t amount = packet.arg1;
            bool isMe = packet.is_me;

            EterBase::ModernLogger::Debug("ExchangeMoneyHandler: Updating ELK to {}. IsMe: {}", amount, isMe);

            if (isMe) {
                exRef.get().SetElkToSelf(amount);
            } else {
                exRef.get().SetElkToTarget(amount);
            }

            // Publish event for decoupled GUI updates
            UserInterface::Core::EventBus::GetInstance().Publish(ExchangeMoneyUpdateEvent(isMe, amount));
            
            return true;
        });
        
    if (!result) {
        return EterBase::MakeError(EterBase::PacketError::SessionClosed);
    }

    return {};
}

} // namespace UserInterface::Network::Handlers
