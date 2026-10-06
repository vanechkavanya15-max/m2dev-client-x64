#include "../StdAfx.h"
#include "IInstanceEffectController.h"
#include "../Core/EventBus.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"

namespace UserInterface::InstanceControllers {

    struct EffectClearedEvent : public Core::IEvent {
        EterBase::EntityId entityId;
        explicit EffectClearedEvent(EterBase::EntityId id) : entityId(id) {}
    };

    class InstanceEffectClearer {
    public:
        static EterBase::PacketResult<void> ClearAllEffects(IInstanceEffectController& controller, EterBase::EntityId entityId) {
            EterBase::ModernLogger::Debug("Zarzadano wyczyszczenia efektow dla instancji (EntityId: {})", entityId.value());

            controller.ClearAllEffects();

            Core::EventBus::GetInstance().Publish(EffectClearedEvent{entityId});
            
            EterBase::ModernLogger::Info("Pomyslnie odpieto i wyczyszczono efekty (EntityId: {})", entityId.value());

            return {};
        }
    };
} // namespace UserInterface::InstanceControllers
