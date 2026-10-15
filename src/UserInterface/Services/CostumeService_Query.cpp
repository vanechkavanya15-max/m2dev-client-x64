#include "../StdAfx.h"
#include "ISpecialInventoryService.h"
#include "../Packet.h"
#include "../Core/EventBus.h"
#include "EterBase/LogModern.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include <unordered_map>
#include <optional>

namespace UserInterface::Services
{
    class CostumeServiceQuery
    {
    private:
        static inline std::unordered_map<uint32_t, EterBase::ItemVnum> s_equippedCostumes;

    public:
        static void RegisterCostume(EterBase::ItemSlot slot, EterBase::ItemVnum vnum)
        {
            s_equippedCostumes[slot.value()] = vnum;
        }

        static void UnregisterCostume(EterBase::ItemSlot slot)
        {
            s_equippedCostumes.erase(slot.value());
        }

        static std::optional<EterBase::ItemVnum> GetEquippedCostume(EterBase::ItemSlot slot)
        {
            auto it = s_equippedCostumes.find(slot.value());
            if (it != s_equippedCostumes.end())
                return it->second;
            return std::nullopt;
        }

        static void Clear()
        {
            s_equippedCostumes.clear();
        }
    };
}
