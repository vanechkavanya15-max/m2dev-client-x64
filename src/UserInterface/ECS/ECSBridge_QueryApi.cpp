#include "../StdAfx.h"
#include "ECSBridge_QueryApi.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include <algorithm>

namespace UserInterface::ECS
{
    ECSBridgeQueryApi& ECSBridgeQueryApi::GetInstance()
    {
        static ECSBridgeQueryApi instance;
        return instance;
    }

    EterBase::Result<PositionInfo, EterBase::EntityError> ECSBridgeQueryApi::GetPosition(
        const TransformComponentTable& table, 
        EterBase::EntityId entityId) const
    {
        auto it = std::ranges::find(table.entityIds, entityId.value());
        if (it == table.entityIds.end())
        {
            EterBase::ModernLogger::Warning("ECSBridgeQueryApi::GetPosition - Entity {} not found", entityId.value());
            return EterBase::MakeError(EterBase::EntityError::NotFound);
        }

        size_t idx = std::distance(table.entityIds.begin(), it);
        return PositionInfo{
            table.posX[idx],
            table.posY[idx],
            table.posZ[idx],
            table.rotation[idx]
        };
    }

    EterBase::Result<HpInfo, EterBase::EntityError> ECSBridgeQueryApi::GetHp(
        const CombatComponentTable& table, 
        EterBase::EntityId entityId) const
    {
        auto it = std::ranges::find(table.entityIds, entityId.value());
        if (it == table.entityIds.end())
        {
            EterBase::ModernLogger::Warning("ECSBridgeQueryApi::GetHp - Entity {} not found", entityId.value());
            return EterBase::MakeError(EterBase::EntityError::NotFound);
        }

        size_t idx = std::distance(table.entityIds.begin(), it);
        return HpInfo{
            table.currentHp[idx],
            table.maxHp[idx]
        };
    }

    EterBase::PacketResult<void> ECSBridgeQueryApi::NotifyHpUpdate(
        EterBase::EntityId entityId) const
    {
        EterBase::ModernLogger::Info("ECSBridgeQueryApi::NotifyHpUpdate - Broadcasting TargetBoardRefreshEvent for Entity {}", entityId.value());
        Core::EventBus::GetInstance().Publish(Core::TargetBoardRefreshEvent(entityId.value()));
        return {};
    }
}
