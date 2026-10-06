#include "../../StdAfx.h"
#include "../CombatComponentTable.h"
#include "ISIMDCombatEvaluator.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"

#include <vector>

namespace UserInterface::ECS::SIMD
{
    /**
     * @brief Zapisuje zaktualizowane stany walki do tabeli SoA na podstawie evaluatora SIMD.
     * Wykorzystuje architekturę SoA dla wysokiej wydajności oraz EventBus dla Zero-Conflict.
     * 
     * @param table Referencja do tabeli komponentów walki.
     * @param evaluator Referencja do ewaluatora SIMD analizującego stany.
     * @return std::expected<void, EterBase::EntityError> 
     */
    std::expected<void, EterBase::EntityError> UpdateCombatTableFromSIMD(
        CombatComponentTable& table, 
        ISIMDCombatEvaluator& evaluator)
    {
        const size_t count = table.Size();
        if (count == 0)
        {
            return {};
        }

        std::vector<uint8_t> outAliveMask(count, 0);
        
        evaluator.EvaluateAliveMask(table.currentHp.data(), outAliveMask.data(), count);

        for (size_t i = 0; i < count; ++i)
        {
            if (outAliveMask[i] == 0 && table.isDead[i] == 0)
            {
                table.isDead[i] = 1;
                
                Core::EventBus::GetInstance().Publish(Core::ActorDeadEvent(table.entityIds[i]));
                
                EterBase::ModernLogger::Debug(
                    "UpdateCombatTableFromSIMD: Entity {} is dead", 
                    table.entityIds[i]);
            }
            else if (outAliveMask[i] == 1 && table.isDead[i] == 1)
            {
                table.isDead[i] = 0;
                
                EterBase::ModernLogger::Debug(
                    "UpdateCombatTableFromSIMD: Entity {} resurrected", 
                    table.entityIds[i]);
            }
        }

        return {};
    }
}
