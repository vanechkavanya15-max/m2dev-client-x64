#pragma once

#include <cstdint>
#include <span>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::Network
{
    class IActorNetworkDispatcher
    {
    public:
        virtual ~IActorNetworkDispatcher() = default;

        virtual EterBase::PacketResult<void> HandleCharacterAdd(std::span<const uint8_t> payload) = 0;
        virtual EterBase::PacketResult<void> HandleCharacterUpdate(std::span<const uint8_t> payload) = 0;
        virtual EterBase::PacketResult<void> HandleCharacterDelete(std::span<const uint8_t> payload) = 0;
        virtual EterBase::PacketResult<void> HandleCharacterMove(std::span<const uint8_t> payload) = 0;
        virtual EterBase::PacketResult<void> HandleDamageInfo(std::span<const uint8_t> payload) = 0;
        virtual EterBase::PacketResult<void> HandleCharacterDie(std::span<const uint8_t> payload) = 0;
        virtual void Clear() = 0;
    };
}
