#include "../src/Client/Gameplay/CombatCommandHandler.h"
#include <iostream>
#include <cassert>
#include <unordered_map>

using namespace Client;

// ============================================================================
// Mock interfejsu zapytan o encje
// ============================================================================
class MockCombatEntityQuery : public Gameplay::ICombatEntityQuery {
public:
    struct EntityData {
        bool isValid = false;
        bool isDead = false;
    };

    void SetupEntity(Core::EntityVid vid, bool isValid, bool isDead) {
        m_entities[vid] = EntityData{isValid, isDead};
    }

    [[nodiscard]] bool IsValidEntity(Core::EntityVid vid) const noexcept override {
        auto it = m_entities.find(vid);
        if (it != m_entities.end()) {
            return it->second.isValid;
        }
        return false;
    }

    [[nodiscard]] bool IsEntityDead(Core::EntityVid vid) const noexcept override {
        auto it = m_entities.find(vid);
        if (it != m_entities.end()) {
            return it->second.isDead;
        }
        return true; 
    }

private:
    std::unordered_map<Core::EntityVid, EntityData> m_entities;
};

// ============================================================================
// Przypadki Testowe
// ============================================================================

void Test_HandleAttack_PlayerIsDead() {
    MockCombatEntityQuery query;
    Gameplay::CombatCommandHandler handler(query);

    Core::WorldContext ctx;
    ctx.localPlayerVid = Core::EntityVid{100};
    ctx.isDead = true; 

    Core::EntityVid targetVid{200};
    query.SetupEntity(targetVid, true, false); 

    Core::AttackCommand cmd;
    cmd.targetVid = targetVid;
    cmd.attackType = 0;

    auto result = handler.HandleAttack(cmd, ctx);
    
    assert(!result.has_value());
    assert(result.error() == Core::CommandError::MovementBlocked);
    std::cout << "[OK] Test_HandleAttack_PlayerIsDead\n";
}

void Test_HandleAttack_TargetIdIsZero() {
    MockCombatEntityQuery query;
    Gameplay::CombatCommandHandler handler(query);

    Core::WorldContext ctx;
    ctx.localPlayerVid = Core::EntityVid{100};
    ctx.isDead = false;

    Core::AttackCommand cmd;
    cmd.targetVid = Core::EntityVid{0}; 
    cmd.attackType = 0;

    auto result = handler.HandleAttack(cmd, ctx);
    
    assert(!result.has_value());
    assert(result.error() == Core::CommandError::InvalidTarget);
    std::cout << "[OK] Test_HandleAttack_TargetIdIsZero\n";
}

void Test_HandleAttack_TargetIsSelf() {
    MockCombatEntityQuery query;
    Gameplay::CombatCommandHandler handler(query);

    Core::WorldContext ctx;
    ctx.localPlayerVid = Core::EntityVid{100};
    ctx.isDead = false;

    query.SetupEntity(Core::EntityVid{100}, true, false);

    Core::AttackCommand cmd;
    cmd.targetVid = Core::EntityVid{100}; 
    cmd.attackType = 0;

    auto result = handler.HandleAttack(cmd, ctx);
    
    assert(!result.has_value());
    assert(result.error() == Core::CommandError::InvalidTarget);
    std::cout << "[OK] Test_HandleAttack_TargetIsSelf\n";
}

void Test_HandleAttack_TargetDoesNotExist() {
    MockCombatEntityQuery query;
    Gameplay::CombatCommandHandler handler(query);

    Core::WorldContext ctx;
    ctx.localPlayerVid = Core::EntityVid{100};
    ctx.isDead = false;

    // Encja 999 nie zostala dodana do moka

    Core::AttackCommand cmd;
    cmd.targetVid = Core::EntityVid{999}; 
    cmd.attackType = 0;

    auto result = handler.HandleAttack(cmd, ctx);
    
    assert(!result.has_value());
    assert(result.error() == Core::CommandError::InvalidTarget);
    std::cout << "[OK] Test_HandleAttack_TargetDoesNotExist\n";
}

void Test_HandleAttack_TargetIsDead() {
    MockCombatEntityQuery query;
    Gameplay::CombatCommandHandler handler(query);

    Core::WorldContext ctx;
    ctx.localPlayerVid = Core::EntityVid{100};
    ctx.isDead = false;

    Core::EntityVid targetVid{200};
    query.SetupEntity(targetVid, true, true); // Istnieje, ale jest martwy

    Core::AttackCommand cmd;
    cmd.targetVid = targetVid;
    cmd.attackType = 0;

    auto result = handler.HandleAttack(cmd, ctx);
    
    assert(!result.has_value());
    assert(result.error() == Core::CommandError::InvalidTarget);
    std::cout << "[OK] Test_HandleAttack_TargetIsDead\n";
}

void Test_HandleAttack_ValidCommand() {
    MockCombatEntityQuery query;
    Gameplay::CombatCommandHandler handler(query);

    Core::WorldContext ctx;
    ctx.localPlayerVid = Core::EntityVid{100};
    ctx.isDead = false; // Gracz zyje

    Core::EntityVid targetVid{200};
    query.SetupEntity(targetVid, true, false); // Cel istnieje i zyje

    Core::AttackCommand cmd;
    cmd.targetVid = targetVid;
    cmd.attackType = 1;

    auto result = handler.HandleAttack(cmd, ctx);
    
    assert(result.has_value());
    std::cout << "[OK] Test_HandleAttack_ValidCommand\n";
}

int main() {
    std::cout << "Running CombatCommandHandler tests...\n";
    
    Test_HandleAttack_PlayerIsDead();
    Test_HandleAttack_TargetIdIsZero();
    Test_HandleAttack_TargetIsSelf();
    Test_HandleAttack_TargetDoesNotExist();
    Test_HandleAttack_TargetIsDead();
    Test_HandleAttack_ValidCommand();
    
    std::cout << "All CombatCommandHandler tests passed successfully!\n";
    return 0;
}
