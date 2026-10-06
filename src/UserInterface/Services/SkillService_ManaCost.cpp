#include "../StdAfx.h"
#include "ISkillService.h"
#include "IPlayerStatsService.h"
#include "../Packet.h"
#include "../Core/EventBus.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"

namespace UserInterface::Services
{
    /**
     * @brief Zdarzenie emitowane, gdy gracz probuje uzyc umiejetnosci, ale brakuje mu many (SP).
     */
    struct SkillManaCostFailedEvent : public Core::IEvent
    {
        EterBase::SkillId skillId;
        uint32_t requiredSp;
        uint32_t currentSp;

        SkillManaCostFailedEvent(EterBase::SkillId id, uint32_t req, uint32_t cur)
            : skillId(id), requiredSp(req), currentSp(cur)
        {
        }
    };

    /**
     * @brief Kalkulator weryfikujacy zapotrzebowanie na mane (SP) dla umiejetnosci.
     */
    class SkillManaCostCalculator
    {
    public:
        /**
         * @brief Weryfikuje, czy gracz posiada wystarczajaca ilosc many do uzycia umiejetnosci.
         * 
         * @param statsService Serwis ze statystykami gracza.
         * @param skillId Identyfikator umiejetnosci.
         * @param requiredSp Wymagana ilosc punktow many (SP).
         * @return std::expected<void, EterBase::CombatError> Sukces lub blad w przypadku braku many.
         */
        static std::expected<void, EterBase::CombatError> CheckManaCost(
            const IPlayerStatsService& statsService,
            EterBase::SkillId skillId,
            uint32_t requiredSp)
        {
            const uint32_t currentSp = statsService.GetPoints().sp;

            if (currentSp < requiredSp)
            {
                EterBase::ModernLogger::Warning(
                    "Skill {} requires {} SP, but only {} SP available.",
                    skillId.value(), requiredSp, currentSp
                );

                Core::EventBus::GetInstance().Publish(
                    SkillManaCostFailedEvent{skillId, requiredSp, currentSp}
                );

                return std::unexpected(EterBase::CombatError::InvalidAction);
            }

            EterBase::ModernLogger::Debug(
                "Mana check passed for Skill {}. (Req: {}, Cur: {})",
                skillId.value(), requiredSp, currentSp
            );

            return {};
        }
    };
}
