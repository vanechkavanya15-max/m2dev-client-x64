#pragma once

#include <cstdint>
#include <span>
#include <functional>
#include "../../../EterBase/Result.h"
#include "../../World/ECSComponents.h"
#include "../../World/ActorMotionMachine.h"

namespace Client::Network::Handlers {

    /**
     * @brief Handler sieciowy do pakietu GC::DEAD dla architektury C++23.
     * Wykorzystuje wstrzykiwanie zaleznosci (DI) przez callback w celu unikniecia singletonow.
     */
    class DeadHandler {
    public:
        using MotionStateCallback = std::function<void(Client::World::EntityVid, World::MotionState)>;

        /**
         * @brief Konstruktor wstrzykujacy zaleznosc umozliwiajaca zmiane stanu animacji.
         * @param motionCallback Callback wywolywany z EntityVid oraz nowym MotionState.
         */
        explicit DeadHandler(MotionStateCallback motionCallback);

        /**
         * @brief Przetwarza pakiet TPacketGCDead i aktywuje callback.
         * @param buffer Bufor z danymi pakietu.
         * @return Sukces lub blad dekodowania (EterBase::PacketResult<void>).
         */
        EterBase::PacketResult<void> ProcessDeadPacket(std::span<const uint8_t> buffer);

    private:
        MotionStateCallback m_motionCallback;
    };

} // namespace Client::Network::Handlers
