#include "../StdAfx.h"
#include "TransformComponentTable.h"
#include "../Packet.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"

namespace UserInterface::ECS
{

    struct TransformLookupSuccessEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;
        size_t index;

        TransformLookupSuccessEvent(EterBase::EntityId id, size_t idx)
            : entityId(id), index(idx)
        {
        }
    };

    std::expected<size_t, EterBase::EntityError> FindEntityIndex(
        const TransformComponentTable& table, 
        EterBase::EntityId entityId)
    {
        EterBase::ModernLogger::Debug("Looking up EntityId: {}", entityId.value());

        const auto& map = table.GetIdToIndexMap();
        auto it = map.find(entityId.value());
        
        if (it == map.end())
        {
            EterBase::ModernLogger::Warn("EntityId {} not found in TransformComponentTable", entityId.value());
            return std::unexpected(EterBase::EntityError::NotFound);
        }

        size_t index = it->second;
        EterBase::ModernLogger::Info("Successfully found EntityId {} at index {}", entityId.value(), index);

        Core::EventBus::GetInstance().Publish(TransformLookupSuccessEvent{entityId, index});

        return index;
    }

} // namespace UserInterface::ECS
