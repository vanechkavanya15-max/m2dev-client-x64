#include "../../StdAfx.h"
#include "../../PythonCharacterManager.h"
#include "../../Packet.h"
#include "../../Core/EventBus.h"
#include "../Handlers/FishingHandler.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"
#include "../../../GameLib/ItemManager.h"
#include <optional>

namespace UserInterface::Network
{
    using namespace EterBase;
    namespace Handlers = ::Network::Handlers;

    PacketResult<void> DispatchFishing(const TPacketGCFishing& packet)
    {
        auto* instance = CPythonCharacterManager::Instance().GetInstancePtr(packet.info);
        if (packet.subheader != FishingSub::GC::FISH)
        {
            if (!instance)
            {
                ModernLogger::Debug("DispatchFishing: Target instance {} not found", packet.info);
                return {};
            }
        }

        switch (packet.subheader)
        {
            case FishingSub::GC::START:
                if (instance) {
                    instance->StartFishing(static_cast<float>(packet.dir) * 5.0f);
                }
                break;

            case FishingSub::GC::STOP:
                if (instance && instance->IsFishing()) {
                    instance->StopFishing();
                }
                break;

            case FishingSub::GC::REACT:
                if (instance && instance->IsFishing()) {
                    instance->SetFishEmoticon();
                    instance->ReactFishing();
                }
                break;

            case FishingSub::GC::SUCCESS:
                if (instance) {
                    instance->CatchSuccess();
                }
                break;

            case FishingSub::GC::FAIL:
                if (instance) {
                    instance->CatchFail();
                    if (instance == CPythonCharacterManager::Instance().GetMainInstancePtr()) {
                        UserInterface::Core::EventBus::GetInstance().Publish(Handlers::FishingFailureEvent());
                    }
                }
                break;

            case FishingSub::GC::FISH:
            {
                ItemVnum fishId(packet.info);

                if (fishId.value() == 0)
                {
                    UserInterface::Core::EventBus::GetInstance().Publish(Handlers::FishingNotifyUnknownEvent());
                    return {};
                }

                CItemData* itemDataPtr = nullptr;
                if (!CItemManager::Instance().GetItemDataPointer(fishId.value(), &itemDataPtr)) {
                    ModernLogger::Warning("DispatchFishing: Fish ItemData for vnum {} not found", fishId.value());
                    return {};
                }

                auto* mainInstance = CPythonCharacterManager::Instance().GetMainInstancePtr();
                if (!mainInstance)
                {
                    return {};
                }

                bool isFishType = (itemDataPtr->GetType() == CItemData::ITEM_TYPE_FISH);
                std::string fishName = itemDataPtr->GetName();

                if (mainInstance->IsFishing())
                {
                    UserInterface::Core::EventBus::GetInstance().Publish(Handlers::FishingNotifyEvent(isFishType, fishName));
                }
                else
                {
                    UserInterface::Core::EventBus::GetInstance().Publish(Handlers::FishingSuccessEvent(isFishType, fishName));
                }
                break;
            }

            default:
                ModernLogger::Error("DispatchFishing: unknown subheader {}", packet.subheader);
                return MakeError(PacketError::MalformedPayload);
        }

        return {};
    }
}
