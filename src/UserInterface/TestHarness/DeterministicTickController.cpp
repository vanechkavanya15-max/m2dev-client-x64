#include "DeterministicTickController.h"
#include <cstring>
#include <algorithm>

namespace UserInterface::TestHarness
{
    DeterministicTickController& DeterministicTickController::Instance() noexcept
    {
        static DeterministicTickController s_instance;
        return s_instance;
    }

    void DeterministicTickController::InitFromCommandLine(const char* lpCmdLine) noexcept
    {
        if (!lpCmdLine)
            return;

        if (std::strstr(lpCmdLine, "--test-mode") != nullptr)
        {
            m_isTestMode.store(true, std::memory_order_relaxed);
        }

        if (std::strstr(lpCmdLine, "--freeze-on-start") != nullptr)
        {
            m_isFrozen.store(true, std::memory_order_relaxed);
        }
    }

    void DeterministicTickController::Step(uint32_t count, float deltaTime) noexcept
    {
        if (count == 0)
            return;

        m_stepDeltaTime.store((std::max)(0.0001f, deltaTime), std::memory_order_relaxed);
        m_pendingSteps.fetch_add(count, std::memory_order_relaxed);
    }

    uint32_t DeterministicTickController::ConsumePendingSteps() noexcept
    {
        return m_pendingSteps.exchange(0, std::memory_order_relaxed);
    }

    void DeterministicTickController::Reset() noexcept
    {
        m_isTestMode.store(false, std::memory_order_relaxed);
        m_isFrozen.store(false, std::memory_order_relaxed);
        m_pendingSteps.store(0, std::memory_order_relaxed);
        m_stepDeltaTime.store(1.0f / 60.0f, std::memory_order_relaxed);
        m_currentTick.store(0, std::memory_order_relaxed);
    }
}
