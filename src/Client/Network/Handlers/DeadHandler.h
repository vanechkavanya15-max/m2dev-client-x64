#pragma once

#include <cstdint>
#include <span>
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "Client/World/ActorMotionMachine.h"

namespace Client::Network::Handlers {

    /**
     * @brief Handler sieciowy do pakietu GC::DEAD dla architektury C++23.
     */
    class DeadHandler {
    public:
        using MotionStateCallback = void(*)(EterBase::EntityId, ::World::MotionState);

        explicit DeadHandler(MotionStateCallback motionCallback = nullptr) noexcept;

        EterBase::PacketResult<void> ProcessDeadPacket(std::span<const uint8_t> buffer);

    private:
        MotionStateCallback m_motionCallback{nullptr};
    };

} // namespace Client::Network::Handlers
