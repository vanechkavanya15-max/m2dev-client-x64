#pragma once

#include <cstdint>
#include <span>
#include <expected>

#include "Client/Core/EventBus.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "Client/Network/Protocol/Protocol.h"

namespace Client::Network::Handlers {

/**
 * @brief Zdarzenie emitowane, gdy zostanie odebrana animacja uzycia umiejetnosci (skilla) przez inna postac.
 * Pozwala to na synchronizacje animacji czarow ofensywnych/obronnych z systemem ActorMotionMachine.
 */
struct SkillMotionReceivedEvent : public Client::Core::IEvent
{
    /** @brief ID bytu (VID), ktory uzywa skilla. */
    EterBase::EntityId vid{0};
    /** @brief ID celu (victim VID), na ktory skill jest rzucany (0 jesli brak). */
    EterBase::EntityId victimVid{0};
    /** @brief ID animacji (Motion ID) odpowiadajace uzytej umiejetnosci. */
    uint16_t motionId{0};

    SkillMotionReceivedEvent() = default;
    explicit SkillMotionReceivedEvent(EterBase::EntityId v, EterBase::EntityId vVid, uint16_t mId)
        : vid(v), victimVid(vVid), motionId(mId) {}
};

/**
 * @brief Handler odpowiedzialny za parsowanie pakietu TPacketGCMotion.
 * Rozprowadza informacje po systemie uzywajac EventBus w architekturze Zero-Conflict.
 */
class SkillMotionPacketHandler
{
public:
    /**
     * @brief Dekoduje pakiet TPacketGCMotion i publikuje zdarzenie dla innych systemow.
     * 
     * @param buffer Widok pamieci na bufor z pakietem.
     * @return EterBase::PacketResult<void> Wynik operacji (sukces lub blad m.in underflow).
     */
    static EterBase::PacketResult<void> HandlePacket(std::span<const uint8_t> buffer);
};

} // namespace Client::Network::Handlers
