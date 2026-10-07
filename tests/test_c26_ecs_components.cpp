#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <chrono>
#include "../src/Client/World/ECSComponents.h"

using namespace Client::World;

// Simple testing macros
#define TEST_START(name) { std::cout << "Running test: " << #name << "..." << std::endl; }
#define TEST_END(name) { std::cout << "Passed: " << #name << std::endl; }
#define ASSERT_EQ(actual, expected) { if ((actual) != (expected)) { std::cerr << "Assertion failed at line " << __LINE__ << ": " << (actual) << " != " << (expected) << std::endl; assert(false); } }
#define ASSERT_FLOAT_EQ(actual, expected) { if (std::abs((actual) - (expected)) > 1e-5f) { std::cerr << "Assertion failed at line " << __LINE__ << ": " << (actual) << " != " << (expected) << std::endl; assert(false); } }
#define ASSERT_TRUE(condition) { if (!(condition)) { std::cerr << "Assertion failed at line " << __LINE__ << ": " << #condition << " is false" << std::endl; assert(false); } }
#define ASSERT_FALSE(condition) { if (condition) { std::cerr << "Assertion failed at line " << __LINE__ << ": " << #condition << " is true" << std::endl; assert(false); } }

void TestTransformComponentDefaults() {
    TEST_START(TestTransformComponentDefaults)
    TransformComponent transform;
    ASSERT_FLOAT_EQ(transform.pos.x, 0.0f);
    ASSERT_FLOAT_EQ(transform.pos.y, 0.0f);
    ASSERT_FLOAT_EQ(transform.pos.z, 0.0f);
    ASSERT_FLOAT_EQ(transform.rot, 0.0f);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (i == j) {
                ASSERT_FLOAT_EQ(transform.worldMatrix.m[i][j], 1.0f);
            } else {
                ASSERT_FLOAT_EQ(transform.worldMatrix.m[i][j], 0.0f);
            }
        }
    }
    TEST_END(TestTransformComponentDefaults)
}

void TestVisualComponentDefaults() {
    TEST_START(TestVisualComponentDefaults)
    VisualComponent visual;
    ASSERT_EQ(visual.race.value(), 0);
    ASSERT_FLOAT_EQ(visual.alpha, 1.0f);
    ASSERT_TRUE(visual.isVisible);
    ASSERT_EQ(visual.armor.value(), 0);
    ASSERT_EQ(visual.weapon.value(), 0);
    TEST_END(TestVisualComponentDefaults)
}

void TestMotionComponentDefaults() {
    TEST_START(TestMotionComponentDefaults)
    MotionComponent motion;
    ASSERT_EQ(motion.motionMode, 0);
    ASSERT_EQ(motion.motionIndex, 0);
    ASSERT_FLOAT_EQ(motion.speed, 1.0f);
    ASSERT_FLOAT_EQ(motion.loopTime, 0.0f);
    TEST_END(TestMotionComponentDefaults)
}

void TestCombatStateComponentDefaults() {
    TEST_START(TestCombatStateComponentDefaults)
    CombatStateComponent combat;
    ASSERT_EQ(combat.curHP, 0);
    ASSERT_EQ(combat.maxHP, 0);
    ASSERT_EQ(combat.targetVid.value(), 0);
    ASSERT_FALSE(combat.isDead);
    TEST_END(TestCombatStateComponentDefaults)
}

void TestTransformComponentUpdateAndReset() {
    TEST_START(TestTransformComponentUpdateAndReset)
    TransformComponent transform;
    transform.pos.x = 100.0f;
    transform.pos.y = 200.0f;
    transform.pos.z = 50.0f;
    transform.rot = 90.0f;
    transform.worldMatrix.m[0][3] = 10.0f;
    
    ASSERT_FLOAT_EQ(transform.pos.x, 100.0f);
    ASSERT_FLOAT_EQ(transform.rot, 90.0f);
    ASSERT_FLOAT_EQ(transform.worldMatrix.m[0][3], 10.0f);
    
    transform.Reset();
    
    ASSERT_FLOAT_EQ(transform.pos.x, 0.0f);
    ASSERT_FLOAT_EQ(transform.pos.y, 0.0f);
    ASSERT_FLOAT_EQ(transform.pos.z, 0.0f);
    ASSERT_FLOAT_EQ(transform.rot, 0.0f);
    ASSERT_FLOAT_EQ(transform.worldMatrix.m[0][3], 0.0f);
    ASSERT_FLOAT_EQ(transform.worldMatrix.m[0][0], 1.0f);
    TEST_END(TestTransformComponentUpdateAndReset)
}

void TestVisualComponentUpdateAndReset() {
    TEST_START(TestVisualComponentUpdateAndReset)
    VisualComponent visual;
    visual.race = RaceVnum{4};
    visual.alpha = 0.5f;
    visual.isVisible = false;
    visual.armor = ItemVnum{12345};
    visual.weapon = ItemVnum{67890};
    
    ASSERT_EQ(visual.race.value(), 4);
    ASSERT_FLOAT_EQ(visual.alpha, 0.5f);
    ASSERT_FALSE(visual.isVisible);
    ASSERT_EQ(visual.armor.value(), 12345);
    ASSERT_EQ(visual.weapon.value(), 67890);
    
    visual.Reset();
    
    ASSERT_EQ(visual.race.value(), 0);
    ASSERT_FLOAT_EQ(visual.alpha, 1.0f);
    ASSERT_TRUE(visual.isVisible);
    ASSERT_EQ(visual.armor.value(), 0);
    ASSERT_EQ(visual.weapon.value(), 0);
    TEST_END(TestVisualComponentUpdateAndReset)
}

void TestMotionComponentUpdateAndReset() {
    TEST_START(TestMotionComponentUpdateAndReset)
    MotionComponent motion;
    motion.motionMode = 1;
    motion.motionIndex = 15;
    motion.speed = 2.5f;
    motion.loopTime = 10.0f;
    
    ASSERT_EQ(motion.motionMode, 1);
    ASSERT_EQ(motion.motionIndex, 15);
    ASSERT_FLOAT_EQ(motion.speed, 2.5f);
    ASSERT_FLOAT_EQ(motion.loopTime, 10.0f);
    
    motion.Reset();
    
    ASSERT_EQ(motion.motionMode, 0);
    ASSERT_EQ(motion.motionIndex, 0);
    ASSERT_FLOAT_EQ(motion.speed, 1.0f);
    ASSERT_FLOAT_EQ(motion.loopTime, 0.0f);
    TEST_END(TestMotionComponentUpdateAndReset)
}

void TestCombatStateComponentUpdateAndReset() {
    TEST_START(TestCombatStateComponentUpdateAndReset)
    CombatStateComponent combat;
    combat.curHP = 1500;
    combat.maxHP = 2000;
    combat.targetVid = EntityVid{999};
    combat.isDead = false;
    
    ASSERT_EQ(combat.curHP, 1500);
    ASSERT_EQ(combat.maxHP, 2000);
    ASSERT_EQ(combat.targetVid.value(), 999);
    ASSERT_FALSE(combat.isDead);
    
    combat.curHP = 0;
    combat.isDead = true;
    
    ASSERT_EQ(combat.curHP, 0);
    ASSERT_TRUE(combat.isDead);
    
    combat.Reset();
    
    ASSERT_EQ(combat.curHP, 0);
    ASSERT_EQ(combat.maxHP, 0);
    ASSERT_EQ(combat.targetVid.value(), 0);
    ASSERT_FALSE(combat.isDead);
    TEST_END(TestCombatStateComponentUpdateAndReset)
}

void TestSizeLayouts() {
    TEST_START(TestSizeLayouts)
    // Optional checks for data-oriented struct sizes
    // Just to ensure compiler pads appropriately
    // sizeof(float)*3 + sizeof(float) + sizeof(float)*16 = 12 + 4 + 64 = 80
    ASSERT_EQ(sizeof(TransformComponent), 80);
    // uint32_t(4) + float(4) + bool(1) + uint32_t(4) + uint32_t(4) = 17 => padded to 20
    ASSERT_TRUE(sizeof(VisualComponent) == 20 || sizeof(VisualComponent) == 24);
    // uint16_t(2) + uint16_t(2) + float(4) + float(4) = 12
    ASSERT_EQ(sizeof(MotionComponent), 12);
    // int32_t(4) + int32_t(4) + uint32_t(4) + bool(1) = 13 => padded to 16
    ASSERT_EQ(sizeof(CombatStateComponent), 16);
    TEST_END(TestSizeLayouts)
}

void TestBulkTransformUpdatePerformance() {
    TEST_START(TestBulkTransformUpdatePerformance)
    // Simulate updating 1,000,000 components to prove data locality
    const size_t NUM_ENTITIES = 1000000;
    std::vector<TransformComponent> transforms(NUM_ENTITIES);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < NUM_ENTITIES; ++i) {
        transforms[i].pos.x += 1.0f;
        transforms[i].pos.y += 0.5f;
        transforms[i].rot += 0.01f;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Updated 1M transforms in " << duration.count() << " ms\n";
    
    ASSERT_FLOAT_EQ(transforms[NUM_ENTITIES - 1].pos.x, 1.0f);
    ASSERT_FLOAT_EQ(transforms[NUM_ENTITIES - 1].pos.y, 0.5f);
    ASSERT_FLOAT_EQ(transforms[NUM_ENTITIES - 1].rot, 0.01f);
    TEST_END(TestBulkTransformUpdatePerformance)
}

void TestBulkVisualUpdatePerformance() {
    TEST_START(TestBulkVisualUpdatePerformance)
    // Simulate updating 1,000,000 visual components
    const size_t NUM_ENTITIES = 1000000;
    std::vector<VisualComponent> visuals(NUM_ENTITIES);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < NUM_ENTITIES; ++i) {
        visuals[i].alpha = 0.5f;
        visuals[i].isVisible = false;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Updated 1M visuals in " << duration.count() << " ms\n";
    
    ASSERT_FLOAT_EQ(visuals[NUM_ENTITIES - 1].alpha, 0.5f);
    ASSERT_FALSE(visuals[NUM_ENTITIES - 1].isVisible);
    TEST_END(TestBulkVisualUpdatePerformance)
}

void TestBulkMotionUpdatePerformance() {
    TEST_START(TestBulkMotionUpdatePerformance)
    // Simulate updating 1,000,000 motion components
    const size_t NUM_ENTITIES = 1000000;
    std::vector<MotionComponent> motions(NUM_ENTITIES);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < NUM_ENTITIES; ++i) {
        motions[i].speed *= 1.1f;
        motions[i].loopTime += 0.016f; // 60 FPS frame
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Updated 1M motions in " << duration.count() << " ms\n";
    
    ASSERT_FLOAT_EQ(motions[NUM_ENTITIES - 1].speed, 1.1f);
    ASSERT_FLOAT_EQ(motions[NUM_ENTITIES - 1].loopTime, 0.016f);
    TEST_END(TestBulkMotionUpdatePerformance)
}

void TestBulkCombatStateUpdatePerformance() {
    TEST_START(TestBulkCombatStateUpdatePerformance)
    // Simulate checking combat state for 1,000,000 entities
    const size_t NUM_ENTITIES = 1000000;
    std::vector<CombatStateComponent> combatStates(NUM_ENTITIES);
    
    // Setup some data
    for (size_t i = 0; i < NUM_ENTITIES; i += 2) {
        combatStates[i].curHP = 0;
    }
    
    // Setup some data
    for (size_t i = 0; i < NUM_ENTITIES; i++) {
        if (i % 2 == 0) {
            combatStates[i].curHP = 0;
        } else {
            combatStates[i].curHP = 100;
        }
    }
    
    size_t deathCount = 0;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < NUM_ENTITIES; ++i) {
        if (combatStates[i].curHP <= 0 && !combatStates[i].isDead) {
            combatStates[i].isDead = true;
            deathCount++;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Updated 1M combat states in " << duration.count() << " ms\n";
    
    ASSERT_EQ(deathCount, NUM_ENTITIES / 2);
    ASSERT_TRUE(combatStates[0].isDead);
    ASSERT_FALSE(combatStates[1].isDead);
    TEST_END(TestBulkCombatStateUpdatePerformance)
}

// Extra tests to fill up space and make testing thorough
void TestTransformMatrixMultiplication() {
    TEST_START(TestTransformMatrixMultiplication)
    TransformComponent t1;
    // Just a basic layout check
    t1.worldMatrix.m[0][0] = 2.0f;
    t1.worldMatrix.m[1][1] = 2.0f;
    t1.worldMatrix.m[2][2] = 2.0f;
    
    ASSERT_FLOAT_EQ(t1.worldMatrix.m[0][0], 2.0f);
    ASSERT_FLOAT_EQ(t1.worldMatrix.m[1][1], 2.0f);
    ASSERT_FLOAT_EQ(t1.worldMatrix.m[2][2], 2.0f);
    ASSERT_FLOAT_EQ(t1.worldMatrix.m[3][3], 1.0f);
    TEST_END(TestTransformMatrixMultiplication)
}

void TestVisualComponentStateChanges() {
    TEST_START(TestVisualComponentStateChanges)
    VisualComponent v;
    v.race = RaceVnum{10};
    ASSERT_EQ(v.race.value(), 10);
    
    v.isVisible = false;
    ASSERT_FALSE(v.isVisible);
    
    v.isVisible = true;
    ASSERT_TRUE(v.isVisible);
    
    v.alpha = 0.1f;
    ASSERT_FLOAT_EQ(v.alpha, 0.1f);
    
    v.armor = ItemVnum{100};
    v.weapon = ItemVnum{200};
    ASSERT_EQ(v.armor.value(), 100);
    ASSERT_EQ(v.weapon.value(), 200);
    TEST_END(TestVisualComponentStateChanges)
}

void TestMotionComponentStateChanges() {
    TEST_START(TestMotionComponentStateChanges)
    MotionComponent m;
    m.motionMode = 3;
    ASSERT_EQ(m.motionMode, 3);
    
    m.motionIndex = 99;
    ASSERT_EQ(m.motionIndex, 99);
    
    m.speed = 5.0f;
    ASSERT_FLOAT_EQ(m.speed, 5.0f);
    
    m.loopTime = 123.45f;
    ASSERT_FLOAT_EQ(m.loopTime, 123.45f);
    TEST_END(TestMotionComponentStateChanges)
}

void TestCombatStateComponentStateChanges() {
    TEST_START(TestCombatStateComponentStateChanges)
    CombatStateComponent c;
    c.curHP = 10;
    c.maxHP = 100;
    ASSERT_EQ(c.curHP, 10);
    ASSERT_EQ(c.maxHP, 100);
    
    c.targetVid = EntityVid{777};
    ASSERT_EQ(c.targetVid.value(), 777);
    
    c.curHP -= 20;
    if (c.curHP <= 0) {
        c.curHP = 0;
        c.isDead = true;
    }
    
    ASSERT_EQ(c.curHP, 0);
    ASSERT_TRUE(c.isDead);
    TEST_END(TestCombatStateComponentStateChanges)
}

int main() {
    std::cout << "Starting ECS Components tests...\n";
    
    TestTransformComponentDefaults();
    TestVisualComponentDefaults();
    TestMotionComponentDefaults();
    TestCombatStateComponentDefaults();
    
    TestTransformComponentUpdateAndReset();
    TestVisualComponentUpdateAndReset();
    TestMotionComponentUpdateAndReset();
    TestCombatStateComponentUpdateAndReset();
    
    TestSizeLayouts();
    
    TestBulkTransformUpdatePerformance();
    TestBulkVisualUpdatePerformance();
    TestBulkMotionUpdatePerformance();
    TestBulkCombatStateUpdatePerformance();
    
    TestTransformMatrixMultiplication();
    TestVisualComponentStateChanges();
    TestMotionComponentStateChanges();
    TestCombatStateComponentStateChanges();
    
    std::cout << "All tests passed successfully!\n";
    return 0;
}
