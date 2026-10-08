#pragma once

#include <cstdint>
#include <string>

namespace EterLib::Render
{
    struct FrameStatsSummary
    {
        uint32_t totalDrawCallsSubmitted = 0;
        uint32_t actualDrawCallsExecuted = 0;
        uint32_t drawCallsSavedByBatching = 0;
        uint32_t stateChangesAttempted = 0;
        uint32_t stateChangesSkippedByDeduplication = 0;
        uint64_t sortTimeMicroseconds = 0;
        uint64_t executionTimeMicroseconds = 0;
    };

    class FrameStatisticsTracker
    {
    public:
        FrameStatisticsTracker() = default;
        ~FrameStatisticsTracker() = default;

        void BeginFrame() noexcept;
        void EndFrame() noexcept;

        [[nodiscard]] FrameStatsSummary GetSummary() const noexcept;
        [[nodiscard]] std::string FormatTelemetryJSON() const;

        uint32_t totalDrawCallsSubmitted = 0;
        uint32_t actualDrawCallsExecuted = 0;
        uint32_t drawCallsSavedByBatching = 0;
        uint32_t stateChangesAttempted = 0;
        uint32_t stateChangesSkippedByDeduplication = 0;
        uint64_t sortTimeMicroseconds = 0;
        uint64_t executionTimeMicroseconds = 0;
    };
}

