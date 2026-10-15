#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>

#include "Client/Core/AITelemetry.h"
#include "rapidjson/document.h"

using namespace Client::Core;

static void TestPacketTxEmission() {
    std::cout << "[TEST] 1. Emisja zdarzenia TX (EmitPacketTx)..." << std::endl;
    AITelemetry telem(32);

    telem.EmitPacketTx("MartySama", 0x1A, 128, 1001);
    assert(telem.GetBufferedCount() == 1);
    assert(!telem.IsEmpty());

    std::string jsonStr = telem.GetLastEventJson();
    assert(!jsonStr.empty());
    assert(jsonStr.back() == '\n');

    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());
    assert(!doc.HasParseError());
    assert(doc.IsObject());

    assert(doc.HasMember("ts") && doc["ts"].IsUint64() && doc["ts"].GetUint64() > 0);
    assert(doc.HasMember("level") && std::string(doc["level"].GetString()) == "INFO");
    assert(doc.HasMember("cat") && std::string(doc["cat"].GetString()) == "NET");
    assert(doc.HasMember("event") && std::string(doc["event"].GetString()) == "PACKET_TX");
    assert(doc.HasMember("driver") && std::string(doc["driver"].GetString()) == "MartySama");
    assert(doc.HasMember("opcode") && doc["opcode"].GetUint() == 0x1A);
    assert(doc.HasMember("op") && std::string(doc["op"].GetString()) == "0x1A");
    assert(doc.HasMember("len") && doc["len"].GetUint() == 128);
    assert(doc.HasMember("length") && doc["length"].GetUint() == 128);
    assert(doc.HasMember("seq") && doc["seq"].GetUint() == 1001);

    std::cout << "[PASS] Emisja TX poprawna." << std::endl;
}

static void TestPacketRxEmission() {
    std::cout << "[TEST] 2. Emisja zdarzenia RX (EmitPacketRx)..." << std::endl;
    AITelemetry telem(32);

    telem.EmitPacketRx("Luria", 0x05, 42);
    assert(telem.GetBufferedCount() == 1);

    std::string jsonStr = telem.GetLastEventJson();
    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());
    assert(!doc.HasParseError());

    assert(doc.HasMember("ts") && doc["ts"].IsUint64());
    assert(doc.HasMember("level") && std::string(doc["level"].GetString()) == "INFO");
    assert(doc.HasMember("cat") && std::string(doc["cat"].GetString()) == "NET");
    assert(doc.HasMember("event") && std::string(doc["event"].GetString()) == "PACKET_RX");
    assert(doc.HasMember("driver") && std::string(doc["driver"].GetString()) == "Luria");
    assert(doc.HasMember("opcode") && doc["opcode"].GetUint() == 0x05);
    assert(doc.HasMember("op") && std::string(doc["op"].GetString()) == "0x05");
    assert(doc.HasMember("len") && doc["len"].GetUint() == 42);

    std::cout << "[PASS] Emisja RX poprawna." << std::endl;
}

static void TestCombatEventEmission() {
    std::cout << "[TEST] 3. Emisja zdarzenia walki (EmitCombatEvent)..." << std::endl;
    AITelemetry telem(32);

    telem.EmitCombatEvent(10001, 20002, 1250, false);
    assert(telem.GetBufferedCount() == 1);

    std::string jsonStr1 = telem.GetLastEventJson();
    rapidjson::Document doc1;
    doc1.Parse(jsonStr1.c_str());
    assert(!doc1.HasParseError());

    assert(doc1.HasMember("cat") && std::string(doc1["cat"].GetString()) == "COMBAT");
    assert(doc1.HasMember("event") && std::string(doc1["event"].GetString()) == "COMBAT_EVENT");
    assert(doc1.HasMember("attackerVid") && doc1["attackerVid"].GetUint() == 10001);
    assert(doc1.HasMember("victimVid") && doc1["victimVid"].GetUint() == 20002);
    assert(doc1.HasMember("damage") && doc1["damage"].GetUint() == 1250);
    assert(doc1.HasMember("isDead") && doc1["isDead"].IsFalse());

    telem.EmitCombatEvent(10001, 20002, 5000, true);
    assert(telem.GetBufferedCount() == 2);

    std::string jsonStr2 = telem.GetLastEventJson();
    rapidjson::Document doc2;
    doc2.Parse(jsonStr2.c_str());
    assert(!doc2.HasParseError());
    assert(doc2.HasMember("isDead") && doc2["isDead"].IsTrue());
    assert(doc2.HasMember("is_dead") && doc2["is_dead"].IsTrue());

    std::cout << "[PASS] Emisja walki poprawna." << std::endl;
}

static void TestPhaseTransitionEmission() {
    std::cout << "[TEST] 4. Emisja tranzycji faz (EmitPhaseTransition)..." << std::endl;
    AITelemetry telem(32);

    telem.EmitPhaseTransition("Select", "Loading");
    std::string jsonStr = telem.GetLastEventJson();

    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());
    assert(!doc.HasParseError());

    assert(doc.HasMember("cat") && std::string(doc["cat"].GetString()) == "PHASE");
    assert(doc.HasMember("event") && std::string(doc["event"].GetString()) == "PHASE_TRANSITION");
    assert(doc.HasMember("from") && std::string(doc["from"].GetString()) == "Select");
    assert(doc.HasMember("to") && std::string(doc["to"].GetString()) == "Loading");

    std::cout << "[PASS] Emisja tranzycji faz poprawna." << std::endl;
}

static void TestErrorEmission() {
    std::cout << "[TEST] 5. Emisja bledow i znakow specjalnych (EmitError)..." << std::endl;
    AITelemetry telem(32);

    telem.EmitError("SYSTEM", "ERR_SOCKET_DROP", "Polaczenie zerwane: \"reset\" \\ test");
    std::string jsonStr = telem.GetLastEventJson();

    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());
    assert(!doc.HasParseError());

    assert(doc.HasMember("level") && std::string(doc["level"].GetString()) == "ERROR");
    assert(doc.HasMember("cat") && std::string(doc["cat"].GetString()) == "SYSTEM");
    assert(doc.HasMember("event") && std::string(doc["event"].GetString()) == "ERROR");
    assert(doc.HasMember("code") && std::string(doc["code"].GetString()) == "ERR_SOCKET_DROP");
    assert(doc.HasMember("message") && std::string(doc["message"].GetString()) == "Polaczenie zerwane: \"reset\" \\ test");

    telem.EmitError(TelemetryCategory::Network, "ERR_TIMEOUT", "Timeout zadania");
    std::string jsonStr2 = telem.GetLastEventJson();
    rapidjson::Document doc2;
    doc2.Parse(jsonStr2.c_str());
    assert(!doc2.HasParseError());
    assert(doc2.HasMember("cat") && std::string(doc2["cat"].GetString()) == "NET");

    std::cout << "[PASS] Emisja bledow poprawna." << std::endl;
}

static void TestRingBufferWrapAround() {
    std::cout << "[TEST] 6. Bufor pierscieniowy (Wrap-Around & Zero-Overhead)..." << std::endl;
    constexpr size_t TEST_CAPACITY = 4;
    AITelemetry telem(TEST_CAPACITY);

    assert(telem.IsEmpty());
    assert(!telem.IsFull());
    assert(telem.GetCapacity() == TEST_CAPACITY);
    assert(telem.GetBufferedCount() == 0);

    for (uint32_t i = 1; i <= 6; ++i) {
        telem.EmitPacketTx("DriverTest", static_cast<uint8_t>(i), 64, i);
    }

    assert(telem.GetBufferedCount() == TEST_CAPACITY);
    assert(telem.IsFull());
    assert(telem.GetTotalEmittedCount() == 6);

    auto events = telem.GetEventsAsJsonLines();
    assert(events.size() == TEST_CAPACITY);

    for (size_t i = 0; i < TEST_CAPACITY; ++i) {
        rapidjson::Document doc;
        doc.Parse(events[i].c_str());
        assert(!doc.HasParseError());
        uint32_t expectedSeq = static_cast<uint32_t>(i + 3);
        assert(doc["seq"].GetUint() == expectedSeq);
    }

    telem.Clear();
    assert(telem.IsEmpty());
    assert(telem.GetBufferedCount() == 0);

    std::cout << "[PASS] Test bufora pierscieniowego zaliczony." << std::endl;
}

static void TestStreamingAndCallback() {
    std::cout << "[TEST] 7. Strumieniowanie w locie i callback..." << std::endl;
    AITelemetry telem(16);

    std::ostringstream oss;
    telem.SetOutputStream(&oss);
    telem.EnableStreaming(true);
    assert(telem.IsStreamingEnabled());

    telem.EmitPacketRx("StreamDriver", 0x11, 256);
    std::string streamOutput = oss.str();
    assert(!streamOutput.empty());

    rapidjson::Document doc;
    doc.Parse(streamOutput.c_str());
    assert(!doc.HasParseError());
    assert(doc["driver"].GetString() == std::string("StreamDriver"));

    std::string capturedCallback;
    telem.SetOutputStream(nullptr);
    telem.SetStreamCallback([&capturedCallback](std::string_view line) {
        capturedCallback = std::string(line);
    });

    telem.EmitPhaseTransition("Login", "Select");
    assert(!capturedCallback.empty());
    rapidjson::Document cbDoc;
    cbDoc.Parse(capturedCallback.c_str());
    assert(!cbDoc.HasParseError());
    assert(cbDoc["from"].GetString() == std::string("Login"));

    telem.EnableStreaming(false);
    capturedCallback.clear();
    telem.EmitPhaseTransition("Select", "Game");
    assert(capturedCallback.empty());

    std::cout << "[PASS] Strumieniowanie i callback dzialaja poprawnie." << std::endl;
}

static void TestFlushToFile() {
    std::cout << "[TEST] 8. Zrzut bufora do pliku (FlushToFile)..." << std::endl;
    AITelemetry telem(16);

    telem.EmitPacketTx("FileDriver", 1, 10, 101);
    telem.EmitPacketRx("FileDriver", 2, 20);
    telem.EmitCombatEvent(100, 200, 300, false);

    const char* testFile = "ai_telemetry_test_dump.jsonl";
    bool flushOk = telem.FlushToFile(testFile);
    assert(flushOk);

    std::ifstream infile(testFile);
    assert(infile.is_open());

    std::string line;
    size_t lineCount = 0;
    while (std::getline(infile, line)) {
        if (line.empty()) continue;
        rapidjson::Document doc;
        doc.Parse(line.c_str());
        assert(!doc.HasParseError());
        assert(doc.HasMember("cat"));
        lineCount++;
    }
    infile.close();
    assert(lineCount == 3);

    std::remove(testFile);
    std::cout << "[PASS] Zrzut do pliku zakonczony sukcesem." << std::endl;
}

static void TestFlushToStreamAndDump() {
    std::cout << "[TEST] 9. Zrzut do strumienia i DumpToString..." << std::endl;
    AITelemetry telem(16);

    telem.EmitPacketTx("DumpDriver", 5, 50, 500);
    telem.EmitPacketRx("DumpDriver", 6, 60);

    std::ostringstream oss;
    telem.FlushToStream(oss);
    std::string streamDump = oss.str();
    assert(!streamDump.empty());

    std::string strDump = telem.DumpToString();
    assert(streamDump == strDump);

    std::cout << "[PASS] Zrzut do strumienia i zrzut tekstowy zgodne." << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "TEST SUITE: AITelemetry JSON-Lines Engine" << std::endl;
    std::cout << "========================================" << std::endl;

    TestPacketTxEmission();
    TestPacketRxEmission();
    TestCombatEventEmission();
    TestPhaseTransitionEmission();
    TestErrorEmission();
    TestRingBufferWrapAround();
    TestStreamingAndCallback();
    TestFlushToFile();
    TestFlushToStreamAndDump();

    std::cout << "========================================" << std::endl;
    std::cout << "WSZYSTKIE TESTY AITelemetry ZDANE NA 100%!" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
