#include "../StdAfx.h"
#include "ICharacterAppearanceService.h"
#include "../Core/EventBus.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include <unordered_map>
#include <mutex>
#include <optional>

namespace UserInterface::Actors
{
    /**
     * @brief Zdarzenie publikowane, gdy wyglad aktora zostanie zmodyfikowany.
     */
    struct CharacterAppearanceChangedEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;
        CharacterVisualPartView newAppearance;

        CharacterAppearanceChangedEvent(EterBase::EntityId id, const CharacterVisualPartView& appearance)
            : entityId(id), newAppearance(appearance) {}
    };

    /**
     * @brief Implementacja serwisu do zarzadzania wygladem aktorow.
     * 
     * Przechowuje biezacy stan wizualny aktorow w pamieci uzywajac std::unordered_map.
     * Zapewnia synchronizacje poprzez std::mutex i publikuje zdarzenia po zmianach.
     */
    class CharacterAppearanceService final : public ICharacterAppearanceService
    {
    public:
        CharacterAppearanceService() = default;
        ~CharacterAppearanceService() override = default;

        void SetArmor(EterBase::EntityId id, uint32_t armorVnum) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto& appearance = m_appearances[id];
            
            if (appearance.armorVnum != armorVnum)
            {
                appearance.armorVnum = armorVnum;
                EterBase::ModernLogger::Debug("SetArmor: Entity {} updated armor to {}", id.value(), armorVnum);
                PublishAppearanceChanged(id, appearance);
            }
        }

        void SetWeapon(EterBase::EntityId id, uint32_t weaponVnum) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto& appearance = m_appearances[id];
            
            if (appearance.weaponVnum != weaponVnum)
            {
                appearance.weaponVnum = weaponVnum;
                EterBase::ModernLogger::Debug("SetWeapon: Entity {} updated weapon to {}", id.value(), weaponVnum);
                PublishAppearanceChanged(id, appearance);
            }
        }

        void SetHair(EterBase::EntityId id, uint32_t hairVnum) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto& appearance = m_appearances[id];
            
            if (appearance.hairVnum != hairVnum)
            {
                appearance.hairVnum = hairVnum;
                EterBase::ModernLogger::Debug("SetHair: Entity {} updated hair to {}", id.value(), hairVnum);
                PublishAppearanceChanged(id, appearance);
            }
        }

        void SetSash(EterBase::EntityId id, uint32_t sashVnum) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto& appearance = m_appearances[id];
            
            if (appearance.sashVnum != sashVnum)
            {
                appearance.sashVnum = sashVnum;
                EterBase::ModernLogger::Debug("SetSash: Entity {} updated sash to {}", id.value(), sashVnum);
                PublishAppearanceChanged(id, appearance);
            }
        }

        CharacterVisualPartView GetAppearance(EterBase::EntityId id) const override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (auto it = m_appearances.find(id); it != m_appearances.end())
            {
                return it->second;
            }
            EterBase::ModernLogger::Trace("GetAppearance: Entity {} not found, returning default", id.value());
            return CharacterVisualPartView{};
        }

        void RemoveCharacter(EterBase::EntityId id) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_appearances.erase(id))
            {
                EterBase::ModernLogger::Debug("RemoveCharacter: Entity {} removed from appearance service", id.value());
            }
        }

        void Clear() override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_appearances.clear();
            EterBase::ModernLogger::Info("CharacterAppearanceService: Cleared all appearances");
        }

    private:
        void PublishAppearanceChanged(EterBase::EntityId id, const CharacterVisualPartView& appearance)
        {
            Core::EventBus::GetInstance().Publish(CharacterAppearanceChangedEvent{id, appearance});
        }

        mutable std::mutex m_mutex;
        std::unordered_map<EterBase::EntityId, CharacterVisualPartView> m_appearances;
    };
    
    // Z uwagi na wymog implementacji ICharacterAppearanceService, tworzymy instancje. 
    // Poniewaz klasa jest wewnetrzna dla pliku, nie eksportujemy jej tutaj, 
    // zalezy od architektury czy jest to singleton, czy service locator. 
    // W kodzie docelowym prawdopodobnie bedzie wstrzykiwana lub zarzadzana przez jakis menadzer.
}
