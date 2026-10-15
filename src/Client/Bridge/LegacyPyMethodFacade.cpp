
#include "LegacyPyMethodFacade.h"
#include "EterBase/EventBus.h"

namespace Client::Bridge {

LegacyPyMethodFacade& LegacyPyMethodFacade::Instance() noexcept {
    static LegacyPyMethodFacade instance;
    return instance;
}

[[nodiscard]] Core::Result<void, PyBridgeError> LegacyPyMethodFacade::NotifyAnimHitFrame(uint32_t entityId, uint32_t motionKey, uint8_t hitIndex) noexcept {
    if (entityId == 0) {
        return std::unexpected(PyBridgeError::InvalidEntityId);
    }
    
    EterBase::AnimHitFrameEvent event(EterBase::EntityId{entityId}, motionKey, hitIndex);
    EterBase::EventBus::Instance().Publish(event);
    
    return {};
}

[[nodiscard]] Core::Result<void, PyBridgeError> LegacyPyMethodFacade::NotifyTextTailVisibilityChanged(uint32_t entityId, bool isVisible) noexcept {
    if (entityId == 0) {
        return std::unexpected(PyBridgeError::InvalidEntityId);
    }
    
    EterBase::TextTailVisibilityChangedEvent event(entityId, isVisible);
    EterBase::EventBus::Instance().Publish(event);
    
    return {};
}

[[nodiscard]] Core::Result<void, PyBridgeError> LegacyPyMethodFacade::NotifyCustomTitleChanged(std::string_view title, uint32_t color) noexcept {
    if (title.empty()) {
        return std::unexpected(PyBridgeError::EmptyString);
    }
    
    EterBase::CustomTitleChangedEvent event(title, color);
    EterBase::EventBus::Instance().Publish(event);
    
    return {};
}

[[nodiscard]] Core::Result<void, PyBridgeError> LegacyPyMethodFacade::NotifyAnimFinished(uint32_t entityId, uint32_t motionKey) noexcept {
    if (entityId == 0) {
        return std::unexpected(PyBridgeError::InvalidEntityId);
    }
    
    EterBase::AnimFinishedEvent event(EterBase::EntityId{entityId}, motionKey);
    EterBase::EventBus::Instance().Publish(event);
    
    return {};
}

[[nodiscard]] Core::Result<void, PyBridgeError> LegacyPyMethodFacade::NotifyActorDead(uint32_t entityId) noexcept {
    if (entityId == 0) {
        return std::unexpected(PyBridgeError::InvalidEntityId);
    }
    
    EterBase::ActorDeadEvent event(entityId);
    EterBase::EventBus::Instance().Publish(event);
    
    return {};
}

} // namespace Client::Bridge
