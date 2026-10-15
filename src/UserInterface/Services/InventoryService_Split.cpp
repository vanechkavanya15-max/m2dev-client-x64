#include "../StdAfx.h"
#include "IInventoryService.h"
#include "../Packet.h"
#include "../Core/EventBus.h"
#include "EterBase/LogModern.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::Services
{
    struct InventoryItemSplitEvent : public Core::IEvent
    {
        EterBase::ItemSlot sourceSlot;
        EterBase::ItemSlot destSlot;
        uint8_t count;

        InventoryItemSplitEvent(EterBase::ItemSlot src, EterBase::ItemSlot dst, uint8_t cnt)
            : sourceSlot(src), destSlot(dst), count(cnt) {}
    };

    class InventoryServiceSplit
    {
    public:
        static EterBase::PacketResult<void> SplitItem(IInventoryService& service, EterBase::ItemSlot sourceSlot, EterBase::ItemSlot destSlot, uint8_t splitCount)
        {
            if (sourceSlot == destSlot || splitCount == 0)
            {
                EterBase::ModernLogger::Error("InventoryServiceSplit: Invalid split arguments (src: {}, dst: {}, count: {})",
                    sourceSlot.value(), destSlot.value(), splitCount);
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            if (service.IsItemLocked(sourceSlot) || service.IsItemLocked(destSlot))
            {
                EterBase::ModernLogger::Error("InventoryServiceSplit: Cannot split locked slot");
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            auto srcOpt = service.GetItem(sourceSlot);
            if (!srcOpt.has_value() || srcOpt->count <= splitCount)
            {
                EterBase::ModernLogger::Error("InventoryServiceSplit: Source item count insufficient");
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            auto dstOpt = service.GetItem(destSlot);
            if (dstOpt.has_value())
            {
                EterBase::ModernLogger::Error("InventoryServiceSplit: Destination slot not empty");
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            InventoryItemView newSrc = *srcOpt;
            newSrc.count -= splitCount;
            auto setSrcRes = service.SetItem(sourceSlot, newSrc);
            if (!setSrcRes)
                return setSrcRes;

            InventoryItemView newDst = *srcOpt;
            newDst.slot = destSlot;
            newDst.count = splitCount;
            auto setDstRes = service.SetItem(destSlot, newDst);
            if (!setDstRes)
                return setDstRes;

            Core::EventBus::GetInstance().Publish(InventoryItemSplitEvent(sourceSlot, destSlot, splitCount));
            EterBase::ModernLogger::Info("InventoryServiceSplit: Split {} items from slot {} to slot {}",
                splitCount, sourceSlot.value(), destSlot.value());

            return {};
        }
    };
}
