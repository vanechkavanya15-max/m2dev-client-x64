#include "../StdAfx.h"
#include "IInstanceEquipmentModelController.h"
#include "../InstanceBase.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "../Core/EventBus.h"
#include <format>

namespace UserInterface::InstanceControllers {

    struct EquipmentPartChangedEvent : public ::UserInterface::Core::IEvent {
        ModelPart part;
        EterBase::ItemVnum vnum;

        EquipmentPartChangedEvent(ModelPart part, EterBase::ItemVnum vnum)
            : part(part), vnum(vnum) {}
    };

    class InstanceEquipmentHairController final : public IInstanceEquipmentModelController {
    public:
        explicit InstanceEquipmentHairController(CInstanceBase* instance) 
            : m_instance(instance) 
        {
        }

        ~InstanceEquipmentHairController() override = default;

        EterBase::PacketResult<void> SetPart(ModelPart part, EterBase::ItemVnum vnum) override {
            if (part != ModelPart::Hair) {
                EterBase::ModernLogger::Error("InstanceEquipmentHairController::SetPart - Invalid part {}. Expected Hair.", static_cast<uint8_t>(part));
                return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
            }

            if (!m_instance) {
                EterBase::ModernLogger::Error("InstanceEquipmentHairController::SetPart - Instance is null");
                return EterBase::MakeError(EterBase::PacketError::SessionClosed);
            }

            m_instance->SetHair(vnum.value());
            EterBase::ModernLogger::Info("InstanceEquipmentHairController::SetPart - Successfully set hair vnum {}", vnum.value());
            
            ::UserInterface::Core::EventBus::GetInstance().Publish(EquipmentPartChangedEvent{part, vnum});
            return {};
        }

        EterBase::PacketResult<void> ClearPart(ModelPart part) override {
            if (part != ModelPart::Hair) {
                EterBase::ModernLogger::Error("InstanceEquipmentHairController::ClearPart - Invalid part {}. Expected Hair.", static_cast<uint8_t>(part));
                return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
            }

            if (!m_instance) {
                return EterBase::MakeError(EterBase::PacketError::SessionClosed);
            }

            m_instance->SetHair(0);
            EterBase::ModernLogger::Info("InstanceEquipmentHairController::ClearPart - Cleared hair");
            
            ::UserInterface::Core::EventBus::GetInstance().Publish(EquipmentPartChangedEvent{part, EterBase::ItemVnum{0}});
            return {};
        }

        EterBase::ItemVnum GetPartVnum(ModelPart part) const override {
            if (part != ModelPart::Hair || !m_instance) {
                return EterBase::ItemVnum{0};
            }

            return EterBase::ItemVnum{ m_instance->GetPart(CRaceData::PART_HAIR) };
        }

        void SetLODLevel(uint8_t lodLevel) override {
            // No-op for hair equipment controller
        }

        void ClearAllParts() override {
            if (m_instance) {
                m_instance->SetHair(0);
            }
        }

    private:
        CInstanceBase* m_instance;
    };

} // namespace UserInterface::InstanceControllers
