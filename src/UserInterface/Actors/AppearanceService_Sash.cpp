#include "../StdAfx.h"
#include "ICharacterAppearanceService.h"
#include "../Packet.h"
#include "../Core/EventBus.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../PythonCharacterManager.h"
#include "../InstanceBase.h"

namespace UserInterface::Actors
{
    struct CharacterSashUpdatedEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId entityId;
        uint32_t sashVnum;
        CharacterSashUpdatedEvent(EterBase::EntityId id, uint32_t vnum) : entityId(id), sashVnum(vnum) {}
    };

    class AppearanceService_Sash
    {
    public:
        static void SetSash(EterBase::EntityId id, uint32_t sashVnum)
        {
            auto* pInstance = CPythonCharacterManager::Instance().GetInstancePtr(id.value());
            if (!pInstance)
            {
                EterBase::ModernLogger::Error("AppearanceService_Sash: Entity {} not found", id.value());
                return;
            }

            UserInterface::Core::EventBus::GetInstance().Publish(CharacterSashUpdatedEvent(id, sashVnum));
            EterBase::ModernLogger::Info("AppearanceService_Sash: Set sash {} for entity {}", sashVnum, id.value());
        }
    };
}
