#include "StdAfx.h"
#include "TargetHPHandler.h"
#include "../../PythonPlayer.h"

namespace Network::Handlers
{
    EterBase::PacketResult<void> ProcessTargetHP(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketTargetHP))
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketTargetHP*>(buffer.data());
        const EterBase::EntityId targetId(packet->targetVid);

        // Aktualizacja celu w CPythonPlayer
        CPythonPlayer::Instance().SetTarget(targetId.value());

        return {};
    }

    bool HandleTargetHP(std::span<const uint8_t> buffer)
    {
        return ProcessTargetHP(buffer).has_value();
    }
}
