#pragma once

#include <memory>
#include "../Core/Result.h"
#include "../Core/DomainCommands.h"
#include "../Core/INetworkPort.h"

namespace Client::Gameplay {

class ShopCommandHandler {
public:
    explicit ShopCommandHandler(std::shared_ptr<Client::Core::INetworkPort> networkPort);
    ~ShopCommandHandler() = default;

    ShopCommandHandler(const ShopCommandHandler&) = delete;
    ShopCommandHandler& operator=(const ShopCommandHandler&) = delete;
    ShopCommandHandler(ShopCommandHandler&&) noexcept = default;
    ShopCommandHandler& operator=(ShopCommandHandler&&) noexcept = default;

    void OpenShop() noexcept;
    void CloseShop() noexcept;
    [[nodiscard]] bool IsShopOpen() const noexcept;

    [[nodiscard]] Client::Core::Result<void, Client::Core::CommandError> HandleBuy(const Client::Core::ShopBuyCommand& cmd);
    [[nodiscard]] Client::Core::Result<void, Client::Core::CommandError> HandleSell(const Client::Core::ShopSellCommand& cmd);

private:
    std::shared_ptr<Client::Core::INetworkPort> m_networkPort;
    bool m_isShopOpen;
};

} // namespace Client::Gameplay
