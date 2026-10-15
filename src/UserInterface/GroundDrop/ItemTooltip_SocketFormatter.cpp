#include "../StdAfx.h"
#include "ItemTooltip_SocketFormatter.h"
#include "EterBase/ModernLogger.h"
#include "../Core/EventBus.h"
#include "GameLib/ItemManager.h"

namespace UserInterface::GroundDrop
{
    EterBase::Result<std::string> SocketFormatter::FormatSockets(
        EterBase::ItemVnum itemVnum, 
        std::span<const int32_t> sockets)
    {
        CItemData* pItemData = nullptr;
        if (!CItemManager::Instance().GetItemDataPointer(itemVnum.value(), &pItemData))
        {
            EterBase::ModernLogger::Error("SocketFormatter: Item not found with vnum {}", itemVnum.value());
            return EterBase::MakeError(std::string_view("ItemNotFound"));
        }

        if (sockets.empty())
        {
            return std::string{};
        }

        std::string result;
        int maxSockets = pItemData->GetSocketCount();
        
        for (size_t i = 0; i < std::min(sockets.size(), static_cast<size_t>(maxSockets)); ++i)
        {
            int32_t socketVal = sockets[i];
            
            if (socketVal > 0)
            {
                CItemData* pSocketItem = nullptr;
                if (CItemManager::Instance().GetItemDataPointer(socketVal, &pSocketItem))
                {
                    result += std::format("- {}\n", pSocketItem->GetName());
                    
                    for (int j = 0; j < CItemData::ITEM_APPLY_MAX_NUM; ++j)
                    {
                        CItemData::TItemApply apply;
                        if (pSocketItem->GetApply(j, &apply) && apply.bType != CItemData::APPLY_NONE)
                        {
                             result += std::format("  * Typ: {}, Wartosc: {}\n", apply.bType, apply.lValue);
                        }
                    }
                }
                else
                {
                    result += std::format("- Nieznany kamien (vnum: {})\n", socketVal);
                }
            }
            else if (socketVal == 0) // Empty socket
            {
                 result += "- Pusty slot na kamien\n";
            }
        }

        return result;
    }
}
