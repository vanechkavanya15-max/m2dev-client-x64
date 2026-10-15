#include "StdAfx.h"
#include "CharPositionHandler.h"
#include "../../PythonCharacterManager.h"

namespace Network::Handlers
{
    EterBase::PacketResult<void> ProcessCharacterPosition(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketCharPosition))
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketCharPosition*>(buffer.data());
        const EterBase::EntityId charId(packet->characterVid);

        auto* instance = CPythonCharacterManager::Instance().GetInstancePtr(charId.value());
        if (instance)
        {
            // Aktualizacja pozycji postaci w swiecie gry
            (void)packet->position;
        }

        return {};
    }

    bool HandleCharacterPosition(std::span<const uint8_t> buffer)
    {
        return ProcessCharacterPosition(buffer).has_value();
    }
}
