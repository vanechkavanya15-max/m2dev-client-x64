#include "../StdAfx.h"
#include "TransformComponentTable.h"
#include "CombatComponentTable.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "../Packet.h"
#include "../Core/EventBus.h"

namespace UserInterface::ECS {

/**
 * @brief Zdarzenie rozglaszane przez EventBus po wypieciu encji.
 */
struct EntityDespawnedEvent : public Core::IEvent {
    uint32_t entityId;

    explicit EntityDespawnedEvent(uint32_t id) : entityId(id) {}
};

/**
 * @brief Hak wywolywany podczas usuwania instancji encji z ECS.
 * Dziala w oparciu o tabele SoA (Transform i Combat).
 */
class ECSBridge_DespawnHook {
public:
    /**
     * @brief Usuwa encje z tabel komponentow.
     * @param entityId Unikalny identyfikator encji.
     * @param transformTable Tabela pozycji i rotacji.
     * @param combatTable Tabela HP i stanow walki.
     * @return Zwraca EterBase::PacketResult<void> dla spelnienia wymogow C++23.
     */
    static EterBase::PacketResult<void> DespawnActor(
        EterBase::EntityId entityId, 
        TransformComponentTable& transformTable, 
        CombatComponentTable& combatTable) 
    {
        const uint32_t rawId = entityId.value();
        
        // Remove from SoA tables
        transformTable.Remove(rawId);
        combatTable.Remove(rawId);

        // Dekouplowane powiadomienie przez EventBus
        Core::EventBus::GetInstance().Publish(EntityDespawnedEvent{rawId});
        
        EterBase::ModernLogger::Info("ECSBridge_DespawnHook: Aktor {} wyrejestrowany z tabel SoA.", rawId);
        
        return {};
    }
};

} // namespace UserInterface::ECS
