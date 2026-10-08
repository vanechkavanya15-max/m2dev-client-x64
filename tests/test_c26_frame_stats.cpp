// ============================================================================
// FrameStatisticsTracker Tests
// ============================================================================

#include "../src/EterLib/Render/FrameStatisticsTracker.h"
#include <rapidjson/document.h>
#include <cassert>
#include <iostream>
#include <string>

// ----------------------------------------------------------------------------
// Test: Initialization
// Ensures all metrics default to zero.
// ----------------------------------------------------------------------------
void TestInitialization()
{
    EterLib::Render::FrameStatisticsTracker tracker;
    auto summary = tracker.GetSummary();
    
    assert(summary.totalDrawCallsSubmitted == 0);
    assert(summary.actualDrawCallsExecuted == 0);
    assert(summary.drawCallsSavedByBatching == 0);
    assert(summary.stateChangesAttempted == 0);
    assert(summary.stateChangesSkippedByDeduplication == 0);
    assert(summary.sortTimeMicroseconds == 0);
    assert(summary.executionTimeMicroseconds == 0);

    std::cout << "TestInitialization passed.\n";
}

// ----------------------------------------------------------------------------
// Test: MetricsUpdate
// Ensures manual metrics update correctly reflect in summary.
// ----------------------------------------------------------------------------
void TestMetricsUpdate()
{
    EterLib::Render::FrameStatisticsTracker tracker;
    tracker.totalDrawCallsSubmitted = 100;
    tracker.actualDrawCallsExecuted = 80;
    tracker.drawCallsSavedByBatching = 20;
    tracker.stateChangesAttempted = 50;
    tracker.stateChangesSkippedByDeduplication = 15;
    tracker.sortTimeMicroseconds = 500;
    tracker.executionTimeMicroseconds = 2000;
    
    auto summary = tracker.GetSummary();
    assert(summary.totalDrawCallsSubmitted == 100);
    assert(summary.actualDrawCallsExecuted == 80);
    assert(summary.drawCallsSavedByBatching == 20);
    assert(summary.stateChangesAttempted == 50);
    assert(summary.stateChangesSkippedByDeduplication == 15);
    assert(summary.sortTimeMicroseconds == 500);
    assert(summary.executionTimeMicroseconds == 2000);
    
    std::cout << "TestMetricsUpdate passed.\n";
}

// ----------------------------------------------------------------------------
// Test: BeginFrameReset
// Ensures BeginFrame resets all metrics to zero for the new frame.
// ----------------------------------------------------------------------------
void TestBeginFrameReset()
{
    EterLib::Render::FrameStatisticsTracker tracker;
    tracker.totalDrawCallsSubmitted = 100;
    tracker.actualDrawCallsExecuted = 80;
    tracker.drawCallsSavedByBatching = 20;
    tracker.stateChangesAttempted = 50;
    tracker.stateChangesSkippedByDeduplication = 15;
    tracker.sortTimeMicroseconds = 500;
    tracker.executionTimeMicroseconds = 2000;
    
    tracker.BeginFrame();
    
    auto summary = tracker.GetSummary();
    assert(summary.totalDrawCallsSubmitted == 0);
    assert(summary.actualDrawCallsExecuted == 0);
    assert(summary.drawCallsSavedByBatching == 0);
    assert(summary.stateChangesAttempted == 0);
    assert(summary.stateChangesSkippedByDeduplication == 0);
    assert(summary.sortTimeMicroseconds == 0);
    assert(summary.executionTimeMicroseconds == 0);
    
    std::cout << "TestBeginFrameReset passed.\n";
}

// ----------------------------------------------------------------------------
// Test: EndFrameNoOp
// Ensures EndFrame operates as expected (currently a no-op).
// ----------------------------------------------------------------------------
void TestEndFrameNoOp()
{
    EterLib::Render::FrameStatisticsTracker tracker;
    tracker.totalDrawCallsSubmitted = 10;
    
    tracker.EndFrame();
    
    auto summary = tracker.GetSummary();
    assert(summary.totalDrawCallsSubmitted == 10);
    
    std::cout << "TestEndFrameNoOp passed.\n";
}

// ----------------------------------------------------------------------------
// Test: FormatTelemetryJSON
// Ensures the serialized JSON contains exactly the correct metrics and types.
// ----------------------------------------------------------------------------
void TestFormatTelemetryJSON()
{
    EterLib::Render::FrameStatisticsTracker tracker;
    tracker.totalDrawCallsSubmitted = 150;
    tracker.actualDrawCallsExecuted = 100;
    tracker.drawCallsSavedByBatching = 50;
    tracker.stateChangesAttempted = 300;
    tracker.stateChangesSkippedByDeduplication = 120;
    tracker.sortTimeMicroseconds = 1234;
    tracker.executionTimeMicroseconds = 5678;
    
    std::string jsonStr = tracker.FormatTelemetryJSON();
    
    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());
    
    assert(!doc.HasParseError());
    assert(doc.IsObject());
    
    assert(doc.HasMember("totalDrawCallsSubmitted"));
    assert(doc["totalDrawCallsSubmitted"].IsUint());
    assert(doc["totalDrawCallsSubmitted"].GetUint() == 150);
    
    assert(doc.HasMember("actualDrawCallsExecuted"));
    assert(doc["actualDrawCallsExecuted"].IsUint());
    assert(doc["actualDrawCallsExecuted"].GetUint() == 100);
    
    assert(doc.HasMember("drawCallsSavedByBatching"));
    assert(doc["drawCallsSavedByBatching"].IsUint());
    assert(doc["drawCallsSavedByBatching"].GetUint() == 50);
    
    assert(doc.HasMember("stateChangesAttempted"));
    assert(doc["stateChangesAttempted"].IsUint());
    assert(doc["stateChangesAttempted"].GetUint() == 300);
    
    assert(doc.HasMember("stateChangesSkippedByDeduplication"));
    assert(doc["stateChangesSkippedByDeduplication"].IsUint());
    assert(doc["stateChangesSkippedByDeduplication"].GetUint() == 120);
    
    assert(doc.HasMember("sortTimeMicroseconds"));
    assert(doc["sortTimeMicroseconds"].IsUint64());
    assert(doc["sortTimeMicroseconds"].GetUint64() == 1234);
    
    assert(doc.HasMember("executionTimeMicroseconds"));
    assert(doc["executionTimeMicroseconds"].IsUint64());
    assert(doc["executionTimeMicroseconds"].GetUint64() == 5678);
    
    std::cout << "TestFormatTelemetryJSON passed.\n";
}

// ----------------------------------------------------------------------------
// Test: FormatTelemetryJSON_EmptyFrame
// Ensures empty metrics format correctly as 0.
// ----------------------------------------------------------------------------
void TestFormatTelemetryJSON_EmptyFrame()
{
    EterLib::Render::FrameStatisticsTracker tracker;
    tracker.BeginFrame();
    tracker.EndFrame();
    
    std::string jsonStr = tracker.FormatTelemetryJSON();
    
    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());
    
    assert(!doc.HasParseError());
    assert(doc["totalDrawCallsSubmitted"].GetUint() == 0);
    assert(doc["actualDrawCallsExecuted"].GetUint() == 0);
    
    std::cout << "TestFormatTelemetryJSON_EmptyFrame passed.\n";
}

// ----------------------------------------------------------------------------
// Test: FormatTelemetryJSON_MaxValues
// Ensures 32-bit and 64-bit max boundaries are correctly serialized and parsed.
// ----------------------------------------------------------------------------
void TestFormatTelemetryJSON_MaxValues()
{
    EterLib::Render::FrameStatisticsTracker tracker;
    tracker.totalDrawCallsSubmitted = UINT32_MAX;
    tracker.actualDrawCallsExecuted = UINT32_MAX;
    tracker.drawCallsSavedByBatching = UINT32_MAX;
    tracker.stateChangesAttempted = UINT32_MAX;
    tracker.stateChangesSkippedByDeduplication = UINT32_MAX;
    tracker.sortTimeMicroseconds = UINT64_MAX;
    tracker.executionTimeMicroseconds = UINT64_MAX;
    
    std::string jsonStr = tracker.FormatTelemetryJSON();
    
    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());
    
    assert(!doc.HasParseError());
    assert(doc["totalDrawCallsSubmitted"].GetUint() == UINT32_MAX);
    assert(doc["actualDrawCallsExecuted"].GetUint() == UINT32_MAX);
    assert(doc["drawCallsSavedByBatching"].GetUint() == UINT32_MAX);
    assert(doc["stateChangesAttempted"].GetUint() == UINT32_MAX);
    assert(doc["stateChangesSkippedByDeduplication"].GetUint() == UINT32_MAX);
    assert(doc["sortTimeMicroseconds"].GetUint64() == UINT64_MAX);
    assert(doc["executionTimeMicroseconds"].GetUint64() == UINT64_MAX);
    
    std::cout << "TestFormatTelemetryJSON_MaxValues passed.\n";
}

// ----------------------------------------------------------------------------
// Test: ExtremeJSONScenarios
// Repeated testing of JSON formats across looping updates.
// ----------------------------------------------------------------------------
void TestExtremeJSONScenarios()
{
    for (int i = 0; i < 100; ++i)
    {
        EterLib::Render::FrameStatisticsTracker tracker;
        tracker.totalDrawCallsSubmitted = i * 10;
        tracker.actualDrawCallsExecuted = i * 5;
        tracker.drawCallsSavedByBatching = i * 5;
        tracker.stateChangesAttempted = i * 20;
        tracker.stateChangesSkippedByDeduplication = i * 10;
        tracker.sortTimeMicroseconds = i * 1000;
        tracker.executionTimeMicroseconds = i * 5000;
        
        std::string jsonStr = tracker.FormatTelemetryJSON();
        rapidjson::Document doc;
        doc.Parse(jsonStr.c_str());
        
        assert(doc["totalDrawCallsSubmitted"].GetUint() == static_cast<unsigned int>(i * 10));
    }
    std::cout << "TestExtremeJSONScenarios passed.\n";
}

int main()
{
    std::cout << "Running FrameStatisticsTracker tests...\n";
    TestInitialization();
    TestMetricsUpdate();
    TestBeginFrameReset();
    TestEndFrameNoOp();
    TestFormatTelemetryJSON();
    TestFormatTelemetryJSON_EmptyFrame();
    TestFormatTelemetryJSON_MaxValues();
    TestExtremeJSONScenarios();
    std::cout << "All tests passed.\n";
    return 0;
}

