#include "../StdAfx.h"
#include "InstanceEquip_Body.h"
#include "../../EterBase/LogModern.h"

namespace UserInterface::InstanceControllers
{
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
}
