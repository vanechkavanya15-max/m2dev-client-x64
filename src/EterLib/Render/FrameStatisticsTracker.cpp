#include "FrameStatisticsTracker.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace EterLib::Render
{
    void FrameStatisticsTracker::BeginFrame() noexcept
    {
        totalDrawCallsSubmitted = 0;
        actualDrawCallsExecuted = 0;
        drawCallsSavedByBatching = 0;
        stateChangesAttempted = 0;
        stateChangesSkippedByDeduplication = 0;
        sortTimeMicroseconds = 0;
        executionTimeMicroseconds = 0;
    }

    void FrameStatisticsTracker::EndFrame() noexcept
    {
        // No-op for now. Metrics can be updated externally by modifying public fields.
    }

    FrameStatsSummary FrameStatisticsTracker::GetSummary() const noexcept
    {
        FrameStatsSummary summary;
        summary.totalDrawCallsSubmitted = totalDrawCallsSubmitted;
        summary.actualDrawCallsExecuted = actualDrawCallsExecuted;
        summary.drawCallsSavedByBatching = drawCallsSavedByBatching;
        summary.stateChangesAttempted = stateChangesAttempted;
        summary.stateChangesSkippedByDeduplication = stateChangesSkippedByDeduplication;
        summary.sortTimeMicroseconds = sortTimeMicroseconds;
        summary.executionTimeMicroseconds = executionTimeMicroseconds;
        return summary;
    }

    std::string FrameStatisticsTracker::FormatTelemetryJSON() const
    {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        writer.StartObject();

        writer.Key("totalDrawCallsSubmitted");
        writer.Uint(totalDrawCallsSubmitted);

        writer.Key("actualDrawCallsExecuted");
        writer.Uint(actualDrawCallsExecuted);

        writer.Key("drawCallsSavedByBatching");
        writer.Uint(drawCallsSavedByBatching);

        writer.Key("stateChangesAttempted");
        writer.Uint(stateChangesAttempted);

        writer.Key("stateChangesSkippedByDeduplication");
        writer.Uint(stateChangesSkippedByDeduplication);

        writer.Key("sortTimeMicroseconds");
        writer.Uint64(sortTimeMicroseconds);

        writer.Key("executionTimeMicroseconds");
        writer.Uint64(executionTimeMicroseconds);

        writer.EndObject();

        return buffer.GetString();
    }
}

