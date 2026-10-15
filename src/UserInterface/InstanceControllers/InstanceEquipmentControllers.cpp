#include "../StdAfx.h"
#include "InstanceEquipmentControllers.h"
#include "../InstanceBase.h"
#include "../../EterBase/LogModern.h"

namespace UserInterface::InstanceControllers
{
    // =========================================================================
    // IInstanceEquipmentModelController - domyslne implementacje
    // =========================================================================

    void IInstanceEquipmentModelController::ClearAllParts()
    {
        EterBase::ModernLogger::Info("Clearing all equipment parts for instance despawn.");

        for (uint8_t i = 0; i < static_cast<uint8_t>(ModelPart::MaxParts); ++i)
        {
            auto result = ClearPart(static_cast<ModelPart>(i));
            if (!result.has_value())
            {
                EterBase::ModernLogger::Error("Failed to clear equipment part {}: {}", i, result.error());
            }
        }

        Core::EventBus::GetInstance().Publish(EquipmentPartsClearedEvent{});
    }

    // =========================================================================
    // InstanceEquip_Body
    // =========================================================================

    InstanceEquip_Body::InstanceEquip_Body() : lodLevel_(0)
    {
        ClearAllParts();
        EterBase::ModernLogger::Info("InstanceEquip_Body created");
    }

    InstanceEquip_Body::~InstanceEquip_Body()
    {
        EterBase::ModernLogger::Info("InstanceEquip_Body destroyed");
    }

    EterBase::PacketResult<void> InstanceEquip_Body::SetPart(ModelPart part, EterBase::ItemVnum vnum)
    {
        if (part >= ModelPart::MaxParts)
        {
            EterBase::ModernLogger::Error("InstanceEquip_Body::SetPart - Invalid model part index");
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        parts_[static_cast<size_t>(part)] = vnum;

        EterBase::ModernLogger::Info("InstanceEquip_Body: Equipment part {} updated to vnum {}",
            static_cast<uint8_t>(part), vnum.value());

        Core::EventBus::GetInstance().Publish(EquipmentPartChangedEvent(part, vnum));
        return {};
    }

    EterBase::PacketResult<void> InstanceEquip_Body::ClearPart(ModelPart part)
    {
        if (part >= ModelPart::MaxParts)
        {
            EterBase::ModernLogger::Error("InstanceEquip_Body::ClearPart - Invalid model part index");
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        parts_[static_cast<size_t>(part)] = EterBase::ItemVnum(0);

        EterBase::ModernLogger::Info("InstanceEquip_Body: Equipment part {} cleared",
            static_cast<uint8_t>(part));

        Core::EventBus::GetInstance().Publish(EquipmentPartChangedEvent(part, EterBase::ItemVnum(0)));
        return {};
    }

    EterBase::ItemVnum InstanceEquip_Body::GetPartVnum(ModelPart part) const
    {
        if (part >= ModelPart::MaxParts)
        {
            return EterBase::ItemVnum(0);
        }
        return parts_[static_cast<size_t>(part)];
    }

    void InstanceEquip_Body::SetLODLevel(uint8_t lodLevel)
    {
        lodLevel_ = lodLevel;
        EterBase::ModernLogger::Info("InstanceEquip_Body: LOD Level set to {}", lodLevel);
    }

    void InstanceEquip_Body::ClearAllParts()
    {
        for (size_t i = 0; i < static_cast<size_t>(ModelPart::MaxParts); ++i)
        {
            parts_[i] = EterBase::ItemVnum(0);
            Core::EventBus::GetInstance().Publish(EquipmentPartChangedEvent(
                static_cast<ModelPart>(i), EterBase::ItemVnum(0)));
        }
        EterBase::ModernLogger::Info("InstanceEquip_Body: All equipment parts cleared");
    }

    // =========================================================================
    // InstanceEquip_CostumeOverlay
    // =========================================================================

    InstanceEquip_CostumeOverlay::InstanceEquip_CostumeOverlay()
    {
        m_parts.fill(EterBase::ItemVnum(0));
    }

    EterBase::PacketResult<void> InstanceEquip_CostumeOverlay::SetPart(ModelPart part, EterBase::ItemVnum vnum)
    {
        auto partIndex = static_cast<size_t>(part);
        if (partIndex >= static_cast<size_t>(ModelPart::MaxParts))
        {
            EterBase::ModernLogger::Error("InstanceEquip_CostumeOverlay: Invalid ModelPart {}", partIndex);
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        m_parts[partIndex] = vnum;
        EterBase::ModernLogger::Debug("InstanceEquip_CostumeOverlay: SetPart {} to Vnum {}", partIndex, vnum.value());

        Core::EventBus::GetInstance().Publish(EquipmentModelChangedEvent(part, vnum));
        return {};
    }

    EterBase::PacketResult<void> InstanceEquip_CostumeOverlay::ClearPart(ModelPart part)
    {
        auto partIndex = static_cast<size_t>(part);
        if (partIndex >= static_cast<size_t>(ModelPart::MaxParts))
        {
            EterBase::ModernLogger::Error("InstanceEquip_CostumeOverlay: ClearPart failed for invalid ModelPart {}", partIndex);
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        m_parts[partIndex] = EterBase::ItemVnum(0);
        EterBase::ModernLogger::Debug("InstanceEquip_CostumeOverlay: ClearPart {}", partIndex);

        Core::EventBus::GetInstance().Publish(EquipmentModelChangedEvent(part, EterBase::ItemVnum(0)));
        return {};
    }

    EterBase::ItemVnum InstanceEquip_CostumeOverlay::GetPartVnum(ModelPart part) const
    {
        switch (part)
        {
            case ModelPart::Main:
                if (m_parts[static_cast<size_t>(ModelPart::CostumeBody)].value() != 0)
                    return m_parts[static_cast<size_t>(ModelPart::CostumeBody)];
                return m_parts[static_cast<size_t>(ModelPart::Main)];

            case ModelPart::Hair:
                if (m_parts[static_cast<size_t>(ModelPart::CostumeHair)].value() != 0)
                    return m_parts[static_cast<size_t>(ModelPart::CostumeHair)];
                return m_parts[static_cast<size_t>(ModelPart::Hair)];

            case ModelPart::Weapon:
                if (m_parts[static_cast<size_t>(ModelPart::CostumeWeapon)].value() != 0)
                    return m_parts[static_cast<size_t>(ModelPart::CostumeWeapon)];
                return m_parts[static_cast<size_t>(ModelPart::Weapon)];

            default:
            {
                auto partIndex = static_cast<size_t>(part);
                if (partIndex < static_cast<size_t>(ModelPart::MaxParts))
                    return m_parts[partIndex];
                return EterBase::ItemVnum(0);
            }
        }
    }

    void InstanceEquip_CostumeOverlay::SetLODLevel(uint8_t lodLevel)
    {
        m_lodLevel = lodLevel;
        EterBase::ModernLogger::Debug("InstanceEquip_CostumeOverlay: Set LOD Level to {}", lodLevel);
    }

    void InstanceEquip_CostumeOverlay::ClearAllParts()
    {
        m_parts.fill(EterBase::ItemVnum(0));
        EterBase::ModernLogger::Debug("InstanceEquip_CostumeOverlay: Cleared all parts");
    }

    // =========================================================================
    // InstanceEquipmentHairController
    // =========================================================================

    InstanceEquipmentHairController::InstanceEquipmentHairController(CInstanceBase* instance)
        : m_instance(instance)
    {
    }

    EterBase::PacketResult<void> InstanceEquipmentHairController::SetPart(ModelPart part, EterBase::ItemVnum vnum)
    {
        if (part != ModelPart::Hair)
        {
            EterBase::ModernLogger::Error("InstanceEquipmentHairController::SetPart - Invalid part {}. Expected Hair.", static_cast<uint8_t>(part));
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }

        if (!m_instance)
        {
            EterBase::ModernLogger::Error("InstanceEquipmentHairController::SetPart - Instance is null");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        m_instance->SetHair(vnum.value());
        EterBase::ModernLogger::Info("InstanceEquipmentHairController::SetPart - Successfully set hair vnum {}", vnum.value());

        Core::EventBus::GetInstance().Publish(EquipmentPartChangedEvent{part, vnum});
        return {};
    }

    EterBase::PacketResult<void> InstanceEquipmentHairController::ClearPart(ModelPart part)
    {
        if (part != ModelPart::Hair)
        {
            EterBase::ModernLogger::Error("InstanceEquipmentHairController::ClearPart - Invalid part {}. Expected Hair.", static_cast<uint8_t>(part));
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }

        if (!m_instance)
        {
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        m_instance->SetHair(0);
        EterBase::ModernLogger::Info("InstanceEquipmentHairController::ClearPart - Cleared hair");

        Core::EventBus::GetInstance().Publish(EquipmentPartChangedEvent{part, EterBase::ItemVnum{0}});
        return {};
    }

    EterBase::ItemVnum InstanceEquipmentHairController::GetPartVnum(ModelPart part) const
    {
        if (part != ModelPart::Hair || !m_instance)
        {
            return EterBase::ItemVnum{0};
        }

        return EterBase::ItemVnum{ m_instance->GetPart(CRaceData::PART_HAIR) };
    }

    void InstanceEquipmentHairController::SetLODLevel(uint8_t /*lodLevel*/)
    {
    }

    void InstanceEquipmentHairController::ClearAllParts()
    {
        if (m_instance)
        {
            m_instance->SetHair(0);
        }
    }

    // =========================================================================
    // InstanceEquip_MainWeapon
    // =========================================================================

    InstanceEquip_MainWeapon::InstanceEquip_MainWeapon(EterBase::EntityId entityId)
        : entityId_(entityId)
    {
    }

    EterBase::PacketResult<void> InstanceEquip_MainWeapon::SetPart(ModelPart part, EterBase::ItemVnum vnum)
    {
        if (part != ModelPart::Weapon && part != ModelPart::Main)
        {
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }

        weaponVnum_ = vnum;
        EterBase::ModernLogger::Info("Equipped main weapon vnum {} (1H/2H/Bow/Bell) on right hand for entity {}",
            vnum.value(), entityId_.value());

        auto eventResult = MainWeaponEquippedEvent::Create(entityId_, vnum);
        if (eventResult.has_value())
        {
            Core::EventBus::GetInstance().Publish(eventResult.value());
        }

        return {};
    }

    EterBase::PacketResult<void> InstanceEquip_MainWeapon::ClearPart(ModelPart part)
    {
        if (part != ModelPart::Weapon && part != ModelPart::Main)
        {
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }

        weaponVnum_ = EterBase::ItemVnum{0};
        EterBase::ModernLogger::Info("Cleared main weapon from right hand for entity {}", entityId_.value());

        auto eventResult = MainWeaponClearedEvent::Create(entityId_);
        if (eventResult.has_value())
        {
            Core::EventBus::GetInstance().Publish(eventResult.value());
        }

        return {};
    }

    EterBase::ItemVnum InstanceEquip_MainWeapon::GetPartVnum(ModelPart part) const
    {
        if (part == ModelPart::Weapon || part == ModelPart::Main)
        {
            return weaponVnum_;
        }
        return EterBase::ItemVnum{0};
    }

    void InstanceEquip_MainWeapon::SetLODLevel(uint8_t lodLevel)
    {
        lodLevel_ = lodLevel;
        EterBase::ModernLogger::Debug("Set LOD level to {} for entity {} weapon", lodLevel, entityId_.value());
    }

    void InstanceEquip_MainWeapon::ClearAllParts()
    {
        if (weaponVnum_.value() != 0)
        {
            ClearPart(ModelPart::Weapon);
        }
    }

    // =========================================================================
    // InstanceEquipOffhandWeapon
    // =========================================================================

    InstanceEquipOffhandWeapon::InstanceEquipOffhandWeapon(EterBase::EntityId entityId, IInstanceEquipmentModelController& modelController)
        : m_entityId(entityId), m_modelController(modelController)
    {
    }

    EterBase::PacketResult<void> InstanceEquipOffhandWeapon::EquipOffhand(EterBase::ItemVnum vnum)
    {
        if (vnum.value() == 0)
        {
            EterBase::ModernLogger::Error("Attempted to equip offhand with invalid vnum: 0");
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }

        EterBase::ModernLogger::Info("Equipping offhand weapon vnum: {} on entity: {}", vnum.value(), m_entityId.value());

        auto result = m_modelController.SetPart(ModelPart::WeaponLeft, vnum);
        if (!result)
        {
            EterBase::ModernLogger::Error("Failed to set offhand model part for vnum: {} on entity: {}", vnum.value(), m_entityId.value());
            return result;
        }

        Core::EventBus::GetInstance().Publish(OffhandWeaponChangedEvent{m_entityId, vnum});
        return {};
    }

    EterBase::PacketResult<void> InstanceEquipOffhandWeapon::UnequipOffhand()
    {
        EterBase::ItemVnum currentVnum = m_modelController.GetPartVnum(ModelPart::WeaponLeft);
        if (currentVnum.value() == 0)
        {
            EterBase::ModernLogger::Debug("Offhand already empty on entity: {}, nothing to unequip.", m_entityId.value());
            return {};
        }

        EterBase::ModernLogger::Info("Unequipping offhand weapon (was vnum: {}) on entity: {}", currentVnum.value(), m_entityId.value());

        auto result = m_modelController.ClearPart(ModelPart::WeaponLeft);
        if (!result)
        {
            EterBase::ModernLogger::Error("Failed to clear offhand model part on entity: {}", m_entityId.value());
            return result;
        }

        Core::EventBus::GetInstance().Publish(OffhandWeaponChangedEvent{m_entityId, EterBase::ItemVnum{0}});
        return {};
    }

    // =========================================================================
    // InstanceEquipSashWingsController
    // =========================================================================

    EterBase::PacketResult<void> InstanceEquipSashWingsController::SetPart(ModelPart part, EterBase::ItemVnum vnum)
    {
        if (part != ModelPart::Sash)
        {
            EterBase::ModernLogger::Warn("InstanceEquipSashWingsController: SetPart only supports Sash.");
            return std::unexpected(EterBase::PacketError::InvalidHeader);
        }

        if (vnum.value() == 0)
        {
            EterBase::ModernLogger::Warn("InstanceEquipSashWingsController: SetPart called with vnum 0.");
            return std::unexpected(EterBase::PacketError::MalformedPayload);
        }

        m_parts[part] = vnum;
        EterBase::ModernLogger::Info("InstanceEquipSashWingsController: SetPart Sash/Wings to vnum {}", vnum.value());

        Core::EventBus::GetInstance().Publish(Events::EquipSashEvent{vnum});
        UpdateBoneDynamics(vnum);

        return {};
    }

    EterBase::PacketResult<void> InstanceEquipSashWingsController::ClearPart(ModelPart part)
    {
        if (part != ModelPart::Sash)
        {
            return std::unexpected(EterBase::PacketError::InvalidHeader);
        }

        if (m_parts.erase(part) > 0)
        {
            EterBase::ModernLogger::Info("InstanceEquipSashWingsController: ClearPart Sash/Wings.");
            Core::EventBus::GetInstance().Publish(Events::ClearSashEvent{});
        }

        return {};
    }

    EterBase::ItemVnum InstanceEquipSashWingsController::GetPartVnum(ModelPart part) const
    {
        if (auto it = m_parts.find(part); it != m_parts.end())
        {
            return it->second;
        }
        return EterBase::ItemVnum{0};
    }

    void InstanceEquipSashWingsController::SetLODLevel(uint8_t lodLevel)
    {
        m_lodLevel = lodLevel;
        EterBase::ModernLogger::Debug("InstanceEquipSashWingsController: SetLODLevel to {}", lodLevel);
    }

    void InstanceEquipSashWingsController::ClearAllParts()
    {
        if (m_parts.contains(ModelPart::Sash))
        {
            ClearPart(ModelPart::Sash);
        }
        m_parts.clear();
        EterBase::ModernLogger::Info("InstanceEquipSashWingsController: ClearAllParts executed.");
    }

    void InstanceEquipSashWingsController::UpdateBoneDynamics(EterBase::ItemVnum vnum)
    {
        EterBase::ModernLogger::Debug("InstanceEquipSashWingsController: Updating bone waving dynamics for vnum {}", vnum.value());
        Core::EventBus::GetInstance().Publish(Events::SashDynamicsUpdatedEvent{vnum});
    }

    // =========================================================================
    // Funkcje pomocnicze i fabryki
    // =========================================================================

    EterBase::PacketResult<void> UpdateEquipmentLODMesh(
        EterBase::EntityId entityId,
        IInstanceEquipmentModelController& controller,
        uint8_t lodLevel)
    {
        if (entityId.value() == 0)
        {
            EterBase::ModernLogger::Error("UpdateEquipmentLODMesh called with invalid EntityId: 0");
            return std::unexpected(EterBase::PacketError::MalformedPayload);
        }

        EterBase::ModernLogger::Debug("UpdateEquipmentLODMesh: Updating LOD for EntityId {} to level {}", entityId.value(), lodLevel);

        controller.SetLODLevel(lodLevel);
        Core::EventBus::GetInstance().Publish(EquipmentLODChangedEvent{entityId, lodLevel});

        return {};
    }

    std::unique_ptr<IInstanceEquipmentModelController> CreateCostumeOverlayController()
    {
        return std::make_unique<InstanceEquip_CostumeOverlay>();
    }

    std::unique_ptr<IInstanceEquipmentModelController> CreateMainWeaponController(EterBase::EntityId entityId)
    {
        return std::make_unique<InstanceEquip_MainWeapon>(entityId);
    }

    std::unique_ptr<IInstanceEquipmentModelController> CreateInstanceEquipSashWingsController()
    {
        return std::make_unique<InstanceEquipSashWingsController>();
    }
}
