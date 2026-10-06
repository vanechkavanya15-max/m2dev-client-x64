#include "../StdAfx.h"
#include "IMountHorseService.h"
#include "../Packet.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include "../Core/CombatEvents.h"
#include "../InstanceBase.h"
#include "../PythonCharacterManager.h"

namespace UserInterface::Actors
{
    /**
     * @brief Zewnetrzny komponent do obslugi walki z konia (implementacja posrednia dla architektury).
     * Zakladamy, ze agent obsluguje ataki poprzez ten serwis dla zachowania SRP.
     */
    class MountCombatManager
    {
    public:
        /**
         * @brief Sprawdza czy dany gracz (CInstanceBase) ma prawo zaatakowac z grzbietu.
         */
        static std::expected<void, Core::CombatEvents::CombatError> ValidateAttack(EterBase::EntityId attackerId, EterBase::EntityId victimId)
        {
            if (!attackerId) return std::unexpected(Core::CombatEvents::CombatError::InvalidAttacker);
            if (!victimId) return std::unexpected(Core::CombatEvents::CombatError::InvalidVictim);
            if (attackerId == victimId) return std::unexpected(Core::CombatEvents::CombatError::SelfHarmNotAllowed);

            // W przyszlosci tutaj mozna podlaczyc PythonCharacterManager by weryfikowac stan CInstanceBase
            return {};
        }

        /**
         * @brief Przetwarza atak z wierzchowca.
         */
        static std::expected<void, Core::CombatEvents::CombatError> ProcessMountAttack(
            EterBase::EntityId attackerId, 
            EterBase::EntityId victimId, 
            uint32_t damageAmount)
        {
            auto validation = ValidateAttack(attackerId, victimId);
            if (!validation)
                return std::unexpected(validation.error());

            EterBase::ModernLogger::Info("Processing mount attack from [{}] to [{}], damage: {}", 
                                        attackerId.value(), victimId.value(), damageAmount);

            auto damageEventResult = Core::CombatEvents::ActorDamaged::Create(
                attackerId, 
                victimId, 
                damageAmount, 
                Core::CombatEvents::AttackType::Melee
            );

            if (!damageEventResult)
            {
                EterBase::ModernLogger::Error("Failed to create mount attack event: {}", 
                                            Core::CombatEvents::CombatErrorToString(damageEventResult.error()));
                return std::unexpected(damageEventResult.error());
            }

            Core::EventBus::GetInstance().Publish(damageEventResult.value());
            
            return {};
        }
    };
} // namespace UserInterface::Actors
