#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <optional>
#include <shared_mutex>
#include <mutex>
#include <chrono>

#include "StrongTypes.h"
#include "../Gameplay/InventoryDomain.h"
#include "../Gameplay/PlayerStatsDomain.h"
#include "../Gameplay/SkillDomain.h"
#include "../Gameplay/QuickslotDomain.h"
#include "../World/ActorRegistry.h"
#include "../World/SpatialHashGrid.h"

namespace Client::Core {

struct WorldEntity {
    uint32_t vid{0};
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
    bool isHostile{false};
};

/**
 * @brief Centralny kontekst swiata gry i stanu gracza (WorldContext) C++23.
 * 
 * Zapewnia Dependency Injection dla glownych domen:
 * - Client::Gameplay::InventoryDomain inventory
 * - Client::Gameplay::PlayerStatsDomain stats
 * - Client::Gameplay::SkillDomain skills
 * - Client::Gameplay::QuickslotDomain quickslot
 * - Client::World::ActorRegistry actors
 * - Client::World::SpatialHashGrid spatialGrid
 * 
 * Oferuje pelne wsparcie dla wielowatkowosci (std::shared_mutex) oraz
 * koordynacje logiki biznesowej i przestrzennej pomiedzy podsystemami.
 */
struct WorldContext {
    // ========================================================================
    // Instancje glownych domen C++23 (Dependency Injection)
    // ========================================================================
    Client::Gameplay::InventoryDomain inventory;
    Client::Gameplay::PlayerStatsDomain stats;
    Client::Gameplay::SkillDomain skills;
    Client::Gameplay::QuickslotDomain quickslot;
    Client::World::ActorRegistry actors;
    Client::World::SpatialHashGrid spatialGrid;

    // ========================================================================
    // Stan bazowy postaci i swiata gry
    // ========================================================================
    uint32_t currentHp{0};
    uint32_t maxHp{0};
    uint32_t currentSp{0};
    uint32_t maxSp{0};
    uint64_t currentExp{0};
    int64_t currentGold{0};
    int64_t currentCheque{0};
    int64_t currentGaya{0};
    uint32_t currentMapIndex{0};
    uint32_t currentChannel{1};
    bool isDead{false};

    EntityVid localPlayerVid{0};
    MapCoords localPlayerCoords;
    float localPlayerRotation{0.0f};
    float posX{0.0f};
    float posY{0.0f};
    float posZ{0.0f};

    std::vector<WorldEntity> entities;
    std::array<int64_t, 255> points{};

    mutable std::shared_mutex m_contextMutex;

    // ========================================================================
    // Konstruktory i zarzadzanie pamiecia
    // ========================================================================
    WorldContext();
    ~WorldContext() = default;

    WorldContext(const WorldContext&) = delete;
    WorldContext& operator=(const WorldContext&) = delete;
    WorldContext(WorldContext&&) = delete;
    WorldContext& operator=(WorldContext&&) = delete;

    // ========================================================================
    // Reset stanu calego kontekstu
    // ========================================================================
    void Reset() noexcept;

    // ========================================================================
    // Punkty i statystyki postaci (wielowatkowa synchronizacja ze stats)
    // ========================================================================
    [[nodiscard]] int64_t GetPoint(uint32_t pointType) const noexcept;
    void SetPoint(uint32_t pointType, int64_t value) noexcept;

    void SetPlayerHp(uint32_t hp, std::optional<uint32_t> maxHpVal = std::nullopt) noexcept;
    void SetPlayerSp(uint32_t sp, std::optional<uint32_t> maxSpVal = std::nullopt) noexcept;
    void SetPlayerDead(bool dead) noexcept;
    [[nodiscard]] bool IsAlive() const noexcept;

    // ========================================================================
    // Koordynacja lokalnego gracza i poruszania
    // ========================================================================
    void SetLocalPlayer(EntityVid vid, float x, float y, float z, float rotation, const std::string& name = "");
    void UpdatePlayerPosition(float x, float y, float z, float rotation) noexcept;

    // ========================================================================
    // Koordynacja aktorow i siatki przestrzennej (SpatialHashGrid + ActorRegistry)
    // ========================================================================
    bool RegisterActor(const Client::World::ActorRecord& record);
    bool UnregisterActor(EntityVid vid);
    bool UpdateActorPosition(EntityVid vid, float x, float y, float z, float rotation);
    [[nodiscard]] std::vector<Client::World::ActorRecord> FindActorsInRadius(float x, float y, float radius) const;
    [[nodiscard]] std::vector<Client::World::ActorRecord> FindNearbyActors(float radius) const;
    [[nodiscard]] std::optional<Client::World::ActorRecord> FindNearestActor(float maxRadius, bool excludeSelf = true) const;

    // ========================================================================
    // Koordynacja umiejetnosci i many (SkillDomain + PlayerStatsDomain)
    // ========================================================================
    [[nodiscard]] bool CanUseSkill(uint32_t skillId) const;
    bool UseSkill(uint32_t skillId, std::chrono::milliseconds cooldownDuration);
    bool UseSkillMs(uint32_t skillId, uint32_t cooldownMs);

    [[nodiscard]] Money64 GetGoldAmount() const noexcept;
    void SetGoldAmount(Money64 gold) noexcept;

    // ========================================================================
    // Koordynacja paska szybkiego dostepu i ekwipunku
    // ========================================================================
    bool BindQuickslotSkill(uint32_t quickslotIndex, uint32_t skillId);
    bool BindQuickslotSkill(SlotIndex quickslotIndex, SkillId skillId);
    bool BindQuickslotItem(uint32_t quickslotIndex, uint16_t inventorySlot);
    bool BindQuickslotItem(SlotIndex quickslotIndex, SlotIndex inventorySlot);

    [[nodiscard]] std::shared_mutex& GetMutex() const noexcept { return m_contextMutex; }
};

} // namespace Client::Core
