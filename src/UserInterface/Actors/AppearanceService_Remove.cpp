// Includes
#ifndef TEST_MOCK
#include "../StdAfx.h"
#endif
#include "ICharacterAppearanceService.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../Core/EventBus.h"

#include <unordered_map>
#include <mutex>

namespace UserInterface::Actors
{
    // C++23 Event definitions for decoupling
    struct CharacterAppearanceRemovedEvent : public Core::IEvent
    {
        EterBase::EntityId characterId;
        explicit CharacterAppearanceRemovedEvent(EterBase::EntityId id) : characterId(id) {}
    };

    struct CharacterAppearanceClearedEvent : public Core::IEvent
    {
        CharacterAppearanceClearedEvent() = default;
    };

    /**
     * @brief Konkretna implementacja uslugi zarzadzania wygladem - czesc odpowiedzialna za usuwanie.
     * Uzywa VoidResult do poprawnego raportowania stanow, aczkolwiek metody interfejsu zwracaja void,
     * wiec zachowujemy void na zewnatrz lub opakowujemy wewnetrzne operacje.
     * Uwaga: poniewaz interfejs wymusza 'void', pozostajemy przy 'void' jako return type
     * dla metod wirtualnych z ICharacterAppearanceService.
     */
    class CharacterAppearanceService final : public ICharacterAppearanceService
    {
    public:
        void SetArmor(EterBase::EntityId id, uint32_t armorVnum) override {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_appearances[id].armorVnum = armorVnum;
        }

        void SetWeapon(EterBase::EntityId id, uint32_t weaponVnum) override {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_appearances[id].weaponVnum = weaponVnum;
        }

        void SetHair(EterBase::EntityId id, uint32_t hairVnum) override {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_appearances[id].hairVnum = hairVnum;
        }

        void SetSash(EterBase::EntityId id, uint32_t sashVnum) override {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_appearances[id].sashVnum = sashVnum;
        }

        CharacterVisualPartView GetAppearance(EterBase::EntityId id) const override {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (auto it = m_appearances.find(id); it != m_appearances.end()) {
                return it->second;
            }
            return CharacterVisualPartView{};
        }

        void RemoveCharacter(EterBase::EntityId id) override;
        void Clear() override;

    private:
        EterBase::VoidResult<EterBase::EntityError> InternalRemoveCharacter(EterBase::EntityId id);

        std::unordered_map<EterBase::EntityId, CharacterVisualPartView> m_appearances;
        mutable std::mutex m_mutex;
    };

    EterBase::VoidResult<EterBase::EntityError> CharacterAppearanceService::InternalRemoveCharacter(EterBase::EntityId id)
    {
        if (!id)
        {
            return EterBase::MakeError(EterBase::EntityError::NotFound);
        }

        auto it = m_appearances.find(id);
        if (it == m_appearances.end())
        {
            return EterBase::MakeError(EterBase::EntityError::NotFound);
        }

        m_appearances.erase(it);
        return {};
    }

    void CharacterAppearanceService::RemoveCharacter(EterBase::EntityId id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (auto result = InternalRemoveCharacter(id); result.has_value())
        {
            EterBase::ModernLogger::Info("Removed appearance data for character ID: {}", id.value());
            Core::EventBus::GetInstance().Publish(CharacterAppearanceRemovedEvent{id});
        }
        else
        {
            if (result.error() == EterBase::EntityError::NotFound)
            {
                EterBase::ModernLogger::Debug("Appearance data for character ID {} not found during removal.", id.value());
            }
            else
            {
                EterBase::ModernLogger::Warning("Failed to remove character appearance. Error: {}", EterBase::ToString(result.error()));
            }
        }
    }

    void CharacterAppearanceService::Clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_appearances.empty())
        {
            EterBase::ModernLogger::Debug("Character appearance service clear requested, but registry is already empty.");
            return;
        }

        const auto count = m_appearances.size();
        m_appearances.clear();

        EterBase::ModernLogger::Info("Cleared all character appearance data. Total removed: {}", count);

        // Decoupled global notification via EventBus
        Core::EventBus::GetInstance().Publish(CharacterAppearanceClearedEvent{});
    }
}
