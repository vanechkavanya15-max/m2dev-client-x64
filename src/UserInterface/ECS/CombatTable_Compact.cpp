#include "../StdAfx.h"
#include "CombatComponentTable.h"
#include "../Packet.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/Events.h"
#include "../Core/EventBus.h"

namespace UserInterface::ECS
{
    /**
     * @brief Compacts the combat component table by removing dead entities.
     * @param table The combat component table to compact.
     * @return EterBase::PacketResult<void> indicating success.
     */
    EterBase::PacketResult<void> CompactCombatTable(CombatComponentTable& table)
    {
        size_t size = table.Size();
        if (size == 0)
        {
            return EterBase::PacketResult<void>{};
        }

        // Iterate backwards so that removal via swap-and-pop doesn't affect unprocessed elements
        for (size_t i = size; i > 0; --i)
        {
            size_t idx = i - 1;
            if (table.isDead[idx])
            {
                EterBase::EntityId entityId(table.entityIds[idx]);
                
                // Remove the entity
                table.Remove(entityId.get());
                
                // Log the compaction
                EterBase::ModernLogger::Info("Compacting dead entity: {}", entityId);
                
                // Publish event to decouple UI
                ::Core::Events::TargetDelete evt{ entityId.get() };
                UserInterface::Core::EventBus::GetInstance().Publish(evt);
            }
        }
        
        return EterBase::PacketResult<void>{};
    }
}
