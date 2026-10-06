#include "../StdAfx.h"
#include "TransformComponentTable.h"
#include "../InstanceBase.h"
#include "../PythonCharacterManager.h"
#include "../Core/EventBus.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/StrongTypes.h"
#include "../Packet.h" // Required by contract rules

namespace UserInterface::ECS
{
    /**
     * @brief Zdarzenie publikowane po zsynchronizowaniu transformacji w ECS.
     */
    struct ECSSyncCompletedEvent : public UserInterface::Core::IEvent
    {
        uint32_t syncedEntitiesCount;
        
        explicit ECSSyncCompletedEvent(uint32_t count) : syncedEntitiesCount(count) {}
    };

    /**
     * @brief Synchronizuje dane z tabeli SoA transformacji do renderera (CInstanceBase).
     * @param transformTable Referencja do tabeli SoA TransformComponentTable.
     * @return PacketResult<void> oznaczajacy sukces lub blad podczas synchronizacji.
     */
    EterBase::PacketResult<void> SyncTransformToRender(const TransformComponentTable& transformTable)
    {
        auto& charMgr = CPythonCharacterManager::Instance();
        
        size_t count = transformTable.Size();
        uint32_t syncedCount = 0;

        for (size_t i = 0; i < count; ++i)
        {
            EterBase::EntityId entityId(transformTable.entityIds[i]);
            CInstanceBase* pInstance = charMgr.GetInstancePtr(entityId.value());

            if (!pInstance)
            {
                EterBase::ModernLogger::Debug("SyncTransformToRender: EntityId {} not found in CharacterManager", entityId.value());
                continue;
            }

            TPixelPosition targetPos;
            targetPos.x = transformTable.posX[i];
            targetPos.y = transformTable.posY[i];
            targetPos.z = transformTable.posZ[i];

            // Uaktualnij pozycje i rotacje instancji
            pInstance->NEW_SetPixelPosition(targetPos);
            pInstance->SetRotation(transformTable.rotation[i]);
            
            // Opcjonalnie: aplikowanie docelowej rotacji i predkosci jesli sa uzywane w InstanceBase
            // pInstance->SetRotationSpeed(transformTable.velocity[i]);

            syncedCount++;
        }

        EterBase::ModernLogger::Info("ECSBridge_SyncToRender: Zsynchronizowano {}/{} encji.", syncedCount, count);

        // Powiadom inne systemy o zakonczeniu synchronizacji klatki
        UserInterface::Core::EventBus::GetInstance().Publish(ECSSyncCompletedEvent{syncedCount});

        return {};
    }
}
