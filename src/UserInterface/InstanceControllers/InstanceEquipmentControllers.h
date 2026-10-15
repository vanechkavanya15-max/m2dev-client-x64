#pragma once

#include <cstdint>
#include <string_view>
#include <memory>
#include <array>
#include <unordered_map>
#include <expected>

#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "../Core/EventBus.h"

class CInstanceBase;

namespace UserInterface::InstanceControllers
{
    /**
     * @brief Czesci modelu 3D postaci / wyposazenia.
     */
    enum class ModelPart : uint8_t
    {
        Main = 0,
        Weapon,
        WeaponLeft,
        Hair,
        Sash,
        CostumeBody,
        CostumeHair,
        CostumeWeapon,
        MaxParts
    };

    // --- Zdarzenia ekwipunku ---

    struct EquipmentModelChangedEvent : public Core::IEvent
    {
        ModelPart part;
        EterBase::ItemVnum vnum;

        EquipmentModelChangedEvent(ModelPart p, EterBase::ItemVnum v)
            : part(p), vnum(v) {}
    };

    struct EquipmentPartChangedEvent : public Core::IEvent
    {
        ModelPart part;
        EterBase::ItemVnum vnum;

        EquipmentPartChangedEvent(ModelPart part, EterBase::ItemVnum vnum)
            : part(part), vnum(vnum) {}
    };

    struct EquipmentPartsClearedEvent : public Core::IEvent
    {
        EquipmentPartsClearedEvent() = default;
    };

    struct EquipmentLODChangedEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;
        uint8_t lodLevel;

        EquipmentLODChangedEvent(EterBase::EntityId id, uint8_t level)
            : entityId(id), lodLevel(level) {}
    };

    struct MainWeaponEquippedEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;
        EterBase::ItemVnum vnum;

        static std::expected<MainWeaponEquippedEvent, EterBase::EntityError> Create(EterBase::EntityId id, EterBase::ItemVnum vnum)
        {
            if (vnum.value() == 0)
                return std::unexpected(EterBase::EntityError::InvalidType);
            if (id.value() == 0)
                return std::unexpected(EterBase::EntityError::NotFound);
            return MainWeaponEquippedEvent(id, vnum);
        }

    private:
        explicit MainWeaponEquippedEvent(EterBase::EntityId id, EterBase::ItemVnum v)
            : entityId(id), vnum(v) {}
    };

    struct MainWeaponClearedEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;

        static std::expected<MainWeaponClearedEvent, EterBase::EntityError> Create(EterBase::EntityId id)
        {
            if (id.value() == 0)
                return std::unexpected(EterBase::EntityError::NotFound);
            return MainWeaponClearedEvent(id);
        }

    private:
        explicit MainWeaponClearedEvent(EterBase::EntityId id) : entityId(id) {}
    };

    struct OffhandWeaponChangedEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;
        EterBase::ItemVnum vnum;

        explicit OffhandWeaponChangedEvent(EterBase::EntityId id, EterBase::ItemVnum v)
            : entityId(id), vnum(v) {}
    };

    namespace Events
    {
        struct EquipSashEvent : public Core::IEvent
        {
            EterBase::ItemVnum vnum;
            explicit EquipSashEvent(EterBase::ItemVnum v) : vnum(v) {}
        };

        struct ClearSashEvent : public Core::IEvent
        {
            explicit ClearSashEvent() = default;
        };

        struct SashDynamicsUpdatedEvent : public Core::IEvent
        {
            EterBase::ItemVnum vnum;
            explicit SashDynamicsUpdatedEvent(EterBase::ItemVnum v) : vnum(v) {}
        };
    }

    /**
     * @brief Glowny interfejs kontrolera modelu ekwipunku postaci.
     */
    class IInstanceEquipmentModelController
    {
    public:
        virtual ~IInstanceEquipmentModelController() = default;

        virtual EterBase::PacketResult<void> SetPart(ModelPart part, EterBase::ItemVnum vnum) = 0;
        virtual EterBase::PacketResult<void> ClearPart(ModelPart part) = 0;
        virtual EterBase::ItemVnum GetPartVnum(ModelPart part) const = 0;
        virtual void SetLODLevel(uint8_t lodLevel) = 0;
        virtual void ClearAllParts();
    };

    /**
     * @brief Kontroler korpusu/czesci modelu 3D z tablica slotow.
     */
    class InstanceEquip_Body : public IInstanceEquipmentModelController
    {
    public:
        InstanceEquip_Body();
        ~InstanceEquip_Body() override;

        EterBase::PacketResult<void> SetPart(ModelPart part, EterBase::ItemVnum vnum) override;
        EterBase::PacketResult<void> ClearPart(ModelPart part) override;
        EterBase::ItemVnum GetPartVnum(ModelPart part) const override;
        void SetLODLevel(uint8_t lodLevel) override;
        void ClearAllParts() override;

    private:
        std::array<EterBase::ItemVnum, static_cast<size_t>(ModelPart::MaxParts)> parts_;
        uint8_t lodLevel_{0};
    };

    /**
     * @brief Kontroler nakladajacy priorytet kostiumow nad zwykly ekwipunek.
     */
    class InstanceEquip_CostumeOverlay final : public IInstanceEquipmentModelController
    {
    public:
        InstanceEquip_CostumeOverlay();
        ~InstanceEquip_CostumeOverlay() override = default;

        EterBase::PacketResult<void> SetPart(ModelPart part, EterBase::ItemVnum vnum) override;
        EterBase::PacketResult<void> ClearPart(ModelPart part) override;
        EterBase::ItemVnum GetPartVnum(ModelPart part) const override;
        void SetLODLevel(uint8_t lodLevel) override;
        void ClearAllParts() override;

    private:
        std::array<EterBase::ItemVnum, static_cast<size_t>(ModelPart::MaxParts)> m_parts;
        uint8_t m_lodLevel{0};
    };

    /**
     * @brief Kontroler fryzury dla instancji postaci.
     */
    class InstanceEquipmentHairController final : public IInstanceEquipmentModelController
    {
    public:
        explicit InstanceEquipmentHairController(CInstanceBase* instance);
        ~InstanceEquipmentHairController() override = default;

        EterBase::PacketResult<void> SetPart(ModelPart part, EterBase::ItemVnum vnum) override;
        EterBase::PacketResult<void> ClearPart(ModelPart part) override;
        EterBase::ItemVnum GetPartVnum(ModelPart part) const override;
        void SetLODLevel(uint8_t lodLevel) override;
        void ClearAllParts() override;

    private:
        CInstanceBase* m_instance{nullptr};
    };

    /**
     * @brief Kontroler broni glownej prawej reki (1H, 2H, luk, dzwon).
     */
    class InstanceEquip_MainWeapon : public IInstanceEquipmentModelController
    {
    public:
        explicit InstanceEquip_MainWeapon(EterBase::EntityId entityId);
        ~InstanceEquip_MainWeapon() override = default;

        EterBase::PacketResult<void> SetPart(ModelPart part, EterBase::ItemVnum vnum) override;
        EterBase::PacketResult<void> ClearPart(ModelPart part) override;
        EterBase::ItemVnum GetPartVnum(ModelPart part) const override;
        void SetLODLevel(uint8_t lodLevel) override;
        void ClearAllParts() override;

    private:
        EterBase::EntityId entityId_{0};
        EterBase::ItemVnum weaponVnum_{0};
        uint8_t lodLevel_{0};
    };

    /**
     * @brief Kontroler wyposazenia broni lewej reki (sztylety, dual wield).
     */
    class InstanceEquipOffhandWeapon
    {
    public:
        InstanceEquipOffhandWeapon(EterBase::EntityId entityId, IInstanceEquipmentModelController& modelController);

        EterBase::PacketResult<void> EquipOffhand(EterBase::ItemVnum vnum);
        EterBase::PacketResult<void> UnequipOffhand();

    private:
        EterBase::EntityId m_entityId{0};
        IInstanceEquipmentModelController& m_modelController;
    };

    /**
     * @brief Kontroler szarf oraz skrzydel z dynamika kosci.
     */
    class InstanceEquipSashWingsController : public IInstanceEquipmentModelController
    {
    public:
        InstanceEquipSashWingsController() = default;
        ~InstanceEquipSashWingsController() override = default;

        EterBase::PacketResult<void> SetPart(ModelPart part, EterBase::ItemVnum vnum) override;
        EterBase::PacketResult<void> ClearPart(ModelPart part) override;
        EterBase::ItemVnum GetPartVnum(ModelPart part) const override;
        void SetLODLevel(uint8_t lodLevel) override;
        void ClearAllParts() override;

    private:
        void UpdateBoneDynamics(EterBase::ItemVnum vnum);

        std::unordered_map<ModelPart, EterBase::ItemVnum> m_parts;
        uint8_t m_lodLevel{0};
    };

    // --- Pomocnicze funkcje domenowe i fabryki ---
    EterBase::PacketResult<void> UpdateEquipmentLODMesh(
        EterBase::EntityId entityId,
        IInstanceEquipmentModelController& controller,
        uint8_t lodLevel);

    std::unique_ptr<IInstanceEquipmentModelController> CreateCostumeOverlayController();
    std::unique_ptr<IInstanceEquipmentModelController> CreateMainWeaponController(EterBase::EntityId entityId);
    std::unique_ptr<IInstanceEquipmentModelController> CreateInstanceEquipSashWingsController();
}
