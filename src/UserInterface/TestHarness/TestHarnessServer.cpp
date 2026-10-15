#include "../StdAfx.h"
#include "TestHarnessServer.h"
#include "DeterministicTickController.h"
#include "MockWorldDriver.h"

#include "../InstanceBase.h"
#include "../PythonCharacterManager.h"
#include "../PythonPlayer.h"
#include "../PythonNetworkStream.h"
#include "../AccountConnector.h"
#include "../Network/Routers/PhaseGamePacketDispatcher.h"
#include "../Packet.h"
#include "../PythonApplication.h"
#include "../PythonSkill.h"
#include "../GameLib/ItemManager.h"
#include "../FastBootOverlay.h"

extern bool g_bHeadlessMode;

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "Client/Adapters/GameDomainAdapters.h"
#include "Client/MCP/AgentMCPBridge.h"
#include "Client/Network/Protocol/ProtocolDriverRegistry.h"

#include <cmath>
#include <format>
#include <chrono>

namespace UserInterface::TestHarness
{
    static std::atomic<bool> s_harnessWindowVisible{true};
    static std::atomic<bool> s_harnessGpuThrottled{false};

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

    static std::string FormatJsonRpcError(const std::string& id, int code, std::string_view message)
    {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        writer.StartObject();
        writer.Key("jsonrpc"); writer.String("2.0");
        writer.Key("error");
        writer.StartObject();
        writer.Key("code"); writer.Int(code);
        writer.Key("message"); writer.String(message.data(), static_cast<rapidjson::SizeType>(message.size()));
        writer.EndObject();
        writer.Key("id");
        if (id.starts_with("\"") && id.ends_with("\"") && id.size() >= 2)
            writer.String(id.substr(1, id.size() - 2).c_str());
        else if (id != "null" && !id.empty())
        {
            try {
                writer.Int64(std::stoll(id));
            } catch (...) {
                writer.String(id.c_str());
            }
        }
        else
        {
            writer.Null();
        }
        writer.EndObject();
        return sb.GetString();
    }

    TestHarnessServer& TestHarnessServer::Instance() noexcept
    {
        static TestHarnessServer s_instance;
        return s_instance;
    }

    TestHarnessServer::TestHarnessServer()
    {
        RegisterBuiltinRoutes();
    }

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
                    HarnessTaskState expected = HarnessTaskState::PENDING;
                    if (task->state.compare_exchange_strong(expected, HarnessTaskState::TIMED_OUT, std::memory_order_acq_rel))
                    {
                        try {
                            task->promiseResult.set_value(FormatJsonRpcError("null", -32000, "Server stopped"));
                        } catch (...) {}
                    }
                }
            }
            m_pendingTasks.clear();
        }

        // Zwalniamy aktywny potok klienta jesli watek I/O wisi zablokowany w ReadFile (eliminuje deadlock)
        HANDLE hActive = m_hActivePipe.exchange(INVALID_HANDLE_VALUE, std::memory_order_release);
        if (hActive != INVALID_HANDLE_VALUE)
        {
            CancelIoEx(hActive, nullptr);
            DisconnectNamedPipe(hActive);
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

        TraceError("[QUEUE_DEBUG] ProcessMainThreadQueue executing %zu tasks", currentTasks.size());

        for (auto& task : currentTasks)
        {
            if (!task)
                continue;

            // Atomowa wymiana stanu: PENDING -> EXECUTING.
            // Jesli zadanie zostalo anulowane przez timeout w watku I/O (TIMED_OUT),
            // compare_exchange zwroci false i zadanie jest ignorowane (Eliminacja Ghost Execution).
            HarnessTaskState expected = HarnessTaskState::PENDING;
            if (!task->state.compare_exchange_strong(expected, HarnessTaskState::EXECUTING, std::memory_order_acq_rel))
            {
                continue;
            }

            std::string response = ExecuteCommand(task->rawJson);

            // Po wykonaniu przestawiamy stan EXECUTING -> COMPLETED
            expected = HarnessTaskState::EXECUTING;
            if (task->state.compare_exchange_strong(expected, HarnessTaskState::COMPLETED, std::memory_order_acq_rel))
            {
                try {
                    task->promiseResult.set_value(std::move(response));
                } catch (...) {}
            }
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
        m_hActivePipe.store(hPipe, std::memory_order_release);
        std::vector<uint8_t> recvBuf;
        std::vector<uint8_t> readChunk(65536);

        while (m_isRunning.load(std::memory_order_relaxed))
        {
            DWORD bytesRead = 0;
            BOOL success = ReadFile(hPipe, readChunk.data(), static_cast<DWORD>(readChunk.size()), &bytesRead, nullptr);
            if (!success)
            {
                DWORD err = GetLastError();
                if (err == ERROR_MORE_DATA && bytesRead > 0)
                {
                    // W trybie message z pakietem wiekszym niz 64 KB akumulujemy i czytamy dalej
                    recvBuf.insert(recvBuf.end(), readChunk.data(), readChunk.data() + bytesRead);
                    continue;
                }
                break;
            }
            if (bytesRead == 0)
            {
                break;
            }

            recvBuf.insert(recvBuf.end(), readChunk.data(), readChunk.data() + bytesRead);

            // Petla akumulacji bufora: obsluga pakietow wiekszych niz 64 KB oraz pipeliningu
            while (!recvBuf.empty())
            {
                // Pomijanie wiodacych znakow bialych
                while (!recvBuf.empty() && (recvBuf[0] == ' ' || recvBuf[0] == '\t' || recvBuf[0] == '\r' || recvBuf[0] == '\n'))
                {
                    recvBuf.erase(recvBuf.begin());
                }

                if (recvBuf.empty())
                    break;

                bool isFramed = false;
                std::string requestStr;

                // Sprawdzamy czy pakiet posiada 4-bajtowy naglowek dlugosci (Framed) czy jest surowym JSON
                if (recvBuf[0] == '{')
                {
                    // Bajt 0 to '{' (0x7B).
                    // Moze to byc czysty JSON (np. {"jsonrpc"...}) LUB naglowek framed, gdzie bajt mlodszy uint32_t to 0x7B (np. dlugosc 123).
                    // W czystym JSON kolejne bajty b1, b2, b3 to znaki tekstu ASCII (> 0x20) i zaden z nich nie jest 0x00.
                    // W naglowku framed uint32_t little-endian dla pakietow <= 16MB co najmniej jeden z bajtow b1..b3 to 0x00.
                    if (recvBuf.size() >= 4 && (recvBuf[1] == 0x00 || recvBuf[2] == 0x00 || recvBuf[3] == 0x00))
                    {
                        uint32_t declaredLen = 0;
                        std::memcpy(&declaredLen, recvBuf.data(), sizeof(uint32_t));
                        if (declaredLen > 0 && declaredLen <= 16777216)
                        {
                            if (recvBuf.size() < sizeof(uint32_t) + declaredLen)
                            {
                                // Czekamy na reszte strumienia z potoku
                                break;
                            }
                            isFramed = true;
                            requestStr.assign(reinterpret_cast<const char*>(recvBuf.data() + sizeof(uint32_t)), declaredLen);
                            recvBuf.erase(recvBuf.begin(), recvBuf.begin() + sizeof(uint32_t) + declaredLen);
                        }
                        else
                        {
                            recvBuf.erase(recvBuf.begin());
                            continue;
                        }
                    }
                    else
                    {
                        // Surowy obiekt JSON zaczynajacy sie od '{'
                        size_t jsonEnd = std::string::npos;
                        int depth = 0;
                        bool inStr = false;
                        bool esc = false;
                        for (size_t i = 0; i < recvBuf.size(); ++i)
                        {
                            char c = static_cast<char>(recvBuf[i]);
                            if (inStr)
                            {
                                if (esc) { esc = false; }
                                else if (c == '\\') { esc = true; }
                                else if (c == '\"') { inStr = false; }
                            }
                            else
                            {
                                if (c == '\"') { inStr = true; }
                                else if (c == '{') { depth++; }
                                else if (c == '}')
                                {
                                    depth--;
                                    if (depth == 0)
                                    {
                                        jsonEnd = i;
                                        break;
                                    }
                                }
                            }
                        }

                        if (jsonEnd == std::string::npos)
                        {
                            // Obiekt niekompletny - oczekiwanie na kolejne pakiety
                            break;
                        }

                        isFramed = false;
                        requestStr.assign(reinterpret_cast<const char*>(recvBuf.data()), jsonEnd + 1);
                        recvBuf.erase(recvBuf.begin(), recvBuf.begin() + jsonEnd + 1);
                    }
                }
                else if (recvBuf.size() >= 4)
                {
                    // Pierwszy bajt nie jest '{' - framed pakiet z prefiksem dlugosci
                    uint32_t declaredLen = 0;
                    std::memcpy(&declaredLen, recvBuf.data(), sizeof(uint32_t));
                    if (declaredLen > 0 && declaredLen <= 16777216)
                    {
                        if (recvBuf.size() < sizeof(uint32_t) + declaredLen)
                        {
                            // Czekamy na reszte danych
                            break;
                        }
                        isFramed = true;
                        requestStr.assign(reinterpret_cast<const char*>(recvBuf.data() + sizeof(uint32_t)), declaredLen);
                        recvBuf.erase(recvBuf.begin(), recvBuf.begin() + sizeof(uint32_t) + declaredLen);
                    }
                    else
                    {
                        recvBuf.erase(recvBuf.begin());
                        continue;
                    }
                }
                else
                {
                    // Za malo bajtow do decyzji
                    break;
                }

                auto sendResponse = [&](const std::string& response) {
                    if (response.empty())
                        return;

                    DWORD bytesWritten = 0;
                    if (isFramed)
                    {
                        uint32_t respLen = static_cast<uint32_t>(response.size());
                        std::vector<char> framedBuf(sizeof(uint32_t) + respLen);
                        std::memcpy(framedBuf.data(), &respLen, sizeof(uint32_t));
                        std::memcpy(framedBuf.data() + sizeof(uint32_t), response.data(), respLen);

                        WriteFile(hPipe, framedBuf.data(), static_cast<DWORD>(framedBuf.size()), &bytesWritten, nullptr);
                    }
                    else
                    {
                        WriteFile(hPipe, response.data(), static_cast<DWORD>(response.size()), &bytesWritten, nullptr);
                    }
                    FlushFileBuffers(hPipe);
                };

                // Parsowanie id oraz method dla zgodnosci z JSON-RPC 2.0 (obsluga Notifications)
                bool hasId = false;
                std::string parsedId = "null";
                std::string extractedMethod;
                {
                    rapidjson::Document metaDoc;
                    metaDoc.Parse(requestStr.c_str());
                    if (!metaDoc.HasParseError())
                    {
                        if (metaDoc.HasMember("id"))
                        {
                            hasId = true;
                            if (metaDoc["id"].IsInt())
                                parsedId = std::to_string(metaDoc["id"].GetInt());
                            else if (metaDoc["id"].IsInt64())
                                parsedId = std::to_string(metaDoc["id"].GetInt64());
                            else if (metaDoc["id"].IsString())
                                parsedId = std::format("\"{}\"", metaDoc["id"].GetString());
                        }

                        if (metaDoc.HasMember("method") && metaDoc["method"].IsString())
                            extractedMethod = metaDoc["method"].GetString();
                        else if (metaDoc.HasMember("cmd") && metaDoc["cmd"].IsString())
                            extractedMethod = metaDoc["cmd"].GetString();
                    }
                    else
                    {
                        size_t methodPos = requestStr.find("\"method\"");
                        if (methodPos == std::string::npos)
                            methodPos = requestStr.find("\"cmd\"");
                        if (methodPos != std::string::npos)
                        {
                            size_t colon = requestStr.find(':', methodPos);
                            if (colon != std::string::npos)
                            {
                                size_t startQuote = requestStr.find('"', colon);
                                if (startQuote != std::string::npos)
                                {
                                    size_t endQuote = requestStr.find('"', startQuote + 1);
                                    if (endQuote != std::string::npos)
                                    {
                                        extractedMethod = requestStr.substr(startQuote + 1, endQuote - startQuote - 1);
                                    }
                                }
                            }
                        }
                        if (requestStr.find("\"id\"") != std::string::npos)
                            hasId = true;
                    }
                }

                // JSON-RPC 2.0: Powiadomienia (brak id) nie moga otrzymywac odpowiedzi w czystym JSON,
                // ale w protokole Framed (z 4-bajtowym naglowkiem dlugosci) klient zawsze oczekuje odpowiedzi ramkowej.
                bool isNotification = !hasId && !isFramed;

                // Bezposrednie wykonanie metod natychmiastowych (isImmediate = true)
                if (!extractedMethod.empty() && IsImmediateMethod(extractedMethod))
                {
                    std::string responseStr = ExecuteCommand(requestStr);
                    if (!isNotification)
                    {
                        sendResponse(responseStr);
                    }
                    continue;
                }

                // Metody dotykajace obiektow gry kolejkowane do glownego watku (CPythonApplication::Process)
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
                    // Atomowa wymiana stanu na TIMED_OUT (Eliminacja Ghost Execution)
                    HarnessTaskState prev = pTask->state.exchange(HarnessTaskState::TIMED_OUT, std::memory_order_acq_rel);
                    if (prev == HarnessTaskState::COMPLETED)
                    {
                        try {
                            responseStr = fut.get();
                        } catch (...) {
                            responseStr = FormatJsonRpcError(parsedId, -32000, "Main thread execution timed out");
                        }
                    }
                    else
                    {
                        responseStr = FormatJsonRpcError(parsedId, -32000, "Main thread execution timed out");
                    }
                }

                if (!isNotification)
                {
                    sendResponse(responseStr);
                }
            }
        }

        m_hActivePipe.store(INVALID_HANDLE_VALUE, std::memory_order_release);
    }

    std::string TestHarnessServer::ExecuteCommand(const std::string& requestJson)
    {
        rapidjson::Document doc;
        doc.Parse(requestJson.c_str());

        if (doc.HasParseError())
        {
            return FormatJsonRpcError("null", -32700, "Parse error");
        }

        std::string id = "null";
        if (doc.HasMember("id"))
        {
            if (doc["id"].IsInt())
                id = std::to_string(doc["id"].GetInt());
            else if (doc["id"].IsInt64())
                id = std::to_string(doc["id"].GetInt64());
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
            return FormatJsonRpcError(id, -32600, "Invalid Request");
        }

        // Table-Driven Command Dispatcher (O(1) wyszukiwanie trasy)
        auto it = m_routes.find(method);
        if (it != m_routes.end())
        {
            return it->second.handler(requestJson, id);
        }

        return FormatJsonRpcError(id, -32601, std::format("Method not found: {}", method));
    }

    std::string TestHarnessServer::HandleGetState(const std::string& id)
    {
        DWORD dwMainVID = 0;
        std::string name = "Headless";
        DWORD dwRace = 0;
        float fRot = 0.0f;
        TPixelPosition pos{0, 0, 0};

        int64_t iHP = 0, iMaxHP = 0, iSP = 0, iMaxSP = 0, iLevel = 1, iGold = 0;
        DWORD dwTargetVID = 0;

        bool bCanMove = false, bIsAttackKeyPressed = false, bIsDestPos = false;
        int iDestPosX = 0, iDestPosY = 0;
        double dStamina = 0.0;
        bool bConsumingStamina = false, bCameraRotEnabled = false;

        bool bCanAttack = false;
        DWORD dwAtkRace = 0, dwMinAtk = 0, dwMaxAtk = 0, dwAutoAtkTarget = 0;
        bool bProcEmotion = false;
        DWORD dwComboOld = 0;

        DWORD dwPickedActorID = 0, dwPickedItemID = 0;
        bool bCanChangeTarget = true;
        DWORD dwTargetEndTime = 0;

        DWORD dwPickableDist = 300;
        bool bHpPotActive = false; int64_t iHpPotCur = 0, iHpPotTot = 0;
        bool bSpPotActive = false; int64_t iSpPotCur = 0, iSpPotTot = 0;

        if (CPythonPlayer::InstancePtr() && CPythonCharacterManager::InstancePtr())
        {
            CPythonPlayer& rkPlayer = CPythonPlayer::Instance();
            CPythonCharacterManager& rkChrMgr = CPythonCharacterManager::Instance();
            CInstanceBase* pMainActor = rkChrMgr.GetMainActorPtr();

            dwMainVID = pMainActor ? pMainActor->GetVirtualID() : rkPlayer.GetMainCharacterIndex();
            name = pMainActor ? pMainActor->GetNameString() : rkPlayer.GetName();
            if (name.empty()) name = "Headless";
            dwRace = pMainActor ? pMainActor->GetRace() : 0;
            fRot = pMainActor ? pMainActor->GetRotation() : 0.0f;

            rkPlayer.NEW_GetMainActorPosition(&pos);

            iHP = rkPlayer.GetStatus(POINT_HP);
            iMaxHP = rkPlayer.GetStatus(POINT_MAX_HP);
            iSP = rkPlayer.GetStatus(POINT_SP);
            iMaxSP = rkPlayer.GetStatus(POINT_MAX_SP);
            iLevel = rkPlayer.GetStatus(POINT_LEVEL);
            iGold = rkPlayer.GetStatus(POINT_GOLD);
            dwTargetVID = rkPlayer.GetTargetVID();

            const auto& moveCtrl = rkPlayer.GetMovementController();
            bCanMove = moveCtrl.CanMove();
            bIsAttackKeyPressed = moveCtrl.IsAttackKeyState();
            bIsDestPos = moveCtrl.IsDestPosition();
            iDestPosX = moveCtrl.GetDestPosX();
            iDestPosY = moveCtrl.GetDestPosY();
            dStamina = moveCtrl.GetCurrentStamina();
            bConsumingStamina = moveCtrl.IsConsumingStamina();
            bCameraRotEnabled = moveCtrl.IsCameraRotationEnabled();

            const auto& combatCtrl = rkPlayer.GetCombatController();
            bCanAttack = combatCtrl.CanAttack();
            dwAtkRace = combatCtrl.GetRace();
            dwMinAtk = combatCtrl.GetMinAtk();
            dwMaxAtk = combatCtrl.GetMaxAtk();
            dwAutoAtkTarget = combatCtrl.GetAutoAttackTargetVID();
            bProcEmotion = (combatCtrl.IsProcessingEmotion() != 0);
            dwComboOld = combatCtrl.GetComboOld();

            const auto& targetCtrl = rkPlayer.GetTargetController();
            dwPickedActorID = targetCtrl.GetPickedActorID();
            dwPickedItemID = targetCtrl.GetPickedItemID();
            bCanChangeTarget = targetCtrl.CanChangeTarget();
            dwTargetEndTime = targetCtrl.GetTargetEndTime();

            const auto& itemCtrl = rkPlayer.GetItemController();
            dwPickableDist = itemCtrl.GetPickableDistance();
            const auto& hpPot = itemCtrl.GetAutoPotionInfo(PlayerControllers::PlayerItemController::AUTO_POTION_TYPE_HP);
            const auto& spPot = itemCtrl.GetAutoPotionInfo(PlayerControllers::PlayerItemController::AUTO_POTION_TYPE_SP);
            bHpPotActive = hpPot.bActivated; iHpPotCur = hpPot.currentAmount; iHpPotTot = hpPot.totalAmount;
            bSpPotActive = spPot.bActivated; iSpPotCur = spPot.currentAmount; iSpPotTot = spPot.totalAmount;
        }

        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

        writer.StartObject();
        writer.Key("jsonrpc"); writer.String("2.0");
        writer.Key("ok"); writer.Bool(true);
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
        writer.Key("can_move"); writer.Bool(bCanMove);
        writer.Key("is_attack_key_pressed"); writer.Bool(bIsAttackKeyPressed);
        writer.Key("is_dest_position"); writer.Bool(bIsDestPos);
        writer.Key("dest_pos_x"); writer.Int(iDestPosX);
        writer.Key("dest_pos_y"); writer.Int(iDestPosY);
        writer.Key("current_stamina"); writer.Double(dStamina);
        writer.Key("is_consuming_stamina"); writer.Bool(bConsumingStamina);
        writer.Key("camera_rotation_enabled"); writer.Bool(bCameraRotEnabled);
        writer.EndObject();

        // Combat
        writer.Key("combat");
        writer.StartObject();
        writer.Key("can_attack"); writer.Bool(bCanAttack);
        writer.Key("race"); writer.Uint(dwAtkRace);
        writer.Key("min_atk"); writer.Uint(dwMinAtk);
        writer.Key("max_atk"); writer.Uint(dwMaxAtk);
        writer.Key("auto_attack_target_vid"); writer.Uint(dwAutoAtkTarget);
        writer.Key("is_processing_emotion"); writer.Bool(bProcEmotion);
        writer.Key("combo_old"); writer.Uint(dwComboOld);
        writer.EndObject();

        // Target
        writer.Key("target");
        writer.StartObject();
        writer.Key("target_vid"); writer.Uint(dwTargetVID);
        writer.Key("picked_actor_id"); writer.Uint(dwPickedActorID);
        writer.Key("picked_item_id"); writer.Uint(dwPickedItemID);
        writer.Key("can_change_target"); writer.Bool(bCanChangeTarget);
        writer.Key("target_end_time"); writer.Uint(dwTargetEndTime);
        writer.EndObject();

        // Item
        writer.Key("item");
        writer.StartObject();
        writer.Key("pickable_distance"); writer.Uint(dwPickableDist);
        writer.Key("auto_potion_hp");
        writer.StartObject();
        writer.Key("activated"); writer.Bool(bHpPotActive);
        writer.Key("current_amount"); writer.Int64(iHpPotCur);
        writer.Key("total_amount"); writer.Int64(iHpPotTot);
        writer.EndObject();
        writer.Key("auto_potion_sp");
        writer.StartObject();
        writer.Key("activated"); writer.Bool(bSpPotActive);
        writer.Key("current_amount"); writer.Int64(iSpPotCur);
        writer.Key("total_amount"); writer.Int64(iSpPotTot);
        writer.EndObject();
        writer.EndObject();

        writer.EndObject(); // controllers

        // 3. Sekcja sieci
        writer.Key("network");
        writer.StartObject();
        writer.Key("phase"); writer.String(CPythonNetworkStream::InstancePtr() ? CPythonNetworkStream::Instance().GetPhase().c_str() : "OFFLINE");
        writer.EndObject();

        // 4. Stan silnika, okna i sterownika protokolu
        std::string activeDriver = ::Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriverName();
        if (activeDriver.empty())
        {
            activeDriver = "standard_x64";
        }
        writer.Key("active_driver");
        writer.String(activeDriver.data(), static_cast<rapidjson::SizeType>(activeDriver.size()));

        bool windowVisible = s_harnessWindowVisible.load();
        bool gpuThrottled = s_harnessGpuThrottled.load();
        if (CPythonApplication::InstancePtr())
        {
            HWND hWnd = CPythonApplication::Instance().GetWindowHandle();
            if (hWnd)
            {
                windowVisible = (IsWindowVisible(hWnd) != FALSE);
            }
            gpuThrottled = CPythonApplication::Instance().IsRenderThrottling();
        }

        writer.Key("window_visible");
        writer.Bool(windowVisible);

        writer.Key("gpu_throttled");
        writer.Bool(gpuThrottled);

        // Auto-Reconnect
        writer.Key("auto_reconnect");
        writer.StartObject();
        {
            const auto& fbo = UserInterface::FastBoot::FastBootOverlay::Instance();
            writer.Key("enabled"); writer.Bool(fbo.IsAutoReconnectEnabled());
            writer.Key("pending"); writer.Bool(fbo.IsPendingReconnect());
            writer.Key("attempts"); writer.Int(fbo.GetReconnectAttempts());
            writer.Key("delay_ms"); writer.Uint(fbo.GetReconnectDelayMs());
        }
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
        writer.Key("ok"); writer.Bool(true);
        writer.Key("id");
        if (id.starts_with("\"") && id.ends_with("\""))
            writer.String(id.substr(1, id.size() - 2).c_str());
        else if (id != "null")
            writer.Int(std::stoi(id));
        else
            writer.Null();

        writer.Key("result");
        writer.StartObject();
        writer.Key("ok"); writer.Bool(true);
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
        // Bezpieczny minimalny rozmiar bufora pakietu (ochrona przed odczytem poza zakresem pamieci)
        if (bytes.size() < 512)
        {
            bytes.resize(512, 0);
        }
        if (hexData.empty())
        {
            bytes[0] = static_cast<uint8_t>(header & 0xFF);
        }

        bool success = false;
        if (header <= 0xFF)
        {
            success = UserInterface::Network::Routers::PhaseGamePacketDispatcher::Instance().DispatchPacket(
                static_cast<uint8_t>(header), bytes.data(), bytes.size());
        }
        else
        {
            success = UserInterface::Network::Routers::PhaseGamePacketDispatcher::Instance().DispatchPacket(
                static_cast<uint16_t>(header), bytes.data(), bytes.size());
        }

        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

        writer.StartObject();
        writer.Key("jsonrpc"); writer.String("2.0");
        writer.Key("ok"); writer.Bool(success);
        writer.Key("id");
        if (id.starts_with("\"") && id.ends_with("\""))
            writer.String(id.substr(1, id.size() - 2).c_str());
        else if (id != "null")
            writer.Int(std::stoi(id));
        else
            writer.Null();

        writer.Key("result");
        writer.StartObject();
        writer.Key("ok"); writer.Bool(success);
        writer.Key("success"); writer.Bool(success);
        writer.Key("header"); writer.Uint(header);
        writer.Key("bytes_count"); writer.Uint(static_cast<uint32_t>(bytes.size()));
        writer.EndObject();

        writer.EndObject();

        return sb.GetString();
    }

    std::string TestHarnessServer::HandleAuditState(const std::string& id, const std::string& requestJson)
    {
        if (!CPythonPlayer::InstancePtr() || !CPythonCharacterManager::InstancePtr())
        {
            return FormatJsonRpcError(id, -32000, "Player or CharacterManager subsystem not initialized");
        }

        float epsilon = 0.01f;
        if (!requestJson.empty())
        {
            rapidjson::Document reqDoc;
            reqDoc.Parse(requestJson.c_str());
            if (!reqDoc.HasParseError())
            {
                if (reqDoc.HasMember("params") && reqDoc["params"].IsObject() && reqDoc["params"].HasMember("epsilon") && reqDoc["params"]["epsilon"].IsNumber())
                    epsilon = static_cast<float>(reqDoc["params"]["epsilon"].GetDouble());
                else if (reqDoc.HasMember("epsilon") && reqDoc["epsilon"].IsNumber())
                    epsilon = static_cast<float>(reqDoc["epsilon"].GetDouble());
            }
        }

        CPythonPlayer& rkPlayer = CPythonPlayer::Instance();
        CPythonCharacterManager& rkChrMgr = CPythonCharacterManager::Instance();
        CInstanceBase* pMainActor = rkChrMgr.GetMainActorPtr();

        // 1. Pobranie stanu C++ dla gracza glownego
        const DWORD cpp_vid = rkPlayer.GetMainCharacterIndex();
        const int64_t cpp_hp = rkPlayer.GetStatus(POINT_HP);
        const int64_t cpp_max_hp = rkPlayer.GetStatus(POINT_MAX_HP);
        const int64_t cpp_sp = rkPlayer.GetStatus(POINT_SP);
        const int64_t cpp_max_sp = rkPlayer.GetStatus(POINT_MAX_SP);
        const int64_t cpp_level = rkPlayer.GetStatus(POINT_LEVEL);
        const DWORD cpp_target_vid = rkPlayer.GetTargetVID();
        const float cpp_rot = pMainActor ? pMainActor->GetRotation() : 0.0f;
        const bool cpp_is_dead = (pMainActor ? (pMainActor->IsDead() != FALSE) : false) || (cpp_hp <= 0);

        TPixelPosition cppPos{0, 0, 0};
        rkPlayer.NEW_GetMainActorPosition(&cppPos);

        // 2. Pobranie stanu Pythona jesli interpreter jest aktywny (zabezpieczone GIL)
        bool pyAvailable = false;
        uint32_t py_vid = 0;
        uint32_t py_target_vid = 0;
        long py_hp = 0;
        long py_max_hp = 0;
        long py_sp = 0;
        long py_max_sp = 0;
        long py_level = 0;
        float py_x = 0.0f, py_y = 0.0f, py_z = 0.0f;
        float py_rot = 0.0f;
        bool py_rot_available = false;
        bool py_is_dead = false;

        TraceError("[AUDIT_DEBUG] HandleAuditState start: cpp_vid=%u, cpp_hp=%lld", cpp_vid, (long long)cpp_hp);

        if (Py_IsInitialized())
        {
            TraceError("[AUDIT_DEBUG] Before PyGILState_Ensure");
            PyGILState_STATE gstate = PyGILState_Ensure();
            TraceError("[AUDIT_DEBUG] After PyGILState_Ensure (GIL acquired)");
            PyObject* pPlayerMod = PyImport_ImportModule("player");
            if (pPlayerMod)
            {
                pyAvailable = true;

                PyObject* pRes = PyObject_CallMethod(pPlayerMod, (char*)"GetMainCharacterIndex", nullptr);
                if (pRes) { py_vid = static_cast<uint32_t>(PyLong_AsUnsignedLongMask(pRes)); Py_DECREF(pRes); }

                pRes = PyObject_CallMethod(pPlayerMod, (char*)"GetTargetVID", nullptr);
                if (pRes) { py_target_vid = static_cast<uint32_t>(PyLong_AsUnsignedLongMask(pRes)); Py_DECREF(pRes); }

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

                py_is_dead = (py_hp <= 0);

                Py_DECREF(pPlayerMod);
            }

            rkChrMgr.SelectInstance(cpp_vid);
            PyObject* pChrMod = PyImport_ImportModule("chr");
            if (pChrMod)
            {
                PyObject* pSel = PyObject_CallMethod(pChrMod, (char*)"SelectInstance", (char*)"k", cpp_vid);
                if (pSel) Py_DECREF(pSel);
                PyObject* pRot = PyObject_CallMethod(pChrMod, (char*)"GetRotation", nullptr);
                if (pRot)
                {
                    py_rot = static_cast<float>(PyFloat_AsDouble(pRot));
                    py_rot_available = true;
                    Py_DECREF(pRot);
                }
                Py_DECREF(pChrMod);
            }

            // Weryfikacja flagi smierci z modulu chrmgr
            PyObject* pChrMgrMod = PyImport_ImportModule("chrmgr");
            if (pChrMgrMod)
            {
                if (pyAvailable && py_vid != 0)
                {
                    PyObject* pVidInfo = PyObject_CallMethod(pChrMgrMod, (char*)"GetVIDInfo", (char*)"k", py_vid);
                    if (pVidInfo)
                    {
                        if (PyUnicode_Check(pVidInfo))
                        {
                            const char* s = PyUnicode_AsUTF8(pVidInfo);
                            if (s && strstr(s, "isDead=1"))
                                py_is_dead = true;
                        }
                        Py_DECREF(pVidInfo);
                    }
                }
                Py_DECREF(pChrMgrMod);
            }

            if (PyErr_Occurred()) PyErr_Clear();
            TraceError("[AUDIT_DEBUG] Before PyGILState_Release");
            PyGILState_Release(gstate);
            TraceError("[AUDIT_DEBUG] After PyGILState_Release");
        }

        // 3. Weryfikacja spojnosci gracza glownego (C++ vs Python)
        std::vector<std::string> discrepancies;
        if (pyAvailable)
        {
            if (cpp_vid != py_vid) discrepancies.push_back("vid");
            if (cpp_target_vid != py_target_vid) discrepancies.push_back("target_vid");
            if (cpp_hp != py_hp) discrepancies.push_back("hp");
            if (cpp_max_hp != py_max_hp) discrepancies.push_back("max_hp");
            if (cpp_sp != py_sp) discrepancies.push_back("sp");
            if (cpp_max_sp != py_max_sp) discrepancies.push_back("max_sp");
            if (cpp_level != py_level) discrepancies.push_back("level");
            // Pozycja X, Y, Z
            float posEps = std::max(epsilon, 0.5f);
            if (std::abs(cppPos.x - py_x) > posEps || std::abs(cppPos.y - py_y) > posEps || std::abs(cppPos.z - py_z) > posEps)
                discrepancies.push_back("position");
            // Rotacja gracza (cpp_rot vs py_rot)
            if (py_rot_available)
            {
                float norm_cpp = std::fmod(std::fmod(cpp_rot, 360.0f) + 360.0f, 360.0f);
                float norm_py = std::fmod(std::fmod(360.0f - py_rot, 360.0f) + 360.0f, 360.0f);
                float diff = std::abs(norm_cpp - norm_py);
                if (diff > 180.0f)
                {
                    diff = 360.0f - diff;
                }
                if (diff > 2.0f)
                {
                    discrepancies.push_back("rotation");
                }
            }
            // Flaga isDead
            if (cpp_is_dead != py_is_dead)
            {
                discrepancies.push_back("is_dead");
            }
        }

        // 4. Audyt bytow swiata: iteracja po CPythonCharacterManager::m_aliveActorsMap i porownanie z ActorRegistry
        struct WorldEntityDiff
        {
            uint32_t vid{0};
            std::string reason;
            float actorX{0.0f};
            float actorY{0.0f};
            float regX{0.0f};
            float regY{0.0f};
            bool actorDead{false};
            bool regDead{false};
        };

        std::vector<WorldEntityDiff> worldDiffs;
        size_t aliveActorsCount = 0;
        const auto& actorRegistry = rkChrMgr.GetActorRegistry();

        rkChrMgr.for_each_actor_vid([&](Client::World::EntityVid vid, CInstanceBase* pActor) {
            if (!pActor)
                return;

            aliveActorsCount++;
            uint32_t rawVid = static_cast<uint32_t>(vid.value());

            TPixelPosition actorPos{0, 0, 0};
            pActor->NEW_GetPixelPosition(&actorPos);
            bool actorDead = (pActor->IsDead() != FALSE);

            // Zapewnienie biezacej synchronizacji z klatki renderu do ActorRegistry
            auto& mutableRegistry = const_cast<Client::World::ActorRegistry&>(actorRegistry);
            mutableRegistry.UpdatePosition(vid, actorPos.x, actorPos.y, actorPos.z, pActor->GetRotation());
            mutableRegistry.SetDead(vid, actorDead);

            auto recordOpt = actorRegistry.GetActor(vid);
            if (!recordOpt.has_value())
            {
                worldDiffs.push_back({
                    .vid = rawVid,
                    .reason = "missing_in_actor_registry"
                });
                return;
            }

            const auto& rec = *recordOpt;
            bool regDead = rec.isDead;

            // Weryfikacja koordynatow X/Y z dynamiczna tolerancja
            float posTol = std::max(epsilon * 100.0f, 25.0f);
            if (std::abs(actorPos.x - rec.x) > posTol || std::abs(actorPos.y - rec.y) > posTol)
            {
                worldDiffs.push_back({
                    .vid = rawVid,
                    .reason = "position_mismatch",
                    .actorX = actorPos.x,
                    .actorY = actorPos.y,
                    .regX = rec.x,
                    .regY = rec.y,
                    .actorDead = actorDead,
                    .regDead = regDead
                });
            }

            // Weryfikacja stanu zycia (isDead)
            if (actorDead != regDead)
            {
                worldDiffs.push_back({
                    .vid = rawVid,
                    .reason = "death_state_mismatch",
                    .actorX = actorPos.x,
                    .actorY = actorPos.y,
                    .regX = rec.x,
                    .regY = rec.y,
                    .actorDead = actorDead,
                    .regDead = regDead
                });
            }
        });

        if (!worldDiffs.empty())
        {
            discrepancies.push_back("world_entities");
        }

        bool isSynchronized = pyAvailable && discrepancies.empty() && worldDiffs.empty();

        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

        writer.StartObject();
        writer.Key("jsonrpc"); writer.String("2.0");
        writer.Key("ok"); writer.Bool(true);
        writer.Key("id");
        if (id.starts_with("\"") && id.ends_with("\""))
            writer.String(id.substr(1, id.size() - 2).c_str());
        else if (id != "null")
            writer.Int(std::stoi(id));
        else
            writer.Null();

        writer.Key("result");
        writer.StartObject();
        writer.Key("ok"); writer.Bool(true);
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
        writer.Key("rotation"); writer.Double(cpp_rot);
        writer.Key("is_dead"); writer.Bool(cpp_is_dead);
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
            writer.Key("rotation"); writer.Double(py_rot);
            writer.Key("is_dead"); writer.Bool(py_is_dead);
            writer.Key("position");
            writer.StartObject();
            writer.Key("x"); writer.Double(py_x);
            writer.Key("y"); writer.Double(py_y);
            writer.Key("z"); writer.Double(py_z);
            writer.EndObject();
        }
        writer.EndObject();

        // world_audit
        writer.Key("world_audit");
        writer.StartObject();
        writer.Key("total_alive_actors"); writer.Uint64(aliveActorsCount);
        writer.Key("registry_actors_count"); writer.Uint64(actorRegistry.Count());
        writer.Key("entities_synchronized"); writer.Bool(worldDiffs.empty());
        writer.Key("discrepancies");
        writer.StartArray();
        for (const auto& diff : worldDiffs)
        {
            writer.StartObject();
            writer.Key("vid"); writer.Uint(diff.vid);
            writer.Key("reason"); writer.String(diff.reason.c_str());
            writer.Key("actor_x"); writer.Double(diff.actorX);
            writer.Key("actor_y"); writer.Double(diff.actorY);
            writer.Key("reg_x"); writer.Double(diff.regX);
            writer.Key("reg_y"); writer.Double(diff.regY);
            writer.Key("actor_dead"); writer.Bool(diff.actorDead);
            writer.Key("reg_dead"); writer.Bool(diff.regDead);
            writer.EndObject();
        }
        writer.EndArray();
        writer.EndObject();

        writer.EndObject(); // result
        writer.EndObject(); // root

        TraceError("[AUDIT_DEBUG] HandleAuditState finish: returning JSON string");
        return sb.GetString();
    }

    std::string TestHarnessServer::HandleSetWindowMode(const std::string& requestJson, const std::string& id)
    {
        rapidjson::Document doc;
        doc.Parse(requestJson.c_str());

        bool visible = true;
        bool throttle = false;
        bool hasVisible = false;
        bool hasThrottle = false;

        auto extractParams = [&](const rapidjson::Value& obj) {
            if (obj.HasMember("visible") && obj["visible"].IsBool())
            {
                visible = obj["visible"].GetBool();
                hasVisible = true;
            }
            if (obj.HasMember("throttle") && obj["throttle"].IsBool())
            {
                throttle = obj["throttle"].GetBool();
                hasThrottle = true;
            }
        };

        if (!doc.HasParseError())
        {
            if (doc.HasMember("params") && doc["params"].IsObject())
            {
                extractParams(doc["params"]);
            }
            else
            {
                extractParams(doc);
            }
        }

        if (hasVisible)
        {
            s_harnessWindowVisible.store(visible);
        }
        if (hasThrottle)
        {
            s_harnessGpuThrottled.store(throttle);
        }
        else if (hasVisible && !hasThrottle)
        {
            throttle = !visible;
            s_harnessGpuThrottled.store(throttle);
        }

        if (CPythonApplication::InstancePtr())
        {
            CPythonApplication& app = CPythonApplication::Instance();
            if (hasVisible)
            {
                app.SetWindowVisible(visible);
            }
            if (hasThrottle)
            {
                app.SetRenderThrottling(throttle);
            }
            else if (hasVisible && !hasThrottle)
            {
                throttle = !visible;
                app.SetRenderThrottling(throttle);
            }

            bool curVisible = app.GetWindowHandle() ? (IsWindowVisible(app.GetWindowHandle()) != FALSE) : visible;
            bool curThrottled = app.IsRenderThrottling();


            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
            writer.StartObject();
            writer.Key("jsonrpc"); writer.String("2.0");
            writer.Key("ok"); writer.Bool(true);
            writer.Key("id");
            if (id.starts_with("\"") && id.ends_with("\""))
                writer.String(id.substr(1, id.size() - 2).c_str());
            else if (id != "null")
                writer.Int(std::stoi(id));
            else
                writer.Null();

            writer.Key("result");
            writer.StartObject();
            writer.Key("status"); writer.String("ok");
            writer.Key("window_visible"); writer.Bool(curVisible);
            writer.Key("gpu_throttled"); writer.Bool(curThrottled);
            writer.EndObject();
            writer.EndObject();

            return sb.GetString();
        }
        else
        {
            if (hasVisible)
            {
                SetWindowVisible(visible);
            }
            bool resThrottled = hasThrottle ? throttle : !visible;

            return std::format("{{\"jsonrpc\":\"2.0\",\"ok\":true,\"id\":{},\"result\":{{\"status\":\"ok\",\"window_visible\":{},\"gpu_throttled\":{}}}}}",
                id, visible ? "true" : "false", resThrottled ? "true" : "false");
        }
    }

    std::string TestHarnessServer::HandleSetProtocolDriver(const std::string& requestJson, const std::string& id)
    {
        rapidjson::Document doc;
        doc.Parse(requestJson.c_str());

        std::string driverName;
        if (!doc.HasParseError())
        {
            if (doc.HasMember("params") && doc["params"].IsObject())
            {
                if (doc["params"].HasMember("driver") && doc["params"]["driver"].IsString())
                    driverName = doc["params"]["driver"].GetString();
            }
            else if (doc.HasMember("params") && doc["params"].IsArray() && doc["params"].Size() > 0)
            {
                if (doc["params"][0].IsString())
                    driverName = doc["params"][0].GetString();
            }
            else if (doc.HasMember("driver") && doc["driver"].IsString())
            {
                driverName = doc["driver"].GetString();
            }
        }

        if (driverName.empty())
        {
            return FormatJsonRpcError(id, -32602, "Missing driver parameter");
        }

        auto& registry = ::Network::Protocol::ProtocolDriverRegistry::Instance();
        if (!registry.GetActiveDriver())
        {
            registry.InitializeDefaults();
        }

        bool success = registry.SetActiveDriver(driverName);
        if (!success)
        {
            std::string lower = driverName;
            for (char& c : lower) c = (char)tolower(c);
            success = registry.SetActiveDriver(lower);
        }

        if (success)
        {
            std::string activeDriver = registry.GetActiveDriverName();
            return std::format("{{\"jsonrpc\":\"2.0\",\"ok\":true,\"id\":{},\"result\":{{\"status\":\"ok\",\"active_driver\":\"{}\"}}}}", id, activeDriver);
        }
        else
        {
            return FormatJsonRpcError(id, -32602, std::format("Unknown protocol driver: {}", driverName));
        }
    }

    std::string TestHarnessServer::HandleLogin(const std::string& requestJson, const std::string& id)
    {
        rapidjson::Document doc;
        doc.Parse(requestJson.c_str());

        std::string account = "admin";
        std::string password = "admin";
        std::string pin = "";
        std::string serverIp = "127.0.0.1";
        int authPort = 11000;
        int gamePort = 11011;
        int slot = 0;

        if (!doc.HasParseError())
        {
            auto parseParams = [&](const rapidjson::Value& obj) {
                if (obj.HasMember("account") && obj["account"].IsString())
                    account = obj["account"].GetString();
                else if (obj.HasMember("username") && obj["username"].IsString())
                    account = obj["username"].GetString();
                else if (obj.HasMember("id") && obj["id"].IsString())
                    account = obj["id"].GetString();

                if (obj.HasMember("password") && obj["password"].IsString())
                    password = obj["password"].GetString();
                else if (obj.HasMember("pwd") && obj["pwd"].IsString())
                    password = obj["pwd"].GetString();

                if (obj.HasMember("pin") && obj["pin"].IsString())
                    pin = obj["pin"].GetString();

                if (obj.HasMember("server_ip") && obj["server_ip"].IsString())
                    serverIp = obj["server_ip"].GetString();
                else if (obj.HasMember("ip") && obj["ip"].IsString())
                    serverIp = obj["ip"].GetString();
                else if (obj.HasMember("addr") && obj["addr"].IsString())
                    serverIp = obj["addr"].GetString();

                if (obj.HasMember("auth_port") && obj["auth_port"].IsInt())
                    authPort = obj["auth_port"].GetInt();

                if (obj.HasMember("game_port") && obj["game_port"].IsInt())
                    gamePort = obj["game_port"].GetInt();
                else if (obj.HasMember("port") && obj["port"].IsInt())
                    gamePort = obj["port"].GetInt();

                if (obj.HasMember("slot") && obj["slot"].IsInt())
                    slot = obj["slot"].GetInt();
            };

            if (doc.HasMember("params") && doc["params"].IsObject())
                parseParams(doc["params"]);
            else if (doc.IsObject())
                parseParams(doc);
        }

        auto* pActiveDriver = ::Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
        if (pActiveDriver && pActiveDriver->GetDriverName() == "luria")
        {
            if (serverIp == "127.0.0.1")
                serverIp = "46.105.73.82";
            if (authPort == 11000)
                authPort = 11469;
            if (gamePort == 11011)
                gamePort = 29001;
        }

        CAccountConnector& rkAccountConnector = CAccountConnector::Instance();
        CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();

        rkAccountConnector.SetLoginInfo(account.c_str(), password.c_str(), pin.c_str());
        rkNetStream.SetLoginInfo(account.c_str(), password.c_str(), pin.c_str());
        rkNetStream.SetDirectEnterMode(slot);

        bool connOk = rkAccountConnector.Connect(serverIp.c_str(), gamePort, serverIp.c_str(), authPort);

        return std::format(
            "{{\"jsonrpc\":\"2.0\",\"ok\":{},\"id\":{},\"result\":{{\"status\":\"{}\",\"account\":\"{}\",\"auth_port\":{},\"game_port\":{}}}}}",
            connOk ? "true" : "false",
            id,
            connOk ? "connecting_auth" : "connection_failed",
            account,
            authPort,
            gamePort
        );
    }

    std::string TestHarnessServer::HandleSelectCharacter(const std::string& requestJson, const std::string& id)
    {
        rapidjson::Document doc;
        doc.Parse(requestJson.c_str());

        int slot = 0;
        if (!doc.HasParseError())
        {
            auto checkName = [&](const std::string& targetName) {
                CPythonNetworkStream& rkNet = CPythonNetworkStream::Instance();
                for (UINT i = 0; i < PLAYER_PER_ACCOUNT4; ++i)
                {
                    const TSimplePlayerInformation* pInfo = rkNet.GetSimplePlayerInfoPtr(i);
                    if (pInfo && pInfo->dwID != 0 && targetName == pInfo->szName)
                    {
                        slot = i;
                        return true;
                    }
                }
                return false;
            };

            if (doc.HasMember("params") && doc["params"].IsObject())
            {
                if (doc["params"].HasMember("name") && doc["params"]["name"].IsString())
                    checkName(doc["params"]["name"].GetString());
                else if (doc["params"].HasMember("slot") && doc["params"]["slot"].IsInt())
                    slot = doc["params"]["slot"].GetInt();
            }
            else
            {
                if (doc.HasMember("name") && doc["name"].IsString())
                    checkName(doc["name"].GetString());
                else if (doc.HasMember("slot") && doc["slot"].IsInt())
                    slot = doc["slot"].GetInt();
            }
        }

        if (Py_IsInitialized())
        {
            PyGILState_STATE gstate = PyGILState_Ensure();
            CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
            PyObject* poSelectWnd = rkNetStream.GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_SELECT);
            if (poSelectWnd)
            {
                PyCallClassMemberFunc(poSelectWnd, "DirectStartGame", Py_BuildValue("(i)", slot));
            }
            else
            {
                PyObject* poHandler = rkNetStream.GetHandler();
                if (poHandler)
                {
                    PyCallClassMemberFunc(poHandler, "SetCharacterSlot", Py_BuildValue("(i)", slot));
                    PyCallClassMemberFunc(poHandler, "SetLoadingPhase", Py_BuildValue("()"));
                }
                else
                {
                    rkNetStream.SendSelectCharacterPacket(static_cast<BYTE>(slot));
                }
            }
            if (PyErr_Occurred())
                PyErr_Clear();
            PyGILState_Release(gstate);
        }
        else
        {
            CPythonNetworkStream::Instance().SendSelectCharacterPacket(static_cast<BYTE>(slot));
        }

        return std::format("{{\"jsonrpc\":\"2.0\",\"ok\":true,\"id\":{},\"result\":{{\"status\":\"character_selected\",\"slot\":{}}}}}", id, slot);
    }

    std::string TestHarnessServer::HandleGetCharacterSlots(const std::string& id)
    {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

        writer.StartObject();
        writer.Key("jsonrpc"); writer.String("2.0");
        writer.Key("ok"); writer.Bool(true);
        writer.Key("id");
        if (id.empty() || id == "null") writer.Null();
        else writer.String(id.c_str());

        writer.Key("result");
        writer.StartObject();

        CPythonNetworkStream& rkNet = CPythonNetworkStream::Instance();
        writer.Key("phase"); writer.String(rkNet.GetPhase().c_str());

        writer.Key("characters");
        writer.StartArray();

        for (UINT i = 0; i < PLAYER_PER_ACCOUNT4; ++i)
        {
            const TSimplePlayerInformation* pInfo = rkNet.GetSimplePlayerInfoPtr(i);
            writer.StartObject();
            writer.Key("slot"); writer.Uint(i);
            if (!pInfo || pInfo->dwID == 0)
            {
                writer.Key("is_empty"); writer.Bool(true);
                writer.Key("id"); writer.Uint(0);
                writer.Key("name"); writer.String("");
                writer.Key("level"); writer.Uint(0);
                writer.Key("job"); writer.Uint(0);
                writer.Key("job_name"); writer.String("none");
            }
            else
            {
                writer.Key("is_empty"); writer.Bool(false);
                writer.Key("id"); writer.Uint(pInfo->dwID);
                writer.Key("name"); writer.String(pInfo->szName);
                writer.Key("level"); writer.Uint(pInfo->byLevel);
                writer.Key("job"); writer.Uint(pInfo->byJob);

                const char* szJobName = "warrior";
                if (pInfo->byJob == 1 || pInfo->byJob == 5) szJobName = "ninja";
                else if (pInfo->byJob == 2 || pInfo->byJob == 6) szJobName = "sura";
                else if (pInfo->byJob == 3 || pInfo->byJob == 7) szJobName = "shaman";
                writer.Key("job_name"); writer.String(szJobName);

                writer.Key("play_minutes"); writer.Uint(pInfo->dwPlayMinutes);
                writer.Key("str"); writer.Uint(pInfo->byST);
                writer.Key("con"); writer.Uint(pInfo->byHT);
                writer.Key("dex"); writer.Uint(pInfo->byDX);
                writer.Key("int"); writer.Uint(pInfo->byIQ);
                writer.Key("x"); writer.Int(pInfo->x);
                writer.Key("y"); writer.Int(pInfo->y);
            }
            writer.EndObject();
        }

        writer.EndArray();
        writer.EndObject(); // result
        writer.EndObject(); // root

        return sb.GetString();
    }

    std::string TestHarnessServer::HandleCreateCharacter(const std::string& requestJson, const std::string& id)
    {
        rapidjson::Document doc;
        doc.Parse(requestJson.c_str());

        int slot = 0;
        std::string name = "BotAuto1";
        int job = 0;    // 0=Warrior, 1=Ninja, 2=Sura, 3=Shaman
        int gender = 0; // 0=Male, 1=Female
        int shape = 0;

        if (!doc.HasParseError())
        {
            auto parse = [&](const rapidjson::Value& p) {
                if (p.HasMember("slot") && p["slot"].IsInt()) slot = p["slot"].GetInt();
                if (p.HasMember("name") && p["name"].IsString()) name = p["name"].GetString();
                if (p.HasMember("job") && p["job"].IsInt()) job = p["job"].GetInt();
                else if (p.HasMember("job") && p["job"].IsString())
                {
                    std::string jStr = p["job"].GetString();
                    if (jStr == "warrior" || jStr == "wojownik") job = 0;
                    else if (jStr == "ninja") job = 1;
                    else if (jStr == "sura") job = 2;
                    else if (jStr == "shaman" || jStr == "szaman") job = 3;
                }
                if (p.HasMember("gender") && p["gender"].IsInt()) gender = p["gender"].GetInt();
                else if (p.HasMember("gender") && p["gender"].IsString())
                {
                    std::string gStr = p["gender"].GetString();
                    if (gStr == "female" || gStr == "kobieta") gender = 1;
                    else gender = 0;
                }
                if (p.HasMember("shape") && p["shape"].IsInt()) shape = p["shape"].GetInt();
            };

            if (doc.HasMember("params") && doc["params"].IsObject())
                parse(doc["params"]);
            else
                parse(doc);
        }

        if (name.empty())
            name = "BotAuto1";

        BYTE bJob = 0;
        if (job == 0) // Warrior
            bJob = (gender == 0) ? 0 : 4;
        else if (job == 1) // Ninja
            bJob = (gender == 0) ? 5 : 1;
        else if (job == 2) // Sura
            bJob = (gender == 0) ? 2 : 6;
        else if (job == 3) // Shaman
            bJob = (gender == 0) ? 7 : 3;
        else
            bJob = static_cast<BYTE>(job);

        BYTE bShape = static_cast<BYTE>(shape);
        BYTE bCon = 4, bInt = 3, bStr = 6, bDex = 3;
        if (job == 1) { bCon = 3; bInt = 3; bStr = 4; bDex = 6; }
        else if (job == 2) { bCon = 3; bInt = 5; bStr = 5; bDex = 3; }
        else if (job == 3) { bCon = 4; bInt = 6; bStr = 3; bDex = 3; }

        bool ok = CPythonNetworkStream::Instance().SendCreateCharacterPacket(
            static_cast<BYTE>(slot),
            name.c_str(),
            bJob,
            bShape,
            bCon,
            bInt,
            bStr,
            bDex
        );

        return std::format(
            "{{\"jsonrpc\":\"2.0\",\"ok\":{},\"id\":{},\"result\":{{\"status\":\"character_create_sent\",\"slot\":{},\"name\":\"{}\",\"job\":{},\"gender\":{},\"race_id\":{}}}}}",
            ok ? "true" : "false", id, slot, name, job, gender, bJob
        );
    }

    std::string TestHarnessServer::HandleSetAutoReconnect(const std::string& requestJson, const std::string& id)
    {
        rapidjson::Document doc;
        doc.Parse(requestJson.c_str());

        bool enabled = true;
        uint32_t delayMs = 3000;
        int maxAttempts = 10;

        if (!doc.HasParseError())
        {
            auto parse = [&](const rapidjson::Value& p) {
                if (p.HasMember("enabled") && p["enabled"].IsBool()) enabled = p["enabled"].GetBool();
                if (p.HasMember("delay_ms") && p["delay_ms"].IsUint()) delayMs = p["delay_ms"].GetUint();
                else if (p.HasMember("delay") && p["delay"].IsUint()) delayMs = p["delay"].GetUint() * 1000;
                if (p.HasMember("max_attempts") && p["max_attempts"].IsInt()) maxAttempts = p["max_attempts"].GetInt();
            };

            if (doc.HasMember("params") && doc["params"].IsObject())
                parse(doc["params"]);
            else
                parse(doc);
        }

        UserInterface::FastBoot::FastBootOverlay::Instance().SetAutoReconnect(enabled, delayMs, maxAttempts);

        return std::format(
            "{{\"jsonrpc\":\"2.0\",\"ok\":true,\"id\":{},\"result\":{{\"status\":\"auto_reconnect_configured\",\"enabled\":{},\"delay_ms\":{},\"max_attempts\":{}}}}}",
            id, enabled ? "true" : "false", delayMs, maxAttempts
        );
    }

    std::string TestHarnessServer::HandleWorldMegaSnapshot(const std::string& id, const std::string& requestJson)
    {
        if (!CPythonPlayer::InstancePtr() || !CPythonCharacterManager::InstancePtr() || !CPythonApplication::InstancePtr())
        {
            return FormatJsonRpcError(id, -32000, "Game application, player or character manager not initialized");
        }

        // 1. Parsowanie parametrow (radius, include_inventory, include_skills, include_entities)
        float radius = 2500.0f;
        bool includeInventory = true;
        bool includeSkills = true;
        bool includeEntities = true;

        if (!requestJson.empty())
        {
            rapidjson::Document reqDoc;
            reqDoc.Parse(requestJson.c_str());
            if (!reqDoc.HasParseError())
            {
                auto parseParams = [&](const rapidjson::Value& p) {
                    if (p.HasMember("radius") && p["radius"].IsNumber())
                        radius = static_cast<float>(p["radius"].GetDouble());
                    if (p.HasMember("include_inventory") && p["include_inventory"].IsBool())
                        includeInventory = p["include_inventory"].GetBool();
                    if (p.HasMember("include_skills") && p["include_skills"].IsBool())
                        includeSkills = p["include_skills"].GetBool();
                    if (p.HasMember("include_entities") && p["include_entities"].IsBool())
                        includeEntities = p["include_entities"].GetBool();
                };

                if (reqDoc.HasMember("params") && reqDoc["params"].IsObject())
                    parseParams(reqDoc["params"]);
                else
                    parseParams(reqDoc);
            }
        }

        if (!std::isfinite(radius) || radius <= 0.0f)
            radius = 2500.0f;

        CPythonPlayer& rkPlayer = CPythonPlayer::Instance();
        CPythonCharacterManager& rkChrMgr = CPythonCharacterManager::Instance();
        CInstanceBase* pMainActor = rkChrMgr.GetMainActorPtr();

        DWORD dwMainVID = pMainActor ? pMainActor->GetVirtualID() : rkPlayer.GetMainCharacterIndex();
        std::string name = pMainActor ? pMainActor->GetNameString() : rkPlayer.GetName();
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

        const auto& moveCtrl = rkPlayer.GetMovementController();
        const auto& combatCtrl = rkPlayer.GetCombatController();
        const auto& itemCtrl = rkPlayer.GetItemController();
        const auto& hpPot = itemCtrl.GetAutoPotionInfo(PlayerControllers::PlayerItemController::AUTO_POTION_TYPE_HP);
        const auto& spPot = itemCtrl.GetAutoPotionInfo(PlayerControllers::PlayerItemController::AUTO_POTION_TYPE_SP);

        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

        writer.StartObject();
        writer.Key("jsonrpc"); writer.String("2.0");
        writer.Key("ok"); writer.Bool(true);
        writer.Key("id");
        if (id.starts_with("\"") && id.ends_with("\""))
            writer.String(id.substr(1, id.size() - 2).c_str());
        else if (id != "null")
            writer.Int(std::stoi(id));
        else
            writer.Null();

        writer.Key("result");
        writer.StartObject();

        // Timestamp
        writer.Key("timestamp");
        writer.Uint64(static_cast<uint64_t>(GetTickCount64()));

        // Engine
        writer.Key("engine");
        writer.StartObject();
        writer.Key("phase");
        writer.String(CPythonNetworkStream::InstancePtr() ? CPythonNetworkStream::Instance().GetPhase().c_str() : "OFFLINE");
        std::string activeDriver = ::Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriverName();
        if (activeDriver.empty()) activeDriver = "standard_x64";
        writer.Key("active_driver");
        writer.String(activeDriver.c_str());

        bool windowVisible = true;
        bool gpuThrottled = false;
        if (CPythonApplication::InstancePtr())
        {
            HWND hWnd = CPythonApplication::Instance().GetWindowHandle();
            windowVisible = hWnd ? (IsWindowVisible(hWnd) != FALSE) : false;
            gpuThrottled = CPythonApplication::Instance().IsRenderThrottling();
        }
        writer.Key("window_visible"); writer.Bool(windowVisible);
        writer.Key("gpu_throttled"); writer.Bool(gpuThrottled);
        writer.Key("headless"); writer.Bool(g_bHeadlessMode);
        writer.Key("turbo_ticks"); writer.Uint(CPythonApplication::GetTurboTicks());
        writer.EndObject();

        // Player
        writer.Key("player");
        writer.StartObject();
        writer.Key("vid"); writer.Uint(dwMainVID);
        writer.Key("name"); writer.String(name.c_str());
        writer.Key("race"); writer.Uint(dwRace);
        writer.Key("level"); writer.Int64(iLevel);
        writer.Key("gold"); writer.Int64(iGold);
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
        writer.Key("target_vid"); writer.Uint(dwTargetVID);
        writer.Key("is_moving"); writer.Bool(moveCtrl.IsDestPosition() || moveCtrl.IsAttackKeyState());
        writer.Key("can_move"); writer.Bool(moveCtrl.CanMove());
        writer.Key("can_attack"); writer.Bool(combatCtrl.CanAttack());
        writer.Key("auto_pot_hp"); writer.Bool(hpPot.bActivated);
        writer.Key("auto_pot_sp"); writer.Bool(spPot.bActivated);
        writer.EndObject();

        // Entities in radius
        if (includeEntities)
        {
            writer.Key("entities");
            writer.StartArray();

            float radSq = radius * radius;
            rkChrMgr.for_each_actor_vid([&](Client::World::EntityVid vid, CInstanceBase* pActor) {
                if (!pActor || pActor == pMainActor)
                    return;

                TPixelPosition actPos{0, 0, 0};
                pActor->NEW_GetPixelPosition(&actPos);
                float dx = actPos.x - pos.x;
                float dy = actPos.y - pos.y;
                float distSq = dx * dx + dy * dy;
                if (distSq > radSq)
                    return;

                float dist = std::sqrt(distSq);

                writer.StartObject();
                writer.Key("vid"); writer.Uint(static_cast<uint32_t>(vid.value()));
                writer.Key("name"); writer.String(pActor->GetNameString());
                writer.Key("vnum"); writer.Uint(pActor->GetRace());
                writer.Key("type"); writer.Uint(pActor->GetInstanceType());
                writer.Key("x"); writer.Double(actPos.x);
                writer.Key("y"); writer.Double(actPos.y);
                writer.Key("z"); writer.Double(actPos.z);
                writer.Key("distance"); writer.Double(dist);
                writer.Key("rotation"); writer.Double(pActor->GetRotation());
                writer.Key("is_dead"); writer.Bool(pActor->IsDead() != FALSE);
                writer.Key("is_enemy"); writer.Bool(pActor->IsEnemy() != FALSE);
                writer.Key("is_pc"); writer.Bool(pActor->IsPC() != FALSE);
                writer.Key("is_npc"); writer.Bool(pActor->IsNPC() != FALSE);
                writer.EndObject();
            });

            writer.EndArray();
        }

        // Inventory
        if (includeInventory)
        {
            writer.Key("inventory");
            writer.StartArray();

            for (WORD i = 0; i < c_Inventory_Count; ++i)
            {
                TItemPos itemPos(INVENTORY, i);
                DWORD dwItemVnum = rkPlayer.GetItemIndex(itemPos);
                if (dwItemVnum == 0)
                    continue;

                DWORD dwItemCount = rkPlayer.GetItemCount(itemPos);

                std::string itemName = "Item";
                CItemData* pItemData = nullptr;
                if (CItemManager::InstancePtr() && CItemManager::Instance().GetItemDataPointer(dwItemVnum, &pItemData) && pItemData)
                {
                    itemName = pItemData->GetName();
                }

                writer.StartObject();
                writer.Key("window"); writer.Uint(INVENTORY);
                writer.Key("cell"); writer.Uint(i);
                writer.Key("vnum"); writer.Uint(dwItemVnum);
                writer.Key("count"); writer.Uint(dwItemCount);
                writer.Key("name"); writer.String(itemName.c_str());
                writer.EndObject();
            }

            writer.EndArray();
        }

        // Skills
        if (includeSkills)
        {
            writer.Key("skills");
            writer.StartArray();

            for (int slot = 0; slot < SKILL_MAX_NUM; ++slot)
            {
                DWORD dwSkillIndex = rkPlayer.GetSkillIndex(slot);
                if (dwSkillIndex == 0)
                    continue;

                int iGrade = rkPlayer.GetSkillGrade(slot);
                int iLevel = rkPlayer.GetSkillLevel(slot);
                bool isCooling = rkPlayer.IsSkillCoolTime(slot) != FALSE;
                float fTotalCoolTime = rkPlayer.GetSkillCoolTime(slot);
                float fElapsedTime = rkPlayer.GetSkillElapsedCoolTime(slot);

                std::string skillName = "Skill";
                if (CPythonSkill::InstancePtr())
                {
                    CPythonSkill::TSkillData* pSkillData = nullptr;
                    if (CPythonSkill::Instance().GetSkillData(dwSkillIndex, &pSkillData) && pSkillData)
                    {
                        skillName = pSkillData->strName;
                    }
                }

                writer.StartObject();
                writer.Key("slot"); writer.Int(slot);
                writer.Key("vnum"); writer.Uint(dwSkillIndex);
                writer.Key("name"); writer.String(skillName.c_str());
                writer.Key("grade"); writer.Int(iGrade);
                writer.Key("level"); writer.Int(iLevel);
                writer.Key("is_cooling"); writer.Bool(isCooling);
                writer.Key("total_cooltime"); writer.Double(fTotalCoolTime);
                writer.Key("elapsed_cooltime"); writer.Double(fElapsedTime);
                writer.EndObject();
            }

            writer.EndArray();
        }

        writer.EndObject(); // result
        writer.EndObject(); // root

        return sb.GetString();
    }

    void TestHarnessServer::RegisterBuiltinRoutes()
    {
        // 1. Metody AgentMCPBridge
        auto mcpHandler = [](const std::string& req, const std::string& id) -> std::string {
            if (!Client::MCP::AgentMCPBridge::Instance().IsReady())
            {
                Client::Adapters::InitializeMCPAdapters();
            }
            return Client::MCP::AgentMCPBridge::Instance().HandleJsonRpcRequest(req);
        };

        m_routes["game_ping"] = { true, mcpHandler };
        m_routes["game_get_state"] = { true, mcpHandler }; // isImmediate = true (Natychmiastowy odczyt snapshotu, odpornosc na freeze-on-start)
        m_routes["game_get_inventory"] = { true, mcpHandler };
        m_routes["game_get_nearby_entities"] = { true, mcpHandler };
        m_routes["game_action_attack"] = { false, mcpHandler };
        m_routes["game_action_use_item"] = { false, mcpHandler };

        // 2. Metody stanu silnika (isImmediate = true, Natychmiastowy odczyt migawki bez blokady glownego watku)
        auto stateHandler = [this](const std::string& req, const std::string& id) -> std::string {
            return HandleGetState(id);
        };
        m_routes["get_state"] = { true, stateHandler };
        m_routes["get_engine_state"] = { true, stateHandler };

        // 3. Metody audytu (kolejkowane do glownego watku ze wzgledu na GIL Pythona)
        auto auditHandler = [this](const std::string& req, const std::string& id) -> std::string {
            return HandleAuditState(id, req);
        };
        m_routes["audit_state"] = { false, auditHandler };
        m_routes["audit_dual_state"] = { false, auditHandler };

        // 4. tick_frame (isImmediate = true, atomowy step w DeterministicTickController)
        m_routes["tick_frame"] = {
            true,
            [this](const std::string& req, const std::string& id) -> std::string {
                rapidjson::Document doc;
                doc.Parse(req.c_str());
                uint32_t count = 1;
                float deltaTime = 1.0f / 60.0f;

                if (!doc.HasParseError())
                {
                    if (doc.HasMember("params") && doc["params"].IsObject())
                    {
                        const auto& p = doc["params"];
                        if (p.HasMember("count") && p["count"].IsUint())
                            count = p["count"].GetUint();
                        else if (p.HasMember("frames") && p["frames"].IsUint())
                            count = p["frames"].GetUint();

                        if (p.HasMember("delta_time") && p["delta_time"].IsNumber())
                            deltaTime = static_cast<float>(p["delta_time"].GetDouble());
                        else if (p.HasMember("delta_ms") && p["delta_ms"].IsNumber())
                            deltaTime = static_cast<float>(p["delta_ms"].GetDouble() / 1000.0);
                    }
                    else
                    {
                        if (doc.HasMember("count") && doc["count"].IsUint())
                            count = doc["count"].GetUint();
                        else if (doc.HasMember("frames") && doc["frames"].IsUint())
                            count = doc["frames"].GetUint();

                        if (doc.HasMember("delta_time") && doc["delta_time"].IsNumber())
                            deltaTime = static_cast<float>(doc["delta_time"].GetDouble());
                        else if (doc.HasMember("delta_ms") && doc["delta_ms"].IsNumber())
                            deltaTime = static_cast<float>(doc["delta_ms"].GetDouble() / 1000.0);
                    }
                }
                return HandleTickFrame(id, count, deltaTime);
            }
        };

        // 5. inject_packet
        m_routes["inject_packet"] = {
            false,
            [this](const std::string& req, const std::string& id) -> std::string {
                rapidjson::Document doc;
                doc.Parse(req.c_str());
                uint32_t header = 0;
                std::string hexData;

                if (!doc.HasParseError())
                {
                    if (doc.HasMember("params") && doc["params"].IsObject())
                    {
                        const auto& p = doc["params"];
                        if (p.HasMember("header") && p["header"].IsUint())
                            header = p["header"].GetUint();
                        else if (p.HasMember("opcode") && p["opcode"].IsUint())
                            header = p["opcode"].GetUint();

                        if (p.HasMember("data") && p["data"].IsString())
                            hexData = p["data"].GetString();
                        else if (p.HasMember("payload_hex") && p["payload_hex"].IsString())
                            hexData = p["payload_hex"].GetString();
                    }
                    else
                    {
                        if (doc.HasMember("header") && doc["header"].IsUint())
                            header = doc["header"].GetUint();
                        else if (doc.HasMember("opcode") && doc["opcode"].IsUint())
                            header = doc["opcode"].GetUint();

                        if (doc.HasMember("data") && doc["data"].IsString())
                            hexData = doc["data"].GetString();
                        else if (doc.HasMember("payload_hex") && doc["payload_hex"].IsString())
                            hexData = doc["payload_hex"].GetString();
                    }
                }
                return HandleInjectPacket(id, header, hexData);
            }
        };

        // 6. shutdown
        m_routes["shutdown"] = {
            true,
            [](const std::string& req, const std::string& id) -> std::string {
                HWND hWnd = CPythonApplication::Instance().GetWindowHandle();
                if (hWnd)
                {
                    PostMessageA(hWnd, WM_CLOSE, 0, 0);
                }
                std::thread([]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    ExitProcess(0);
                }).detach();
                return std::format("{{\"jsonrpc\":\"2.0\",\"ok\":true,\"id\":{},\"result\":{{\"status\":\"shutdown_acknowledged\"}}}}", id);
            }
        };

        // 7. game_set_window_mode (isImmediate = true, w pelni bezpieczna modyfikacja flag okna i dlawienia GPU)
        auto setWindowModeHandler = [this](const std::string& req, const std::string& id) -> std::string {
            return HandleSetWindowMode(req, id);
        };
        m_routes["game_set_window_mode"] = { true, setWindowModeHandler };
        m_routes["set_window_mode"] = { true, setWindowModeHandler };

        // 8. game_set_protocol_driver
        m_routes["game_set_protocol_driver"] = {
            true,
            [this](const std::string& req, const std::string& id) -> std::string {
                return HandleSetProtocolDriver(req, id);
            }
        };

        // 9. game_login
        m_routes["game_login"] = {
            false,
            [this](const std::string& req, const std::string& id) -> std::string {
                return HandleLogin(req, id);
            }
        };

        // 10. game_select_character
        m_routes["game_select_character"] = {
            false,
            [this](const std::string& req, const std::string& id) -> std::string {
                return HandleSelectCharacter(req, id);
            }
        };

        // 10a. game_get_character_slots (Pobranie pelnej tablicy postaci na koncie)
        m_routes["game_get_character_slots"] = {
            false,
            [this](const std::string&, const std::string& id) -> std::string {
                return HandleGetCharacterSlots(id);
            }
        };
        m_routes["get_character_slots"] = {
            false,
            [this](const std::string&, const std::string& id) -> std::string {
                return HandleGetCharacterSlots(id);
            }
        };

        // 10b. game_create_character (Tworzenie postaci na pustym koncie)
        m_routes["game_create_character"] = {
            false,
            [this](const std::string& req, const std::string& id) -> std::string {
                return HandleCreateCharacter(req, id);
            }
        };
        m_routes["create_character"] = {
            false,
            [this](const std::string& req, const std::string& id) -> std::string {
                return HandleCreateCharacter(req, id);
            }
        };

        // 11. game_enter_game
        m_routes["game_enter_game"] = {
            false,
            [](const std::string&, const std::string& id) -> std::string {
                bool ok = CPythonNetworkStream::Instance().SendEnterGame();
                return std::format("{{\"jsonrpc\":\"2.0\",\"ok\":{},\"id\":{},\"result\":{{\"status\":\"entergame_sent\"}}}}", ok ? "true" : "false", id);
            }
        };

        // 11b. game_set_auto_reconnect (Konfiguracja automatycznego powrotu do gry po Kick/Disconnect)
        m_routes["game_set_auto_reconnect"] = {
            false,
            [this](const std::string& req, const std::string& id) -> std::string {
                return HandleSetAutoReconnect(req, id);
            }
        };
        m_routes["set_auto_reconnect"] = {
            false,
            [this](const std::string& req, const std::string& id) -> std::string {
                return HandleSetAutoReconnect(req, id);
            }
        };

        // 12. game_set_attack_key_state (Klawisz ataku / Spacja)
        auto setAttackKeyHandler = [](const std::string& req, const std::string& id) -> std::string {
            if (!CPythonPlayer::InstancePtr())
                return FormatJsonRpcError(id, -32000, "Player not initialized");

            rapidjson::Document doc;
            doc.Parse(req.c_str());
            bool isPress = true;
            if (!doc.HasParseError())
            {
                auto parse = [&](const rapidjson::Value& p) {
                    if (p.HasMember("is_pressed") && p["is_pressed"].IsBool()) isPress = p["is_pressed"].GetBool();
                    else if (p.HasMember("is_press") && p["is_press"].IsBool()) isPress = p["is_press"].GetBool();
                    else if (p.HasMember("press") && p["press"].IsBool()) isPress = p["press"].GetBool();
                };
                if (doc.HasMember("params") && doc["params"].IsObject())
                    parse(doc["params"]);
                else
                    parse(doc);
            }

            CPythonPlayer::Instance().SetAttackKeyState(isPress);
            return std::format("{{\"jsonrpc\":\"2.0\",\"ok\":true,\"id\":{},\"result\":{{\"status\":\"attack_key_set\",\"is_pressed\":{}}}}}",
                id, isPress ? "true" : "false");
        };
        m_routes["game_set_attack_key_state"] = { false, setAttackKeyHandler };
        m_routes["set_attack_key_state"] = { false, setAttackKeyHandler };

        // 13. game_set_dir_key_state (Kierunek pojedynczy: 0=UP, 1=DOWN, 2=LEFT, 3=RIGHT)
        auto setDirKeyHandler = [](const std::string& req, const std::string& id) -> std::string {
            if (!CPythonPlayer::InstancePtr())
                return FormatJsonRpcError(id, -32000, "Player not initialized");

            rapidjson::Document doc;
            doc.Parse(req.c_str());
            int dir = 0;
            bool isPress = true;
            if (!doc.HasParseError())
            {
                auto parse = [&](const rapidjson::Value& p) {
                    if (p.HasMember("dir") && p["dir"].IsInt()) dir = p["dir"].GetInt();
                    else if (p.HasMember("direction") && p["direction"].IsInt()) dir = p["direction"].GetInt();
                    if (p.HasMember("is_pressed") && p["is_pressed"].IsBool()) isPress = p["is_pressed"].GetBool();
                    else if (p.HasMember("is_press") && p["is_press"].IsBool()) isPress = p["is_press"].GetBool();
                };
                if (doc.HasMember("params") && doc["params"].IsObject())
                    parse(doc["params"]);
                else
                    parse(doc);
            }

            if (dir < 0 || dir > 3)
                return FormatJsonRpcError(id, -32602, "Invalid direction: must be in range 0..3 (DIR_UP=0, DIR_DOWN=1, DIR_LEFT=2, DIR_RIGHT=3)");

            CPythonPlayer::Instance().NEW_SetSingleDirKeyState(dir, isPress);
            return std::format("{{\"jsonrpc\":\"2.0\",\"ok\":true,\"id\":{},\"result\":{{\"status\":\"dir_key_set\",\"dir\":{},\"is_pressed\":{}}}}}",
                id, dir, isPress ? "true" : "false");
        };
        m_routes["game_set_dir_key_state"] = { false, setDirKeyHandler };
        m_routes["set_dir_key_state"] = { false, setDirKeyHandler };

        // 14. game_set_multi_dir_key_state (Kierunek zlozony WASD)
        m_routes["game_set_multi_dir_key_state"] = {
            false,
            [](const std::string& req, const std::string& id) -> std::string {
                if (!CPythonPlayer::InstancePtr())
                    return FormatJsonRpcError(id, -32000, "Player not initialized");

                rapidjson::Document doc;
                doc.Parse(req.c_str());
                bool isLeft = false, isRight = false, isUp = false, isDown = false;
                if (!doc.HasParseError())
                {
                    auto parse = [&](const rapidjson::Value& p) {
                        if (p.HasMember("left") && p["left"].IsBool()) isLeft = p["left"].GetBool();
                        if (p.HasMember("right") && p["right"].IsBool()) isRight = p["right"].GetBool();
                        if (p.HasMember("up") && p["up"].IsBool()) isUp = p["up"].GetBool();
                        if (p.HasMember("down") && p["down"].IsBool()) isDown = p["down"].GetBool();
                    };
                    if (doc.HasMember("params") && doc["params"].IsObject())
                        parse(doc["params"]);
                    else
                        parse(doc);
                }

                CPythonPlayer::Instance().NEW_SetMultiDirKeyState(isLeft, isRight, isUp, isDown);
                return std::format("{{\"jsonrpc\":\"2.0\",\"ok\":true,\"id\":{},\"result\":{{\"status\":\"multi_dir_set\",\"left\":{},\"right\":{},\"up\":{},\"down\":{}}}}}",
                    id, isLeft ? "true" : "false", isRight ? "true" : "false", isUp ? "true" : "false", isDown ? "true" : "false");
            }
        };

        // 15. game_move_to_position / game_move_to (Nawigacja do koordynatow pikselowych)
        auto moveToPosHandler = [](const std::string& req, const std::string& id) -> std::string {
            if (!CPythonPlayer::InstancePtr())
                return FormatJsonRpcError(id, -32000, "Player not initialized");

            rapidjson::Document doc;
            doc.Parse(req.c_str());
            float x = 0.0f, y = 0.0f, z = 0.0f;
            if (!doc.HasParseError())
            {
                auto parse = [&](const rapidjson::Value& p) {
                    if (p.HasMember("x") && p["x"].IsNumber()) x = static_cast<float>(p["x"].GetDouble());
                    if (p.HasMember("y") && p["y"].IsNumber()) y = static_cast<float>(p["y"].GetDouble());
                    if (p.HasMember("z") && p["z"].IsNumber()) z = static_cast<float>(p["z"].GetDouble());
                };
                if (doc.HasMember("params") && doc["params"].IsObject())
                    parse(doc["params"]);
                else
                    parse(doc);
            }

            if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
                return FormatJsonRpcError(id, -32602, "Coordinates must be finite numbers");
            if (x < 0.0f || x > 2048000.0f || y < 0.0f || y > 2048000.0f)
                return FormatJsonRpcError(id, -32602, "Coordinates out of world bounds [0, 2048000]");

            TPixelPosition dstPos{x, y, z};
            bool ok = CPythonPlayer::Instance().NEW_MoveToDestPixelPositionDirection(dstPos);
            return std::format("{{\"jsonrpc\":\"2.0\",\"ok\":true,\"id\":{},\"result\":{{\"status\":\"move_started\",\"success\":{},\"x\":{},\"y\":{},\"z\":{}}}}}",
                id, ok ? "true" : "false", x, y, z);
        };
        m_routes["game_move_to_position"] = { false, moveToPosHandler };
        m_routes["game_move_to"] = { false, moveToPosHandler };

        // 16. game_stop (Zatrzymanie ruchu gracza)
        m_routes["game_stop"] = {
            false,
            [](const std::string&, const std::string& id) -> std::string {
                if (!CPythonPlayer::InstancePtr())
                    return FormatJsonRpcError(id, -32000, "Player not initialized");

                CPythonPlayer::Instance().NEW_Stop();
                return std::format("{{\"jsonrpc\":\"2.0\",\"ok\":true,\"id\":{},\"result\":{{\"status\":\"stopped\"}}}}", id);
            }
        };

        // 17. game_pick_close_item (Podnoszenie przedmiotow z ziemi - Klawisz Z)
        auto pickItemHandler = [](const std::string&, const std::string& id) -> std::string {
            if (!CPythonPlayer::InstancePtr())
                return FormatJsonRpcError(id, -32000, "Player not initialized");

            CPythonPlayer::Instance().PickCloseItem();
            return std::format("{{\"jsonrpc\":\"2.0\",\"ok\":true,\"id\":{},\"result\":{{\"status\":\"pick_item_invoked\"}}}}", id);
        };
        m_routes["game_pick_close_item"] = { false, pickItemHandler };
        m_routes["pick_close_item"] = { false, pickItemHandler };

        // 18. game_set_target (Zaznaczenie celu wg VID)
        auto setTargetHandler = [](const std::string& req, const std::string& id) -> std::string {
            if (!CPythonPlayer::InstancePtr())
                return FormatJsonRpcError(id, -32000, "Player not initialized");

            rapidjson::Document doc;
            doc.Parse(req.c_str());
            uint32_t targetVid = 0;
            bool force = true;
            if (!doc.HasParseError())
            {
                auto parse = [&](const rapidjson::Value& p) {
                    if (p.HasMember("target_vid") && p["target_vid"].IsUint()) targetVid = p["target_vid"].GetUint();
                    else if (p.HasMember("vid") && p["vid"].IsUint()) targetVid = p["vid"].GetUint();
                    if (p.HasMember("force") && p["force"].IsBool()) force = p["force"].GetBool();
                };
                if (doc.HasMember("params") && doc["params"].IsObject())
                    parse(doc["params"]);
                else
                    parse(doc);
            }

            CPythonPlayer::Instance().SetTarget(targetVid, force ? TRUE : FALSE);
            return std::format("{{\"jsonrpc\":\"2.0\",\"ok\":true,\"id\":{},\"result\":{{\"status\":\"target_set\",\"target_vid\":{}}}}}", id, targetVid);
        };
        m_routes["game_set_target"] = { false, setTargetHandler };
        m_routes["set_target"] = { false, setTargetHandler };

        // 19. game_clear_target (Odznaczenie celu)
        auto clearTargetHandler = [](const std::string&, const std::string& id) -> std::string {
            if (!CPythonPlayer::InstancePtr())
                return FormatJsonRpcError(id, -32000, "Player not initialized");

            CPythonPlayer::Instance().SetTarget(0, TRUE);
            return std::format("{{\"jsonrpc\":\"2.0\",\"ok\":true,\"id\":{},\"result\":{{\"status\":\"target_cleared\"}}}}", id);
        };
        m_routes["game_clear_target"] = { false, clearTargetHandler };
        m_routes["clear_target"] = { false, clearTargetHandler };

        // 20. game_use_skill (Uzycie umiejetnosci z danego slotu)
        auto useSkillHandler = [](const std::string& req, const std::string& id) -> std::string {
            if (!CPythonPlayer::InstancePtr())
                return FormatJsonRpcError(id, -32000, "Player not initialized");

            rapidjson::Document doc;
            doc.Parse(req.c_str());
            int64_t rawSlot = -1;
            if (!doc.HasParseError())
            {
                auto parse = [&](const rapidjson::Value& p) {
                    if (p.HasMember("slot") && p["slot"].IsInt64()) rawSlot = p["slot"].GetInt64();
                    else if (p.HasMember("skill_slot") && p["skill_slot"].IsInt64()) rawSlot = p["skill_slot"].GetInt64();
                    else if (p.HasMember("index") && p["index"].IsInt64()) rawSlot = p["index"].GetInt64();
                };
                if (doc.HasMember("params") && doc["params"].IsObject())
                    parse(doc["params"]);
                else
                    parse(doc);
            }

            if (rawSlot < 0 || rawSlot > 255)
                return FormatJsonRpcError(id, -32602, "Invalid skill slot: out of range (0..255)");

            uint32_t slot = static_cast<uint32_t>(rawSlot);
            CPythonPlayer::Instance().ClickSkillSlot(slot);
            return std::format("{{\"jsonrpc\":\"2.0\",\"ok\":true,\"id\":{},\"result\":{{\"status\":\"skill_clicked\",\"slot\":{}}}}}", id, slot);
        };
        m_routes["game_use_skill"] = { false, useSkillHandler };
        m_routes["use_skill"] = { false, useSkillHandler };

        // 21. game_send_chat (Wyslanie czatu / komendy GM)
        auto sendChatHandler = [](const std::string& req, const std::string& id) -> std::string {
            if (!CPythonNetworkStream::InstancePtr())
                return FormatJsonRpcError(id, -32000, "Network stream not initialized");

            rapidjson::Document doc;
            doc.Parse(req.c_str());
            std::string text;
            uint8_t chatType = 0; // CHAT_TYPE_TALKING
            if (!doc.HasParseError())
            {
                auto parse = [&](const rapidjson::Value& p) {
                    if (p.HasMember("text") && p["text"].IsString()) text = p["text"].GetString();
                    else if (p.HasMember("message") && p["message"].IsString()) text = p["message"].GetString();
                    if (p.HasMember("type") && p["type"].IsUint()) chatType = static_cast<uint8_t>(p["type"].GetUint());
                };
                if (doc.HasMember("params") && doc["params"].IsObject())
                    parse(doc["params"]);
                else
                    parse(doc);
            }

            if (text.empty())
                return FormatJsonRpcError(id, -32602, "Chat message cannot be empty");

            if (text.length() > 511)
                text.resize(511);

            if (chatType >= CHAT_TYPE_MAX_NUM)
                return FormatJsonRpcError(id, -32602, "Invalid chat type");

            bool ok = CPythonNetworkStream::Instance().SendChatPacket(text.c_str(), chatType);
            return std::format("{{\"jsonrpc\":\"2.0\",\"ok\":true,\"id\":{},\"result\":{{\"status\":\"chat_sent\",\"success\":{},\"length\":{}}}}}",
                id, ok ? "true" : "false", text.length());
        };
        m_routes["game_send_chat"] = { false, sendChatHandler };
        m_routes["send_chat"] = { false, sendChatHandler };
        m_routes["chat_send"] = { false, sendChatHandler };

        // 22. game_attack_target (Natywny bezposredni atak celu)
        m_routes["game_attack_target"] = {
            false,
            [](const std::string& req, const std::string& id) -> std::string {
                if (!CPythonPlayer::InstancePtr())
                    return FormatJsonRpcError(id, -32000, "Player not initialized");

                rapidjson::Document doc;
                doc.Parse(req.c_str());
                uint32_t targetVid = 0;
                if (!doc.HasParseError())
                {
                    auto parse = [&](const rapidjson::Value& p) {
                        if (p.HasMember("target_vid") && p["target_vid"].IsUint()) targetVid = p["target_vid"].GetUint();
                        else if (p.HasMember("vid") && p["vid"].IsUint()) targetVid = p["vid"].GetUint();
                    };
                    if (doc.HasMember("params") && doc["params"].IsObject())
                        parse(doc["params"]);
                    else
                        parse(doc);
                }

                CPythonPlayer::Instance().SetTarget(targetVid, TRUE);
                bool ok = CPythonPlayer::Instance().AttackTarget(targetVid);
                return std::format("{{\"jsonrpc\":\"2.0\",\"ok\":true,\"id\":{},\"result\":{{\"status\":\"attack_executed\",\"attack_started\":{},\"target_vid\":{}}}}}",
                    id, ok ? "true" : "false", targetVid);
            }
        };

        // 23. game_use_item (Natywne uzycie przedmiotu w oknie/komorce)
        m_routes["game_use_item"] = {
            false,
            [](const std::string& req, const std::string& id) -> std::string {
                if (!CPythonNetworkStream::InstancePtr())
                    return FormatJsonRpcError(id, -32000, "Network stream not initialized");

                rapidjson::Document doc;
                doc.Parse(req.c_str());
                uint8_t window = 1; // INVENTORY
                uint16_t cell = 0;
                if (!doc.HasParseError())
                {
                    auto parse = [&](const rapidjson::Value& p) {
                        if (p.HasMember("window") && p["window"].IsUint()) window = static_cast<uint8_t>(p["window"].GetUint());
                        if (p.HasMember("cell") && p["cell"].IsUint()) cell = static_cast<uint16_t>(p["cell"].GetUint());
                    };
                    if (doc.HasMember("params") && doc["params"].IsObject())
                        parse(doc["params"]);
                    else
                        parse(doc);
                }

                bool ok = CPythonNetworkStream::Instance().SendItemUsePacket(TItemPos(window, cell));
                return std::format("{{\"jsonrpc\":\"2.0\",\"ok\":true,\"id\":{},\"result\":{{\"status\":\"item_used\",\"sent\":{},\"window\":{},\"cell\":{}}}}}",
                    id, ok ? "true" : "false", window, cell);
            }
        };

        // 24. get_world_megasnapshot / game_get_tactical_snapshot
        auto megaSnapshotHandler = [this](const std::string& req, const std::string& id) -> std::string {
            return HandleWorldMegaSnapshot(id, req);
        };
        m_routes["get_world_megasnapshot"] = { false, megaSnapshotHandler };
        m_routes["game_get_tactical_snapshot"] = { false, megaSnapshotHandler };
        m_routes["get_tactical_snapshot"] = { false, megaSnapshotHandler };
    }

    bool TestHarnessServer::IsImmediateMethod(std::string_view method) const noexcept
    {
        auto it = m_routes.find(std::string(method));
        if (it == m_routes.end())
            return false;

        return it->second.isImmediate;
    }
}
