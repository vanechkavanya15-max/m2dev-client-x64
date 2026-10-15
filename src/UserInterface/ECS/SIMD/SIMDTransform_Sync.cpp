#include "../../StdAfx.h"
#include "ISIMDTransformEngine.h"
#include "../TransformComponentTable.h"
#include "../../Domain/ActorRegistryModel.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/ModernLogger.h"
#include "../../Core/EventBus.h"
#include <cstdint>
#include <cstddef>
#include <span>

namespace UserInterface::ECS::SIMD
{
    /**
     * @brief Synchronizes SIMD processed arrays back into the TransformComponentTable SoA.
     * @param table Reference to the TransformComponentTable.
     * @param inX Array of SIMD processed X positions.
     * @param inY Array of SIMD processed Y positions.
     * @param inZ Array of SIMD processed Z positions.
     * @param count Number of elements to synchronize.
     * @return EterBase::PacketResult<void> Returns success or a PacketError.
     */
    EterBase::PacketResult<void> SyncSIMDToTransformTable(
        TransformComponentTable& table,
        const float* inX, const float* inY, const float* inZ,
        size_t count)
    {
        if (!inX || !inY || !inZ)
        {
            EterBase::ModernLogger::Error("SyncSIMDToTransformTable: Nullptr passed for input arrays.");
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        if (count > table.Size())
        {
            EterBase::ModernLogger::Error("SyncSIMDToTransformTable: Count {} exceeds table size {}.", count, table.Size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        for (size_t i = 0; i < count; ++i)
        {
            table.posX[i] = inX[i];
            table.posY[i] = inY[i];
            table.posZ[i] = inZ[i];

            EterBase::EntityId entityId{table.entityIds[i]};
            
            Core::EventBus::GetInstance().Publish(
                Domain::ActorMovedEvent{entityId, inX[i], inY[i], inZ[i], table.rotation[i]}
            );
        }

        EterBase::ModernLogger::Info("SyncSIMDToTransformTable: Successfully synchronized {} entities.", count);
        return {};
    }
}
