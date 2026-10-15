#include "../StdAfx.h"
#include "CombatComponentTable.h"
#include "../Packet.h"
#include "../../EterBase/DynamicBitsetModern.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

namespace UserInterface::ECS
{
    /**
     * @brief Filters the combat component table to find alive targets and notifies the GUI.
     * @param table The combat component table to filter.
     * @return EterBase::Result<EterBase::DynamicBitsetModern, EterBase::EntityError> 
     *         A bitset representing alive entities, or an error if the table is empty.
     */
    EterBase::Result<EterBase::DynamicBitsetModern, EterBase::EntityError> FilterAliveTargets(const CombatComponentTable& table)
    {
        size_t count = table.Size();
        if (count == 0)
        {
            EterBase::ModernLogger::Warning("FilterAliveTargets: Combat component table is empty.");
            return EterBase::MakeError(EterBase::EntityError::NotFound);
        }

        EterBase::DynamicBitsetModern aliveBitset(count);
        size_t aliveCount = 0;

        for (size_t i = 0; i < count; ++i)
        {
            // Sprawdzenie zycia (HP > 0 i brak flagi isDead)
            if (table.currentHp[i] > 0 && table.isDead[i] == 0)
            {
                auto result = aliveBitset.Set(i);
                if (result)
                {
                    aliveCount++;
                    EterBase::EntityId entityId(table.entityIds[i]);
                    
                    // Powiadomienie GUI (Zero-Conflict)
                    UserInterface::Core::TargetBoardRefreshEvent refreshEvent(entityId.value());
                    UserInterface::Core::EventBus::GetInstance().Publish(refreshEvent);
                }
                else
                {
                    EterBase::ModernLogger::Error("FilterAliveTargets: Failed to set bitset for index {}", i);
                }
            }
        }

        EterBase::ModernLogger::Info("FilterAliveTargets: Found {} alive entities out of {}.", aliveCount, count);
        return aliveBitset;
    }
}
