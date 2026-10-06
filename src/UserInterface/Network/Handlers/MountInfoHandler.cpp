#include "StdAfx.h"
#include "MountInfoHandler.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/Result.h"

namespace Network::Handlers
{
    EterBase::PacketResult<void> ProcessMountPacket(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketMount))
        {
            EterBase::ModernLogger::Error("ProcessMountPacket: Buffer too small. Expected {}, got {}", sizeof(PacketMount), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketMount*>(buffer.data());
        const EterBase::EntityId charId(packet->vid);
        const EterBase::ItemVnum mountVnum(packet->mount_vid);

        // Zgodnie ze standardem 2026, handler nie modyfikuje bezposrednio GUI/CInstanceBase.
        // Odczytuje dane i rozglasza zdarzenie do EventBusa.
        UserInterface::Core::EventBus::Instance().Publish(
            UserInterface::Core::MountStateChangedEvent(charId.value(), mountVnum.value(), packet->pos)
        );

        return {};
    }

    bool HandleMountPacket(std::span<const uint8_t> buffer)
    {
        return ProcessMountPacket(buffer).has_value();
    }
}
