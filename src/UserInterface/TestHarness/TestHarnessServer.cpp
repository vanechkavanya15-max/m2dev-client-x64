#include "../StdAfx.h"
#include "TestHarnessServer.h"
#include "DeterministicTickController.h"
#include "MockWorldDriver.h"

#include "../InstanceBase.h"
#include "../PythonCharacterManager.h"
#include "../PythonPlayer.h"
#include "../PythonNetworkStream.h"
#include "../Network/Routers/PhaseGamePacketDispatcher.h"
#include "../Packet.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <cmath>
#include <format>
#include <chrono>

namespace UserInterface::TestHarness
{
    static std::vector<uint8_t> HexStringToBytes(std::string_view hex)
    {
        std::vector<uint8_t> bytes;
        bytes.reserve(hex.size() / 2);

        auto hexCharToInt = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };

        for (size_t i = 0; i + 1 < hex.size(); i += 2)
        {
            int h = hexCharToInt(hex[i]);
            int l = hexCharToInt(hex[i + 1]);
            if (h >= 0 && l >= 0)
            {
                bytes.push_back(static_cast<uint8_t>((h << 4) | l));
            }
        }
        return bytes;
    }

    TestHarnessServer& TestHarnessServer::Instance() noexcept
    {
        static TestHarnessServer s_instance;
        return s_instance;
    }

    TestHarnessServer::TestHarnessServer() = default;

    TestHarnessServer::~TestHarnessServer()
    {
        Stop();
    }

    bool TestHarnessServer::Start(std::string_view pipeName)
    {
        if (m_isRunning.load(std::memory_order_relaxed))
            return true;

        m_pipeName = std::string(pipeName);
        m_isRunning.store(true, std::memory_order_relaxed);

        m_ioThread = std::thread(&TestHarnessServer::ServerThreadProc, this);
        return true;
    }

    void TestHarnessServer::Stop()
    {
        if (!m_isRunning.exchange(false, std::memory_order_relaxed))
            return;

        // Natychmiastowe anulowanie oczekujacych zadan przed join watku (eliminuje 5s timeout)
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            for (auto& task : m_pendingTasks)
            {
                if (task)
                {
                    try {
                        task->promiseResult.set_value("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32000,\"message\":\"Server stopped\"}}");
                    } catch (...) {}
                }
            }
            m_pendingTasks.clear();
        }

        // Budzimy watek I/O przez probe polaczenia z potokiem
        HANDLE hClient = CreateFileA(
            m_pipeName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );
        if (hClient != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hClient);
        }

        if (m_ioThread.joinable())
        {
            m_ioThread.join();
        }
    }

    void TestHarnessServer::ProcessMainThreadQueue()
    {
        std::vector<std::shared_ptr<HarnessTask>> currentTasks;
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (m_pendingTasks.empty())
                return;
            currentTasks.swap(m_pendingTasks);
        }

        for (auto& task : currentTasks)
        {
            if (!task)
                continue;

            std::string response = ExecuteCommand(task->rawJson);
            try {
                task->promiseResult.set_value(std::move(response));
            } catch (...) {}
        }
    }

    void TestHarnessServer::ServerThreadProc()
    {
        while (m_isRunning.load(std::memory_order_relaxed))
        {
            HANDLE hPipe = CreateNamedPipeA(
                m_pipeName.c_str(),
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                1,
                65536,
                65536,
                0,
                nullptr
            );

            if (hPipe == INVALID_HANDLE_VALUE)
            {
                Sleep(200);
                continue;
            }

            BOOL connected = ConnectNamedPipe(hPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

            if (connected && m_isRunning.load(std::memory_order_relaxed))
            {
                HandleClientConnection(hPipe);
            }

            DisconnectNamedPipe(hPipe);
            CloseHandle(hPipe);
        }
    }

    void TestHarnessServer::HandleClientConnection(HANDLE hPipe)
    {
        std::vector<char> buffer(65536);
        DWORD bytesRead = 0;

        while (m_isRunning.load(std::memory_order_relaxed))
        {
            BOOL success = ReadFile(hPipe, buffer.data(), static_cast<DWORD>(buffer.size() - 1), &bytesRead, nullptr);
            if (!success || bytesRead == 0)
            {
                break;
            }

            buffer[bytesRead] = '\0';
            std::string requestStr(buffer.data(), bytesRead);

            auto pTask = std::make_shared<HarnessTask>();
            pTask->rawJson = std::move(requestStr);
            auto fut = pTask->promiseResult.get_future();

            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                m_pendingTasks.push_back(pTask);
            }

            // Oczekiwanie na przetworzenie przez glowny watek gry (z limitem 5 sekund)
            std::string responseStr;
            if (fut.wait_for(std::chrono::seconds(5)) == std::future_status::ready)
            {
                responseStr = fut.get();
            }
            else
            {
                responseStr = "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32000,\"message\":\"Main thread execution timed out\"}}";
            }

            DWORD bytesWritten = 0;
            WriteFile(hPipe, responseStr.data(), static_cast<DWORD>(responseStr.size()), &bytesWritten, nullptr);
            FlushFileBuffers(hPipe);
        }
    }

    std::string TestHarnessServer::ExecuteCommand(const std::string& requestJson)
    {
        rapidjson::Document doc;
        doc.Parse(requestJson.c_str());

        if (doc.HasParseError())
        {
            return "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32700,\"message\":\"Parse error\"}}";
        }

        std::string id = "null";
        if (doc.HasMember("id"))
        {
            if (doc["id"].IsInt())
                id = std::to_string(doc["id"].GetInt());
            else if (doc["id"].IsString())
                id = std::format("\"{}\"", doc["id"].GetString());
        }

        std::string method;
        if (doc.HasMember("method") && doc["method"].IsString())
        {
            method = doc["method"].GetString();
        }
        else if (doc.HasMember("cmd") && doc["cmd"].IsString())
        {
            method = doc["cmd"].GetString();
        }
        else
        {
            return std::format("{{\"jsonrpc\":\"2.0\",\"id\":{},\"error\":{{\"code\":-32600,\"message\":\"Invalid Request\"}}}}", id);
        }

        if (method == "get_state")
        {
            return HandleGetState(id);
        }
        else if (method == "tick_frame")
        {
            uint32_t count = 1;
            float deltaTime = 1.0f / 60.0f;

            if (doc.HasMember("params") && doc["params"].IsObject())
            {
                const auto& p = doc["params"];
                if (p.HasMember("count") && p["count"].IsUint())
                    count = p["count"].GetUint();
                else if (p.HasMember("frames") && p["frames"].IsUint())
                    count = p["frames"].GetUint();

                if (p.HasMember("delta_time") && p["delta_time"].IsNumber())
                    deltaTime = static_cast<float>(p["delta_time"].GetDouble());
            }

            return HandleTickFrame(id, count, deltaTime);
        }
        else if (method == "inject_packet")
        {
            uint32_t header = 0;
            std::string hexData;

            if (doc.HasMember("params") && doc["params"].IsObject())
            {
                const auto& p = doc["params"];
                if (p.HasMember("header") && p["header"].IsUint())
                    header = p["header"].GetUint();
                else if (p.HasMember("opcode") && p["opcode"].IsUint())
                    header = p["opcode"].GetUint();

                if (p.HasMember("data") && p["data"].IsString())
                    hexData = p["data"].GetString();
            }

            return HandleInjectPacket(id, header, hexData);
        }
        else if (method == "audit_state")
        {
            return HandleAuditState(id);
        }
        else
        {
            return std::format("{{\"jsonrpc\":\"2.0\",\"id\":{},\"error\":{{\"code\":-32601,\"message\":\"Method not found: {}\"}}}}", id, method);
        }
    }

    std::string TestHarnessServer::HandleGetState(const std::string& id)
    {
        if (!CPythonPlayer::InstancePtr() || !CPythonCharacterManager::InstancePtr())
        {
            return std::format("{{\"jsonrpc\":\"2.0\",\"id\":{},\"result\":{{\"status\":\"not_initialized\"}}}}", id);
        }

        CPythonPlayer& rkPlayer = CPythonPlayer::Instance();
        CPythonCharacterManager& rkChrMgr = CPythonCharacterManager::Instance();
        CInstanceBase* pMainActor = rkChrMgr.GetMainActorPtr();

        DWORD dwMainVID = pMainActor ? pMainActor->GetVirtualID() : rkPlayer.GetMainCharacterIndex();
        std::string name = pMainActor ? pMainActor->GetNameString() : "";
        DWORD dwRace = pMainActor ? pMainActor->GetRace() : 0;
        float fRot = pMainActor ? pMainActor->GetRotation() : 0.0f;

        TPixelPosition pos{0, 0, 0};
        rkPlayer.NEW_GetMainActorPosition(&pos);

        int64_t iHP = rkPlayer.GetStatus(POINT_HP);
        int64_t iMaxHP = rkPlayer.GetStatus(POINT_MAX_HP);
        int64_t iSP = rkPlayer.GetStatus(POINT_SP);
        int64_t iMaxSP = rkPlayer.GetStatus(POINT_MAX_SP);
        int64_t iLevel = rkPlayer.GetStatus(POINT_LEVEL);
        int64_t iGold = rkPlayer.GetStatus(POINT_GOLD);
        DWORD dwTargetVID = rkPlayer.GetTargetVID();

        // Kontrolery gracza
        const auto& moveCtrl = rkPlayer.GetMovementController();
        const auto& combatCtrl = rkPlayer.GetCombatController();
        const auto& targetCtrl = rkPlayer.GetTargetController();
        const auto& itemCtrl = rkPlayer.GetItemController();

        const auto& hpPot = itemCtrl.GetAutoPotionInfo(PlayerControllers::PlayerItemController::AUTO_POTION_TYPE_HP);
        const auto& spPot = itemCtrl.GetAutoPotionInfo(PlayerControllers::PlayerItemController::AUTO_POTION_TYPE_SP);

        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

        writer.StartObject();
        writer.Key("jsonrpc"); writer.String("2.0");
        writer.Key("id");
        if (id.starts_with("\"") && id.ends_with("\""))
            writer.String(id.substr(1, id.size() - 2).c_str());
        else if (id != "null")
            writer.Int(std::stoi(id));
        else
            writer.Null();

        writer.Key("result");
        writer.StartObject();

        // 1. Sekcja gracza
        writer.Key("player");
        writer.StartObject();
        writer.Key("vid"); writer.Uint(dwMainVID);
        writer.Key("name"); writer.String(name.c_str());
        writer.Key("race"); writer.Uint(dwRace);
        writer.Key("position");
        writer.StartObject();
        writer.Key("x"); writer.Double(pos.x);
        writer.Key("y"); writer.Double(pos.y);
        writer.Key("z"); writer.Double(pos.z);
        writer.EndObject();
        writer.Key("rotation"); writer.Double(fRot);
        writer.Key("hp"); writer.Int64(iHP);
        writer.Key("max_hp"); writer.Int64(iMaxHP);
        writer.Key("sp"); writer.Int64(iSP);
        writer.Key("max_sp"); writer.Int64(iMaxSP);
        writer.Key("level"); writer.Int64(iLevel);
        writer.Key("gold"); writer.Int64(iGold);
        writer.Key("target_vid"); writer.Uint(dwTargetVID);
        writer.EndObject();

        // 2. Sekcja kontrolerow (Movement, Combat, Target, Item)
        writer.Key("controllers");
        writer.StartObject();

        // Movement
        writer.Key("movement");
        writer.StartObject();
        writer.Key("can_move"); writer.Bool(moveCtrl.CanMove());
        writer.Key("is_attack_key_pressed"); writer.Bool(moveCtrl.IsAttackKeyState());
        writer.Key("is_dest_position"); writer.Bool(moveCtrl.IsDestPosition());
        writer.Key("dest_pos_x"); writer.Int(moveCtrl.GetDestPosX());
        writer.Key("dest_pos_y"); writer.Int(moveCtrl.GetDestPosY());
        writer.Key("current_stamina"); writer.Double(moveCtrl.GetCurrentStamina());
        writer.Key("is_consuming_stamina"); writer.Bool(moveCtrl.IsConsumingStamina());
        writer.Key("camera_rotation_enabled"); writer.Bool(moveCtrl.IsCameraRotationEnabled());
        writer.EndObject();

        // Combat
        writer.Key("combat");
        writer.StartObject();
        writer.Key("can_attack"); writer.Bool(combatCtrl.CanAttack());
        writer.Key("race"); writer.Uint(combatCtrl.GetRace());
        writer.Key("min_atk"); writer.Uint(combatCtrl.GetMinAtk());
        writer.Key("max_atk"); writer.Uint(combatCtrl.GetMaxAtk());
        writer.Key("auto_attack_target_vid"); writer.Uint(combatCtrl.GetAutoAttackTargetVID());
        writer.Key("is_processing_emotion"); writer.Bool(combatCtrl.IsProcessingEmotion() != 0);
        writer.Key("combo_old"); writer.Uint(combatCtrl.GetComboOld());
        writer.EndObject();

        // Target
        writer.Key("target");
        writer.StartObject();
        writer.Key("target_vid"); writer.Uint(targetCtrl.GetTargetVID());
        writer.Key("picked_actor_id"); writer.Uint(targetCtrl.GetPickedActorID());
        writer.Key("picked_item_id"); writer.Uint(targetCtrl.GetPickedItemID());
        writer.Key("can_change_target"); writer.Bool(targetCtrl.CanChangeTarget());
        writer.Key("target_end_time"); writer.Uint(targetCtrl.GetTargetEndTime());
        writer.EndObject();

        // Item
        writer.Key("item");
        writer.StartObject();
        writer.Key("pickable_distance"); writer.Uint(itemCtrl.GetPickableDistance());
        writer.Key("auto_potion_hp");
        writer.StartObject();
        writer.Key("activated"); writer.Bool(hpPot.bActivated);
        writer.Key("current_amount"); writer.Int64(hpPot.currentAmount);
        writer.Key("total_amount"); writer.Int64(hpPot.totalAmount);
        writer.EndObject();
        writer.Key("auto_potion_sp");
        writer.StartObject();
        writer.Key("activated"); writer.Bool(spPot.bActivated);
        writer.Key("current_amount"); writer.Int64(spPot.currentAmount);
        writer.Key("total_amount"); writer.Int64(spPot.totalAmount);
        writer.EndObject();
        writer.EndObject();

        writer.EndObject(); // controllers

        // 3. Sekcja sieci
        writer.Key("network");
        writer.StartObject();
        writer.Key("phase"); writer.String(CPythonNetworkStream::Instance().GetPhase().c_str());
        writer.EndObject();

        writer.EndObject(); // result
        writer.EndObject(); // root

        return sb.GetString();
    }

    std::string TestHarnessServer::HandleTickFrame(const std::string& id, uint32_t count, float deltaTime)
    {
        DeterministicTickController::Instance().Step(count, deltaTime);
        uint64_t currentTick = DeterministicTickController::Instance().GetCurrentTick();

        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

        writer.StartObject();
        writer.Key("jsonrpc"); writer.String("2.0");
        writer.Key("id");
        if (id.starts_with("\"") && id.ends_with("\""))
            writer.String(id.substr(1, id.size() - 2).c_str());
        else if (id != "null")
            writer.Int(std::stoi(id));
        else
            writer.Null();

        writer.Key("result");
        writer.StartObject();
        writer.Key("success"); writer.Bool(true);
        writer.Key("stepped_frames"); writer.Uint(count);
        writer.Key("delta_time"); writer.Double(deltaTime);
        writer.Key("current_tick"); writer.Uint64(currentTick);
        writer.EndObject();

        writer.EndObject();

        return sb.GetString();
    }

    std::string TestHarnessServer::HandleInjectPacket(const std::string& id, uint32_t header, const std::string& hexData)
    {
        std::vector<uint8_t> bytes = HexStringToBytes(hexData);
        if (bytes.empty())
        {
            bytes.resize(sizeof(uint64_t) * 8, 0);
            bytes[0] = static_cast<uint8_t>(header & 0xFF);
        }

        bool success = false;
        if (header <= 0xFF)
        {
            success = UserInterface::Network::Routers::PhaseGamePacketDispatcher::Instance().DispatchPacket(
                static_cast<uint8_t>(header), bytes.data());
        }
        else
        {
            success = UserInterface::Network::Routers::PhaseGamePacketDispatcher::Instance().DispatchPacket(
                static_cast<uint16_t>(header), bytes.data());
        }

        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

        writer.StartObject();
        writer.Key("jsonrpc"); writer.String("2.0");
        writer.Key("id");
        if (id.starts_with("\"") && id.ends_with("\""))
            writer.String(id.substr(1, id.size() - 2).c_str());
        else if (id != "null")
            writer.Int(std::stoi(id));
        else
            writer.Null();

        writer.Key("result");
        writer.StartObject();
        writer.Key("success"); writer.Bool(success);
        writer.Key("header"); writer.Uint(header);
        writer.Key("bytes_count"); writer.Uint(static_cast<uint32_t>(bytes.size()));
        writer.EndObject();

        writer.EndObject();

        return sb.GetString();
    }

    std::string TestHarnessServer::HandleAuditState(const std::string& id)
    {
        if (!CPythonPlayer::InstancePtr())
        {
            return std::format("{{\"jsonrpc\":\"2.0\",\"id\":{},\"result\":{{\"verdict\":\"NOT_INITIALIZED\",\"discrepancies\":[]}}}}", id);
        }

        CPythonPlayer& rkPlayer = CPythonPlayer::Instance();

        // Pobranie stanu C++
        const DWORD cpp_vid = rkPlayer.GetMainCharacterIndex();
        const int64_t cpp_hp = rkPlayer.GetStatus(POINT_HP);
        const int64_t cpp_max_hp = rkPlayer.GetStatus(POINT_MAX_HP);
        const int64_t cpp_sp = rkPlayer.GetStatus(POINT_SP);
        const int64_t cpp_max_sp = rkPlayer.GetStatus(POINT_MAX_SP);
        const int64_t cpp_level = rkPlayer.GetStatus(POINT_LEVEL);
        const DWORD cpp_target_vid = rkPlayer.GetTargetVID();

        TPixelPosition cppPos{0, 0, 0};
        rkPlayer.NEW_GetMainActorPosition(&cppPos);

        // Pobranie stanu Pythona jesli interpreter jest aktywny
        bool pyAvailable = false;
        long py_vid = 0;
        long py_target_vid = 0;
        long py_hp = 0;
        long py_max_hp = 0;
        long py_sp = 0;
        long py_max_sp = 0;
        long py_level = 0;
        float py_x = 0.0f, py_y = 0.0f, py_z = 0.0f;

        if (Py_IsInitialized())
        {
            PyObject* pPlayerMod = PyImport_ImportModule("player");
            if (pPlayerMod)
            {
                pyAvailable = true;

                PyObject* pRes = PyObject_CallMethod(pPlayerMod, (char*)"GetMainCharacterIndex", nullptr);
                if (pRes) { py_vid = PyLong_AsLong(pRes); Py_DECREF(pRes); }

                pRes = PyObject_CallMethod(pPlayerMod, (char*)"GetTargetVID", nullptr);
                if (pRes) { py_target_vid = PyLong_AsLong(pRes); Py_DECREF(pRes); }

                pRes = PyObject_CallMethod(pPlayerMod, (char*)"GetMainCharacterPosition", nullptr);
                if (pRes && PyTuple_Check(pRes) && PyTuple_Size(pRes) >= 2)
                {
                    py_x = static_cast<float>(PyFloat_AsDouble(PyTuple_GetItem(pRes, 0)));
                    py_y = static_cast<float>(PyFloat_AsDouble(PyTuple_GetItem(pRes, 1)));
                    if (PyTuple_Size(pRes) >= 3)
                        py_z = static_cast<float>(PyFloat_AsDouble(PyTuple_GetItem(pRes, 2)));
                    Py_DECREF(pRes);
                }

                pRes = PyObject_CallMethod(pPlayerMod, (char*)"GetStatus", (char*)"i", POINT_HP);
                if (pRes) { py_hp = PyLong_AsLong(pRes); Py_DECREF(pRes); }

                pRes = PyObject_CallMethod(pPlayerMod, (char*)"GetStatus", (char*)"i", POINT_MAX_HP);
                if (pRes) { py_max_hp = PyLong_AsLong(pRes); Py_DECREF(pRes); }

                pRes = PyObject_CallMethod(pPlayerMod, (char*)"GetStatus", (char*)"i", POINT_SP);
                if (pRes) { py_sp = PyLong_AsLong(pRes); Py_DECREF(pRes); }

                pRes = PyObject_CallMethod(pPlayerMod, (char*)"GetStatus", (char*)"i", POINT_MAX_SP);
                if (pRes) { py_max_sp = PyLong_AsLong(pRes); Py_DECREF(pRes); }

                pRes = PyObject_CallMethod(pPlayerMod, (char*)"GetStatus", (char*)"i", POINT_LEVEL);
                if (pRes) { py_level = PyLong_AsLong(pRes); Py_DECREF(pRes); }

                Py_DECREF(pPlayerMod);
            }
            if (PyErr_Occurred()) PyErr_Clear();
        }

        std::vector<std::string> discrepancies;
        if (pyAvailable)
        {
            if (static_cast<long>(cpp_vid) != py_vid) discrepancies.push_back("vid");
            if (static_cast<long>(cpp_target_vid) != py_target_vid) discrepancies.push_back("target_vid");
            if (cpp_hp != py_hp) discrepancies.push_back("hp");
            if (cpp_max_hp != py_max_hp) discrepancies.push_back("max_hp");
            if (cpp_sp != py_sp) discrepancies.push_back("sp");
            if (cpp_max_sp != py_max_sp) discrepancies.push_back("max_sp");
            if (cpp_level != py_level) discrepancies.push_back("level");
            if (std::abs(cppPos.x - py_x) > 0.1f || std::abs(cppPos.y - py_y) > 0.1f) discrepancies.push_back("position");
        }

        bool isSynchronized = pyAvailable && discrepancies.empty();

        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

        writer.StartObject();
        writer.Key("jsonrpc"); writer.String("2.0");
        writer.Key("id");
        if (id.starts_with("\"") && id.ends_with("\""))
            writer.String(id.substr(1, id.size() - 2).c_str());
        else if (id != "null")
            writer.Int(std::stoi(id));
        else
            writer.Null();

        writer.Key("result");
        writer.StartObject();
        writer.Key("is_synchronized"); writer.Bool(isSynchronized);
        writer.Key("discrepancies");
        writer.StartArray();
        for (const auto& item : discrepancies)
        {
            writer.String(item.c_str());
        }
        writer.EndArray();

        // cpp_state
        writer.Key("cpp_state");
        writer.StartObject();
        writer.Key("vid"); writer.Uint(cpp_vid);
        writer.Key("hp"); writer.Int64(cpp_hp);
        writer.Key("max_hp"); writer.Int64(cpp_max_hp);
        writer.Key("sp"); writer.Int64(cpp_sp);
        writer.Key("max_sp"); writer.Int64(cpp_max_sp);
        writer.Key("level"); writer.Int64(cpp_level);
        writer.Key("target_vid"); writer.Uint(cpp_target_vid);
        writer.Key("position");
        writer.StartObject();
        writer.Key("x"); writer.Double(cppPos.x);
        writer.Key("y"); writer.Double(cppPos.y);
        writer.Key("z"); writer.Double(cppPos.z);
        writer.EndObject();
        writer.EndObject();

        // python_state
        writer.Key("python_state");
        writer.StartObject();
        writer.Key("available"); writer.Bool(pyAvailable);
        if (pyAvailable)
        {
            writer.Key("vid"); writer.Int64(py_vid);
            writer.Key("hp"); writer.Int64(py_hp);
            writer.Key("max_hp"); writer.Int64(py_max_hp);
            writer.Key("sp"); writer.Int64(py_sp);
            writer.Key("max_sp"); writer.Int64(py_max_sp);
            writer.Key("level"); writer.Int64(py_level);
            writer.Key("target_vid"); writer.Int64(py_target_vid);
            writer.Key("position");
            writer.StartObject();
            writer.Key("x"); writer.Double(py_x);
            writer.Key("y"); writer.Double(py_y);
            writer.Key("z"); writer.Double(py_z);
            writer.EndObject();
        }
        writer.EndObject();

        writer.EndObject(); // result
        writer.EndObject(); // root

        return sb.GetString();
    }
}
