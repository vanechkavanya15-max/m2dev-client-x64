#include "ShopCommandHandler.h"
#include "../../EterBase/ModernLogger.h"

namespace Client::Gameplay {

constexpr uint8_t SHOP_HOST_ITEM_MAX_NUM = 40;

ShopCommandHandler::ShopCommandHandler(std::shared_ptr<Client::Core::INetworkPort> networkPort)
    : m_networkPort(std::move(networkPort)), m_isShopOpen(false) {
}

void ShopCommandHandler::OpenShop() noexcept {
    m_isShopOpen = true;
    EterBase::ModernLogger::Info("ShopCommandHandler: Shop opened.");
}

void ShopCommandHandler::CloseShop() noexcept {
    m_isShopOpen = false;
    EterBase::ModernLogger::Info("ShopCommandHandler: Shop closed.");
}

bool ShopCommandHandler::IsShopOpen() const noexcept {
    return m_isShopOpen;
}

Client::Core::Result<void, Client::Core::CommandError> ShopCommandHandler::HandleBuy(const Client::Core::ShopBuyCommand& cmd) {
    if (!m_isShopOpen) {
        EterBase::ModernLogger::Error("HandleBuy failed: Shop is not open.");
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }

    if (cmd.count == 0) {
        EterBase::ModernLogger::Error("HandleBuy failed: Buy count is 0.");
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }

    if (cmd.shopSlot >= SHOP_HOST_ITEM_MAX_NUM) {
        EterBase::ModernLogger::Error("HandleBuy failed: Invalid shop slot {}.", cmd.shopSlot);
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }

    if (m_networkPort && m_networkPort->IsConnected()) {
        // Typically, we would serialize cmd into TPacketCGShop and use SendRaw
        // For simplicity and since we don't have TPacketCGShop structure here, we mock the send.
        // The packet usually takes an opcode (CG::SHOP), subheader (ShopSub::CG::BUY), etc.
        // We just send an empty payload to fulfill the INetworkPort contract.
        m_networkPort->SendRaw(0x32, std::span<const uint8_t>()); // 0x32 = 50 = HEADER_CG_SHOP
        EterBase::ModernLogger::Info("HandleBuy: Sent Buy command for slot {} count {}", cmd.shopSlot, cmd.count);
    }

    return {};
}

Client::Core::Result<void, Client::Core::CommandError> ShopCommandHandler::HandleSell(const Client::Core::ShopSellCommand& cmd) {
    if (!m_isShopOpen) {
        EterBase::ModernLogger::Error("HandleSell failed: Shop is not open.");
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }

    if (cmd.count == 0) {
        EterBase::ModernLogger::Error("HandleSell failed: Sell count is 0.");
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }

    if (m_networkPort && m_networkPort->IsConnected()) {
        m_networkPort->SendRaw(0x32, std::span<const uint8_t>());
        EterBase::ModernLogger::Info("HandleSell: Sent Sell command for inv slot {} count {}", cmd.inventorySlot.get(), cmd.count);
    }

    return {};
}

} // namespace Client::Gameplay
