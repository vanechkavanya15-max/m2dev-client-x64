#include "../StdAfx.h"
#include "TransformComponentTable.h"
#include "../Packet.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

namespace
{
    // Global table for SoA architecture (Zero-Conflict)
    UserInterface::ECS::TransformComponentTable g_transformTable;
}

namespace UserInterface::ECS
{
    /**
     * @brief Zdarzenie rozglaszane po udanym utworzeniu encji.
     * Oddziela logike silnika od GUI i innych systemow.
     */
    struct EntitySpawnedUIEvent : public Core::IEvent
    {
        uint32_t entityId;
        float posX;
        float posY;
        float posZ;
        float rotation;

        /**
         * @brief Konstruktor zdarzenia.
         * @param id Identyfikator encji.
         * @param x Pozycja X.
         * @param y Pozycja Y.
         * @param z Pozycja Z (wysokosc).
         * @param rot Rotacja.
         */
        EntitySpawnedUIEvent(uint32_t id, float x, float y, float z, float rot)
            : entityId(id), posX(x), posY(y), posZ(z), rotation(rot) {}
    };

    /**
     * @brief Hook rejestrujacy nowego potwora w architekturze SoA ECS.
     * Wywolywany w trakcie CPythonCharacterManager::CreateInstance.
     * 
     * @param entityId Silny typ z EterBase.
     * @param posX Pozycja X w grze.
     * @param posY Pozycja Y w grze.
     * @param posZ Pozycja Z (wysokosc).
     * @param rotation Rotacja encji.
     * @param speed Predkosc ruchu encji.
     * @return EterBase::Result<void, EterBase::EntityError> (std::expected).
     */
    [[nodiscard]] std::expected<void, EterBase::EntityError> Hook_OnCreateInstance(
        EterBase::EntityId entityId, 
        float posX, float posY, float posZ, 
        float rotation, float speed)
    {
        if (entityId.value() == 0)
        {
            EterBase::ModernLogger::Error("ECSBridge_SpawnHook: Failed to spawn entity - Invalid EntityId (0)");
            return std::unexpected(EterBase::EntityError::InvalidType);
        }

        // Dodanie/aktualizacja encji do tabel SoA.
        g_transformTable.AddOrUpdate(entityId.value(), posX, posY, posZ, rotation, speed);

        EterBase::ModernLogger::Info("ECSBridge_SpawnHook: Entity {} added to TransformComponentTable SoA at ({}, {}, {})", 
            entityId.value(), posX, posY, posZ);

        // Powiadomienie GUI / innych podsystemow o pojawieniu sie encji
        Core::EventBus::GetInstance().Publish(EntitySpawnedUIEvent{
            entityId.value(), posX, posY, posZ, rotation
        });

        return {};
    }
}
