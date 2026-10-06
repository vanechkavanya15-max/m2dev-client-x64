#include "StdAfx.h"
#include "FishingHandler.h"
#include "../../Packet.h"
#include "../../PythonCharacterManager.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"
#include "../../../GameLib/ItemManager.h"
#include <optional>

namespace Network::Handlers {

    /**
     * @brief Helper to get an instance wrapped in std::optional
     */
    static std::optional<CInstanceBase*> GetOptionalInstance(uint32_t vid) {
        if (auto* instance = CPythonCharacterManager::Instance().GetInstancePtr(vid)) {
            return instance;
        }
        return std::nullopt;
    }

    /**
     * @brief Modern C++20 handler for fishing packets.
     * 
     * @param buffer Binary span representing the incoming network packet.
     * @return EterBase::PacketResult<void> Returns success or a specific packet error.
     */
    EterBase::PacketResult<void> ProcessFishingPacket(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCFishing))
        {
            EterBase::ModernLogger::Error("ProcessFishingPacket: Buffer underflow. Expected >= {} bytes, got {}", sizeof(TPacketGCFishing), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCFishing*>(buffer.data());

        std::optional<CInstanceBase*> fishingInstanceOpt = std::nullopt;

        if (packet->subheader != FishingSub::GC::FISH)
        {
            EterBase::EntityId entityId(packet->info);
            fishingInstanceOpt = GetOptionalInstance(entityId.value());
            
            if (!fishingInstanceOpt.has_value())
            {
                EterBase::ModernLogger::Debug("ProcessFishingPacket: Target instance {} not found", entityId.value());
                return {};
            }
        }

        switch (packet->subheader)
        {
            case FishingSub::GC::START:
                fishingInstanceOpt.and_then([packet](CInstanceBase* instance) -> std::optional<CInstanceBase*> {
                    instance->StartFishing(static_cast<float>(packet->dir) * 5.0f);
                    return instance;
                });
                break;

            case FishingSub::GC::STOP:
                fishingInstanceOpt.and_then([](CInstanceBase* instance) -> std::optional<CInstanceBase*> {
                    if (instance->IsFishing()) instance->StopFishing();
                    return instance;
                });
                break;

            case FishingSub::GC::REACT:
                fishingInstanceOpt.and_then([](CInstanceBase* instance) -> std::optional<CInstanceBase*> {
                    if (instance->IsFishing()) {
                        instance->SetFishEmoticon();
                        instance->ReactFishing();
                    }
                    return instance;
                });
                break;

            case FishingSub::GC::SUCCESS:
                fishingInstanceOpt.and_then([](CInstanceBase* instance) -> std::optional<CInstanceBase*> {
                    instance->CatchSuccess();
                    return instance;
                });
                break;

            case FishingSub::GC::FAIL:
                fishingInstanceOpt.and_then([](CInstanceBase* instance) -> std::optional<CInstanceBase*> {
                    instance->CatchFail();
                    if (instance == CPythonCharacterManager::Instance().GetMainInstancePtr()) {
                        UserInterface::Core::EventBus::GetInstance().Publish(FishingFailureEvent());
                    }
                    return instance;
                });
                break;

            case FishingSub::GC::FISH:
            {
                EterBase::ItemVnum fishId(packet->info);

                if (fishId.value() == 0)
                {
                    UserInterface::Core::EventBus::GetInstance().Publish(FishingNotifyUnknownEvent());
                    return {};
                }

                std::optional<CItemData*> itemDataOpt = std::nullopt;
                CItemData* itemDataPtr = nullptr;
                if (CItemManager::Instance().GetItemDataPointer(fishId.value(), &itemDataPtr)) {
                    itemDataOpt = itemDataPtr;
                }
                
                if (!itemDataOpt.has_value())
                {
                    EterBase::ModernLogger::Warning("ProcessFishingPacket: Fish ItemData for vnum {} not found", fishId.value());
                    return {};
                }
                
                auto* itemData = itemDataOpt.value();

                CInstanceBase* mainInstancePtr = CPythonCharacterManager::Instance().GetMainInstancePtr();
                std::optional<CInstanceBase*> mainInstanceOpt = mainInstancePtr ? std::make_optional(mainInstancePtr) : std::nullopt;

                if (!mainInstanceOpt.has_value())
                {
                    return {};
                }
                
                auto* mainInstance = mainInstanceOpt.value();

                bool isFishType = (itemData->GetType() == CItemData::ITEM_TYPE_FISH);
                std::string fishName = itemData->GetName();

                if (mainInstance->IsFishing())
                {
                    UserInterface::Core::EventBus::GetInstance().Publish(FishingNotifyEvent(isFishType, fishName));
                }
                else
                {
                    UserInterface::Core::EventBus::GetInstance().Publish(FishingSuccessEvent(isFishType, fishName));
                }
                break;
            }

            default:
                EterBase::ModernLogger::Error("ProcessFishingPacket: unknown subheader {}", packet->subheader);
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        return {};
    }

    /**
     * @brief Exposes the modern handler for network dispatcher usage.
     * @param buffer Binary span representing the packet payload.
     * @return true if successful, false otherwise.
     */
    bool HandleFishing(std::span<const uint8_t> buffer)
    {
        return ProcessFishingPacket(buffer).has_value();
    }
}
