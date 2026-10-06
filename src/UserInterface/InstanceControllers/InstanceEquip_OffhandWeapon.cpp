#include "../StdAfx.h"
#include "IInstanceEquipmentModelController.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/ModernLogger.h"
#include "UserInterface/Core/EventBus.h"

namespace UserInterface::InstanceControllers {

/**
 * @brief Event published when the offhand weapon changes.
 */
struct OffhandWeaponChangedEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId entityId;
    EterBase::ItemVnum vnum;

    explicit OffhandWeaponChangedEvent(EterBase::EntityId id, EterBase::ItemVnum v)
        : entityId(id), vnum(v) {}
};

/**
 * @class InstanceEquipOffhandWeapon
 * @brief Controller for equipping and unequipping offhand weapons (dual wield).
 * 
 * Complies with the Zero-Conflict Rule and C++23 standards.
 * Dedicated to managing the 'WeaponLeft' model part.
 */
class InstanceEquipOffhandWeapon {
public:
    /**
     * @brief Constructor for the offhand weapon controller.
     * @param entityId The ID of the entity.
     * @param modelController The model controller to manipulate.
     */
    InstanceEquipOffhandWeapon(EterBase::EntityId entityId, IInstanceEquipmentModelController& modelController)
        : m_entityId(entityId), m_modelController(modelController) {}

    /**
     * @brief Equips an offhand weapon.
     * @param vnum The vnum of the weapon to equip.
     * @return Success or PacketError.
     */
    EterBase::PacketResult<void> EquipOffhand(EterBase::ItemVnum vnum) {
        if (vnum.value() == 0) {
            EterBase::ModernLogger::Error( "Attempted to equip offhand with invalid vnum: 0");
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader); // Treat as an invalid operation/header for packet mapping
        }

        EterBase::ModernLogger::Info( "Equipping offhand weapon vnum: {} on entity: {}", vnum.value(), m_entityId.value());
        
        auto result = m_modelController.SetPart(ModelPart::WeaponLeft, vnum);
        if (!result) {
            EterBase::ModernLogger::Error( "Failed to set offhand model part for vnum: {} on entity: {}", vnum.value(), m_entityId.value());
            return result;
        }

        UserInterface::Core::EventBus::GetInstance().Publish(OffhandWeaponChangedEvent{m_entityId, vnum});

        return {};
    }

    /**
     * @brief Unequips the currently equipped offhand weapon.
     * @return Success or PacketError.
     */
    EterBase::PacketResult<void> UnequipOffhand() {
        EterBase::ItemVnum currentVnum = m_modelController.GetPartVnum(ModelPart::WeaponLeft);
        if (currentVnum.value() == 0) {
            EterBase::ModernLogger::Debug( "Offhand already empty on entity: {}, nothing to unequip.", m_entityId.value());
            return {}; 
        }

        EterBase::ModernLogger::Info( "Unequipping offhand weapon (was vnum: {}) on entity: {}", currentVnum.value(), m_entityId.value());

        auto result = m_modelController.ClearPart(ModelPart::WeaponLeft);
        if (!result) {
             EterBase::ModernLogger::Error( "Failed to clear offhand model part on entity: {}", m_entityId.value());
             return result;
        }
        
        UserInterface::Core::EventBus::GetInstance().Publish(OffhandWeaponChangedEvent{m_entityId, EterBase::ItemVnum{0}});

        return {};
    }

private:
    EterBase::EntityId m_entityId;
    IInstanceEquipmentModelController& m_modelController;
};

} // namespace UserInterface::InstanceControllers
