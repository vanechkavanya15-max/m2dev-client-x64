#include <gtest/gtest.h>
#include "../src/Client/Gameplay/CombatTargetValidator.h"

using namespace Client::Gameplay;
using namespace Client::Core;

class CombatTargetValidatorTest : public ::testing::Test {
protected:
    CombatEntityState attacker;
    CombatEntityState target;

    void SetUp() override {
        // Default attacker (Player, Lv 30, Empire 1)
        attacker.vid = EntityVid{100};
        attacker.type = EntityType::Player;
        attacker.level = 30;
        attacker.empire = 1;
        attacker.isInvincible = false;
        attacker.inSafeZone = false;
        attacker.isDuelActive = false;
        attacker.isPvPModeActive = false;

        // Default target (Monster)
        target.vid = EntityVid{200};
        target.type = EntityType::Monster;
        target.level = 30;
        target.empire = 0;
        target.isInvincible = false;
        target.inSafeZone = false;
        target.isDuelActive = false;
        target.isPvPModeActive = false;
    }
};

TEST_F(CombatTargetValidatorTest, CannotAttackSelf) {
    target.vid = attacker.vid; // Same VID
    auto result = CombatTargetValidator::ValidateTarget(attacker, target);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CommandError::InvalidTarget);
}

TEST_F(CombatTargetValidatorTest, CannotAttackInSafeZone) {
    attacker.inSafeZone = true;
    auto result1 = CombatTargetValidator::ValidateTarget(attacker, target);
    EXPECT_FALSE(result1.has_value());
    EXPECT_EQ(result1.error(), CommandError::InvalidTarget);

    attacker.inSafeZone = false;
    target.inSafeZone = true;
    auto result2 = CombatTargetValidator::ValidateTarget(attacker, target);
    EXPECT_FALSE(result2.has_value());
    EXPECT_EQ(result2.error(), CommandError::InvalidTarget);
}

TEST_F(CombatTargetValidatorTest, CannotAttackInvincibleTarget) {
    target.isInvincible = true;
    auto result = CombatTargetValidator::ValidateTarget(attacker, target);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CommandError::InvalidTarget);
}

TEST_F(CombatTargetValidatorTest, CannotAttackNPC) {
    target.type = EntityType::NPC;
    auto result = CombatTargetValidator::ValidateTarget(attacker, target);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CommandError::InvalidTarget);
}

TEST_F(CombatTargetValidatorTest, PlayerVsPlayerUnderLevel15) {
    target.type = EntityType::Player;
    target.empire = 2; // Different empire

    // Attacker under 15
    attacker.level = 14;
    auto result1 = CombatTargetValidator::ValidateTarget(attacker, target);
    EXPECT_FALSE(result1.has_value());
    EXPECT_EQ(result1.error(), CommandError::InvalidTarget);

    // Target under 15
    attacker.level = 30;
    target.level = 14;
    auto result2 = CombatTargetValidator::ValidateTarget(attacker, target);
    EXPECT_FALSE(result2.has_value());
    EXPECT_EQ(result2.error(), CommandError::InvalidTarget);
}

TEST_F(CombatTargetValidatorTest, PlayerVsPlayerSameEmpireRequiresPvPMode) {
    target.type = EntityType::Player;
    target.empire = 1; // Same empire

    // Without PvP mode
    auto result1 = CombatTargetValidator::ValidateTarget(attacker, target);
    EXPECT_FALSE(result1.has_value());
    EXPECT_EQ(result1.error(), CommandError::InvalidTarget);

    // With PvP mode
    attacker.isPvPModeActive = true;
    auto result2 = CombatTargetValidator::ValidateTarget(attacker, target);
    EXPECT_TRUE(result2.has_value());
    EXPECT_TRUE(result2.value());
}

TEST_F(CombatTargetValidatorTest, PlayerVsPlayerDifferentEmpireAllowed) {
    target.type = EntityType::Player;
    target.empire = 2; // Different empire

    auto result = CombatTargetValidator::ValidateTarget(attacker, target);
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result.value());
}

TEST_F(CombatTargetValidatorTest, PlayerVsPlayerDuelOverridesEmpire) {
    target.type = EntityType::Player;
    target.empire = 1; // Same empire
    
    // Both active duel
    attacker.isDuelActive = true;
    target.isDuelActive = true;

    auto result = CombatTargetValidator::ValidateTarget(attacker, target);
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result.value());
}

TEST_F(CombatTargetValidatorTest, ValidMonsterAttack) {
    // Default setup is valid monster attack
    auto result = CombatTargetValidator::ValidateTarget(attacker, target);
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result.value());
}
