#include "../src/UserInterface/StdAfx.h"
#include "../src/UserInterface/InstanceBase.h"
#include "../src/UserInterface/PythonApplication.h"
#include "../src/UserInterface/PythonItem.h"
#include "EterLib/Camera.h"
#include "EffectLib/EffectManager.h"

// CCamera mock
float CCamera::CAMERA_MAX_DISTANCE = 5000.0f;

// CPythonApplication mock
CPythonApplication* CPythonApplication::ms_pInstance = nullptr;
int CPythonApplication::GetCursorMode() { return 0; }
int CPythonApplication::SetCursorNum(int) { return 0; }
void CPythonApplication::SetCursorVisible(int, bool) {}

// CPythonItem mock
bool CPythonItem::GetCloseItem(const D3DXVECTOR3&, unsigned long*, unsigned long) { return false; }
bool CPythonItem::GetCloseMoney(const D3DXVECTOR3&, unsigned long*, unsigned long) { return false; }

// CEffectManager mock
int CEffectManager::RegisterEffect(const char*, bool, bool) { return 0; }
int CEffectManager::CreateEffect(const char*, const D3DXVECTOR3&, const D3DXVECTOR3&) { return 0; }

// CInstanceBase mock
bool CInstanceBase::CanAttack() { return true; }
bool CInstanceBase::CanChangeTarget() { return true; }
bool CInstanceBase::CanMove() { return true; }
bool CInstanceBase::CanUseSkill() { return true; }
bool CInstanceBase::IsAffect(unsigned int) { return false; }
bool CInstanceBase::IsAttackableInstance(CInstanceBase&) { return true; }
bool CInstanceBase::IsConflictAlignmentInstance(CInstanceBase&) { return false; }
bool CInstanceBase::IsKiller() { return false; }
bool CInstanceBase::IsPVPInstance(CInstanceBase&) { return false; }
bool CInstanceBase::IsTargetableInstance(CInstanceBase&) { return true; }
bool CInstanceBase::NEW_AttackToDestInstanceDirection(CInstanceBase&) { return true; }
bool CInstanceBase::NEW_MoveToDestPixelPositionDirection(const D3DXVECTOR3&) { return true; }
float CInstanceBase::GetDistance(CInstanceBase*) { return 0.0f; }
float CInstanceBase::NEW_GetDistanceFromDestPixelPosition(const D3DXVECTOR3&) { return 0.0f; }
int CInstanceBase::CanAttackHorseLevel() { return 1; }
int CInstanceBase::GetAlignment() { return 0; }
int CInstanceBase::IsBuilding() { return 0; }
int CInstanceBase::IsDead() { return 0; }
int CInstanceBase::IsEnemy() { return 0; }
int CInstanceBase::IsInSafe() { return 0; }
int CInstanceBase::isLock() { return 0; }
int CInstanceBase::IsMountingHorse() { return 0; }
int CInstanceBase::IsNewMount() { return 0; }
int CInstanceBase::IsPC() { return 1; }
int CInstanceBase::IsPoly() { return 0; }
int CInstanceBase::IsSameEmpire(CInstanceBase&) { return 1; }
int CInstanceBase::IsSleep() { return 0; }
int CInstanceBase::IsStone() { return 0; }
int CInstanceBase::IsUsingMovingSkill() { return 0; }
int CInstanceBase::IsUsingSkill() { return 0; }
int CInstanceBase::IsWoodenDoor() { return 0; }
int CInstanceBase::NEW_IsClickableDistanceDestInstance(CInstanceBase&) { return 1; }
unsigned char CInstanceBase::GetPKMode() { return 0; }
unsigned long CInstanceBase::GetDuelMode() { return 0; }
unsigned long CInstanceBase::GetGuildID() { return 0; }
unsigned long CInstanceBase::GetVirtualID() { return 0; }
void CInstanceBase::ClearFlyTargetInstance() {}
void CInstanceBase::NEW_Attack(float) {}
void CInstanceBase::NEW_Attack() {}
void CInstanceBase::NEW_GetPixelPosition(D3DXVECTOR3* p) { if (p) { p->x = 0; p->y = 0; p->z = 0; } }
void CInstanceBase::NEW_MoveToDestInstanceDirection(CInstanceBase&) {}
void CInstanceBase::NEW_MoveToDirection(float) {}
void CInstanceBase::NEW_Stop() {}
void CInstanceBase::OnTargeted() {}
void CInstanceBase::OnUntargeted() {}
void CInstanceBase::SetComboType(unsigned int) {}
void CInstanceBase::SetFlyTargetInstance(CInstanceBase&) {}
