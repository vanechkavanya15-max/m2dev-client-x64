#pragma once

#include <windows.h>
#include <string>
#include <string_view>
#include <memory>
#include <thread>
#include <mutex>
#include <vector>
#include <future>
#include <atomic>
#include <cstdint>

namespace UserInterface::TestHarness
{
    struct HarnessTask
    {
        std::string rawJson;
        std::promise<std::string> promiseResult;
    };

    /**
     * @brief Serwer komunikacji Named Pipe dla agentow AI i MCP (\\.\pipe\M2ClientEngineHarness).
     * 
     * Protokol JSON-RPC dla komend:
     * - get_state: zwraca JSON ze stanem gracza, koordynatami, HP, celem dwTargetVID, stany kontrolerow (Movement, Combat, Target, Item).
     * - tick_frame: przepycha dokladnie N klatek gry (deterministyczny step).
     * - inject_packet: wstrzykuje pakiet bezposrednio do PhaseGamePacketDispatcher.
     * - audit_state: porownuje stan C++ z Pythonem.
     * 
     * Bezpieczna synchronizacja:
     * Watek I/O Named Pipe wrzuca zadania do zsynchronizowanej kolejki glownego watku gry,
     * a CPythonApplication::Process() wykonuje je synchronicznie w bezpiecznym kontekscie.
     */
    class TestHarnessServer
    {
    public:
        static TestHarnessServer& Instance() noexcept;

        TestHarnessServer();
        ~TestHarnessServer();

        TestHarnessServer(const TestHarnessServer&) = delete;
        TestHarnessServer& operator=(const TestHarnessServer&) = delete;

        /**
         * @brief Uruchamia watek sluchajacy Named Pipe.
         * @param pipeName Nazwa potoku (domyslnie "\\\\.\\pipe\\M2ClientEngineHarness").
         */
        bool Start(std::string_view pipeName = "\\\\.\\pipe\\M2ClientEngineHarness");

        /**
         * @brief Zatrzymuje serwer i zwalnia zasoby potoku.
         */
        void Stop();

        [[nodiscard]] bool IsRunning() const noexcept { return m_isRunning.load(std::memory_order_relaxed); }

        /**
         * @brief Przetwarza zadania oczekujace na wykonanie w glownym watku gry (CPythonApplication::Process).
         */
        void ProcessMainThreadQueue();

        /**
         * @brief Bezposrednie wykonanie zapytania JSON-RPC (uzywane przez watek glowny oraz testy).
         */
        std::string ExecuteCommand(const std::string& requestJson);

    private:
        void ServerThreadProc();
        void HandleClientConnection(HANDLE hPipe);

        std::string HandleGetState(const std::string& id);
        std::string HandleTickFrame(const std::string& id, uint32_t count, float deltaTime);
        std::string HandleInjectPacket(const std::string& id, uint32_t header, const std::string& hexData);
        std::string HandleAuditState(const std::string& id);

    private:
        std::string m_pipeName{"\\\\.\\pipe\\M2ClientEngineHarness"};
        std::atomic<bool> m_isRunning{false};
        std::thread m_ioThread;

        std::mutex m_queueMutex;
        std::vector<std::shared_ptr<HarnessTask>> m_pendingTasks;
    };
}
