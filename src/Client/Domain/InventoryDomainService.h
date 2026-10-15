#pragma once

#include <cstdint>
#include <span>
#include <expected>

#include "../Gameplay/InventoryDomain.h"
#include "../../EterBase/Result.h"
#include "../Core/DomainErrors.h"
#include "../Core/DomainCommands.h"
#include "../Core/StrongTypes.h"

namespace Client::Domain {

/**
 * @brief Serwis logiki biznesowej ekwipunku, nieposiadajacy stanow wewnetrznych,
 * odseparowany od UI. Udostepnia metody uzywajace std::expected / Result.
 */
class InventoryDomainService {
public:
    explicit InventoryDomainService(Gameplay::InventoryDomain& inventory) noexcept;
    ~InventoryDomainService() = default;

    InventoryDomainService(const InventoryDomainService&) = delete;
    InventoryDomainService& operator=(const InventoryDomainService&) = delete;
    InventoryDomainService(InventoryDomainService&&) = delete;
    InventoryDomainService& operator=(InventoryDomainService&&) = delete;

    /**
     * @brief Przenosi przedmiot pomiedzy slotami ekwipunku.
     */
    [[nodiscard]] Core::Result<void, Core::InventoryError> MoveItem(
        Gameplay::InventoryWindow srcWindow, 
        Core::ItemSlot srcSlot,
        Gameplay::InventoryWindow dstWindow, 
        Core::ItemSlot dstSlot
    ) const;

    /**
     * @brief Dzieli stos przedmiotow w ekwipunku.
     */
    [[nodiscard]] Core::Result<void, Core::InventoryError> SplitItem(
        Gameplay::InventoryWindow window,
        Core::ItemSlot srcSlot,
        Core::ItemSlot dstSlot,
        uint32_t amount
    ) const;

    /**
     * @brief Upuszcza przedmiot z danego slotu.
     */
    [[nodiscard]] Core::Result<Gameplay::ItemData, Core::InventoryError> DropItem(
        Gameplay::InventoryWindow window,
        Core::ItemSlot slot
    ) const;

private:
    Gameplay::InventoryDomain& m_inventory;
};

} // namespace Client::Domain
