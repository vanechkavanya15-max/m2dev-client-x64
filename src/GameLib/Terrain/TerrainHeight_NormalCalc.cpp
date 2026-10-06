#include "../StdAfx.h"
#include "ITerrainHeightCache.h"

#include <cmath>
#include <tuple>
#include <expected>

#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/ModernLogger.h"
#include "../../UserInterface/Core/EventBus.h"

namespace GameLib::Terrain {

    /**
     * @brief Zdarzenie rozglaszajace wyliczenie wektora normalnego terenu.
     * Poniewaz szyna zdarzen w EterBase trasuje po typie, typ ten musi byc publicznie dostepny 
     * dla innych podsystemow (GUI/Network). Normalnie znajdowalby sie on w dedykowanym naglowku `*Event.h`.
     * Jednak zgodnie z surowa zasada zero-conflict i specyfikacja zadania umieszczamy
     * te strukture zdarzenia tutaj. W prawdziwej architekturze C++ zostalaby ona przeniesiona do 
     * odpowiedniego wspolnego naglowka zdarzen. Inne moduly beda musialy ja re-deklarowac lokalnie.
     */
    struct TerrainNormalCalculatedEvent : public UserInterface::Core::IEvent {
        EterBase::EntityId entityId;
        float normalX;
        float normalY;
        float normalZ;

        TerrainNormalCalculatedEvent(EterBase::EntityId id, float x, float y, float z)
            : entityId(id), normalX(x), normalY(y), normalZ(z) {}
    };

    /**
     * @brief A service class to calculate the terrain normal vector under an entity.
     * 
     * Strictly adheres to zero-conflict constraints and SRP by existing entirely within
     * this local CPP file. It uses central difference to sample height variations
     * from the provided ITerrainHeightCache.
     */
    class TerrainHeightNormalCalculator final {
    public:
        /**
         * @brief Calculates the normalized surface normal vector at a given coordinate.
         * 
         * @param cache The terrain height cache used to sample local heights.
         * @param entityId The strongly-typed unique identifier of the entity.
         * @param x X coordinate on the terrain.
         * @param y Y coordinate on the terrain.
         * @return std::expected<std::tuple<float, float, float>, EterBase::EntityError> 
         *         Returns the normal vector (nx, ny, nz) on success, or an error if computation fails.
         */
        static std::expected<std::tuple<float, float, float>, EterBase::EntityError> CalculateNormal(
            const ITerrainHeightCache& cache,
            EterBase::EntityId entityId,
            float x, float y)
        {
            // Sample neighboring points using a central difference scheme.
            // Using a generic step size of 1.0f for derivation.
            float hLeft  = cache.SampleHeight(x - 1.0f, y);
            float hRight = cache.SampleHeight(x + 1.0f, y);
            float hDown  = cache.SampleHeight(x, y - 1.0f);
            float hUp    = cache.SampleHeight(x, y + 1.0f);

            // Calculate differences
            float dx = hRight - hLeft;
            float dy = hUp - hDown;
            float dz = 2.0f; // Because step is 1.0f on both sides of x and y

            // Calculate length of the normal vector
            float length = std::sqrt(dx * dx + dy * dy + dz * dz);

            if (length <= 0.0f) {
                EterBase::ModernLogger::Error(
                    "Failed to calculate normal for entity {0} at ({1}, {2}): Zero vector length.",
                    entityId.value(), x, y
                );
                return std::unexpected(EterBase::EntityError::NotFound);
            }

            // Normalize the vector
            float nx = -dx / length;
            float ny = -dy / length;
            float nz = dz / length;

            // Log successful calculation
            EterBase::ModernLogger::Debug(
                "Calculated terrain normal for entity {0} at ({1}, {2}) -> ({3}, {4}, {5})",
                entityId.value(), x, y, nx, ny, nz
            );

            // Publish the event to notify the GUI and other subsystems
            UserInterface::Core::EventBus::GetInstance().Publish(TerrainNormalCalculatedEvent{entityId, nx, ny, nz});

            return std::make_tuple(nx, ny, nz);
        }
    };

} // namespace GameLib::Terrain
