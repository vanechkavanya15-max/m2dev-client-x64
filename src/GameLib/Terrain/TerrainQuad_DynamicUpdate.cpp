#include "../StdAfx.h"
#include "ITerrainQuadtreeCuller.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../../UserInterface/Core/EventBus.h"
#include <expected>

namespace GameLib::Terrain
{
    /**
     * @brief Zdarzenie lokalne emitowane po zaktualizowaniu wezlow dynamicznych w quadtree.
     */
    struct TerrainQuadDynamicUpdatedEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId entityId;
        float newX;
        float newY;
        
        TerrainQuadDynamicUpdatedEvent(EterBase::EntityId id, float x, float y)
            : entityId(id), newX(x), newY(y) {}
    };

    /**
     * @brief C++23 Standalone Handler dla dynamicznej aktualizacji QuadTree.
     * Zgodny z SRP, Zasada Zero-Conflict. Otrzymuje interfejs przez wstrzykiwanie.
     */
    class TerrainQuadDynamicUpdater
    {
    public:
        /**
         * @brief Aktualizuje dynamiczne wezly quadtree dla zadanego obiektu (EntityId).
         * 
         * @param culler Referencja do interfejsu ITerrainQuadtreeCuller.
         * @param entityId Silny typ okreslajacy identyfikator obiektu.
         * @param newX Nowa pozycja X.
         * @param newY Nowa pozycja Y.
         * @return std::expected<void, EterBase::EntityError> Wynik operacji (sukces lub kod bledu).
         */
        static std::expected<void, EterBase::EntityError> UpdateDynamicNodes(ITerrainQuadtreeCuller& culler, EterBase::EntityId entityId, float newX, float newY)
        {
            EterBase::ModernLogger::Debug("Updating dynamic quad tree nodes for Entity: {}, NewPos: [{}, {}]", entityId.value(), newX, newY);

            // Weryfikacja wejscia
            if (!entityId)
            {
                EterBase::ModernLogger::Error("Invalid EntityId provided for terrain dynamic update");
                return std::unexpected(EterBase::EntityError::NotFound);
            }

            // Implementacja logiki aktualizacji dynamicznej QuadTree przy uzyciu culler.
            // Konwertujemy swiatowe wspolrzedne X, Y na lokalne wspolrzedne sektora.
            // W silniku Metin2 domyslny rozmiar patcha i sektora determinuje te przeliczenia.
            // Ograniczamy interakcje tylko do zdefiniowanego kontraktu ITerrainQuadtreeCuller.
            
            // Konwersja (np. uzycie wspolrzednych mapy) - tu uproszczona dla demonstracji i testu
            int32_t sectorX = static_cast<int32_t>(newX / 25600.0f); // 25600.0f is a typical sector size
            int32_t sectorY = static_cast<int32_t>(newY / 25600.0f);
            
            // Zbudowanie drzewa dla sektora zapewnia, ze podzial dynamiczny jest wlasciwy
            culler.BuildQuadtree(sectorX, sectorY);
            
            // Publish event using fully qualified name to avoid compile errors
            TerrainQuadDynamicUpdatedEvent event(entityId, newX, newY);
            UserInterface::Core::EventBus::GetInstance().Publish(event);

            EterBase::ModernLogger::Info("Terrain dynamic node successfully updated for EntityId: {}", entityId.value());
            
            return {};
        }
    };
}
