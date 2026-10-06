#pragma once

#include <chrono>
#include <cstdint>

/**
 * @brief High-precision timer using modern C++20 chrono.
 * 
 * Replaces the legacy CTimer and ELTimer_* functions that used timeGetTime().
 * Uses std::chrono::steady_clock for monotonic, high-precision timing.
 */
class ChronoTimer
{
public:
    /**
     * @brief Constructor that initializes the base time.
     */
    ChronoTimer()
    {
        Init();
    }

    /**
     * @brief Destructor.
     */
    ~ChronoTimer() = default;

    /**
     * @brief Initializes the timer, setting the base time.
     */
    void Init()
    {
        baseTime = std::chrono::steady_clock::now();
        useRealTime = true;
        currentTime = 0;
        currentTimeFloat = 0.0f;
        elapsedTime = 0;
        customIndex = 0;
        serverTime = 0;
        clientSyncTime = 0;
        frameTime = 0;
    }

    /**
     * @brief Advances the timer. Should be called once per frame.
     */
    void Advance()
    {
        if (!useRealTime)
        {
            ++customIndex;

            if (customIndex == 1)
            {
                customIndex = -1;
            }

            currentTime += 16 + (customIndex & 1);
            currentTimeFloat = static_cast<float>(currentTime) / 1000.0f;
        }
        else
        {
            uint32_t currentMs = GetElapsedMilliseconds();

            if (currentTime == 0)
            {
                currentTime = currentMs;
            }

            elapsedTime = currentMs - currentTime;
            currentTime = currentMs;
        }
    }

    /**
     * @brief Adjusts the current time by a specified gap.
     * @param timeGap The amount of time to adjust by, in milliseconds.
     */
    void Adjust(int32_t timeGap)
    {
        currentTime += timeGap;
    }

    /**
     * @brief Sets the base time to 0.
     */
    void SetBaseTime()
    {
        currentTime = 0;
    }

    /**
     * @brief Retrieves the current time in seconds.
     * @return The current time in seconds.
     */
    float GetCurrentSecond() const
    {
        if (useRealTime)
        {
            return static_cast<float>(GetElapsedMilliseconds()) / 1000.0f;
        }

        return currentTimeFloat;
    }

    /**
     * @brief Retrieves the current time in milliseconds.
     * @return The current time in milliseconds.
     */
    uint32_t GetCurrentMillisecond() const
    {
        if (useRealTime)
        {
            return GetElapsedMilliseconds();
        }

        return currentTime;
    }

    /**
     * @brief Retrieves the elapsed time since the last Advance() call, in seconds.
     * @return The elapsed time in seconds.
     */
    float GetElapsedSecond() const
    {
        return static_cast<float>(GetElapsedMillisecond()) / 1000.0f;
    }

    /**
     * @brief Retrieves the elapsed time since the last Advance() call, in milliseconds.
     * @return The elapsed time in milliseconds.
     */
    uint32_t GetElapsedMillisecond() const
    {
        if (!useRealTime)
        {
            return 16 + (customIndex & 1);
        }

        return elapsedTime;
    }

    /**
     * @brief Switches the timer to use custom, fixed-step time (16/17ms).
     */
    void UseCustomTime()
    {
        useRealTime = false;
    }

    /**
     * @brief Retrieves the total elapsed milliseconds since initialization.
     * @return Elapsed time in milliseconds.
     */
    uint32_t GetElapsedMilliseconds() const
    {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - baseTime);
        return static_cast<uint32_t>(duration.count());
    }

    /**
     * @brief Sets the synchronized server time.
     * @param newServerTime The current time from the server in milliseconds.
     */
    void SetServerMillisecond(uint32_t newServerTime)
    {
        if (newServerTime != 0)
        {
            serverTime = newServerTime;
            clientSyncTime = GetCurrentMillisecond();
        }
    }

    /**
     * @brief Retrieves the synchronized server time.
     * @return The server time in milliseconds.
     */
    uint32_t GetServerMillisecond() const
    {
        return GetCurrentMillisecond() - clientSyncTime + serverTime;
    }

    /**
     * @brief Sets the current frame time.
     */
    void SetFrameMillisecond()
    {
        frameTime = GetElapsedMilliseconds();
    }

    /**
     * @brief Retrieves the current frame time.
     * @return The frame time in milliseconds.
     */
    uint32_t GetFrameMillisecond() const
    {
        return frameTime;
    }

    /**
     * @brief Retrieves the server time based on the frame time.
     * @return The server frame time in milliseconds.
     */
    uint32_t GetServerFrameMillisecond() const
    {
        return frameTime - clientSyncTime + serverTime;
    }

private:
    std::chrono::steady_clock::time_point baseTime;
    bool useRealTime = true;
    uint32_t currentTime = 0;
    float currentTimeFloat = 0.0f;
    uint32_t elapsedTime = 0;
    int32_t customIndex = 0;
    
    uint32_t serverTime = 0;
    uint32_t clientSyncTime = 0;
    uint32_t frameTime = 0;
};
