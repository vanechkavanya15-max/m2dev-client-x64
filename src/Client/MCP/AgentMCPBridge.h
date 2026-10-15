#pragma once

#include "Client/Contracts/IPlayerDomainContract.h"
#include "Client/Contracts/IInventoryDomainContract.h"
#include "Client/Contracts/ICombatDomainContract.h"
#include "Client/Contracts/IWorldQueryContract.h"

#include <string>
#include <string_view>
#include <memory>
#include <chrono>
#include <optional>

namespace Network::Protocol
{
    class IServerProtocolDriver;
}

namespace Client::MCP
{
    /**
     * @brief Agent MCP Bridge - natywny mostek Model Context Protocol.
     * Konwertuje zapytania JSON-RPC 2.0 od Agentow AI (Antigravity, Claude, Jules)
     * na wywolania domenowych kontraktow C++23 bez memory hackingu.
     */
    class AgentMCPBridge
    {
    public:
        static AgentMCPBridge& Instance() noexcept;

        void Initialize(
            Contracts::IPlayerDomainContract* pPlayer,
            Contracts::IInventoryDomainContract* pInventory,
            Contracts::ICombatDomainContract* pCombat,
            Contracts::IWorldQueryContract* pWorld,
            const ::Network::Protocol::IServerProtocolDriver* pDriver = nullptr
        ) noexcept;

        void SetProtocolDriver(const ::Network::Protocol::IServerProtocolDriver* pDriver) noexcept { m_pProtocolDriver = pDriver; }
        [[nodiscard]] const ::Network::Protocol::IServerProtocolDriver* GetProtocolDriver() const noexcept { return m_pProtocolDriver; }

        // Glowny dispatch zapytan JSON-RPC od Agenta AI
        [[nodiscard]] std::string HandleJsonRpcRequest(std::string_view jsonRequest);

        // Narzedzia MCP (Natywne metody JSON)
        [[nodiscard]] std::string Tool_GetGameState() const;
        [[nodiscard]] std::string Tool_GetInventory(uint8_t windowType) const;
        [[nodiscard]] std::string Tool_GetNearbyEntities(float radius) const;
        [[nodiscard]] std::string Tool_ActionAttack(uint32_t targetVid);
        [[nodiscard]] std::string Tool_ActionUseItem(uint8_t windowType, uint16_t cell);

        // Rate Limiter
        [[nodiscard]] uint32_t GetMinAttackIntervalMs() const noexcept;
        [[nodiscard]] uint32_t GetMinActionIntervalMs() const noexcept;
        void SetMinAttackIntervalMs(std::optional<uint32_t> intervalMs) noexcept { m_overrideMinAttackIntervalMs = intervalMs; }
        void SetMinActionIntervalMs(std::optional<uint32_t> intervalMs) noexcept { m_overrideMinActionIntervalMs = intervalMs; }
        [[nodiscard]] bool CanExecuteAttack() const noexcept;
        [[nodiscard]] bool CanExecuteAction() const noexcept;
        void ResetRateLimiter() noexcept;

        [[nodiscard]] bool IsReady() const noexcept { return m_pPlayer != nullptr; }

        [[nodiscard]] Contracts::IPlayerDomainContract* GetPlayerContract() const noexcept { return m_pPlayer; }
        [[nodiscard]] Contracts::IInventoryDomainContract* GetInventoryContract() const noexcept { return m_pInventory; }
        [[nodiscard]] Contracts::ICombatDomainContract* GetCombatContract() const noexcept { return m_pCombat; }
        [[nodiscard]] Contracts::IWorldQueryContract* GetWorldContract() const noexcept { return m_pWorld; }

    private:
        AgentMCPBridge() = default;
        ~AgentMCPBridge() = default;
        AgentMCPBridge(const AgentMCPBridge&) = delete;
        AgentMCPBridge& operator=(const AgentMCPBridge&) = delete;

    private:
        Contracts::IPlayerDomainContract* m_pPlayer{nullptr};
        Contracts::IInventoryDomainContract* m_pInventory{nullptr};
        Contracts::ICombatDomainContract* m_pCombat{nullptr};
        Contracts::IWorldQueryContract* m_pWorld{nullptr};
        const ::Network::Protocol::IServerProtocolDriver* m_pProtocolDriver{nullptr};

        // Zmienne stanu Rate Limitera
        std::chrono::steady_clock::time_point m_lastAttackTime{};
        std::chrono::steady_clock::time_point m_lastActionTime{};
        bool m_hasLastAttackTime{false};
        bool m_hasLastActionTime{false};
        std::optional<uint32_t> m_overrideMinAttackIntervalMs{std::nullopt};
        std::optional<uint32_t> m_overrideMinActionIntervalMs{std::nullopt};
    };
}
