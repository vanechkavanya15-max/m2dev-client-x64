 #include <queue>
 #include <optional>
 #include <chrono>
#include "../../EterBase/StrongTypes.h"
 
 namespace Client::Gameplay {
 
     static std::pair<int32_t, DamageFlag> CalculateSkillStrike(const AttackerStats& attacker, const TargetStats& target, int32_t skillBaseDamage, int32_t skillMultiplier);
 };
 
class CombatDomain {
public:
    static void UpdateTargetHP(EterBase::EntityId targetVid, uint8_t hpPercent);
    static std::optional<std::pair<EterBase::EntityId, uint8_t>> GetCurrentTarget();

private:
    static std::optional<EterBase::EntityId> m_currentTargetVid;
    static uint8_t m_currentTargetHpPercent;
};

 } // namespace Client::Gameplay
