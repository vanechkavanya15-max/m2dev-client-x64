#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <string>

namespace Client::Contracts
{
    struct ItemCellAddress
    {
        uint8_t windowType{1}; // 1 = INVENTORY, 2 = EQUIPMENT, 10 = BELT, 11 = DRAGON_SOUL
        uint16_t cellIndex{0};
    };

    struct ItemDataSnapshot
    {
        ItemCellAddress pos;
        uint32_t vnum{0};
        uint32_t count{1};
        uint32_t sockets[6]{0};
        int16_t attrTypes[7]{0};
        int32_t attrValues[7]{0};
        bool isEquipped{false};
        std::string localizedName;
    };

    /**
     * @brief Czysty interfejs domenowy ekwipunku dla Agentow AI i MCP.
     */
    class IInventoryDomainContract
    {
    public:
        virtual ~IInventoryDomainContract() = default;

        [[nodiscard]] virtual std::optional<ItemDataSnapshot> GetItem(const ItemCellAddress& pos) const = 0;
        [[nodiscard]] virtual std::vector<ItemDataSnapshot> GetAllItems(uint8_t windowType) const = 0;

        [[nodiscard]] virtual bool IsCellEmpty(const ItemCellAddress& pos) const noexcept = 0;
        [[nodiscard]] virtual size_t GetFreeSlotCount(uint8_t windowType) const noexcept = 0;

        virtual bool RequestUseItem(const ItemCellAddress& pos) = 0;
        virtual bool RequestMoveItem(const ItemCellAddress& src, const ItemCellAddress& dst, uint32_t count) = 0;
        virtual bool RequestDropItem(const ItemCellAddress& pos, uint32_t count) = 0;
    };
}
