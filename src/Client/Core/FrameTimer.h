#pragma once

#include <chrono>
#include <deque>
#include <cstdint>
#include <algorithm>

namespace Client::Core
{
    class FrameTimer
    {
    public:
        using Clock = std::chrono::steady_clock;
        using TimePoint = Clock::time_point;
        using Duration = std::chrono::duration<double>;

        explicit FrameTimer(double fixedTimestep = 1.0 / 60.0, double maxDeltaTime = 0.1);
        ~FrameTimer() = default;

        void Tick();
        void Tick(TimePoint currentTime);

        [[nodiscard]] double GetDeltaTime() const;
        [[nodiscard]] double GetFixedDeltaTime() const;

        bool ConsumeFixedStep();

        [[nodiscard]] uint32_t GetFPS() const;
        [[nodiscard]] double GetAverageFrameTime() const;
        [[nodiscard]] double GetFrameTimeVariance() const;

        void Reset();

    private:
        double m_fixedTimestep;
        double m_maxDeltaTime;
        double m_accumulator;
        double m_currentDeltaTime;

        TimePoint m_lastTime;
        bool m_isFirstTick;

        static constexpr size_t METRICS_SAMPLES = 60;
        double m_frameTimes[METRICS_SAMPLES];
        size_t m_frameTimeIndex;
        size_t m_frameTimeCount;

        uint32_t m_currentFPS;
        uint32_t m_frameCount;
        double m_timeSinceLastFPSUpdate;

        void UpdateMetrics(double deltaTime);
    };
} // namespace Client::Core
