#include "../src/Client/World/ActorAlphaBlender.h"
#include "EterBase/LogModern.h"
#include <iostream>
#include <cmath>

namespace {
    int g_testsPassed = 0;
    int g_testsFailed = 0;

    void AssertTrue(bool condition, const char* testName) {
        if (condition) {
            EterBase::ModernLogger::Info("PASS: {}", testName);
            g_testsPassed++;
        } else {
            EterBase::ModernLogger::Error("FAIL: {}", testName);
            g_testsFailed++;
        }
    }

    void AssertFloatEq(float a, float b, const char* testName, float epsilon = 0.001f) {
        if (std::abs(a - b) < epsilon) {
            EterBase::ModernLogger::Info("PASS: {}", testName);
            g_testsPassed++;
        } else {
            EterBase::ModernLogger::Error("FAIL: {} (Expected: {}, Got: {})", testName, b, a);
            g_testsFailed++;
        }
    }
}

void TestInitialState() {
    Client::World::ActorAlphaBlender blender;
    const auto& state = blender.GetState();
    
    AssertFloatEq(state.currentAlpha, 1.0f, "TestInitialState_AlphaIsOne");
    AssertTrue(state.isVisible, "TestInitialState_IsVisible");
    AssertTrue(!state.isBlending, "TestInitialState_IsNotBlending");
}

void TestFadeOutLinear() {
    Client::World::ActorAlphaBlender blender;
    
    Client::World::AlphaBlendConfig config;
    config.duration = std::chrono::milliseconds(1000);
    config.easing = Client::World::BlendEasing::Linear;
    
    auto result = blender.StartFadeOut(config);
    AssertTrue(result.has_value(), "TestFadeOutLinear_StartSuccess");
    
    const auto& state = blender.GetState();
    AssertTrue(state.isBlending, "TestFadeOutLinear_IsBlending");
    AssertTrue(state.isVisible, "TestFadeOutLinear_IsVisibleDuringFade");
    
    // Halfway
    blender.Update(std::chrono::milliseconds(500));
    AssertFloatEq(state.currentAlpha, 0.5f, "TestFadeOutLinear_HalfwayAlpha");
    
    // Finished
    blender.Update(std::chrono::milliseconds(500));
    AssertFloatEq(state.currentAlpha, 0.0f, "TestFadeOutLinear_FinishedAlpha");
    AssertTrue(!state.isBlending, "TestFadeOutLinear_FinishedNotBlending");
    AssertTrue(state.isVisible, "TestFadeOutLinear_FinishedIsVisibleTrue"); // Rule 2
}

void TestFadeInLinear() {
    Client::World::ActorAlphaBlender blender;
    
    // First fade out instantly
    Client::World::AlphaBlendConfig configOut;
    configOut.duration = std::chrono::milliseconds(1);
    blender.StartFadeOut(configOut);
    blender.Update(std::chrono::milliseconds(1));
    
    AssertFloatEq(blender.GetState().currentAlpha, 0.0f, "TestFadeInLinear_SetupZeroAlpha");
    
    // Now fade in
    Client::World::AlphaBlendConfig configIn;
    configIn.duration = std::chrono::milliseconds(2000);
    configIn.easing = Client::World::BlendEasing::Linear;
    
    auto result = blender.StartFadeIn(configIn);
    AssertTrue(result.has_value(), "TestFadeInLinear_StartSuccess");
    
    const auto& state = blender.GetState();
    
    // Quarter way
    blender.Update(std::chrono::milliseconds(500));
    AssertFloatEq(state.currentAlpha, 0.25f, "TestFadeInLinear_QuarterAlpha");
    
    // Finished
    blender.Update(std::chrono::milliseconds(1500));
    AssertFloatEq(state.currentAlpha, 1.0f, "TestFadeInLinear_FinishedAlpha");
    AssertTrue(!state.isBlending, "TestFadeInLinear_FinishedNotBlending");
    AssertTrue(state.isVisible, "TestFadeInLinear_FinishedIsVisibleTrue");
}

void TestCancelTransition() {
    Client::World::ActorAlphaBlender blender;
    
    Client::World::AlphaBlendConfig config;
    config.duration = std::chrono::milliseconds(1000);
    
    blender.StartFadeOut(config);
    blender.Update(std::chrono::milliseconds(500));
    
    const auto& state = blender.GetState();
    AssertFloatEq(state.currentAlpha, 0.5f, "TestCancelTransition_HalfwayAlpha");
    
    blender.CancelTransition();
    
    AssertFloatEq(state.currentAlpha, 1.0f, "TestCancelTransition_RestoredAlpha");
    AssertTrue(state.isVisible, "TestCancelTransition_RestoredVisibility");
    AssertTrue(!state.isBlending, "TestCancelTransition_NotBlending");
}

void TestInvalidConfig() {
    Client::World::ActorAlphaBlender blender;
    
    Client::World::AlphaBlendConfig config;
    config.duration = std::chrono::milliseconds(0);
    
    auto result = blender.StartFadeOut(config);
    AssertTrue(!result.has_value(), "TestInvalidConfig_RejectsZeroDuration");
}

int main() {
    EterBase::ModernLogger::Info("--- Starting ActorAlphaBlender Tests ---");
    
    TestInitialState();
    TestFadeOutLinear();
    TestFadeInLinear();
    TestCancelTransition();
    TestInvalidConfig();
    
    EterBase::ModernLogger::Info("--- Test Summary ---");
    EterBase::ModernLogger::Info("Passed: {}", g_testsPassed);
    EterBase::ModernLogger::Info("Failed: {}", g_testsFailed);
    
    return g_testsFailed == 0 ? 0 : 1;
}
