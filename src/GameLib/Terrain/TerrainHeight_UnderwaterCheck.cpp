#include "../StdAfx.h"
#include "ITerrainHeightCache.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/ModernLogger.h"
#include "../../UserInterface/Core/EventBus.h"

namespace GameLib::Terrain {

    /**
     * @brief Zdarzenie emitowane gdy encja znajduje sie pod woda.
     */
    struct EntityUnderwaterEvent : public UserInterface::Core::IEvent {
        EterBase::EntityId entityId;
        float x;
        float y;
        float z;
        float waterHeight;

        EntityUnderwaterEvent(EterBase::EntityId id, float px, float py, float pz, float wHeight)
            : entityId(id), x(px), y(py), z(pz), waterHeight(wHeight) {}
    };

    /**
     * @brief Klasa sprawdzajaca czy dana pozycja (x, y, z) znajduje sie pod woda.
     */
    class TerrainHeight_UnderwaterCheck {
    public:
        explicit TerrainHeight_UnderwaterCheck(const ITerrainHeightCache* cache)
            : m_heightCache(cache) {
            if (!m_heightCache) {
                EterBase::ModernLogger::Error("TerrainHeight_UnderwaterCheck: zaleznosc cache wysokosci jest null");
            }
        }

        /**
         * @brief Sprawdza czy encja jest pod woda.
         * @param entityId ID encji.
         * @param x Pozycja X.
         * @param y Pozycja Y.
         * @param z Pozycja Z (wysokosc encji).
         * @return EterBase::Result<bool, EterBase::NavigationError> True jesli encja jest pod woda, False jesli nad, Error w razie bledu.
         */
        EterBase::Result<bool, EterBase::NavigationError> CheckUnderwater(EterBase::EntityId entityId, float x, float y, float z) const {
            if (!m_heightCache) {
                return EterBase::MakeError(EterBase::NavigationError::MapNotLoaded);
            }

            float waterHeight = m_heightCache->SampleWaterHeight(x, y);

            // Zwykle w Metin2 jesli woda nie wystepuje, zwracana jest specyficzna wartosc (np. -1 albo niska wartosc zaleznie od implementacji).
            // Zakladamy, ze jezeli z < waterHeight, to jestemy pod woda.
            bool isUnderwater = (z < waterHeight);

            if (isUnderwater) {
                EterBase::ModernLogger::Debug("Encja {} znajduje sie pod woda (Z: {}, WaterZ: {})", entityId.get(), z, waterHeight);
                UserInterface::Core::EventBus::GetInstance().Publish(EntityUnderwaterEvent(entityId, x, y, z, waterHeight));
            }

            return isUnderwater;
        }

    private:
        const ITerrainHeightCache* m_heightCache;
    };

} // namespace GameLib::Terrain
