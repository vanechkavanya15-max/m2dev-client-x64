#include "../StdAfx.h"
#include "ICharacterAffectService.h"
#include "../InstanceBase.h"
#include "../PythonCharacterManager.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include "../Packet.h"

namespace UserInterface::Actors
{
    // Define an event for the EventBus when the affect changes
    struct FireAffectChangedEvent : public Core::IEvent {
        EterBase::EntityId entityId;
        bool enabled;

        FireAffectChangedEvent(EterBase::EntityId id, bool en)
            : entityId(id), enabled(en) {}
    };

    class AffectService_Fire : public ICharacterAffectService
    {
    public:
        virtual ~AffectService_Fire() = default;

        void SetAffect(EterBase::EntityId id, uint32_t affectIndex, bool enabled) override
        {
            if (affectIndex != CInstanceBase::NEW_AFFECT_FIRE && affectIndex != CInstanceBase::AFFECT_FIRE)
            {
                return;
            }
            
            CInstanceBase* pInstance = CPythonCharacterManager::Instance().GetInstancePtr(id.value());
            if (!pInstance)
            {
                EterBase::ModernLogger::Warn("AffectService_Fire::SetAffect - Instance {} not found", id.value());
                return;
            }

            pInstance->SCRIPT_SetAffect(CInstanceBase::AFFECT_FIRE, enabled);
            
            // Publish event to decouple backend state from GUI
            Core::EventBus::GetInstance().Publish(FireAffectChangedEvent{id, enabled});
            
            if (enabled) {
                EterBase::ModernLogger::Info("AffectService_Fire::SetAffect - Fire affect enabled on instance {}", id.value());
            } else {
                EterBase::ModernLogger::Info("AffectService_Fire::SetAffect - Fire affect disabled on instance {}", id.value());
            }
        }

        bool HasAffect(EterBase::EntityId id, uint32_t affectIndex) const override
        {
            if (affectIndex != CInstanceBase::NEW_AFFECT_FIRE && affectIndex != CInstanceBase::AFFECT_FIRE)
            {
                return false;
            }
            
            CInstanceBase* pInstance = CPythonCharacterManager::Instance().GetInstancePtr(id.value());
            if (!pInstance)
            {
                return false;
            }

            return pInstance->IsAffect(CInstanceBase::AFFECT_FIRE) || pInstance->IsAffect(CInstanceBase::NEW_AFFECT_FIRE);
        }

        void ClearAffects(EterBase::EntityId id) override
        {
            SetAffect(id, CInstanceBase::AFFECT_FIRE, false);
        }

        void Clear() override
        {
            // Handled globally
        }
        
        // Example method demonstrating required return types
        EterBase::PacketResult<void> HandleFireAffectPacket(EterBase::EntityId id, bool enabled) {
            if (id.value() == 0) {
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }
            SetAffect(id, CInstanceBase::AFFECT_FIRE, enabled);
            return {};
        }
    };
    
    // Factory function to expose the service to the rest of the application
    std::unique_ptr<ICharacterAffectService> CreateFireAffectService() {
        return std::make_unique<AffectService_Fire>();
    }
}
