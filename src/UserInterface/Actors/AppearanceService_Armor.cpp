#include "../StdAfx.h"
#include "ICharacterAppearanceService.h"
#include "../Packet.h"
#include "../Core/EventBus.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../PythonCharacterManager.h"
#include "../InstanceBase.h"
#include <memory>

namespace UserInterface::Actors
{
    /**
     * @brief Zdarzenie aktualizacji czesci wizualnej uzytkownika. (Custom Event dla EventBus)
     */
    struct CharacterArmorUpdatedEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId entityId;
        EterBase::ItemVnum armorVnum;
        
        CharacterArmorUpdatedEvent(EterBase::EntityId id, EterBase::ItemVnum vnum)
            : entityId(id), armorVnum(vnum) {}
    };

    class AppearanceService_Armor : public ICharacterAppearanceService
    {
    public:
        void SetArmor(EterBase::EntityId id, uint32_t armorVnum) override
        {
            auto* pInstance = CPythonCharacterManager::Instance().GetInstancePtr(id.value());
            if (!pInstance)
            {
                EterBase::ModernLogger::Error("AppearanceService_Armor::SetArmor: Entity {} not found", id.value());
                return; 
            }

            if (pInstance->ChangeArmor(armorVnum))
            {
                EterBase::ModernLogger::Info("AppearanceService_Armor::SetArmor: Successfully set armor {} for entity {}", armorVnum, id.value());
                
                // EventBus do powiadamiania GUI
                UserInterface::Core::EventBus::GetInstance().Publish(CharacterArmorUpdatedEvent{id, EterBase::ItemVnum{armorVnum}});
            }
            else
            {
                EterBase::ModernLogger::Warning("AppearanceService_Armor::SetArmor: Failed or unnecessary to set armor {} for entity {}", armorVnum, id.value());
            }
        }

        void SetWeapon(EterBase::EntityId id, uint32_t weaponVnum) override {}
        void SetHair(EterBase::EntityId id, uint32_t hairVnum) override {}
        void SetSash(EterBase::EntityId id, uint32_t sashVnum) override {}

        CharacterVisualPartView GetAppearance(EterBase::EntityId id) const override
        {
            auto* pInstance = CPythonCharacterManager::Instance().GetInstancePtr(id.value());
            if (!pInstance)
            {
                return CharacterVisualPartView{};
            }
            
            CharacterVisualPartView view{};
            view.armorVnum = pInstance->GetPart(CRaceData::PART_MAIN);
            view.weaponVnum = pInstance->GetPart(CRaceData::PART_WEAPON);
            view.hairVnum = pInstance->GetPart(CRaceData::PART_HAIR);
            view.sashVnum = 0; 
            view.effectFlags = 0; 
            return view;
        }

        void RemoveCharacter(EterBase::EntityId id) override {}
        void Clear() override {}
    };

    /**
     * @brief Fabryka instancji AppearanceService_Armor opakowujaca rezultat.
     */
    EterBase::Result<std::unique_ptr<ICharacterAppearanceService>, EterBase::EntityError> CreateArmorAppearanceService()
    {
        return std::make_unique<AppearanceService_Armor>();
    }
}
