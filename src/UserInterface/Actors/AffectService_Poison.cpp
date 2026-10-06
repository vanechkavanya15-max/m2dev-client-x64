#include "../StdAfx.h"
#include "ICharacterAffectService.h"
#include "../InstanceBase.h"
#include "../PythonCharacterManager.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "../Core/EventBus.h"

#include <unordered_set>
#include <mutex>
#include <format>

namespace UserInterface::Actors
{
    class CharacterAffectService_Poison : public ICharacterAffectService
    {
    public:
        ~CharacterAffectService_Poison() override
        {
            Clear();
        }

        void SetAffect(EterBase::EntityId id, uint32_t affectIndex, bool enabled) override
        {
            if (affectIndex != CInstanceBase::AFFECT_POISON && affectIndex != CInstanceBase::NEW_AFFECT_POISON)
            {
                return;
            }

            EterBase::ModernLogger::Info("CharacterAffectService_Poison::SetAffect - EntityId: {}, AffectIndex: {}, Enabled: {}", id.value(), affectIndex, enabled);

            CInstanceBase* instance = CPythonCharacterManager::Instance().GetInstancePtr(id.value());
            if (!instance)
            {
                EterBase::ModernLogger::Error("CharacterAffectService_Poison::SetAffect - Instance not found for EntityId: {}", id.value());
                return;
            }

            std::lock_guard<std::mutex> lock(mutex_);

            if (enabled)
            {
                instance->SetModulateRenderMode();
                instance->SetAddColor(D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f)); // Green
                poisonedEntities_.insert(id.value());
            }
            else
            {
                instance->RestoreRenderMode();
                instance->SetAddColor(D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f)); // Reset color
                poisonedEntities_.erase(id.value());
            }
        }

        bool HasAffect(EterBase::EntityId id, uint32_t affectIndex) const override
        {
            if (affectIndex != CInstanceBase::AFFECT_POISON && affectIndex != CInstanceBase::NEW_AFFECT_POISON)
            {
                return false;
            }

            std::lock_guard<std::mutex> lock(mutex_);
            return poisonedEntities_.find(id.value()) != poisonedEntities_.end();
        }

        void ClearAffects(EterBase::EntityId id) override
        {
            SetAffect(id, CInstanceBase::AFFECT_POISON, false);
        }

        void Clear() override
        {
            std::unordered_set<uint32_t> entitiesToClear;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                entitiesToClear = poisonedEntities_;
            }

            for (uint32_t entity_id : entitiesToClear)
            {
                SetAffect(EterBase::EntityId(entity_id), CInstanceBase::AFFECT_POISON, false);
            }
        }

    private:
        mutable std::mutex mutex_;
        std::unordered_set<uint32_t> poisonedEntities_;
    };
}
