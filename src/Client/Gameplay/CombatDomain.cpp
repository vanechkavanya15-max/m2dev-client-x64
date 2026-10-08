     return {damageAfterDef, flags};
 }
 


std::optional<EterBase::EntityId> CombatDomain::m_currentTargetVid = std::nullopt;
uint8_t CombatDomain::m_currentTargetHpPercent = 0;

void CombatDomain::UpdateTargetHP(EterBase::EntityId targetVid, uint8_t hpPercent) {
    m_currentTargetVid = targetVid;
    m_currentTargetHpPercent = hpPercent;
}

std::optional<std::pair<EterBase::EntityId, uint8_t>> CombatDomain::GetCurrentTarget() {
    if (m_currentTargetVid) {
        return std::make_pair(*m_currentTargetVid, m_currentTargetHpPercent);
    }
    return std::nullopt;
}
 } // namespace Client::Gameplay
