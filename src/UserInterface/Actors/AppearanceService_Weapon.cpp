#include "../StdAfx.h"
#include "ICharacterAppearanceService.h"
#include "../PythonCharacterManager.h"
#include "../InstanceBase.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"

namespace UserInterface::Actors
{
    /**
     * @brief Implementacja serwisu dla czesci Appearance (Weapon).
     */
    class AppearanceService_Weapon : public ICharacterAppearanceService
    {
    public:
        virtual ~AppearanceService_Weapon() = default;

        void SetWeapon(EterBase::EntityId id, uint32_t weaponVnum) override
        {
            if (!id)
            {
                EterBase::ModernLogger::Error("AppearanceService_Weapon: Invalid EntityId provided.");
                return;
            }

            auto* pInstance = CPythonCharacterManager::Instance().GetInstancePtr(id.value());
            if (!pInstance)
            {
                EterBase::ModernLogger::Error("AppearanceService_Weapon: Character not found for EntityId: {}", id.value());
                return;
            }

            if (!pInstance->SetWeapon(weaponVnum))
            {
                EterBase::ModernLogger::Warning("AppearanceService_Weapon: Failed to set weapon {} for EntityId: {}. It might be restricted or poly-morphed.", weaponVnum, id.value());
                return;
            }

            EterBase::ModernLogger::Info("AppearanceService_Weapon: Successfully set weapon {} for EntityId: {}", weaponVnum, id.value());
        }

        // --- Pozostale metody interfejsu to atrapy dla celow Zero-Conflict (Single Responsibility Principle) ---

        void SetArmor(EterBase::EntityId, uint32_t) override {}
        void SetHair(EterBase::EntityId, uint32_t) override {}
        void SetSash(EterBase::EntityId, uint32_t) override {}

        CharacterVisualPartView GetAppearance(EterBase::EntityId) const override 
        { 
            return CharacterVisualPartView{}; 
        }

        void RemoveCharacter(EterBase::EntityId) override {}
        void Clear() override {}
    };
}
