#include "../../StdAfx.h"
#include "DamageHandler.h"
#include "../../PythonCharacterManager.h"
#include "../../PythonPlayer.h"

namespace Network::Handlers
{
    EterBase::PacketResult<void> ProcessDamagePacket(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketDamageInfo))
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketDamageInfo*>(buffer.data());
        const EterBase::EntityId victimId(packet->victimVid);

        auto& charMgr = CPythonCharacterManager::Instance();
        auto* victimInstance = charMgr.GetInstancePtr(victimId.value());

        if (victimInstance)
        {
            // Aktualizacja wizualna i logiczna efektu uderzenia/obrazen
            bool bSelf = (victimInstance == charMgr.GetMainInstancePtr());
            victimInstance->AddDamageEffect(packet->damageValue, packet->damageFlag, bSelf, false);
        }

        return {};
    }

    bool HandleDamagePacket(std::span<const uint8_t> buffer)
    {
        return ProcessDamagePacket(buffer).has_value();
    }
}
