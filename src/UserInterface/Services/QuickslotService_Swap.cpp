#include "../StdAfx.h"
#include "IQuickslotService.h"
#include "../Packet.h"
class CPythonNetworkStream {
public:
    static CPythonNetworkStream& Instance();
    bool Send(int size, const void* buffer);
};

#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../Core/EventBus.h"
#include "../Domain/QuickslotContainerModel.h"

#include <cstring>
#include <span>
#include <optional>

namespace UserInterface::Services
{
#pragma pack(push, 1)
    /**
     * @brief Proxy structure for CG::QUICKSLOT_SWAP (0x050B) network payload.
     * 
     * Ensures strict 1-byte alignment and modern C++ naming conventions.
     * Maps to legacy TPacketCGQuickSlotSwap memory layout.
     */
    struct ProxyPacketCGQuickSlotSwap
    {
        uint16_t header;         ///< Packet opcode (CG::QUICKSLOT_SWAP).
        uint16_t length;         ///< Total length of the packet.
        uint8_t  pos;            ///< The source quickslot position.
        uint8_t  change_pos;     ///< The destination quickslot position.
    };
    static_assert(sizeof(ProxyPacketCGQuickSlotSwap) == 6, "ProxyPacketCGQuickSlotSwap must be exactly 6 bytes");
#pragma pack(pop)

    /**
     * @brief Partial implementation of IQuickslotService for swapping quickslots.
     */
    class QuickslotService_Swap : public IQuickslotService
    {
    public:
        virtual ~QuickslotService_Swap() = default;

        void SetQuickslot(uint8_t slotIndex, const QuickslotView& slot) override { }
        void DeleteQuickslot(uint8_t slotIndex) override { }
        
        void SwapQuickslots(uint8_t fromIndex, uint8_t toIndex) override
        {
            if (fromIndex == toIndex)
            {
                return;
            }

            if (fromIndex >= Domain::QuickslotContainerModel::QUICKSLOT_MAX_NUM || 
                toIndex >= Domain::QuickslotContainerModel::QUICKSLOT_MAX_NUM)
            {
                EterBase::ModernLogger::Log(
                    EterBase::LogLevel::Error, 
                    "Failed to swap quickslots: Index out of bounds ({} -> {})", 
                    fromIndex, toIndex
                );
                return;
            }

            ProxyPacketCGQuickSlotSwap packet{};
            std::memset(&packet, 0, sizeof(packet));
            packet.header = 0x050B; // CG::QUICKSLOT_SWAP
            packet.length = sizeof(ProxyPacketCGQuickSlotSwap);
            packet.pos = fromIndex;
            packet.change_pos = toIndex;

            if (!CPythonNetworkStream::Instance().Send(sizeof(ProxyPacketCGQuickSlotSwap), &packet))
            {
                EterBase::ModernLogger::Log(
                    EterBase::LogLevel::Error, 
                    "Failed to transmit QUICKSLOT_SWAP packet ({} -> {}). Session might be closed.", 
                    fromIndex, toIndex
                );
                return;
            }

            // Publish event to update domain state without direct GUI coupling
            Core::EventBus::GetInstance().Publish(Domain::QuickslotSwappedEvent(fromIndex, toIndex));

            EterBase::ModernLogger::Log(
                EterBase::LogLevel::Info, 
                "Successfully dispatched QUICKSLOT_SWAP packet ({} -> {})", 
                fromIndex, toIndex
            );
        }

        std::optional<QuickslotView> GetQuickslot(uint8_t slotIndex) const override { return std::nullopt; }
        void Clear() override { }
    };
}
