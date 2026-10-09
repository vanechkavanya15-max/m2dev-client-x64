#pragma once

#include <memory>
#include <string_view>
#include <cstdint>
#include "Client/Core/WorldContext.h"
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
    // Konfiguracja fasady (wstrzykiwanie kontekstu)
    // ========================================================================
    void SetWorldContext(Client::Core::WorldContext* context) noexcept;
    void ClearWorldContext() noexcept;
    [[nodiscard]] Client::Core::WorldContext* GetWorldContext() const noexcept;

    // ========================================================================
    // Operacje na Aktorach / Instancjach
    // ========================================================================
    
    /// @brief Rejestruje nowa instancje w ActorRegistry i SpatialHashGrid
    [[nodiscard]] EterBase::VoidResult<> RegisterInstance(uint32_t vid, uint32_t race, uint8_t type, float x, float y, float z, float rotation, std::string_view name);

    /// @brief Wyrejestrowuje instancje z domen (przy niszczeniu obiektu)
    [[nodiscard]] EterBase::VoidResult<> UnregisterInstance(uint32_t vid);

    /// @brief Aktualizuje koordynaty przestrzenne instancji
    [[nodiscard]] EterBase::VoidResult<> UpdatePosition(uint32_t vid, float x, float y, float z, float rotation);

    /// @brief Oznacza instancje jako martwa
    [[nodiscard]] EterBase::VoidResult<> SetDead(uint32_t vid, bool isDead);

    /// @brief Ustawia instancje glownego gracza (MainActor)
    [[nodiscard]] EterBase::VoidResult<> SetMainInstance(uint32_t vid, float x, float y, float z, float rotation, std::string_view name);

private:
    StranglerInstanceFacade() = default;
    ~StranglerInstanceFacade() = default;

    Client::Core::WorldContext* m_context{nullptr};
};

} // namespace Client::Bridge
