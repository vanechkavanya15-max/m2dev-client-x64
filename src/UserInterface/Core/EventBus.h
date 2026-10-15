#pragma once

#include "EterBase/EventBus.h"

namespace UserInterface::Core {

using IEvent = ::EterBase::IEvent;
using IEventHandler = ::EterBase::IEventHandler;

template <typename EventType>
using EventHandler = ::EterBase::EventHandler<EventType>;

using EventBus = ::EterBase::EventBus;

using TargetBoardRefreshEvent = ::EterBase::TargetBoardRefreshEvent;
using NetworkPacketReceivedEvent = ::EterBase::NetworkPacketReceivedEvent;
using MountStateChangedEvent = ::EterBase::MountStateChangedEvent;
using TextTailVisibilityChangedEvent = ::EterBase::TextTailVisibilityChangedEvent;
using SIMDCullingCompletedEvent = ::EterBase::SIMDCullingCompletedEvent;
using ItemTooltipCachedEvent = ::EterBase::ItemTooltipCachedEvent;
using AnimHitFrameEvent = ::EterBase::AnimHitFrameEvent;
using AnimFinishedEvent = ::EterBase::AnimFinishedEvent;
using CustomTitleChangedEvent = ::EterBase::CustomTitleChangedEvent;
using ActorDeadEvent = ::EterBase::ActorDeadEvent;

} // namespace UserInterface::Core

namespace Core {
    using EventBus = ::EterBase::EventBus;
    using IEvent = ::EterBase::IEvent;
    using ActorDeadEvent = ::EterBase::ActorDeadEvent;
}
