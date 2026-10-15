#include "../StdAfx.h"
#include "ICharacterAppearanceService.h"
#include "../Packet.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../Core/EventBus.h"

#include <unordered_map>
#include <mutex>
#include <optional>
#include <expected>

namespace UserInterface::Actors {

/**
 * @brief Event triggered when a character's appearance changes.
 */
struct AppearanceChangedEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId entityId;
    CharacterVisualPartView newAppearance;

    AppearanceChangedEvent(EterBase::EntityId id, const CharacterVisualPartView& appearance)
        : entityId(id), newAppearance(appearance) {}
};

} // namespace UserInterface::Actors

namespace UserInterface::Actors {

/**
 * @brief Implementation of ICharacterAppearanceService handling character visual updates.
 */
class CharacterAppearanceService : public ICharacterAppearanceService {
public:
    CharacterAppearanceService() = default;
    ~CharacterAppearanceService() override = default;

    void SetArmor(EterBase::EntityId id, uint32_t armorVnum) override {
        UpdateAppearance(id, [armorVnum](CharacterVisualPartView& view) {
            view.armorVnum = armorVnum;
        });
    }

    void SetWeapon(EterBase::EntityId id, uint32_t weaponVnum) override {
        UpdateAppearance(id, [this, weaponVnum](CharacterVisualPartView& view) {
            view.weaponVnum = weaponVnum;
            view.effectFlags = CalculateWeaponEffectFlags(weaponVnum);
        });
    }

    void SetHair(EterBase::EntityId id, uint32_t hairVnum) override {
        UpdateAppearance(id, [hairVnum](CharacterVisualPartView& view) {
            view.hairVnum = hairVnum;
        });
    }

    void SetSash(EterBase::EntityId id, uint32_t sashVnum) override {
        UpdateAppearance(id, [sashVnum](CharacterVisualPartView& view) {
            view.sashVnum = sashVnum;
        });
    }

    CharacterVisualPartView GetAppearance(EterBase::EntityId id) const override {
        return GetAppearanceSafe(id).value_or(CharacterVisualPartView{});
    }

    void RemoveCharacter(EterBase::EntityId id) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_appearances.erase(id) > 0) {
            EterBase::ModernLogger::Debug("CharacterAppearanceService: Removed character {}", id.value());
        }
    }

    void Clear() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_appearances.clear();
        EterBase::ModernLogger::Debug("CharacterAppearanceService: Cleared all appearances");
    }

private:
    std::unordered_map<EterBase::EntityId, CharacterVisualPartView> m_appearances;
    mutable std::mutex m_mutex;

    std::expected<CharacterVisualPartView, EterBase::EntityError> GetAppearanceSafe(EterBase::EntityId id) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (auto it = m_appearances.find(id); it != m_appearances.end()) {
            return it->second;
        }
        return std::unexpected(EterBase::EntityError::NotFound);
    }

    template <typename Func>
    void UpdateAppearance(EterBase::EntityId id, Func&& updateFunc) {
        CharacterVisualPartView updatedView;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto& view = m_appearances[id];
            updateFunc(view);
            updatedView = view;
        }

        EterBase::ModernLogger::Debug("CharacterAppearanceService: Updated appearance for {}", id.value());
        Core::EventBus::Instance().Publish(AppearanceChangedEvent(id, updatedView));
    }

    uint32_t CalculateWeaponEffectFlags(uint32_t weaponVnum) const {
        if (weaponVnum == 0) return 0;

        uint32_t refineLevel = weaponVnum % 10;
        uint32_t effectFlags = 0;

        // Weapon refine levels 7, 8, 9 give glowing effects
        if (refineLevel == 7) {
            effectFlags = 1; // Base glow
        } else if (refineLevel == 8) {
            effectFlags = 2; // Strong glow
        } else if (refineLevel == 9) {
            effectFlags = 3; // Max glow
        }

        return effectFlags;
    }
};

} // namespace UserInterface::Actors
