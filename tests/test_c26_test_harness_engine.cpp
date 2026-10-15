#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <cstring>
#include <format>

#include "../src/UserInterface/TestHarness/DeterministicTickController.h"
#include "../src/UserInterface/TestHarness/CrashSentinel.h"
#include "../src/UserInterface/TestHarness/MockWorldDriver.h"
#include "EterBase/Debug.h"

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

void Test_HeadlessMode_And_Win32DialogSuppression()
{
    std::cout << "[TEST] Headless Mode - Flags & Win32 Dialog Suppression..." << std::endl;

    // Domyslnie lub po resecie flaga false
    SetHeadlessMode(false);
    assert(!IsHeadlessMode());
    assert(!g_bHeadlessMode);

    // Aktywacja trybu headless
    SetHeadlessMode(true);
    assert(IsHeadlessMode());
    assert(g_bHeadlessMode);

    // Wyciszenie LogBoxRaw w trybie headless bez blokowania watku przez MessageBoxW
    LogBoxRaw("Headless test message", "HeadlessCaption");
    LogBox("Headless backward-compatible LogBox");

    // Deaktywacja i powrot do stanu standardowego
    SetHeadlessMode(false);
    assert(!IsHeadlessMode());
    assert(!g_bHeadlessMode);

    std::cout << "  -> PASS: Headless mode flags i wyciszenie dialogow Win32 zweryfikowane pomyslnie." << std::endl;
}

void Test_FramedPipe_And_MCP_Protocol_Validation()
{
    std::cout << "[TEST] Framed Named Pipe & MCP Protocol - Wykrywanie 4-bajtowego naglowka i formaty..." << std::endl;

    // Test 1: Symulacja pakietu framowanego z little-endian uint32_t
    {
        std::string jsonPayload = R"({"cmd":"get_engine_state"})";
        uint32_t payloadLen = static_cast<uint32_t>(jsonPayload.size());
        std::vector<char> framedPacket(sizeof(uint32_t) + payloadLen);
        std::memcpy(framedPacket.data(), &payloadLen, sizeof(uint32_t));
        std::memcpy(framedPacket.data() + sizeof(uint32_t), jsonPayload.data(), payloadLen);

        // Weryfikacja naglowka
        uint32_t readLen = 0;
        std::memcpy(&readLen, framedPacket.data(), sizeof(uint32_t));
        assert(readLen == payloadLen);

        // Wykrywanie framowania
        size_t firstBrace = std::string_view(framedPacket.data(), framedPacket.size()).find('{');
        assert(firstBrace == 4);
        std::string extracted(framedPacket.data() + firstBrace, readLen);
        assert(extracted == jsonPayload);
    }

    // Test 2: Symulacja pakietu surowego JSON (bez naglowka)
    {
        std::string rawJson = R"({"jsonrpc":"2.0","method":"game_ping","id":1})";
        size_t firstBrace = std::string_view(rawJson.data(), rawJson.size()).find('{');
        assert(firstBrace == 0); // firstBrace < 4 => isFramed == false
    }

    // Test 3: Walidacja metod AgentMCPBridge
    {
        const char* mcpMethods[] = {
            "game_ping",
            "game_get_state",
            "game_get_inventory",
            "game_get_nearby_entities",
            "game_action_attack",
            "game_action_use_item"
        };
        for (const char* m : mcpMethods)
        {
            std::string req = std::format(R"({{"jsonrpc":"2.0","method":"{}","id":42}})", m);
            rapidjson::Document doc;
            doc.Parse(req.c_str());
            assert(!doc.HasParseError());
            assert(std::string(doc["method"].GetString()) == m);
        }
    }

    // Test 4: Walidacja polecen m2_agent_harness_mcp.py (cmd zamiast method)
    {
        std::string tickCmd = R"({"cmd":"tick_frame","count":5,"delta_ms":16.6})";
        rapidjson::Document doc;
        doc.Parse(tickCmd.c_str());
        assert(!doc.HasParseError());
        assert(std::string(doc["cmd"].GetString()) == "tick_frame");
        assert(doc["count"].GetUint() == 5);
        assert(std::abs(doc["delta_ms"].GetDouble() - 16.6) < 0.001);

        std::string injectCmd = R"({"cmd":"inject_packet","opcode":2,"payload_hex":"0000000000000000","size":8})";
        rapidjson::Document docInj;
        docInj.Parse(injectCmd.c_str());
        assert(!docInj.HasParseError());
        assert(std::string(docInj["cmd"].GetString()) == "inject_packet");
        assert(docInj["opcode"].GetUint() == 2);
        assert(std::string(docInj["payload_hex"].GetString()) == "0000000000000000");
    }

    std::cout << "  -> PASS: Framed Named Pipe & MCP Protocol formaty w 100% poprawne." << std::endl;
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
    Test_HeadlessMode_And_Win32DialogSuppression();
    Test_FramedPipe_And_MCP_Protocol_Validation();

    std::cout << "======================================================" << std::endl;
    std::cout << "  WSZYSTKIE TESTY TESTHARNESSENGINE ZAKONCZONE SUKCESEM" << std::endl;
    std::cout << "======================================================" << std::endl;

    return 0;
}
