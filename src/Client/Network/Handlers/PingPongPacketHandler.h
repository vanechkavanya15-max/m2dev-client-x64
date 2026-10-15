#pragma once

#include <span>
#include <cstdint>
#include <cstring>
#include "EterBase/Result.h"
#include "EterBase/EventBus.h"

namespace Client::Network::Handlers {

/**
 * @brief Zdarzenie emitowane po odebraniu pakietu PING od serwera.
 */
struct PingReceivedEvent : public EterBase::IEvent {
    uint32_t serverTime;

    explicit PingReceivedEvent(uint32_t time = 0) : serverTime(time) {}
};

/**
 * @brief Lekka asynchroniczna obsluga pakietow podtrzymania polaczenia ping/pong.
 */
class PingPongPacketHandler {
public:
    PingPongPacketHandler() = default;
    ~PingPongPacketHandler() = default;

    /**
     * @brief Przetwarza przychodzacy pakiet PING i publikuje zdarzenie.
     * @param payload Dane pakietu PING.
     * @return Sukces lub blad walidacji bufora.
     */
    [[nodiscard]] static inline EterBase::PacketResult<void> HandlePingPacket(std::span<const uint8_t> payload) {
        uint32_t serverTime = 0;
        if (payload.size() >= sizeof(uint32_t)) {
            std::memcpy(&serverTime, payload.data(), sizeof(uint32_t));
        }
        EterBase::EventBus::GetInstance().Publish(PingReceivedEvent(serverTime));
        return {};
    }
};

} // namespace Client::Network::Handlers
