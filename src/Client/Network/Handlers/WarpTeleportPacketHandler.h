#pragma once

#include "EterBase/StdAfx.h"
#include <cstdint>
#include <span>
#include <cstring>
#include "EterBase/Result.h"
#include "../Protocol/Protocol.h"
#include "Client/Core/EventBus.h"

namespace Client::Network::Handlers {

/**
 * @brief Zdarzenie domenowe symbolizujace zadanie teleportacji (warp) gracza na nowa mape lub koordynaty.
 */
struct WarpTeleportEvent : public Client::Core::IEvent {
    int32_t targetX{0};
    int32_t targetY{0};
    uint32_t targetAddress{0};
    uint16_t targetPort{0};

    constexpr WarpTeleportEvent() = default;
    constexpr WarpTeleportEvent(int32_t x, int32_t y, uint32_t addr, uint16_t port)
        : targetX(x), targetY(y), targetAddress(addr), targetPort(port) {}
};

/**
 * @brief Klasa odpowiedzialna za obsluge pakietu teleportacji (Warp).
 *
 * Komponent architektonicznie zamkniety (Self-Contained) z pelnym bezpieczenstwem pamieci (C++20/23).
 * Weryfikuje bufor i emituje zdarzenie domenowe do EventBus-a.
 */
class WarpTeleportPacketHandler {
public:
    WarpTeleportPacketHandler() = delete;
    ~WarpTeleportPacketHandler() = delete;

    /**
     * @brief Przetwarza pakiet Warp i emituje odpowiednie zdarzenie na szyne EventBus.
     * 
     * @param buffer Bufor pakietu.
     * @return PacketResult Zwraca blad jesli pakiet jest niepoprawny lub uszkodzony.
     */
    [[nodiscard]] static EterBase::PacketResult<void> Handle(std::span<const uint8_t> buffer) noexcept {
        if (buffer.size() < sizeof(TPacketGCWarp)) {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCWarp packet{};
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCWarp));

        if (packet.wPort == 0) {
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        const uint32_t address = static_cast<uint32_t>(packet.lAddr);
        Client::Core::EventBus::GetInstance().Publish(
            WarpTeleportEvent{packet.lX, packet.lY, address, packet.wPort}
        );

        return {};
    }
};

} // namespace Client::Network::Handlers
