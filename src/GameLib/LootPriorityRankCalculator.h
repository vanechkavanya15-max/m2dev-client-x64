#pragma once

#include <cstdint>
#include <optional>
#include <expected>
#include <format>
#include "../EterBase/StrongTypes.h"
#include "../EterBase/Result.h"
#include "../EterBase/LogModern.h"
#include "../UserInterface/Core/EventBus.h"
#include "ItemManager.h"
#include "ItemData.h"

namespace GameLib {

/**
 * @brief Zdarzenie rozglaszane gdy priorytet dropu zostanie pomyslnie obliczony.
 */
struct LootPriorityCalculatedEvent : public UserInterface::Core::IEvent {
    EterBase::ItemVnum vnum;
    uint32_t rank;

    LootPriorityCalculatedEvent(EterBase::ItemVnum vnum, uint32_t rank)
        : vnum(vnum), rank(rank) {}
};

/**
 * @brief Calculator class to compute the loot priority of ground items based on their properties.
 * 
 * Uses C++23 std::expected and EterBase::ItemVnum to prioritize items for the character to pick up.
 */
class LootPriorityRankCalculator {
public:
    /**
     * @brief Computes the priority rank for the given item VNUM.
     * @param vnum The strong type ItemVnum representing the item.
     * @return EterBase::Result<uint32_t, EterBase::InventoryError> representing the calculated rank, or an error.
     */
    static EterBase::Result<uint32_t, EterBase::InventoryError> CalculateRank(EterBase::ItemVnum vnum) {
        if (!vnum) {
            EterBase::ModernLogger::Warn("LootPriorityRankCalculator: VNUM is empty (0).");
            return std::unexpected(EterBase::InventoryError::InvalidVnum);
        }

        auto rankResult = GetItemData(vnum)
            .transform([](const CItemData* itemData) -> uint32_t {
                uint32_t rank = itemData->GetISellItemPrice();
                uint8_t itemType = itemData->GetType();

                switch (itemType) {
                    case CItemData::ITEM_TYPE_WEAPON:
                    case CItemData::ITEM_TYPE_ARMOR:
                        rank += 10000;
                        break;
                    case CItemData::ITEM_TYPE_METIN:
                        rank += 50000;
                        break;
                    case CItemData::ITEM_TYPE_ELK: // Gold/Yang
                        rank += 5000;
                        break;
                    default:
                        break;
                }
                return rank;
            });

        if (!rankResult.has_value()) {
            return std::unexpected(EterBase::InventoryError::InvalidVnum);
        }

        uint32_t rank = rankResult.value();
        EterBase::ModernLogger::Debug("LootPriorityRankCalculator: Calculated rank {} for VNUM {}", rank, vnum.get());
        
        // Emitowanie zdarzenia EventBus zgodnie z zasadami odpiecia od GUI
        UserInterface::Core::EventBus::GetInstance().Publish(LootPriorityCalculatedEvent{vnum, rank});
        
        return rank;
    }

private:
    /**
     * @brief Fetches item data pointer using CItemManager.
     * @param vnum The ItemVnum to fetch.
     * @return std::optional<const CItemData*>
     */
    static std::optional<const CItemData*> GetItemData(EterBase::ItemVnum vnum) {
        CItemData* itemData = nullptr;
        if (CItemManager::Instance().GetItemDataPointer(vnum.get(), &itemData) && itemData != nullptr) {
            return itemData;
        }
        EterBase::ModernLogger::Warn("LootPriorityRankCalculator: No item data found in ItemManager for VNUM {}", vnum.get());
        return std::nullopt;
    }
};

} // namespace GameLib
