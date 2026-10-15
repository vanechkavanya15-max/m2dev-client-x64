#pragma once

#include "EterBase/EventBus.h"
#include "DomainEvents.h"

namespace Client::Core {

using IEvent = ::EterBase::IEvent;
using IEventHandler = ::EterBase::IEventHandler;

template <typename EventType>
using EventHandler = ::EterBase::EventHandler<EventType>;

using EventBus = ::EterBase::EventBus;

// Eksport zdarzen bazowych
using TargetBoardRefreshEvent = ::EterBase::TargetBoardRefreshEvent;
using NetworkPacketReceivedEvent = ::EterBase::NetworkPacketReceivedEvent;
using MountStateChangedEvent = ::EterBase::MountStateChangedEvent;
using TextTailVisibilityChangedEvent = ::EterBase::TextTailVisibilityChangedEvent;
using SIMDCullingCompletedEvent = ::EterBase::SIMDCullingCompletedEvent;
using ItemTooltipCachedEvent = ::EterBase::ItemTooltipCachedEvent;
using AnimHitFrameEvent = ::EterBase::AnimHitFrameEvent;
using AnimFinishedEvent = ::EterBase::AnimFinishedEvent;
using CustomTitleChangedEvent = ::EterBase::CustomTitleChangedEvent;

} // namespace Client::Core
