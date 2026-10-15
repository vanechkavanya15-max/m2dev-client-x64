#pragma once

#include "Client/Contracts/IPlayerDomainContract.h"
#include "Client/Contracts/IInventoryDomainContract.h"
#include "Client/Contracts/ICombatDomainContract.h"
#include "Client/Contracts/IWorldQueryContract.h"

namespace Client::Adapters
{
    /**
     * @brief Adapter domeny gracza mapujacy IPlayerDomainContract na CPythonPlayer i CInstanceBase.
     */
    class PlayerDomainAdapter final : public Client::Contracts::IPlayerDomainContract
    {
    public:
        PlayerDomainAdapter() = default;
        ~PlayerDomainAdapter() override = default;

        [[nodiscard]] uint32_t GetPlayerVID() const noexcept override;
        [[nodiscard]] std::string_view GetPlayerName() const noexcept override;
        [[nodiscard]] int GetPlayerLevel() const noexcept override;

        [[nodiscard]] int64_t GetCurrentHP() const noexcept override;
        [[nodiscard]] int64_t GetMaxHP() const noexcept override;
        [[nodiscard]] int64_t GetCurrentSP() const noexcept override;
        [[nodiscard]] int64_t GetMaxSP() const noexcept override;
        [[nodiscard]] int64_t GetYang() const noexcept override;
        [[nodiscard]] int64_t GetExp() const noexcept override;

        [[nodiscard]] Client::Contracts::PlayerCoordinates GetCoordinates() const noexcept override;
        [[nodiscard]] uint32_t GetTargetVID() const noexcept override;
        [[nodiscard]] bool IsInCombat() const noexcept override;

        [[nodiscard]] Client::Contracts::PlayerStatusSnapshot CaptureSnapshot() const override;
        [[nodiscard]] int64_t GetStatus(int statusType) const override;
        bool RequestMove(float x, float y) override;
    };

    /**
     * @brief Adapter domeny ekwipunku mapujacy IInventoryDomainContract na CPythonPlayer.
     */
    class InventoryDomainAdapter final : public Client::Contracts::IInventoryDomainContract
    {
    public:
        InventoryDomainAdapter() = default;
        ~InventoryDomainAdapter() override = default;

        [[nodiscard]] std::optional<Client::Contracts::ItemDataSnapshot> GetItem(const Client::Contracts::ItemCellAddress& pos) const override;
        [[nodiscard]] std::vector<Client::Contracts::ItemDataSnapshot> GetAllItems(uint8_t windowType) const override;

        [[nodiscard]] bool IsCellEmpty(const Client::Contracts::ItemCellAddress& pos) const noexcept override;
        [[nodiscard]] size_t GetFreeSlotCount(uint8_t windowType) const noexcept override;

        bool RequestUseItem(const Client::Contracts::ItemCellAddress& pos) override;
        bool RequestMoveItem(const Client::Contracts::ItemCellAddress& src, const Client::Contracts::ItemCellAddress& dst, uint32_t count) override;
        bool RequestDropItem(const Client::Contracts::ItemCellAddress& pos, uint32_t count) override;
    };

    /**
     * @brief Adapter domeny walki mapujacy ICombatDomainContract na CPythonPlayer.
     */
    class CombatDomainAdapter final : public Client::Contracts::ICombatDomainContract
    {
    public:
        CombatDomainAdapter() = default;
        ~CombatDomainAdapter() override = default;

        [[nodiscard]] Client::Contracts::CombatVerdict EvaluateAttack(uint32_t targetVid) const override;
        [[nodiscard]] float GetAttackCooldown() const noexcept override;
        [[nodiscard]] uint32_t GetLastTargetVID() const noexcept override;

        bool ExecuteAttack(uint32_t targetVid) override;
        void UpdateCombatState(float deltaTime) override;
    };

    /**
     * @brief Adapter percepcji swiata mapujacy IWorldQueryContract na CPythonCharacterManager.
     */
    class WorldQueryAdapter final : public Client::Contracts::IWorldQueryContract
    {
    public:
        WorldQueryAdapter() = default;
        ~WorldQueryAdapter() override = default;

        [[nodiscard]] std::vector<Client::Contracts::EntitySnapshot> GetEntitiesInRadius(float radius) const override;
        [[nodiscard]] std::optional<Client::Contracts::EntitySnapshot> GetEntityByVID(uint32_t vid) const override;
        [[nodiscard]] std::optional<Client::Contracts::EntitySnapshot> GetNearestHostile() const override;
        [[nodiscard]] std::optional<Client::Contracts::EntitySnapshot> GetNearestItemGround() const override;
        int ForEachEntityInRadius(float radius, Client::Contracts::EntityVisitor visitor, void* userData) const override;
    };

    /**
     * @brief Inicjalizuje natywny mostek MCP domyslnymi adapterami silnika gry.
     */
    void InitializeMCPAdapters();
}
