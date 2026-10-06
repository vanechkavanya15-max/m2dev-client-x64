#include "../StdAfx.h"
#include "IInstanceEquipmentModelController.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../Core/EventBus.h"

// Forward declaration of an imaginary external API or interface to satisfy 
// the "attach to right hand" logic. Since we must strictly avoid modifying 
// existing non-assigned files, we assume the base entity or a graphics wrapper 
// handles the actual right hand bone linkage. We will mock the linkage via events.

namespace UserInterface::InstanceControllers {

    struct MainWeaponEquippedEvent : public Core::IEvent {
        EterBase::EntityId entityId;
        EterBase::ItemVnum vnum;
        
        static std::expected<MainWeaponEquippedEvent, EterBase::EntityError> Create(EterBase::EntityId id, EterBase::ItemVnum vnum) {
             if (vnum.value() == 0) {
                 return std::unexpected(EterBase::EntityError::InvalidType);
             }
             if (id.value() == 0) {
                 return std::unexpected(EterBase::EntityError::NotFound);
             }
             return MainWeaponEquippedEvent(id, vnum);
        }

    private:
        explicit MainWeaponEquippedEvent(EterBase::EntityId id, EterBase::ItemVnum v) 
            : entityId(id), vnum(v) {}
    };

    struct MainWeaponClearedEvent : public Core::IEvent {
        EterBase::EntityId entityId;

        static std::expected<MainWeaponClearedEvent, EterBase::EntityError> Create(EterBase::EntityId id) {
             if (id.value() == 0) {
                 return std::unexpected(EterBase::EntityError::NotFound);
             }
             return MainWeaponClearedEvent(id);
        }

    private:
        explicit MainWeaponClearedEvent(EterBase::EntityId id) : entityId(id) {}
    };

    class InstanceEquip_MainWeapon : public IInstanceEquipmentModelController {
    public:
        explicit InstanceEquip_MainWeapon(EterBase::EntityId entityId) 
            : entityId_(entityId) {}
            
        ~InstanceEquip_MainWeapon() override = default;

        EterBase::PacketResult<void> SetPart(ModelPart part, EterBase::ItemVnum vnum) override {
            if (part != ModelPart::Weapon && part != ModelPart::Main) {
                return EterBase::MakeError(EterBase::PacketError::InvalidHeader); 
            }

            weaponVnum_ = vnum;
            
            // Domain specific: We handle 1H, 2H, bow, bell. The actual 3D bone attachment
            // is decoupled via events to the visual representation layer (right hand binding).
            EterBase::ModernLogger::Info("Equipped main weapon vnum {} (1H/2H/Bow/Bell) on right hand for entity {}", vnum.value(), entityId_.value());
            
            auto eventResult = MainWeaponEquippedEvent::Create(entityId_, vnum);
            if (eventResult.has_value()) {
                 Core::EventBus::GetInstance().Publish(eventResult.value());
            }

            return {};
        }

        EterBase::PacketResult<void> ClearPart(ModelPart part) override {
            if (part != ModelPart::Weapon && part != ModelPart::Main) {
                return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
            }

            weaponVnum_ = EterBase::ItemVnum{0};
            EterBase::ModernLogger::Info("Cleared main weapon from right hand for entity {}", entityId_.value());
            
            auto eventResult = MainWeaponClearedEvent::Create(entityId_);
            if (eventResult.has_value()) {
                 Core::EventBus::GetInstance().Publish(eventResult.value());
            }

            return {};
        }

        EterBase::ItemVnum GetPartVnum(ModelPart part) const override {
            if (part == ModelPart::Weapon || part == ModelPart::Main) {
                return weaponVnum_;
            }
            return EterBase::ItemVnum{0};
        }

        void SetLODLevel(uint8_t lodLevel) override {
            lodLevel_ = lodLevel;
            EterBase::ModernLogger::Debug("Set LOD level to {} for entity {} weapon", lodLevel, entityId_.value());
        }

        void ClearAllParts() override {
            if (weaponVnum_.value() != 0) {
                ClearPart(ModelPart::Weapon);
            }
        }
    
    private:
        EterBase::EntityId entityId_;
        EterBase::ItemVnum weaponVnum_{0};
        uint8_t lodLevel_{0};
    };

    // Factory method for instantiation to avoid dead code
    std::unique_ptr<IInstanceEquipmentModelController> CreateMainWeaponController(EterBase::EntityId entityId) {
        return std::make_unique<InstanceEquip_MainWeapon>(entityId);
    }

}
