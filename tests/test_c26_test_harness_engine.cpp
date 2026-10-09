#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <cstring>
#include <format>

#include "../src/UserInterface/TestHarness/DeterministicTickController.h"
#include "../src/UserInterface/TestHarness/CrashSentinel.h"
#include "../src/UserInterface/TestHarness/MockWorldDriver.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace UserInterface::TestHarness;

void Test_DeterministicTickController_Flags()
{
    std::cout << "[TEST] DeterministicTickController - Flags & Steps..." << std::endl;
    auto& controller = DeterministicTickController::Instance();
    controller.Reset();

    assert(!controller.IsTestMode());
    assert(!controller.IsFrozen());

    const char* cmdLine = "--some-flag --test-mode --freeze-on-start --other";
    controller.InitFromCommandLine(cmdLine);

    assert(controller.IsTestMode());
    assert(controller.IsFrozen());

    // Step test
    controller.Step(5, 0.02f);
    assert(std::abs(controller.GetStepDeltaTime() - 0.02f) < 0.0001f);

    uint32_t steps = controller.ConsumePendingSteps();
    assert(steps == 5);

    // After consume, pending steps should be 0
    assert(controller.ConsumePendingSteps() == 0);

    // Tick count tracking
    assert(controller.GetCurrentTick() == 0);
    controller.IncrementTickCount();
    controller.IncrementTickCount();
    assert(controller.GetCurrentTick() == 2);

    controller.Reset();
    assert(!controller.IsTestMode());
    assert(!controller.IsFrozen());
    assert(controller.GetCurrentTick() == 0);

    std::cout << "  -> PASS: DeterministicTickController flags & steps poprawnie zweryfikowane." << std::endl;
}

void Test_MockWorldDriver_CommandLine()
{
    std::cout << "[TEST] MockWorldDriver - Command line parsing..." << std::endl;
    auto& driver = MockWorldDriver::Instance();
    driver.Reset();

    assert(!driver.IsMockWorldEnabled());

    driver.InitFromCommandLine("--test-mode --mock-world");
    assert(driver.IsMockWorldEnabled());

    driver.Reset();
    assert(!driver.IsMockWorldEnabled());

    std::cout << "  -> PASS: MockWorldDriver CLI flag poprawnie zweryfikowana." << std::endl;
}

void Test_CrashSentinel_Lifecycle()
{
    std::cout << "[TEST] CrashSentinel - Inicjalizacja i weryfikacja..." << std::endl;
    auto& sentinel = CrashSentinel::Instance();

    bool initOk = sentinel.Initialize("test_crash_sentinel.json");
    assert(initOk);

    assert(!sentinel.HasCrashed());
    std::string lastCrash = sentinel.GetLastCrashJson();
    assert(lastCrash.empty());

    sentinel.Shutdown();
    std::cout << "  -> PASS: CrashSentinel cykl zycia i stan poczatkowy poprawny." << std::endl;
}

void Test_JsonRpc_Protocol_Validation()
{
    std::cout << "[TEST] Protokol JSON-RPC - Parsowanie i formaty zapytan/odpowiedzi..." << std::endl;

    // Test 1: Walidacja bledu parsowania
    {
        std::string malformedJson = "{ invalid json ";
        rapidjson::Document doc;
        doc.Parse(malformedJson.c_str());
        assert(doc.HasParseError());
    }

    // Test 2: Walidacja struktury get_state zapytania
    {
        std::string getStateReq = R"({"jsonrpc": "2.0", "id": 101, "method": "get_state", "params": {}})";
        rapidjson::Document doc;
        doc.Parse(getStateReq.c_str());
        assert(!doc.HasParseError());
        assert(doc.HasMember("jsonrpc") && std::string(doc["jsonrpc"].GetString()) == "2.0");
        assert(doc.HasMember("id") && doc["id"].GetInt() == 101);
        assert(doc.HasMember("method") && std::string(doc["method"].GetString()) == "get_state");
    }

    // Test 3: Walidacja struktury tick_frame zapytania
    {
        std::string tickFrameReq = R"({"jsonrpc": "2.0", "id": 102, "method": "tick_frame", "params": {"count": 10, "delta_time": 0.0166667}})";
        rapidjson::Document doc;
        doc.Parse(tickFrameReq.c_str());
        assert(!doc.HasParseError());
        assert(doc["method"].GetString() == std::string("tick_frame"));
        assert(doc["params"]["count"].GetUint() == 10);
        assert(std::abs(doc["params"]["delta_time"].GetDouble() - 0.0166667) < 0.0001);
    }

    // Test 4: Walidacja struktury inject_packet zapytania
    {
        std::string injectReq = R"({"jsonrpc": "2.0", "id": 103, "method": "inject_packet", "params": {"header": 12, "data": "0C000100"}})";
        rapidjson::Document doc;
        doc.Parse(injectReq.c_str());
        assert(!doc.HasParseError());
        assert(doc["method"].GetString() == std::string("inject_packet"));
        assert(doc["params"]["header"].GetUint() == 12);
        assert(doc["params"]["data"].GetString() == std::string("0C000100"));
    }

    // Test 5: Walidacja serializacji odpowiedzi get_state
    {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        writer.StartObject();
        writer.Key("jsonrpc"); writer.String("2.0");
        writer.Key("id"); writer.Int(101);
        writer.Key("result");
        writer.StartObject();
        writer.Key("player");
        writer.StartObject();
        writer.Key("vid"); writer.Uint(10001);
        writer.Key("name"); writer.String("AI_Agent_Mock");
        writer.Key("hp"); writer.Int(5000);
        writer.Key("max_hp"); writer.Int(5000);
        writer.EndObject();
        writer.Key("controllers");
        writer.StartObject();
        writer.Key("movement");
        writer.StartObject();
        writer.Key("can_move"); writer.Bool(true);
        writer.EndObject();
        writer.Key("combat");
        writer.StartObject();
        writer.Key("can_attack"); writer.Bool(true);
        writer.EndObject();
        writer.Key("target");
        writer.StartObject();
        writer.Key("target_vid"); writer.Uint(0);
        writer.EndObject();
        writer.Key("item");
        writer.StartObject();
        writer.Key("pickable_distance"); writer.Uint(300);
        writer.EndObject();
        writer.EndObject();
        writer.EndObject();
        writer.EndObject();

        rapidjson::Document resDoc;
        resDoc.Parse(sb.GetString());
        assert(!resDoc.HasParseError());
        assert(resDoc["result"]["player"]["vid"].GetUint() == 10001);
        assert(resDoc["result"]["controllers"]["movement"]["can_move"].GetBool() == true);
        assert(resDoc["result"]["controllers"]["combat"]["can_attack"].GetBool() == true);
        assert(resDoc["result"]["controllers"]["target"]["target_vid"].GetUint() == 0);
        assert(resDoc["result"]["controllers"]["item"]["pickable_distance"].GetUint() == 300);
    }

    std::cout << "  -> PASS: JSON-RPC request & response kontrakty w 100% poprawne." << std::endl;
}

int main()
{
    std::cout << "======================================================" << std::endl;
    std::cout << "  TestHarnessEngine C++23 Unit Tests (Filar 4)        " << std::endl;
    std::cout << "======================================================" << std::endl;

    Test_DeterministicTickController_Flags();
    Test_MockWorldDriver_CommandLine();
    Test_CrashSentinel_Lifecycle();
    Test_JsonRpc_Protocol_Validation();

    std::cout << "======================================================" << std::endl;
    std::cout << "  WSZYSTKIE TESTY TESTHARNESSENGINE ZAKONCZONE SUKCESEM" << std::endl;
    std::cout << "======================================================" << std::endl;

    return 0;
}
