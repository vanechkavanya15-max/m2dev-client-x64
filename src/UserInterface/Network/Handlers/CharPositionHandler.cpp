#include "../../StdAfx.h"
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
            instance->NEW_SetPixelPosition(TPixelPosition(static_cast<float>(packet->x), static_cast<float>(packet->y), 0.0f));
        }

        return {};
    }

    bool HandleCharacterPosition(std::span<const uint8_t> buffer)
    {
        return ProcessCharacterPosition(buffer).has_value();
    }
}
