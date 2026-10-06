/**
 * @file ItemValidator.h
 * @brief Provides decoupled, pure C++ domain validation logic for inventory and equipment slots.
 */

#pragma once

#include <cstdint>

#include "../GameType.h"

namespace Domain::ItemValidator
{

    /**
     * @brief Checks if a given cell index is valid within the specified window.
     * 
     * @param windowType The type of the window (e.g., INVENTORY, EQUIPMENT, DRAGON_SOUL_INVENTORY).
     * @param cell The index of the slot/cell to validate.
     * @return true If the cell index falls within the valid range for the given window type.
     * @return false Otherwise.
     */
    [[nodiscard]] inline bool IsValidCell(uint8_t windowType, uint16_t cell) noexcept
    {
        switch (windowType)
        {
            case INVENTORY:
                return cell < c_Inventory_Count;
            case EQUIPMENT:
                return cell < c_DragonSoul_Equip_End;
            case DRAGON_SOUL_INVENTORY:
                return cell < DS_INVENTORY_MAX_NUM;
            default:
                return false;
        }
    }

    /**
     * @brief Checks if a given slot corresponds to an equipment slot.
     * 
     * @param windowType The type of the window.
     * @param cell The index of the slot/cell to validate.
     * @return true If the cell is an equipment slot within the INVENTORY or EQUIPMENT windows.
     * @return false Otherwise.
     */
    [[nodiscard]] inline bool IsEquipCell(uint8_t windowType, uint16_t cell) noexcept
    {
        switch (windowType)
        {
            case INVENTORY:
            case EQUIPMENT:
                return (c_Equipment_Start <= cell) && (cell < c_Equipment_Start + c_Wear_Max);
            case BELT_INVENTORY:
            case DRAGON_SOUL_INVENTORY:
                return false;
            default:
                return false;
        }
    }

#ifdef ENABLE_NEW_EQUIPMENT_SYSTEM
    /**
     * @brief Checks if a given slot corresponds to a belt inventory slot.
     * 
     * @param windowType The type of the window.
     * @param cell The index of the slot/cell to validate.
     * @return true If the cell is a belt inventory slot.
     * @return false Otherwise.
     */
    [[nodiscard]] inline bool IsBeltInventoryCell(uint8_t /*windowType*/, uint16_t cell) noexcept
    {
        return (c_Belt_Inventory_Slot_Start <= cell) && (cell < c_Belt_Inventory_Slot_End);
    }
#endif

} // namespace Domain::ItemValidator
