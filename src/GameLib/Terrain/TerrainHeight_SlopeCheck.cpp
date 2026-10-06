#include "../StdAfx.h"
#include "ITerrainHeightCache.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../../UserInterface/Core/EventBus.h"

namespace {
    struct TerrainSlopeCheckFailedEvent : public UserInterface::Core::IEvent {
        EterBase::EntityId entityId;
        float x;
        float y;
        float maxSlopeAngle;

        TerrainSlopeCheckFailedEvent(EterBase::EntityId id, float px, float py, float maxSlope)
            : entityId(id), x(px), y(py), maxSlopeAngle(maxSlope) {}
    };
}

namespace GameLib::Terrain {

    std::expected<void, EterBase::NavigationError> VerifyWalkableSlope(
        const ITerrainHeightCache& heightCache, 
        EterBase::EntityId entityId, 
        float x, 
        float y, 
        float maxSlopeAngle) 
    {
        if (entityId.value() == 0) {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "VerifyWalkableSlope: Invalid EntityId");
            return std::unexpected(EterBase::NavigationError::PathNotFound);
        }

        bool isWalkable = heightCache.IsWalkableSlope(x, y, maxSlopeAngle);

        EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, 
            "VerifyWalkableSlope: EntityId {}, Pos({}, {}), MaxSlope: {}, IsWalkable: {}", 
            entityId.value(), x, y, maxSlopeAngle, isWalkable);

        if (!isWalkable) {
            TerrainSlopeCheckFailedEvent event(entityId, x, y, maxSlopeAngle);
            UserInterface::Core::EventBus::GetInstance().Publish(event);
            return std::unexpected(EterBase::NavigationError::BlockedTerrain);
        }

        return {};
    }

}
