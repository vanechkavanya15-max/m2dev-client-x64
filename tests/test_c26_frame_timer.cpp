#include "../src/Client/Core/FrameTimer.h"
#include <cassert>
#include <iostream>
#include <cmath>
#include <chrono>

using namespace Client::Core;
using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;

// Helper to assert with message
#define ASSERT_TRUE(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " << #condition << ", " << message << "\n"; \
            assert(condition); \
        } \
    } while (false)

void TestFirstTick()
{
    FrameTimer timer;
    auto now = Clock::now();
    timer.Tick(now);
    ASSERT_TRUE(timer.GetDeltaTime() == 0.0, "First tick should have 0 delta time");
}

void TestDeltaTimeCalculation()
{
    FrameTimer timer(1.0 / 60.0, 0.1);
    auto now = Clock::now();
    timer.Tick(now); // first tick

    now += 16ms; // ~60fps
    timer.Tick(now);

    double dt = timer.GetDeltaTime();
    ASSERT_TRUE(std::abs(dt - 0.016) < 0.001, "Delta time should be ~0.016");
}

void TestDeltaTimeClamping()
{
    FrameTimer timer(1.0 / 60.0, 0.1);
    auto now = Clock::now();
    timer.Tick(now); // first tick

    // Simulating a huge delay / lag spike
    now += 500ms;
    timer.Tick(now);

    double dt = timer.GetDeltaTime();
    ASSERT_TRUE(dt <= 0.1, "Delta time should be clamped to 0.1");
}

void TestFixedTimestep()
{
    FrameTimer timer(1.0 / 60.0, 0.1); // ~0.01666 fixed step
    auto now = Clock::now();
    timer.Tick(now); // first tick

    now += 34ms; // ~2 fixed frames
    timer.Tick(now);

    bool consumed1 = timer.ConsumeFixedStep();
    bool consumed2 = timer.ConsumeFixedStep();
    bool consumed3 = timer.ConsumeFixedStep();

    ASSERT_TRUE(consumed1, "Should consume first fixed step");
    ASSERT_TRUE(consumed2, "Should consume second fixed step");
    ASSERT_TRUE(!consumed3, "Should not have enough accumulator for third step");
}

void TestFPSCalculation()
{
    FrameTimer timer;
    auto now = Clock::now();
    timer.Tick(now);

    // Simulate 60 frames over just over 1 second
    for (int i = 0; i < 60; ++i)
    {
        // 16667us * 60 = 1000020us = 1.00002s
        now += std::chrono::microseconds(16667);
        timer.Tick(now);
    }

    // Now it scales accurately: 60 frames / 1.00002s = 59.998 FPS -> 59 because of static_cast<uint32_t>
    ASSERT_TRUE(timer.GetFPS() == 59 || timer.GetFPS() == 60, "FPS should be 59 or 60");
}

void TestFrameMetrics()
{
    FrameTimer timer;
    auto now = Clock::now();
    timer.Tick(now);
    
    // add exact 10ms frame times
    for (int i = 0; i < 10; ++i)
    {
        now += 10ms;
        timer.Tick(now);
    }

    double avg = timer.GetAverageFrameTime();
    double var = timer.GetFrameTimeVariance();

    ASSERT_TRUE(std::abs(avg - 0.010) < 0.0001, "Average frame time should be 0.010");
    ASSERT_TRUE(var < 0.0000001, "Variance should be very close to 0 for consistent frames");
}

int main()
{
    std::cout << "Running FrameTimer Tests...\n";

    TestFirstTick();
    TestDeltaTimeCalculation();
    TestDeltaTimeClamping();
    TestFixedTimestep();
    TestFPSCalculation();
    TestFrameMetrics();

    std::cout << "All tests passed!\n";
    return 0;
}
