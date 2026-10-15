#pragma once

#include <cstdint>

class ICombatPolicy
{
public:
	virtual ~ICombatPolicy() = default;
	virtual bool CanAttackInstance(uint32_t dwTargetVID) { return true; }
	virtual bool AttackTarget(uint32_t dwTargetVID) { return true; }
	virtual void UpdateAttackState(float fElapsedTime) {}
};
