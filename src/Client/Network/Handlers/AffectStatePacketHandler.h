#pragma once

#include <span>
#include <cstdint>
#include "../../../EterBase/PacketResult.h"
#include "../../../Client/Core/EventBus.h"

namespace Client::Core::Events {

/**
 * @brief Zdarzenie domenowe informujace o nalozeniu nowego buffa/efektu (Affect) na gracza.
 */
struct AffectAddEvent : public Client::Core::IEvent {
    uint32_t type{0};
    uint8_t pointIdxApplyOn{0};
    int32_t applyValue{0};
    uint32_t flag{0};
    int32_t duration{0};
    int32_t spCost{0};

    constexpr AffectAddEvent() noexcept = default;
    constexpr AffectAddEvent(uint32_t t, uint8_t p, int32_t v, uint32_t f, int32_t d, int32_t c) noexcept
        : type(t), pointIdxApplyOn(p), applyValue(v), flag(f), duration(d), spCost(c) {}
};

/**
 * @brief Zdarzenie domenowe informujace o usunieciu/wygasnieciu buffa/efektu (Affect) gracza.
 */
struct AffectRemoveEvent : public Client::Core::IEvent {
    uint32_t type{0};
    uint8_t applyOn{0};

    constexpr AffectRemoveEvent() noexcept = default;
    constexpr AffectRemoveEvent(uint32_t t, uint8_t a) noexcept
        : type(t), applyOn(a) {}
};

} // namespace Client::Core::Events

namespace Client::Network::Handlers {

/**
 * @brief Zero-Conflict, bezstanowy handler do obslugi pakietow AffectAdd i AffectRemove.
 * Odpowiada za bezpieczne parsowanie danych ze strumienia i rozsylanie zdarzen domenowych
 * do UI bez bezposredniego wiazania sie z instancja aplikacji.
 */
class AffectStatePacketHandler {
public:
    AffectStatePacketHandler() = default;
    ~AffectStatePacketHandler() = default;

    /**
     * @brief Przetwarza pakiet TPacketGCAffectAdd.
     * @param payload Dane pakietu w formie read-only span.
     * @return Sukces badz kod bledu parsowania.
     */
    static EterBase::PacketResult<void> HandleAffectAdd(std::span<const uint8_t> payload);

    /**
     * @brief Przetwarza pakiet TPacketGCAffectRemove.
     * @param payload Dane pakietu w formie read-only span.
     * @return Sukces badz kod bledu parsowania.
     */
    static EterBase::PacketResult<void> HandleAffectRemove(std::span<const uint8_t> payload);
};

} // namespace Client::Network::Handlers
