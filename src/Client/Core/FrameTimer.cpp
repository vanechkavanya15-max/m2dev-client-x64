#include "FrameTimer.h"
#include <numeric>
#include <cmath>

namespace Client::Core
{
    FrameTimer::FrameTimer(double fixedTimestep, double maxDeltaTime)
        : m_fixedTimestep(fixedTimestep)
        , m_maxDeltaTime(maxDeltaTime)
        , m_accumulator(0.0)
        , m_currentDeltaTime(0.0)
        , m_isFirstTick(true)
        , m_frameTimeIndex(0)
        , m_frameTimeCount(0)
        , m_currentFPS(0)
        , m_frameCount(0)
        , m_timeSinceLastFPSUpdate(0.0)
    {
        std::fill(std::begin(m_frameTimes), std::end(m_frameTimes), 0.0);
    }

    void FrameTimer::Reset()
    {
        m_accumulator = 0.0;
        m_currentDeltaTime = 0.0;
        m_isFirstTick = true;
        m_frameTimeIndex = 0;
        m_frameTimeCount = 0;
        m_currentFPS = 0;
        m_frameCount = 0;
        m_timeSinceLastFPSUpdate = 0.0;
        std::fill(std::begin(m_frameTimes), std::end(m_frameTimes), 0.0);
    }

    void FrameTimer::Tick()
    {
        Tick(Clock::now());
    }

    void FrameTimer::Tick(TimePoint currentTime)
    {
        if (m_isFirstTick)
        {
            m_lastTime = currentTime;
            m_isFirstTick = false;
            m_currentDeltaTime = 0.0;
            return;
        }

        Duration elapsed = currentTime - m_lastTime;
        m_lastTime = currentTime;

        double dt = elapsed.count();
        double raw_dt = dt;

        // Prevent spiral of death by clamping delta time
        dt = std::clamp(dt, 0.0, m_maxDeltaTime);

        m_currentDeltaTime = dt;
        m_accumulator += dt;

        UpdateMetrics(raw_dt);
    }

    double FrameTimer::GetDeltaTime() const
    {
        return m_currentDeltaTime;
    }

    double FrameTimer::GetFixedDeltaTime() const
    {
        return m_fixedTimestep;
    }

    bool FrameTimer::ConsumeFixedStep()
    {
        if (m_accumulator >= m_fixedTimestep)
        {
            m_accumulator -= m_fixedTimestep;
            return true;
        }
        return false;
    }

    void FrameTimer::UpdateMetrics(double deltaTime)
    {
        m_frameTimes[m_frameTimeIndex] = deltaTime;
        m_frameTimeIndex = (m_frameTimeIndex + 1) % METRICS_SAMPLES;
        if (m_frameTimeCount < METRICS_SAMPLES)
        {
            m_frameTimeCount++;
        }

        m_frameCount++;
        m_timeSinceLastFPSUpdate += deltaTime;

        if (m_timeSinceLastFPSUpdate >= 1.0)
        {
            m_currentFPS = static_cast<uint32_t>(m_frameCount / m_timeSinceLastFPSUpdate);
            m_frameCount = 0;
            // Keep remainder
            m_timeSinceLastFPSUpdate = std::fmod(m_timeSinceLastFPSUpdate, 1.0);
        }
    }

    uint32_t FrameTimer::GetFPS() const
    {
        return m_currentFPS;
    }

    double FrameTimer::GetAverageFrameTime() const
    {
        if (m_frameTimeCount == 0)
        {
            return 0.0;
        }

        double sum = 0.0;
        for (size_t i = 0; i < m_frameTimeCount; ++i)
        {
            sum += m_frameTimes[i];
        }
        return sum / static_cast<double>(m_frameTimeCount);
    }

    double FrameTimer::GetFrameTimeVariance() const
    {
        if (m_frameTimeCount <= 1)
        {
            return 0.0;
        }

        double mean = GetAverageFrameTime();
        double variance = 0.0;

        for (size_t i = 0; i < m_frameTimeCount; ++i)
        {
            double diff = m_frameTimes[i] - mean;
            variance += diff * diff;
        }

        return variance / static_cast<double>(m_frameTimeCount);
    }
} // namespace Client::Core
