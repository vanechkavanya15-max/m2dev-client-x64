#include "../StdAfx.h"
#include "IInstanceSoundController.h"
#include "Core/EventBus.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "GameLib/ItemData.h"
#include "AudioLib/SoundEngine.h"

namespace UserInterface::InstanceControllers {

    struct AttackSwingSoundEvent : public Core::IEvent {
        EterBase::ItemVnum weaponType;
        std::string_view soundName;

        explicit AttackSwingSoundEvent(EterBase::ItemVnum weaponType, std::string_view soundName)
            : weaponType(weaponType), soundName(soundName) {}
    };

    class InstanceSound_AttackSwing {
    public:
        static EterBase::PacketResult<void> PlayAttackSwing(IInstanceSoundController& controller, uint32_t rawWeaponType) {
            EterBase::ItemVnum weaponType{rawWeaponType};
            std::string_view soundFile;

            switch (weaponType.value()) {
                case CItemData::WEAPON_SWORD:
                case CItemData::WEAPON_TWO_HANDED:
                    soundFile = "sound/pc/warrior/sword_swing.wav";
                    break;
                case CItemData::WEAPON_DAGGER:
                    soundFile = "sound/pc/assassin/dagger_swing.wav";
                    break;
                case CItemData::WEAPON_BELL:
                    soundFile = "sound/pc/shaman/bell_swing.wav";
                    break;
                case CItemData::WEAPON_FAN:
                    soundFile = "sound/pc/shaman/fan_swing.wav";
                    break;
                case CItemData::WEAPON_BOW:
                case CItemData::WEAPON_ARROW:
                    soundFile = "sound/pc/assassin/bow_swing.wav";
                    break;
                default:
                    soundFile = "sound/pc/warrior/sword_swing.wav";
                    break;
            }

            if (!soundFile.empty()) {
                SoundEngine::Instance().PlaySound2D(std::string(soundFile));
                EterBase::ModernLogger::Info("Played attack swing sound: {} for weapon type: {}", soundFile, weaponType.value());

                AttackSwingSoundEvent event(weaponType, soundFile);
                UserInterface::Core::EventBus::GetInstance().Publish(event);
            }

            return {};
        }
    };
}
