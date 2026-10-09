#pragma once

#include <memory>
#include <string>
#include <optional>
#include "../Core/WorldContext.h"
#include "../Core/StrongTypes.h"
#include "InventoryDomain.h"
#include "PlayerStatsDomain.h"

namespace Client::Gameplay {

/**
 * @brief Symulator stanu gracza lokalnego do testow automatycznych scenariuszy gry (Headless).
 * 
 * MockPlayerFixture dostarcza izolowane srodowisko testowe oparte na WorldContext.
 * Pozwala na wygodne konfigurowanie stanu gracza (HP, SP, ekwipunek, polozenie)
 * w testach jednostkowych (np. w doctest). Uzywa std::unique_ptr dla bezpieczenstwa pamieci
 * (brak surowych wskaznikow).
 */
class MockPlayerFixture {
public:
    MockPlayerFixture() : m_context(std::make_unique<Client::Core::WorldContext>()) {}
    ~MockPlayerFixture() = default;

    // Brak kopiowania, mozliwe przenoszenie
    MockPlayerFixture(const MockPlayerFixture&) = delete;
    MockPlayerFixture& operator=(const MockPlayerFixture&) = delete;
    MockPlayerFixture(MockPlayerFixture&&) noexcept = default;
    MockPlayerFixture& operator=(MockPlayerFixture&&) noexcept = default;

    /**
     * @brief Zwraca referencje do glownego kontekstu swiata.
     */
    [[nodiscard]] Client::Core::WorldContext& GetContext() const noexcept {
        return *m_context;
    }

    /**
     * @brief Konfiguruje lokalnego gracza (pozycje, VID, nazwe).
     */
    void SetupLocalPlayer(uint32_t vid, float x, float y, float z, float rotation = 0.0f, const std::string& name = "TestPlayer") {
        m_context->SetLocalPlayer(Client::Core::EntityVid{vid}, x, y, z, rotation, name);
    }

    /**
     * @brief Ustawia statystyki punktowe gracza (HP, SP, itd.).
     */
    void SetStats(uint32_t hp, uint32_t maxHp, uint32_t sp, uint32_t maxSp) {
        m_context->SetPlayerHp(hp, maxHp);
        m_context->SetPlayerSp(sp, maxSp);
    }

    /**
     * @brief Dodaje przedmiot do ekwipunku gracza.
     */
    void SetInventoryItem(uint16_t slot, uint32_t vnum, uint32_t count = 1) {
        Client::Gameplay::ItemData item{};
        item.vnum = EterBase::ItemVnum{vnum};
        item.count = count;
        // Wypelniamy item pustymi socketami/atrybutami jesli trzeba, tutaj domyslne starcza.
        (void)m_context->inventory.SetItem(Client::Gameplay::InventoryWindow::Inventory, EterBase::ItemSlot{slot}, item);
    }

    /**
     * @brief Rejestruje wrogiego aktora (moba/przeciwnika) na mapie.
     */
    void SpawnEnemy(uint32_t vid, float x, float y, float z, float rotation = 0.0f) {
        Client::World::ActorRecord record{
            .vid = EterBase::EntityId{vid},
            .race = 101, // Przykladowy race moba
            .type = 1,   // Typ mob
            .x = x,
            .y = y,
            .z = z,
            .rotation = rotation,
            .name = "TestMob",
            .guildId = 0,
            .empire = 1,
            .isDead = false
        };
        m_context->RegisterActor(record);
    }

    /**
     * @brief Cofa wszystkie zmiany - przywraca czysty stan kontekstu.
     */
    void Reset() noexcept {
        m_context->Reset();
    }

private:
    std::unique_ptr<Client::Core::WorldContext> m_context;
};

} // namespace Client::Gameplay
