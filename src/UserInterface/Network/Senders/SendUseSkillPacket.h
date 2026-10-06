#pragma once

#include <cstdint>
#include <span>
#include <optional>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

class CNetworkStream; // Forward declaration

namespace UserInterface::Network::Senders {

/**
 * @brief Zdarzenie emitowane po udanym wyslaniu zadania uzycia umiejetnosci.
 * 
 * Sluzy do rozdzielenia logiki sieciowej (wysylanie) od logiki wewnetrznej i GUI.
 */
struct SkillUseRequestedEvent : public UserInterface::Core::IEvent
{
    EterBase::SkillId skillId;
    std::optional<EterBase::EntityId> targetId;

    /**
     * @brief Konstruktor zdarzenia zadania uzycia umiejetnosci.
     * @param skillId Identyfikator uzywanej umiejetnosci.
     * @param targetId Opcjonalny identyfikator celu umiejetnosci.
     */
    explicit SkillUseRequestedEvent(EterBase::SkillId skillId, std::optional<EterBase::EntityId> targetId = std::nullopt)
        : skillId(skillId), targetId(targetId) {}
};

/**
 * @brief Handler odpowiedzialny za wysylanie pakietow uzycia umiejetnosci do serwera (C++23).
 * 
 * Zastepuje stare wywolania CPythonNetworkStream::SendUseSkillPacket, implementujac
 * SRP (Single Responsibility Principle) i standard C++23, wlacznie z pelna obsluga bledow przez std::expected.
 */
class SendUseSkillHandler
{
public:
    /**
     * @brief Wysyla pakiet uzycia umiejetnosci do serwera.
     * 
     * Wykorzystuje nowoczesne typy silne oraz monadyczne obiekty std::optional. Wymusza 
     * scisle wyrownywanie pakietu sieciowego bez uzycia notacji wegierskiej.
     * 
     * @param skillId Identyfikator umiejetnosci, ktora gracz probuje uzyc.
     * @param targetId Opcjonalny cel uzycia umiejetnosci (w przypadku umiejetnosci obszarowych/na siebie - brak).
     * @param networkStream Wskaznik na strumien sieciowy (wymagany do wyslania pakietu).
     * 
     * @return EterBase::PacketResult<void> oznaczajacy poprawnosc wykonania zadania,
     *         bądź kod bledu z EterBase::PacketError w przypadku porazki.
     */
    static EterBase::PacketResult<void> SendUseSkill(
        EterBase::SkillId skillId, 
        std::optional<EterBase::EntityId> targetId, 
        CNetworkStream* networkStream);
};

} // namespace UserInterface::Network::Senders
