#include "../../StdAfx.h"
#include "InventoryItemValidator.h"
#include "../../GameLib/ItemData.h"
#include "../../UserInterface/GameType.h"

namespace Client::Gameplay {
    EterBase::Result<bool, Client::Core::CommandError> InventoryItemValidator::CanEquipItem(
        const CItemData* itemData, uint8_t playerRace, uint8_t playerLevel, uint16_t targetCell) {
        
        if (!itemData)
            return std::unexpected(Client::Core::CommandError::InvalidParameter);

        // 1. Validate equipment slot range (c_Equipment_Start <= cell < c_Equipment_Start + CItemData::WEAR_MAX_NUM)
        if (targetCell < c_Equipment_Start || targetCell >= c_Equipment_Start + CItemData::WEAR_MAX_NUM)
            return std::unexpected(Client::Core::CommandError::InvalidParameter);

        // 2. Validate Level Limit
        for (int i = 0; i < CItemData::ITEM_LIMIT_MAX_NUM; ++i) {
            CItemData::TItemLimit limit;
            if (itemData->GetLimit(i, &limit)) {
                if (limit.bType == CItemData::LIMIT_LEVEL && playerLevel < limit.lValue) {
                    return std::unexpected(Client::Core::CommandError::InvalidParameter);
                }
            }
        }

        // 3. Validate Anti-Flags (Job check)
        uint32_t antiFlag = 0;
        int job = playerRace % 4; // RaceToJob
        switch (job) {
            case 0: antiFlag = CItemData::ITEM_ANTIFLAG_WARRIOR; break;
            case 1: antiFlag = CItemData::ITEM_ANTIFLAG_ASSASSIN; break;
            case 2: antiFlag = CItemData::ITEM_ANTIFLAG_SURA; break;
            case 3: antiFlag = CItemData::ITEM_ANTIFLAG_SHAMAN; break;
        }
        
        if (itemData->IsAntiFlag(antiFlag))
            return std::unexpected(Client::Core::CommandError::InvalidParameter);

        // 4. Validate Wear Slot
        uint16_t wearSlot = targetCell - c_Equipment_Start;
        uint32_t wearFlag = 0;
        switch (wearSlot) {
            case CItemData::WEAR_BODY: wearFlag = CItemData::WEARABLE_BODY; break;
            case CItemData::WEAR_HEAD: wearFlag = CItemData::WEARABLE_HEAD; break;
            case CItemData::WEAR_FOOTS: wearFlag = CItemData::WEARABLE_FOOTS; break;
            case CItemData::WEAR_WRIST: wearFlag = CItemData::WEARABLE_WRIST; break;
            case CItemData::WEAR_WEAPON: wearFlag = CItemData::WEARABLE_WEAPON; break;
            case CItemData::WEAR_NECK: wearFlag = CItemData::WEARABLE_NECK; break;
            case CItemData::WEAR_EAR: wearFlag = CItemData::WEARABLE_EAR; break;
            case CItemData::WEAR_SHIELD: wearFlag = CItemData::WEARABLE_SHIELD; break;
            case CItemData::WEAR_ARROW: wearFlag = CItemData::WEARABLE_ARROW; break;
            case CItemData::WEAR_UNIQUE1:
            case CItemData::WEAR_UNIQUE2:
            case CItemData::WEAR_RING1:
            case CItemData::WEAR_RING2: wearFlag = CItemData::WEARABLE_UNIQUE; break;
            case CItemData::WEAR_BELT: wearFlag = CItemData::WEARABLE_BELT; break;
            case CItemData::WEAR_COSTUME_BODY: wearFlag = CItemData::WEARABLE_COSTUME_BODY; break;
            case CItemData::WEAR_COSTUME_HAIR: wearFlag = CItemData::WEARABLE_COSTUME_HAIR; break;
            default: return std::unexpected(Client::Core::CommandError::InvalidParameter);
        }

        if (!itemData->IsWearableFlag(wearFlag))
            return std::unexpected(Client::Core::CommandError::InvalidParameter);

        return true;
    }
}
