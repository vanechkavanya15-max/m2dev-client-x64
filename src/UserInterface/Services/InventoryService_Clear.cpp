#include "../StdAfx.h"
#include "IInventoryService.h"
#include "../Packet.h"
#include "../Core/EventBus.h"
#include "EterBase/LogModern.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::Services
{
    struct InventoryClearedEvent : public Core::IEvent
    {
        InventoryClearedEvent() = default;
    };

    class InventoryServiceClear
    {
    public:
        static void ExecuteClear(IInventoryService& service)
        {
            service.Clear();
            Core::EventBus::GetInstance().Publish(InventoryClearedEvent{});
            EterBase::ModernLogger::Info("InventoryServiceClear: All inventory slots cleared and event published.");
        }
    };
}
