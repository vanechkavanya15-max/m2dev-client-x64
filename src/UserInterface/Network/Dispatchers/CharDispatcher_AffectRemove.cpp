#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../PythonPlayer.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

namespace UserInterface::Core {
    /**
     * @brief Event published when an affect is removed from the character.
     * 
     * Replacing old python GUI bindings with decoupled event architecture.
     */
    struct AffectRemoveEvent : public IEvent {
        uint32_t type;
        uint8_t applyOn;

        AffectRemoveEvent(uint32_t type, uint8_t applyOn)
            : type(type), applyOn(applyOn) {}
    };
} // namespace UserInterface::Core

namespace Network::Dispatchers
{
    /**
     * @brief Processes the AFFECT_REMOVE packet in C++23.
     * 
     * @param buffer Incoming packet payload span.
     * @return EterBase::PacketResult<void> Result of the operation.
     */
    EterBase::PacketResult<void> ProcessAffectRemovePacket(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCAffectRemove))
        {
            EterBase::ModernLogger::Error("ProcessAffectRemovePacket: Buffer underflow. Expected >= {} bytes, got {} bytes.",
                sizeof(TPacketGCAffectRemove), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCAffectRemove*>(buffer.data());

        // Update the C++ memory state to remove the affect.
        // This handles deactivating skill slots for toggle skills if applicable.
        CPythonPlayer::Instance().ResetAffect(packet->dwType);

        // Publish event to GUI layer to decouple backend from UI
        UserInterface::Core::EventBus::GetInstance().Publish(
            UserInterface::Core::AffectRemoveEvent(packet->dwType, packet->bApplyOn));

        EterBase::ModernLogger::Info("ProcessAffectRemovePacket: Affect removed - type: {}, applyOn: {}",
            packet->dwType, packet->bApplyOn);

        return {};
    }
}
