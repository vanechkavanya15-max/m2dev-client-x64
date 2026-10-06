#include "../StdAfx.h"
#include "IQuickslotService.h"
#include "../Packet.h"
#include "../PythonNetworkStream.h"
#include "../PythonPlayer.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../Domain/QuickslotContainerModel.h"
#include "../Core/EventBus.h"
#include <memory>

namespace UserInterface::Services
{
    class PlayerFacade_Quickslot : public IQuickslotService
    {
    public:
        ~PlayerFacade_Quickslot() override = default;

        void SetQuickslot(uint8_t slotIndex, const QuickslotView& slot) override
        {
            auto validation = ValidateSlotIndex(slotIndex);
            if (!validation)
            {
                EterBase::ModernLogger::Error("PlayerFacade_Quickslot::SetQuickslot - Invalid slotIndex: {}", slotIndex);
                return;
            }

            CPythonNetworkStream::Instance().SendQuickSlotAddPacket(slotIndex, slot.type, slot.pos);
            EterBase::ModernLogger::Info("PlayerFacade_Quickslot: Sent Quickslot Add Packet (Slot: {}, Type: {}, Pos: {})", slotIndex, slot.type, slot.pos);

            Domain::QuickslotData data;
            data.type = static_cast<Domain::QuickslotType>(slot.type);
            switch (data.type) {
                case Domain::QuickslotType::Item:
                    data.position = EterBase::ItemSlot(static_cast<uint16_t>(slot.pos));
                    break;
                case Domain::QuickslotType::Skill:
                    data.position = EterBase::SkillId(slot.pos);
                    break;
                case Domain::QuickslotType::Command:
                    data.position = static_cast<uint8_t>(slot.pos);
                    break;
                default:
                    data.position = std::monostate{};
                    break;
            }

            Core::EventBus::GetInstance().Publish(Domain::QuickslotAddedEvent(slotIndex, data));
        }

        void DeleteQuickslot(uint8_t slotIndex) override
        {
            auto validation = ValidateSlotIndex(slotIndex);
            if (!validation)
            {
                EterBase::ModernLogger::Error("PlayerFacade_Quickslot::DeleteQuickslot - Invalid slotIndex: {}", slotIndex);
                return;
            }

            CPythonNetworkStream::Instance().SendQuickSlotDelPacket(slotIndex);
            EterBase::ModernLogger::Info("PlayerFacade_Quickslot: Sent Quickslot Delete Packet (Slot: {})", slotIndex);

            Core::EventBus::GetInstance().Publish(Domain::QuickslotDeletedEvent(slotIndex));
        }

        void SwapQuickslots(uint8_t fromIndex, uint8_t toIndex) override
        {
            auto valFrom = ValidateSlotIndex(fromIndex);
            auto valTo = ValidateSlotIndex(toIndex);

            if (!valFrom || !valTo)
            {
                EterBase::ModernLogger::Error("PlayerFacade_Quickslot::SwapQuickslots - Invalid index (From: {}, To: {})", fromIndex, toIndex);
                return;
            }

            CPythonNetworkStream::Instance().SendQuickSlotMovePacket(fromIndex, toIndex);
            EterBase::ModernLogger::Info("PlayerFacade_Quickslot: Sent Quickslot Swap Packet (From: {}, To: {})", fromIndex, toIndex);

            Core::EventBus::GetInstance().Publish(Domain::QuickslotSwappedEvent(fromIndex, toIndex));
        }

        std::optional<QuickslotView> GetQuickslot(uint8_t slotIndex) const override
        {
            if (!ValidateSlotIndex(slotIndex))
            {
                return std::nullopt;
            }

            DWORD type = 0;
            DWORD pos = 0;
            CPythonPlayer::Instance().GetGlobalQuickSlotData(slotIndex, &type, &pos);

            if (type == 0)
            {
                return std::nullopt;
            }

            QuickslotView view;
            view.type = static_cast<uint8_t>(type);
            view.pos = static_cast<uint32_t>(pos);
            return view;
        }

        void Clear() override
        {
            EterBase::ModernLogger::Info("PlayerFacade_Quickslot: Clear all quickslots requested");
            for (uint8_t i = 0; i < Domain::QuickslotContainerModel::QUICKSLOT_MAX_NUM; ++i)
            {
                DeleteQuickslot(i);
            }
        }

    private:
        std::expected<void, EterBase::EntityError> ValidateSlotIndex(uint8_t slotIndex) const
        {
            if (slotIndex >= Domain::QuickslotContainerModel::QUICKSLOT_MAX_NUM)
            {
                return std::unexpected(EterBase::EntityError::OutOfRange);
            }
            return {};
        }
    };

    std::unique_ptr<IQuickslotService> CreateQuickslotService()
    {
        return std::make_unique<PlayerFacade_Quickslot>();
    }
}
