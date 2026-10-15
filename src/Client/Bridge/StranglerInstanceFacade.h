#pragma once

#include <memory>
#include <string_view>
#include <cstdint>
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include "Client/Core/WorldContext.h"
#include "Client/Actor/EntityHandle.h"
#include "EterBase/Result.h"

namespace Client::Bridge {

/**
 * @brief Fasada migracji instancji (Strangler Fig Pattern) C++23.
 * 
 * Sluzy do przekierowywania cyklu zycia i stanow CInstanceBase bezposrednio 
 * do nowoczesnych struktur domenowych (ActorRegistry, SpatialHashGrid) 
 * zarzadzanych przez WorldContext, umozliwiajac bezpieczne i iteracyjne 
 * usuwanie starego kodu z warstwy UI.
 */
class StranglerInstanceFacade {
public:
    static StranglerInstanceFacade& Instance() noexcept;

    // ========================================================================
    // Aliasy typow C++23 dla ujednolicenia interfejsow AI
    // ========================================================================
    using EntityVid = Client::World::EntityVid;
    using EntityHandle = ::Client::Actor::EntityHandle;
    using ActorRecord = Client::World::ActorRecord;

    // ========================================================================
    // Konfiguracja fasady (wstrzykiwanie kontekstu)
    // ========================================================================
    void SetWorldContext(Client::Core::WorldContext* context) noexcept;
    void ClearWorldContext() noexcept;
    [[nodiscard]] Client::Core::WorldContext* GetWorldContext() const noexcept;

    // ========================================================================
    // Operacje na Aktorach / Instancjach (Nowoczesne C++23 z Type Hints)
    // ========================================================================
    
    /// @brief Rejestruje nowa instancje w ActorRegistry i SpatialHashGrid (EntityVid)
    [[nodiscard]] EterBase::VoidResult<> RegisterInstance(EntityVid vid, uint32_t race, uint8_t type, float x, float y, float z, float rotation, std::string_view name);
    /// @brief Przeciazenie wstecznie kompatybilne dla uint32_t
    [[nodiscard]] EterBase::VoidResult<> RegisterInstance(uint32_t vid, uint32_t race, uint8_t type, float x, float y, float z, float rotation, std::string_view name);

    /// @brief Wyrejestrowuje instancje z domen (przy niszczeniu obiektu)
    [[nodiscard]] EterBase::VoidResult<> UnregisterInstance(EntityVid vid);
    /// @brief Przeciazenie wstecznie kompatybilne dla uint32_t
    [[nodiscard]] EterBase::VoidResult<> UnregisterInstance(uint32_t vid);

    /// @brief Aktualizuje koordynaty przestrzenne instancji
    [[nodiscard]] EterBase::VoidResult<> UpdatePosition(EntityVid vid, float x, float y, float z, float rotation);
    /// @brief Przeciazenie wstecznie kompatybilne dla uint32_t
    [[nodiscard]] EterBase::VoidResult<> UpdatePosition(uint32_t vid, float x, float y, float z, float rotation);

    /// @brief Oznacza instancje jako martwa
    [[nodiscard]] EterBase::VoidResult<> SetDead(EntityVid vid, bool isDead);
    /// @brief Przeciazenie wstecznie kompatybilne dla uint32_t
    [[nodiscard]] EterBase::VoidResult<> SetDead(uint32_t vid, bool isDead);

    /// @brief Ustawia instancje glownego gracza (MainActor)
    [[nodiscard]] EterBase::VoidResult<> SetMainInstance(EntityVid vid, float x, float y, float z, float rotation, std::string_view name);
    /// @brief Przeciazenie wstecznie kompatybilne dla uint32_t
    [[nodiscard]] EterBase::VoidResult<> SetMainInstance(uint32_t vid, float x, float y, float z, float rotation, std::string_view name);

    /// @brief Zwraca EntityVid glownego aktora
    [[nodiscard]] EntityVid GetMainActorVid() const noexcept;

    /// @brief Pobiera Generational EntityHandle glownego gracza
    [[nodiscard]] std::optional<EntityHandle> GetMainActorHandle() const noexcept;

    /// @brief Sprawdza czy aktor zyje
    [[nodiscard]] bool IsActorAlive(EntityVid vid) const noexcept;
    [[nodiscard]] bool IsActorAlive(uint32_t vid) const noexcept;

    /// @brief Sprawdza czy aktor jest oznaczony jako martwy
    [[nodiscard]] bool IsActorDead(EntityVid vid) const noexcept;
    [[nodiscard]] bool IsActorDead(uint32_t vid) const noexcept;

    /// @brief Sprawdza czy aktor istnieje w rejestrze
    [[nodiscard]] bool HasActor(EntityVid vid) const noexcept;
    [[nodiscard]] bool HasActor(uint32_t vid) const noexcept;

    /// @brief Pobiera rekord aktora z rejestru swiata
    [[nodiscard]] std::optional<ActorRecord> GetActor(EntityVid vid) const;
    [[nodiscard]] std::optional<ActorRecord> GetActor(uint32_t vid) const;

    /// @brief Zwraca aktualna liczbe zarejestrowanych aktorow
    [[nodiscard]] size_t GetActorCount() const noexcept;

    /// @brief Bezpieczna funkcyjna iteracja po wszystkich zarejestrowanych aktorach w C++23
    template <typename VisitorFn>
    void for_each_actor(VisitorFn&& visitor) const {
        auto* ctx = GetWorldContext();
        if (ctx) {
            ctx->actors.for_each_actor(std::forward<VisitorFn>(visitor));
        }
    }

    /// @brief Rejestruje byt w GenerationalRegistry i zwraca bezpieczny EntityHandle
    [[nodiscard]] EterBase::Result<EntityHandle, EterBase::EntityError> RegisterGenerational(EntityVid vid);
    [[nodiscard]] EterBase::Result<EntityHandle, EterBase::EntityError> RegisterGenerational(uint32_t vid);

    /// @brief Uniewaznia EntityHandle w GenerationalRegistry, inkrementujac numer generacji
    [[nodiscard]] EterBase::Result<void, EterBase::EntityError> UnregisterGenerational(EntityHandle handle);

    /// @brief Rozwiazuje EntityHandle na VID bytu (zwraca nullopt jesli nie istnieje lub byt zmarl/uniewazniony)
    [[nodiscard]] std::optional<uint32_t> ResolveGenerational(EntityHandle handle) const;
    [[nodiscard]] std::optional<EntityVid> ResolveGenerationalVid(EntityHandle handle) const;

    /// @brief Pobiera aktualny EntityHandle dla danego VID
    [[nodiscard]] std::optional<EntityHandle> GetGenerationalHandle(EntityVid vid) const;
    [[nodiscard]] std::optional<EntityHandle> GetGenerationalHandle(uint32_t vid) const;

    /// @brief Resetuje caly stan GenerationalRegistry (czysci rejestr zachowujac historie generacji)
    void ClearGenerational() noexcept;

private:
    StranglerInstanceFacade() = default;
    ~StranglerInstanceFacade() = default;

    Client::Core::WorldContext* m_context{nullptr};

    mutable std::shared_mutex m_genMutex;
    ::Client::Actor::GenerationalRegistry<uint32_t> m_generationalRegistry;
    std::unordered_map<uint32_t, ::Client::Actor::EntityHandle> m_vidToHandle;
};

} // namespace Client::Bridge
