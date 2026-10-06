#include "../StdAfx.h"
#include "IInstanceEquipmentModelController.h"
#include "UserInterface/Core/EventBus.h"
#include "EterBase/LogModern.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

#include <unordered_map>
#include <memory>

namespace UserInterface::InstanceControllers {

namespace Events {
    struct EquipSashEvent : public UserInterface::Core::IEvent {
        EterBase::ItemVnum vnum;
        
        explicit EquipSashEvent(EterBase::ItemVnum v) : vnum(v) {}
    };

    struct ClearSashEvent : public UserInterface::Core::IEvent {
        explicit ClearSashEvent() = default;
    };

    struct SashDynamicsUpdatedEvent : public UserInterface::Core::IEvent {
        EterBase::ItemVnum vnum;
        
        explicit SashDynamicsUpdatedEvent(EterBase::ItemVnum v) : vnum(v) {}
    };
}

class InstanceEquipSashWingsController : public IInstanceEquipmentModelController {
public:
    InstanceEquipSashWingsController() = default;
    ~InstanceEquipSashWingsController() override = default;

    EterBase::PacketResult<void> SetPart(ModelPart part, EterBase::ItemVnum vnum) override {
        if (part != ModelPart::Sash) {
            EterBase::ModernLogger::Warn("InstanceEquipSashWingsController: SetPart only supports Sash.");
            return std::unexpected(EterBase::PacketError::InvalidHeader);
        }

        if (vnum.value() == 0) {
            EterBase::ModernLogger::Warn("InstanceEquipSashWingsController: SetPart called with vnum 0.");
            return std::unexpected(EterBase::PacketError::MalformedPayload);
        }

        m_parts[part] = vnum;
        EterBase::ModernLogger::Info("InstanceEquipSashWingsController: SetPart Sash/Wings to vnum {}", vnum.value());

        // Emit events to decouple from GUI
        UserInterface::Core::EventBus::GetInstance().Publish(Events::EquipSashEvent{vnum});
        
        // Emulate bone dynamics for sash/wings
        UpdateBoneDynamics(vnum);

        return {};
    }

    EterBase::PacketResult<void> ClearPart(ModelPart part) override {
        if (part != ModelPart::Sash) {
            return std::unexpected(EterBase::PacketError::InvalidHeader);
        }

        if (m_parts.erase(part) > 0) {
            EterBase::ModernLogger::Info("InstanceEquipSashWingsController: ClearPart Sash/Wings.");
            UserInterface::Core::EventBus::GetInstance().Publish(Events::ClearSashEvent{});
        }

        return {};
    }

    EterBase::ItemVnum GetPartVnum(ModelPart part) const override {
        if (auto it = m_parts.find(part); it != m_parts.end()) {
            return it->second;
        }
        return EterBase::ItemVnum{0};
    }

    void SetLODLevel(uint8_t lodLevel) override {
        m_lodLevel = lodLevel;
        EterBase::ModernLogger::Debug("InstanceEquipSashWingsController: SetLODLevel to {}", lodLevel);
    }

    void ClearAllParts() override {
        if (m_parts.contains(ModelPart::Sash)) {
            ClearPart(ModelPart::Sash);
        }
        m_parts.clear();
        EterBase::ModernLogger::Info("InstanceEquipSashWingsController: ClearAllParts executed.");
    }

private:
    void UpdateBoneDynamics(EterBase::ItemVnum vnum) {
        EterBase::ModernLogger::Debug("InstanceEquipSashWingsController: Updating bone waving dynamics for vnum {}", vnum.value());
        UserInterface::Core::EventBus::GetInstance().Publish(Events::SashDynamicsUpdatedEvent{vnum});
    }

    std::unordered_map<ModelPart, EterBase::ItemVnum> m_parts;
    uint8_t m_lodLevel{0};
};

std::unique_ptr<IInstanceEquipmentModelController> CreateInstanceEquipSashWingsController() {
    return std::make_unique<InstanceEquipSashWingsController>();
}

} // namespace UserInterface::InstanceControllers
