#pragma once

#include <memory>
#include <optional>
#include "Client/Core/DomainCommands.h"
#include "Client/Gameplay/TradeDomain.h"
#include "EterBase/Result.h"

namespace Client::Gameplay {

class ExchangeCommandHandler {
public:
    ExchangeCommandHandler() = default;
    ~ExchangeCommandHandler() = default;

    // Zabronione kopiowanie i przenoszenie
    ExchangeCommandHandler(const ExchangeCommandHandler&) = delete;
    ExchangeCommandHandler& operator=(const ExchangeCommandHandler&) = delete;

    EterBase::Result<void, Client::Core::CommandError> Open(EterBase::EntityId initiator, EterBase::EntityId target);
    
    EterBase::Result<void, Client::Core::CommandError> AddItem(EterBase::EntityId player, EterBase::ItemSlot slot, EterBase::ItemVnum vnum);
    EterBase::Result<void, Client::Core::CommandError> AddGold(EterBase::EntityId player, Gold amount);
    
    EterBase::Result<void, Client::Core::CommandError> Lock(EterBase::EntityId player);
    EterBase::Result<void, Client::Core::CommandError> Accept(EterBase::EntityId player);
    EterBase::Result<void, Client::Core::CommandError> Cancel();

    [[nodiscard]] const PlayerExchange* GetExchange() const noexcept;
    [[nodiscard]] bool IsActive() const noexcept;

private:
    std::unique_ptr<PlayerExchange> m_exchange;
};

} // namespace Client::Gameplay
