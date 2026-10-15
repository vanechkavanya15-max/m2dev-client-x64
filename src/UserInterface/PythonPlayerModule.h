#pragma once

#include "StdAfx.h"

// ============================================================================
// Enums shared across Player module domains
// ============================================================================

enum
{
	EMOTION_CLAP		= 1,
	EMOTION_CHEERS_1,
	EMOTION_CHEERS_2,
	EMOTION_DANCE_1,
	EMOTION_DANCE_2,
	EMOTION_DANCE_3,
	EMOTION_DANCE_4,
	EMOTION_DANCE_5,
	EMOTION_DANCE_6,		// Gangnam style
	EMOTION_CONGRATULATION,
	EMOTION_FORGIVE,
	EMOTION_ANGRY,
	
	EMOTION_ATTRACTIVE,
	EMOTION_SAD,
	EMOTION_SHY,
	EMOTION_CHEERUP,
	EMOTION_BANTER,
	EMOTION_JOY,

	EMOTION_KISS		= 51,
	EMOTION_FRENCH_KISS,
	EMOTION_SLAP,
};

enum
{
	REFINE_SCROLL_TYPE_MAKE_SOCKET = 1,
	REFINE_SCROLL_TYPE_UP_GRADE = 2,
};

enum
{
	REFINE_CANT,
	REFINE_OK,
	REFINE_ALREADY_MAX_SOCKET_COUNT,
	REFINE_NEED_MORE_GOOD_SCROLL,
	REFINE_CANT_MAKE_SOCKET_ITEM,
	REFINE_NOT_NEXT_GRADE_ITEM,
	REFINE_CANT_REFINE_METIN_TO_EQUIPMENT,
	REFINE_CANT_REFINE_ROD,
};

enum
{
	ATTACH_METIN_CANT,
	ATTACH_METIN_OK,
	ATTACH_METIN_NOT_MATCHABLE_ITEM,
	ATTACH_METIN_NO_MATCHABLE_SOCKET,
	ATTACH_METIN_NOT_EXIST_GOLD_SOCKET,
	ATTACH_METIN_CANT_ATTACH_TO_EQUIPMENT,
};

enum
{
	DETACH_METIN_CANT,
	DETACH_METIN_OK,
};

#ifdef ENABLE_NEW_EQUIPMENT_SYSTEM
class CBeltInventoryHelper
{
public:
	typedef BYTE TGradeUnit;

	static TGradeUnit GetBeltGradeByRefineLevel(int refineLevel)
	{
		static TGradeUnit beltGradeByLevelTable[] = 
		{
			0,			// +0
			1,			// +1
			1,			// +2
			2,			// +3
			2,			// +4
			3,			// +5
			4,			// +6
			5,			// +7
			6,			// +8
			7,			// +9
		};

		return beltGradeByLevelTable[refineLevel];
	}

	static const TGradeUnit* GetAvailableRuleTableByGrade()
	{
		static TGradeUnit availableRuleByGrade[c_Belt_Inventory_Slot_Count] = {
			1, 2, 4, 6,
			3, 3, 4, 6,
			5, 5, 5, 6,
			7, 7, 7, 7
		};

		return availableRuleByGrade;
	}

	static bool IsAvailableCell(WORD cell, int beltGrade)
	{
		const TGradeUnit* ruleTable = GetAvailableRuleTableByGrade();
		return ruleTable[cell] <= beltGrade;
	}
};
#endif


// ============================================================================
// Domain: Inventory (src/UserInterface/PythonPlayerModule_Inventory.cpp)
// ============================================================================
PyObject * playerPickCloseItem(PyObject* poSelf, PyObject* poArgs);
PyObject * playerMoveItem(PyObject* poSelf, PyObject* poArgs);
PyObject * playerSendClickItemPacket(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetItemIndex(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs);
PyObject * playerGetItemFlags(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs);
PyObject * playerGetItemCount(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs);
PyObject * playerSetItemCount(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetItemCountByVnum(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetItemMetinSocket(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetItemAttribute(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetItemLink(PyObject * poSelf, PyObject * poArgs);
PyObject * playerGetISellItemPrice(PyObject * poSelf, PyObject * poArgs);
PyObject * playerisItem(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsBeltInventorySlot(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsEquipmentSlot(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsDSEquipmentSlot(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsCostumeSlot(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsOpenPrivateShop(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsValuableItem(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetItemGrade(PyObject* poSelf, PyObject* poArgs);
PyObject * playerCanRefine(PyObject * poSelf, PyObject * poArgs);
PyObject * playerCanAttachMetin(PyObject* poSelf, PyObject* poArgs);
PyObject * playerCanDetach(PyObject * poSelf, PyObject * poArgs);
PyObject * playerCanUnlock(PyObject * poSelf, PyObject * poArgs);
PyObject * playerIsRefineGradeScroll(PyObject* poSelf, PyObject* poArgs);
PyObject * playerSetItemData(PyObject* poSelf, PyObject* poArgs);
PyObject * playerSetItemMetinSocket(PyObject* poSelf, PyObject* poArgs);
PyObject * playerSetItemAttribute(PyObject* poSelf, PyObject* poArgs);
PyObject * playerSetAutoPotionInfo(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetAutoPotionInfo(PyObject* poSelf, PyObject* poArgs);
PyObject * playerSlotTypeToInvenType(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsEquippingBelt(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsAvailableBeltInventoryCell(PyObject* poSelf, PyObject* poArgs);
PyObject * playerSendDragonSoulRefine(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetElk(PyObject* poSelf, PyObject* poArgs);

// ============================================================================
// Domain: Combat (src/UserInterface/PythonPlayerModule_Combat.cpp)
// ============================================================================
PyObject * playerClearTarget(PyObject* poSelf, PyObject* poArgs);
PyObject * playerSetTarget(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetTargetVID(PyObject* poSelf, PyObject* poArgs);
PyObject * playerOpenCharacterMenu(PyObject* poSelf, PyObject* poArgs);
PyObject * playerCanAttackInstance(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsPVPInstance(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsSameEmpire(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsChallengeInstance(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsRevengeInstance(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsCantFightInstance(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsInSafeArea(PyObject* poSelf, PyObject* poArgs);
PyObject * playerComboAttack(PyObject* poSelf, PyObject* poArgs);
PyObject * playerSetAttackKeyState(PyObject* poSelf, PyObject* poArgs);
PyObject * playerSetWeaponAttackBonusFlag(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetPKMode(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsPartyMember(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsPartyLeader(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsPartyLeaderByPID(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetPartyMemberHPPercentage(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetPartyMemberState(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetPartyMemberAffects(PyObject* poSelf, PyObject* poArgs);
PyObject * playerRemovePartyMember(PyObject* poSelf, PyObject* poArgs);
PyObject * playerExitParty(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetAlignmentData(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetStatus(PyObject* poSelf, PyObject* poArgs);
PyObject * playerSetStatus(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetEXP(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetGuildID(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetGuildName(PyObject* poSelf, PyObject* poArgs);

// ============================================================================
// Domain: Skills (src/UserInterface/PythonPlayerModule_Skills.cpp)
// ============================================================================
PyObject * playerIsSkillCoolTime(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetSkillCoolTime(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs);
PyObject * playerResetSkillCoolTimeForSlot(PyObject* poSelf, PyObject* poArgs);
PyObject* playerResetHorseSkillCoolTime(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsSkillActive(PyObject* poSelf, PyObject* poArgs);
PyObject * playerUseGuildSkill(PyObject* poSelf, PyObject* poArgs);
PyObject * playerAffectIndexToSkillIndex(PyObject* poSelf, PyObject* poArgs);
PyObject * playerSetSkill(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetSkillIndex(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs);
PyObject * playerGetSkillSlotIndex(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetSkillGrade(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetSkillLevel(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs);
PyObject * playerGetSkillCurrentEfficientPercentage(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetSkillNextEfficientPercentage(PyObject* poSelf, PyObject* poArgs);
PyObject * playerClickSkillSlot(PyObject * poSelf, PyObject * poArgs);
PyObject * playerChangeCurrentSkillNumberOnly(PyObject * poSelf, PyObject * poArgs);
PyObject * playerClearSkillDict(PyObject * poSelf, PyObject * poArgs);
PyObject * playerToggleCoolTime(PyObject* poSelf, PyObject* poArgs);
PyObject * playerToggleLevelLimit(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetQuickPage(PyObject* poSelf, PyObject* poArgs);
PyObject * playerSetQuickPage(PyObject* poSelf, PyObject* poArgs);
PyObject * playerLocalQuickSlotIndexToGlobalQuickSlotIndex(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetLocalQuickSlot(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetGlobalQuickSlot(PyObject* poSelf, PyObject* poArgs);
PyObject * playerRequestAddLocalQuickSlot(PyObject * poSelf, PyObject * poArgs);
PyObject * playerRequestAddToEmptyLocalQuickSlot(PyObject* poSelf, PyObject* poArgs);
PyObject * playerRequestDeleteGlobalQuickSlot(PyObject * poSelf, PyObject * poArgs);
PyObject * playerRequestMoveGlobalQuickSlotToLocalQuickSlot(PyObject * poSelf, PyObject * poArgs);
PyObject * playerRequestUseLocalQuickSlot(PyObject* poSelf, PyObject* poArgs);
PyObject * playerRemoveQuickSlotByValue(PyObject* poSelf, PyObject* poArgs);

// ============================================================================
// Domain: Movement (src/UserInterface/PythonPlayerModule_Movement.cpp)
// ============================================================================
PyObject * playerGetMainCharacterPosition(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetCharacterDistance(PyObject* poSelf, PyObject* poArgs);
PyObject * playerSetQuickCameraMode(PyObject* poSelf, PyObject* poArgs);
PyObject* playerSetAutoCameraRotationSpeed(PyObject* poSelf, PyObject* poArgs);
PyObject * playerResetCameraRotation(PyObject* poSelf, PyObject* poArgs);
PyObject * playerSetSingleDIKKeyState(PyObject* poSelf, PyObject* poArgs);
PyObject * playerEndKeyWalkingImmediately(PyObject* poSelf, PyObject* poArgs);
PyObject * playerStartMouseWalking(PyObject* poSelf, PyObject* poArgs);
PyObject * playerEndMouseWalking(PyObject* poSelf, PyObject* poArgs);
PyObject* playerSetMouseState(PyObject* poSelf, PyObject* poArgs);
PyObject* playerSetMouseFunc(PyObject* poSelf, PyObject* poArgs);
PyObject* playerGetMouseFunc(PyObject* poSelf, PyObject* poArgs);
PyObject* playerSetMouseMiddleButtonState(PyObject* poSelf, PyObject* poArgs);
PyObject * playerSetGameWindow(PyObject* poSelf, PyObject* poArgs);
PyObject * playerShowPlayer(PyObject* poSelf, PyObject* poArgs);
PyObject * playerHidePlayer(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsObserverMode(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsMountingHorse(PyObject* poSelf, PyObject* poArgs);
PyObject * playerSetMainCharacterIndex(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetMainCharacterIndex(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetMainCharacterName(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsMainCharacterIndex(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetName(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetRace(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetJob(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetPlayTime(PyObject* poSelf, PyObject* poArgs);
PyObject * playerSetPlayTime(PyObject* poSelf, PyObject* poArgs);
PyObject * playerIsActingEmotion(PyObject* poSelf, PyObject* poArgs);
PyObject * playerActEmotion(PyObject* poSelf, PyObject* poArgs);
PyObject * playerRegisterEmotionIcon(PyObject* poSelf, PyObject* poArgs);
PyObject * playerGetEmotionIconImage(PyObject* poSelf, PyObject* poArgs);
PyObject * playerRegisterEffect(PyObject* poSelf, PyObject* poArgs);
PyObject * playerRegisterCacheEffect(PyObject* poSelf, PyObject* poArgs);

// ============================================================================
// Core Lifecycle (src/UserInterface/PythonPlayerModule.cpp)
// ============================================================================
PyObject * playerUpdate(PyObject* poSelf, PyObject* poArgs);
PyObject * playerRender(PyObject* poSelf, PyObject* poArgs);
PyObject * playerClear(PyObject* poSelf, PyObject* poArgs);

