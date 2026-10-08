#pragma once

#include <span>
#include <cstdint>
#include "../../../EterBase/Result.h"

namespace Client::Network::Handlers
{
    /**
     * @brief Handler for GC::DAMAGE_INFO packet.
     * Validates damage flags and notifies CombatDomain.
     */
    class DamageInfoHandler
    {
    public:
        /**
         * @brief Processes the DAMAGE_INFO packet.
         * @param buffer Byte buffer from the network stream.
         * @return EterBase::PacketResult<void> indicating success or a PacketError.
         */
        [[nodiscard]] static EterBase::PacketResult<void> Handle(std::span<const uint8_t> buffer);
    };
}
