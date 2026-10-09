#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#define TEST_MODE_DISABLE_STDAFX 1

#include "../src/UserInterface/Contracts/IActorProvider.h"
#include "../src/UserInterface/Contracts/INetworkService.h"
#include "../src/UserInterface/PlayerControllers/PlayerMovementController.h"
#include "../src/UserInterface/PlayerControllers/PlayerCombatController.h"
#include "../src/UserInterface/PlayerControllers/PlayerTargetController.h"
#include "../src/UserInterface/PlayerControllers/PlayerSkillExecutor.h"
#include "../src/UserInterface/PlayerControllers/PlayerItemController.h"
#include "../src/UserInterface/PlayerControllers/PlayerPKController.h"

using namespace UserInterface::Contracts;
using namespace UserInterface::PlayerControllers;

// Mock dla INetworkService
class MockNetworkService : public INetworkService
{
public:
    int movePacketsSent = 0;
    int attackPacketsSent = 0;
    int targetPacketsSent = 0;
    int skillPacketsSent = 0;
    int itemUsePacketsSent = 0;
    int itemPickupPacketsSent = 0;
    DWORD lastTargetVID = 0;

    bool SendCharacterMovePacket(LONG lX, LONG lY, float fRot, DWORD dwTime, BYTE bFunc, BYTE bArg) override
    {
        ++movePacketsSent;
        return true;
    }

    bool SendSyncPositionPacket(DWORD dwVID, LONG lX, LONG lY) override
    {
        return true;
    }

    bool SendAttackPacket(DWORD dwVictimVID, BYTE byType) override
    {
        ++attackPacketsSent;
        return true;
    }

    bool SendTargetPacket(DWORD dwVID) override
    {
        ++targetPacketsSent;
        lastTargetVID = dwVID;
        return true;
    }

    bool SendOnClickPacket(DWORD dwVID) override
    {
        return true;
    }

    bool SendUseSkillPacket(DWORD dwSkillIndex, DWORD dwTargetVID) override
    {
        ++skillPacketsSent;
        return true;
    }

    bool SendItemUsePacket(DWORD dwCell) override
    {
        ++itemUsePacketsSent;
        return true;
    }

    bool SendItemPickUpPacket(DWORD dwIID) override
    {
        ++itemPickupPacketsSent;
        return true;
    }

    bool SendItemDropPacket(DWORD dwCell, DWORD dwCount) override
    {
        return true;
    }

    bool SendItemMovePacket(DWORD dwSourceCell, DWORD dwTargetCell, DWORD dwCount) override
    {
        return true;
    }

    bool SendSpecial(const void* pData, int iSize) override
    {
        return true;
    }
};

// Mock dla IActorProvider
class MockActorProvider : public IActorProvider
{
public:
    CInstanceBase* GetInstance(uint32_t dwVID) const override { return nullptr; }
    CInstanceBase* GetMainActor() const override { return nullptr; }
    CInstanceBase* GetPickedActor() const override { return nullptr; }
    bool IsActorAlive(uint32_t dwVID) const override { return dwVID > 0; }
    bool GetActorPosition(uint32_t dwVID, TPixelPosition* pOutPos) const override
    {
        if (pOutPos) { pOutPos->x = 1000.0f; pOutPos->y = 2000.0f; pOutPos->z = 0.0f; }
        return true;
    }
    float GetDistance(uint32_t dwVID1, uint32_t dwVID2) const override { return 150.0f; }
    bool IsSamePartyMember(uint32_t dwVID1, uint32_t dwVID2) const override { return false; }
};

void TestMovementController()
{
    std::cout << "[TEST] PlayerMovementController...\n";
    MockNetworkService net;
    MockActorProvider actors;

    PlayerMovementController moveCtrl(&actors, &net);

    // Test obslugi katow kierunkowych
    float degUp = moveCtrl.GetDegreeFromDirection(PlayerMovementController::KEYBOARD_UD_UP, PlayerMovementController::KEYBOARD_LR_NONE);
    assert(degUp == 0.0f);

    float degUpRight = moveCtrl.GetDegreeFromDirection(PlayerMovementController::KEYBOARD_UD_UP, PlayerMovementController::KEYBOARD_LR_RIGHT);
    assert(degUpRight == -45.0f);

    float degDownLeft = moveCtrl.GetDegreeFromDirection(PlayerMovementController::KEYBOARD_UD_DOWN, PlayerMovementController::KEYBOARD_LR_LEFT);
    assert(degDownLeft == 135.0f);

    // Test zarzadzania stamina
    moveCtrl.StartStaminaConsume(10, 100);
    assert(moveCtrl.IsConsumingStamina() == true);
    assert(moveCtrl.GetCurrentStamina() == 100.0f);

    moveCtrl.Update(1.0f); // 1 sekunda
    assert(moveCtrl.GetCurrentStamina() == 90.0f);

    moveCtrl.StopStaminaConsume(90);
    assert(moveCtrl.IsConsumingStamina() == false);

    // Test pozycji docelowej
    moveCtrl.SetDungeonDestinationPosition(500, 600);
    assert(moveCtrl.IsDestPosition() == true);
    assert(moveCtrl.GetDestPosX() == 500);
    assert(moveCtrl.GetDestPosY() == 600);
    moveCtrl.ClearDestPosition();
    assert(moveCtrl.IsDestPosition() == false);

    std::cout << "[PASS] PlayerMovementController zakonczony sukcesem.\n";
}

void TestCombatController()
{
    std::cout << "[TEST] PlayerCombatController...\n";
    MockNetworkService net;
    MockActorProvider actors;

    PlayerCombatController combatCtrl(&actors, &net);

    combatCtrl.SetWeaponPower(50, 100, 30, 70, 10);
    combatCtrl.SetRace(0); // Wojownik (WARRIOR_M)
    combatCtrl.SetPlayerStats(50, 90, 40, 10, 60);

    DWORD raceStat = combatCtrl.GetRaceStat();
    assert(raceStat == 90); // ST dla wojownika

    DWORD weaponAtk = combatCtrl.GetWeaponAtk(100);
    assert(weaponAtk > 0);

    DWORD totalAtk = combatCtrl.GetTotalAtk(100, 20);
    assert(totalAtk > weaponAtk);

    combatCtrl.SetAutoAttackTargetActorID(1234);
    assert(combatCtrl.GetAutoAttackTargetVID() == 1234);
    combatCtrl.ClearAutoAttackTargetActorID();
    assert(combatCtrl.GetAutoAttackTargetVID() == 0);

    std::cout << "[PASS] PlayerCombatController zakonczony sukcesem.\n";
}

void TestTargetController()
{
    std::cout << "[TEST] PlayerTargetController...\n";
    MockNetworkService net;
    MockActorProvider actors;

    PlayerTargetController targetCtrl(&actors, &net);

    assert(targetCtrl.IsTarget() == false);
    targetCtrl.SetTarget(5555, TRUE);
    assert(targetCtrl.IsTarget() == true);
    assert(targetCtrl.GetTargetVID() == 5555);
    assert(net.targetPacketsSent == 1);
    assert(net.lastTargetVID == 5555);

    // Czyszczenie celu
    targetCtrl.ClearTarget();
    assert(targetCtrl.IsTarget() == false);
    assert(targetCtrl.GetTargetVID() == 0);
    assert(net.targetPacketsSent == 2);
    assert(net.lastTargetVID == 0);

    std::cout << "[PASS] PlayerTargetController zakonczony sukcesem.\n";
}

void TestPKController()
{
    std::cout << "[TEST] PlayerPKController...\n";
    MockNetworkService net;
    MockActorProvider actors;

    PlayerPKController pkCtrl(&actors, &net);

    DWORD targetVID = 777;

    assert(pkCtrl.IsChallengeInstance(targetVID) == false);
    assert(pkCtrl.IsRevengeInstance(targetVID) == false);
    assert(pkCtrl.IsCantFightInstance(targetVID) == false);

    // Rejestracja wyzwania
    pkCtrl.RememberChallengeInstance(targetVID);
    assert(pkCtrl.IsChallengeInstance(targetVID) == true);
    assert(pkCtrl.IsRevengeInstance(targetVID) == false);

    // Przelaczenie na zemste
    pkCtrl.RememberRevengeInstance(targetVID);
    assert(pkCtrl.IsChallengeInstance(targetVID) == false);
    assert(pkCtrl.IsRevengeInstance(targetVID) == true);

    // Niemoznosc walki
    pkCtrl.RememberCantFightInstance(targetVID);
    assert(pkCtrl.IsCantFightInstance(targetVID) == true);

    // Zapomnienie instancji
    pkCtrl.ForgetInstance(targetVID);
    assert(pkCtrl.IsChallengeInstance(targetVID) == false);
    assert(pkCtrl.IsRevengeInstance(targetVID) == false);
    assert(pkCtrl.IsCantFightInstance(targetVID) == false);

    std::cout << "[PASS] PlayerPKController zakonczony sukcesem.\n";
}

void TestItemController()
{
    std::cout << "[TEST] PlayerItemController...\n";
    MockNetworkService net;
    MockActorProvider actors;

    PlayerItemController itemCtrl(&actors, &net);

    // Konfiguracja auto mikstury HP
    PlayerItemController::SAutoPotionInfo hpPotion{};
    hpPotion.bActivated = true;
    hpPotion.totalAmount = 10000;
    hpPotion.currentAmount = 5000;
    hpPotion.inventorySlotIndex = 2;

    itemCtrl.SetAutoPotionInfo(PlayerItemController::AUTO_POTION_TYPE_HP, hpPotion);
    const auto& retrieved = itemCtrl.GetAutoPotionInfo(PlayerItemController::AUTO_POTION_TYPE_HP);
    assert(retrieved.bActivated == true);
    assert(retrieved.currentAmount == 5000);
    assert(retrieved.inventorySlotIndex == 2);

    std::cout << "[PASS] PlayerItemController zakonczony sukcesem.\n";
}

void TestSkillExecutor()
{
    std::cout << "[TEST] PlayerSkillExecutor...\n";
    MockNetworkService net;
    MockActorProvider actors;

    PlayerSkillExecutor skillExec(&actors, &net);

    // Rejestracja slotu umiejetnosci
    skillExec.SetSkillSlot(1, 101, 1, 20); // Aura Miecza na slocie 1
    const auto* pSlot = skillExec.GetSkillSlotData(1);
    assert(pSlot != nullptr);
    assert(pSlot->dwIndex == 101);
    assert(pSlot->iLevel == 20);
    assert(pSlot->isCoolTime == FALSE);

    // Odpalenie cooldownu
    skillExec.RunCoolTime(1, 60.0f);
    assert(skillExec.CheckRestSkillCoolTime(1) == false);

    // Rezerwacja umiejetnosci
    skillExec.ReserveUseSkill(0, 1, 400);
    assert(skillExec.GetReservedSkillSlotIndex() == 1);
    assert(skillExec.GetReservedSkillRange() == 400);
    skillExec.ClearReservedSkill();
    assert(skillExec.GetReservedSkillSlotIndex() == 0);

    std::cout << "[PASS] PlayerSkillExecutor zakonczony sukcesem.\n";
}

int main()
{
    std::cout << "========================================================\n";
    std::cout << "URUCHOMIENIE TESTOW: Player Controllers Suite (C++23)\n";
    std::cout << "========================================================\n";

    TestMovementController();
    TestCombatController();
    TestTargetController();
    TestPKController();
    TestItemController();
    TestSkillExecutor();

    std::cout << "\n>>> WSZYSTKIE TESTY KONTROLEROW GRACZA: 100% PASS <<<\n";
    return 0;
}
