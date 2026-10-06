#include "StdAfx.h"
#include "DeadHandler.h"
#include "../../PythonCharacterManager.h"
#include "../../PythonPlayer.h"

namespace Network::Handlers
{
    EterBase::PacketResult<void> ProcessDeadPacket(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketDead))
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketDead*>(buffer.data());
        const EterBase::EntityId targetId(packet->targetId);
        
        auto& characterManager = CPythonCharacterManager::Instance();
        auto* targetInstance = characterManager.GetInstancePtr(targetId.value());

        if (targetInstance)
        {
            // Oznaczenie encji jako martwej
            targetInstance->Die();

            // Wyczyszczenie celu walki, jesli to aktualny cel
            CPythonPlayer::Instance().NotifyCharacterDead(targetId.value());

            auto* mainInstance = characterManager.GetMainInstancePtr();
            if (mainInstance == targetInstance)
            {
                // Notyfikacja gracza o wlasnej smierci
                CPythonPlayer::Instance().NotifyDeadMainCharacter();
            }
        }

        return {};
    }

    bool HandleDeadPacket(std::span<const uint8_t> buffer)
    {
        return ProcessDeadPacket(buffer).has_value();
    }
}
