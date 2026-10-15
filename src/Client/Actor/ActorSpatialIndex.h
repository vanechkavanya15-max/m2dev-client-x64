#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <cmath>
#include <shared_mutex>
#include <optional>
#include <expected>

#include "../../EterBase/StrongTypes.h"

namespace Client::Actor {

/**
 * @brief Kod bledu dla operacji na siatce przestrzennej.
 */
enum class SpatialIndexError {
    EntityNotFound,
    InvalidRadius,
    GridNotInitialized
};

/**
 * @brief Struktura reprezentujaca pozycje 2D w siatce.
 */
struct Vector2D {
    float x;
    float y;

    constexpr float Distance(const Vector2D& other) const noexcept {
        const float dx = x - other.x;
        const float dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
    }
};

/**
 * @brief Dwuwymiarowa siatka przestrzenna sluzaca do blyskawicznego wyszukiwania
 * sasiadujacych aktorow. Wykorzystuje cell-based bucketing, std::shared_mutex
 * dla bezpieczenstwa wielowatkowego oraz typy EterBase::EntityId.
 */
class ActorSpatialIndex {
public:
    /**
     * @brief Inicjalizuje siatke z zadanym rozmiarem komorki.
     * @param cellSize Rozmiar pojedynczej komorki siatki w jednostkach gry.
     */
    explicit ActorSpatialIndex(float cellSize = 1000.0f) noexcept
        : m_cellSize(cellSize > 0.0f ? cellSize : 1000.0f) {}

    ~ActorSpatialIndex() = default;

    // Brak kopiowania z racji std::shared_mutex
    ActorSpatialIndex(const ActorSpatialIndex&) = delete;
    ActorSpatialIndex& operator=(const ActorSpatialIndex&) = delete;

    /**
     * @brief Dodaje aktora do siatki przestrzennej.
     * @param id Silny identyfikator aktora.
     * @param x Wspolrzedna X.
     * @param y Wspolrzedna Y.
     */
    void Insert(EterBase::EntityId id, float x, float y) {
        std::unique_lock lock(m_mutex);

        const Vector2D pos{x, y};
        const uint64_t cellHash = GetCellHash(pos);

        m_entityPositions.insert_or_assign(id, pos);
        m_cells[cellHash].insert(id);
    }

    /**
     * @brief Aktualizuje pozycje aktora w siatce.
     * @param id Silny identyfikator aktora.
     * @param x Nowa wspolrzedna X.
     * @param y Nowa wspolrzedna Y.
     * @return true jesli zaktualizowano, false jesli nie znaleziono aktora.
     */
    std::expected<void, SpatialIndexError> Update(EterBase::EntityId id, float x, float y) {
        std::unique_lock lock(m_mutex);

        auto it = m_entityPositions.find(id);
        if (it == m_entityPositions.end()) {
            return std::unexpected(SpatialIndexError::EntityNotFound);
        }

        const Vector2D oldPos = it->second;
        const Vector2D newPos{x, y};

        const uint64_t oldCellHash = GetCellHash(oldPos);
        const uint64_t newCellHash = GetCellHash(newPos);

        it->second = newPos;

        if (oldCellHash != newCellHash) {
            RemoveFromCell(oldCellHash, id);
            m_cells[newCellHash].insert(id);
        }

        return {};
    }

    /**
     * @brief Usuwa aktora z siatki. Nie wymaga znajomosci starych koordynatow.
     * @param id Silny identyfikator aktora do usuniecia.
     */
    std::expected<void, SpatialIndexError> Remove(EterBase::EntityId id) {
        std::unique_lock lock(m_mutex);

        auto it = m_entityPositions.find(id);
        if (it == m_entityPositions.end()) {
            return std::unexpected(SpatialIndexError::EntityNotFound);
        }

        const uint64_t cellHash = GetCellHash(it->second);
        RemoveFromCell(cellHash, id);
        
        m_entityPositions.erase(it);
        
        return {};
    }

    /**
     * @brief Calkowicie czysci zawartosc siatki (usuwa wszystkie byty).
     */
    void Clear() noexcept {
        std::unique_lock lock(m_mutex);
        m_cells.clear();
        m_entityPositions.clear();
    }

    /**
     * @brief Pobiera liczbe zarejestrowanych aktorow.
     * @return Liczba bytow w siatce.
     */
    [[nodiscard]] size_t Count() const noexcept {
        std::shared_lock lock(m_mutex);
        return m_entityPositions.size();
    }

    /**
     * @brief Zwraca liste identyfikatorow aktorow znajdujacych sie dokladnie 
     * w zasiegu danego promienia euklidesowego.
     * @param centerX Srodek okregu (X).
     * @param centerY Srodek okregu (Y).
     * @param radius Promien wyszukiwania (musi byc dodatni).
     * @return Wektor z identyfikatorami lub blad.
     */
    [[nodiscard]] std::expected<std::vector<EterBase::EntityId>, SpatialIndexError> 
    QueryRadiusExact(float centerX, float centerY, float radius) const {
        if (radius <= 0.0f) {
            return std::unexpected(SpatialIndexError::InvalidRadius);
        }

        std::shared_lock lock(m_mutex);
        
        std::vector<EterBase::EntityId> result;
        const Vector2D center{centerX, centerY};

        const int32_t minX = GetGridCoord(centerX - radius);
        const int32_t maxX = GetGridCoord(centerX + radius);
        const int32_t minY = GetGridCoord(centerY - radius);
        const int32_t maxY = GetGridCoord(centerY + radius);

        for (int32_t gridX = minX; gridX <= maxX; ++gridX) {
            for (int32_t gridY = minY; gridY <= maxY; ++gridY) {
                const uint64_t cellHash = ComputeHash(gridX, gridY);
                
                auto cellIt = m_cells.find(cellHash);
                if (cellIt != m_cells.end()) {
                    for (const auto& entityId : cellIt->second) {
                        // Dodatkowy check precyzyjny euklidesowy
                        auto posIt = m_entityPositions.find(entityId);
                        if (posIt != m_entityPositions.end()) {
                            if (center.Distance(posIt->second) <= radius) {
                                result.push_back(entityId);
                            }
                        }
                    }
                }
            }
        }

        return result;
    }

    /**
     * @brief Zwraca pozycje konkretnego aktora, jesli istnieje w siatce.
     * @param id Identyfikator aktora.
     * @return Pozycja (Vector2D) lub brak wartosci (std::nullopt).
     */
    [[nodiscard]] std::optional<Vector2D> GetPosition(EterBase::EntityId id) const {
        std::shared_lock lock(m_mutex);
        auto it = m_entityPositions.find(id);
        if (it != m_entityPositions.end()) {
            return it->second;
        }
        return std::nullopt;
    }

private:
    float m_cellSize;
    mutable std::shared_mutex m_mutex;
    
    // Slownik sluzacy do szybkiego wyszukiwania w jakiej komorce
    // i na jakich konkretnie koordynatach znajduje sie byt.
    std::unordered_map<EterBase::EntityId, Vector2D> m_entityPositions;
    
    // Siatka przechowujaca identyfikatory dla danej komorki haszujacej.
    std::unordered_map<uint64_t, std::unordered_set<EterBase::EntityId>> m_cells;

    [[nodiscard]] int32_t GetGridCoord(float coord) const noexcept {
        return static_cast<int32_t>(std::floor(coord / m_cellSize));
    }

    [[nodiscard]] uint64_t ComputeHash(int32_t gridX, int32_t gridY) const noexcept {
        const uint32_t ux = static_cast<uint32_t>(gridX);
        const uint32_t uy = static_cast<uint32_t>(gridY);
        return (static_cast<uint64_t>(ux) << 32) | uy;
    }

    [[nodiscard]] uint64_t GetCellHash(const Vector2D& pos) const noexcept {
        return ComputeHash(GetGridCoord(pos.x), GetGridCoord(pos.y));
    }

    void RemoveFromCell(uint64_t cellHash, EterBase::EntityId id) {
        auto cellIt = m_cells.find(cellHash);
        if (cellIt != m_cells.end()) {
            cellIt->second.erase(id);
            if (cellIt->second.empty()) {
                m_cells.erase(cellIt);
            }
        }
    }
};

} // namespace Client::Actor
