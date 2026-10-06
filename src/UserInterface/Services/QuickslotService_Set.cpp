#include "../StdAfx.h"
#include "IQuickslotService.h"
#include "../Packet.h"
#include "../PythonNetworkStream.h"
#include "../Core/EventBus.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/StrongTypes.h"

namespace UserInterface::Services
{
    /**
     * @brief Zdarzenie wywolywane po udanym zleceniu przypisania do paska szybkiego dostepu.
     */
    struct QuickslotSetEvent : public Core::IEvent
    {
        uint8_t slotIndex;
        QuickslotView view;

        QuickslotSetEvent(uint8_t idx, const QuickslotView& v) : slotIndex(idx), view(v) {}
    };

    /**
     * @brief Implementacja czesciowa mikro-serwisu paska szybkiego dostepu odpowiedzialna za akcje "Set".
     */
    class QuickslotService_Set final : public IQuickslotService
    {
    public:
        void SetQuickslot(uint8_t slotIndex, const QuickslotView& slot) override
        {
            auto result = PerformSetQuickslot(slotIndex, slot);
            if (!result)
            {
                EterBase::ModernLogger::Error("Failed to set quickslot {}: {}", slotIndex, EterBase::ToString(result.error()));
            }
        }

        void DeleteQuickslot(uint8_t) override {}
        void SwapQuickslots(uint8_t, uint8_t) override {}
        std::optional<QuickslotView> GetQuickslot(uint8_t) const override { return std::nullopt; }
        void Clear() override {}

    private:
        EterBase::PacketResult<void> PerformSetQuickslot(uint8_t slotIndex, const QuickslotView& slot)
        {
            if (slotIndex >= QUICKSLOT_MAX_NUM)
            {
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            TPacketCGQuickSlotAdd packet{};
            packet.header = CG::QUICKSLOT_ADD;
            packet.length = sizeof(packet);
            packet.pos = slotIndex;
            packet.slot.Type = slot.type;
            packet.slot.Position = static_cast<uint8_t>(slot.pos);

            auto& stream = CPythonNetworkStream::Instance();
            if (!stream.Send(sizeof(packet), &packet))
            {
                return EterBase::MakeError(EterBase::PacketError::SessionClosed);
            }

            EterBase::ModernLogger::Info("QuickslotSet action requested: slotIndex={}, type={}, pos={}", 
                slotIndex, slot.type, slot.pos);

            Core::EventBus::GetInstance().Publish(QuickslotSetEvent{slotIndex, slot});

            return {};
        }
    };
}
