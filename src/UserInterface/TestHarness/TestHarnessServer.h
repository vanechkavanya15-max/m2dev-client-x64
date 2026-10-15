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
#include <unordered_map>
#include <functional>

namespace UserInterface::TestHarness
{
    enum class HarnessTaskState : uint32_t
    {
        PENDING = 0,
        EXECUTING = 1,
        COMPLETED = 2,
        TIMED_OUT = 3
    };

    struct HarnessTask
    {
        std::string rawJson;
        std::promise<std::string> promiseResult;
        std::atomic<HarnessTaskState> state{HarnessTaskState::PENDING};
    };

    /**
     * @brief Serwer komunikacji Named Pipe dla agentow AI i MCP (\\.\pipe\M2ClientEngineHarness).
     * 
     * Protokol JSON-RPC dla komend:
     * - get_state / get_engine_state: zwraca JSON ze stanem gracza, koordynatami, HP, celem dwTargetVID, stany kontrolerow (Movement, Combat, Target, Item).
     * - tick_frame: przepycha dokladnie N klatek gry (deterministyczny step).
     * - inject_packet: wstrzykuje pakiet bezposrednio do PhaseGamePacketDispatcher.
     * - audit_state / audit_dual_state: porownuje stan C++ z Pythonem oraz weryfikuje spojnosc bytow swiata z ActorRegistry.
     * - Metody AgentMCPBridge: game_ping, game_get_state, game_get_inventory, game_get_nearby_entities, game_action_attack, game_action_use_item.
     * 
     * Kompatybilnosc transportu:
     * Deterministyczna obsluga protokolu z 4-bajtowym prefiksem dlugosci (uint32_t little-endian)
     * z pelna akumulacja bufora dla pakietow > 64 KB oraz obsluga surowego JSON-RPC.
     * 
     * Bezpieczna synchronizacja i eliminacja Data Race:
     * Metody dotykajace obiektow gry (get_state, get_engine_state, game_get_state, tick_frame, inject_packet itd.)
     * sa kolejkowane do glownego watku gry (CPythonApplication::Process) w celu wyeliminowania wspolbieznego
     * dostepu pomiedzy watkiem I/O a CPythonApplication::UpdateGame().
     * Metody bezstanowe/niezalezne od obiektow gry (game_ping, shutdown) moga byc wykonywane od razu (isImmediate = true).
     * 
     * Eliminacja Ghost Execution:
     * Kazde zadanie posiada atomowa maszyne stanow (PENDING -> EXECUTING -> COMPLETED / TIMED_OUT),
     * gwarantujac, ze zadanie anulowane timeoutem 5s nie wykona sie po cichu w tle.
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
        std::string HandleAuditState(const std::string& id, const std::string& requestJson = "");
        std::string HandleSetWindowMode(const std::string& requestJson, const std::string& id);
        std::string HandleSetProtocolDriver(const std::string& requestJson, const std::string& id);
        std::string HandleLogin(const std::string& requestJson, const std::string& id);
        std::string HandleSelectCharacter(const std::string& requestJson, const std::string& id);
        std::string HandleGetCharacterSlots(const std::string& id);
        std::string HandleCreateCharacter(const std::string& requestJson, const std::string& id);
        std::string HandleSetAutoReconnect(const std::string& requestJson, const std::string& id);
        std::string HandleWorldMegaSnapshot(const std::string& id, const std::string& requestJson);

        struct CommandRoute
        {
            bool isImmediate{false};
            std::function<std::string(const std::string& requestJson, const std::string& id)> handler;
        };

        void RegisterBuiltinRoutes();
        [[nodiscard]] bool IsImmediateMethod(std::string_view method) const noexcept;

    private:
        std::string m_pipeName{"\\\\.\\pipe\\M2ClientEngineHarness"};
        std::atomic<bool> m_isRunning{false};
        std::atomic<HANDLE> m_hActivePipe{INVALID_HANDLE_VALUE};
        std::thread m_ioThread;

        std::mutex m_queueMutex;
        std::vector<std::shared_ptr<HarnessTask>> m_pendingTasks;
        std::unordered_map<std::string, CommandRoute> m_routes;
    };
}
