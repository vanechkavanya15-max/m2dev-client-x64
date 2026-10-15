#include "ExchangeCommandHandler.h"
#include "EterBase/ModernLogger.h"

namespace Client::Gameplay {

using Client::Core::CommandError;

EterBase::Result<void, CommandError> ExchangeCommandHandler::Open(EterBase::EntityId initiator, EterBase::EntityId target) {
    if (m_exchange) {
        EterBase::ModernLogger::Error("ExchangeCommandHandler::Open failed: Exchange already active.");
        return EterBase::MakeError(CommandError::InvalidParameter);
    }
    
    if (!static_cast<bool>(initiator) || !static_cast<bool>(target)) {
        EterBase::ModernLogger::Error("ExchangeCommandHandler::Open failed: Invalid entity IDs.");
        return EterBase::MakeError(CommandError::InvalidParameter);
    }
    
    if (initiator == target) {
        EterBase::ModernLogger::Error("ExchangeCommandHandler::Open failed: Cannot exchange with self.");
        return EterBase::MakeError(CommandError::InvalidParameter);
    }

    m_exchange = std::make_unique<PlayerExchange>(initiator, target);
    return {};
}

EterBase::Result<void, CommandError> ExchangeCommandHandler::AddItem(EterBase::EntityId player, EterBase::ItemSlot slot, EterBase::ItemVnum vnum) {
    if (!m_exchange) {
        EterBase::ModernLogger::Error("ExchangeCommandHandler::AddItem failed: No active exchange.");
        return EterBase::MakeError(CommandError::InvalidParameter);
    }

    auto result = m_exchange->AddItem(player, slot, vnum);
    if (!result) {
        EterBase::ModernLogger::Error("ExchangeCommandHandler::AddItem domain error: {}", result.error());
        return EterBase::MakeError(CommandError::InvalidParameter);
    }

    return {};
}

EterBase::Result<void, CommandError> ExchangeCommandHandler::AddGold(EterBase::EntityId player, Gold amount) {
    if (!m_exchange) {
        EterBase::ModernLogger::Error("ExchangeCommandHandler::AddGold failed: No active exchange.");
        return EterBase::MakeError(CommandError::InvalidParameter);
    }

    auto result = m_exchange->AddGold(player, amount);
    if (!result) {
        EterBase::ModernLogger::Error("ExchangeCommandHandler::AddGold domain error: {}", result.error());
        return EterBase::MakeError(CommandError::InvalidParameter);
    }

    return {};
}

EterBase::Result<void, CommandError> ExchangeCommandHandler::Lock(EterBase::EntityId player) {
    if (!m_exchange) {
        EterBase::ModernLogger::Error("ExchangeCommandHandler::Lock failed: No active exchange.");
        return EterBase::MakeError(CommandError::InvalidParameter);
    }

    auto result = m_exchange->Lock(player);
    if (!result) {
        EterBase::ModernLogger::Error("ExchangeCommandHandler::Lock domain error: {}", result.error());
        return EterBase::MakeError(CommandError::InvalidParameter);
    }

    return {};
}

EterBase::Result<void, CommandError> ExchangeCommandHandler::Accept(EterBase::EntityId player) {
    if (!m_exchange) {
        EterBase::ModernLogger::Error("ExchangeCommandHandler::Accept failed: No active exchange.");
        return EterBase::MakeError(CommandError::InvalidParameter);
    }

    auto result = m_exchange->Accept(player);
    if (!result) {
        EterBase::ModernLogger::Error("ExchangeCommandHandler::Accept domain error: {}", result.error());
        return EterBase::MakeError(CommandError::InvalidParameter);
    }

    if (m_exchange->GetState() == ExchangeState::Accept) {
        EterBase::ModernLogger::Info("Exchange completed successfully.");
        m_exchange.reset(); // Zgodnie z CR: resetujemy stan po zaakceptowaniu przez obu graczy
    }

    return {};
}

EterBase::Result<void, CommandError> ExchangeCommandHandler::Cancel() {
    if (!m_exchange) {
        EterBase::ModernLogger::Error("ExchangeCommandHandler::Cancel failed: No active exchange.");
        return EterBase::MakeError(CommandError::InvalidParameter);
    }

    auto result = m_exchange->Cancel();
    if (!result) {
        EterBase::ModernLogger::Error("ExchangeCommandHandler::Cancel domain error: {}", result.error());
        return EterBase::MakeError(CommandError::InvalidParameter);
    }
    
    // Clear exchange when cancelled
    m_exchange.reset();

    return {};
}

const PlayerExchange* ExchangeCommandHandler::GetExchange() const noexcept {
    return m_exchange.get();
}

bool ExchangeCommandHandler::IsActive() const noexcept {
    return m_exchange != nullptr;
}

} // namespace Client::Gameplay
