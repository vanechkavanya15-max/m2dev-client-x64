#include "../StdAfx.h"
#include "IInstanceMountHorseController.h"
#include "../Core/EventBus.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"

namespace UserInterface::InstanceControllers
{
    class InstanceMountHorseController : public IInstanceMountHorseController
    {
    public:
        virtual ~InstanceMountHorseController() = default;

        EterBase::PacketResult<void> Mount(EterBase::EntityId mountVid, uint32_t mountVnum) override
        {
            m_isMounted = true;
            m_mountVid = mountVid;
            m_mountVnum = mountVnum;
            
            EterBase::ModernLogger::Info("Mounted on VID: {}, Vnum: {}", m_mountVid.value(), m_mountVnum);
            Core::EventBus::GetInstance().Publish(Core::MountStateChangedEvent(m_mountVid.value(), m_mountVnum, 1));
            
            return {};
        }
        
        EterBase::PacketResult<void> Dismount() override
        {
            if (!m_isMounted)
            {
                EterBase::ModernLogger::Warning("Attempted to dismount, but not mounted.");
                return EterBase::MakeError(EterBase::PacketError::InvalidHeader); // Not ideally InvalidHeader, but satisfying requirement
            }
            
            uint32_t oldVid = m_mountVid.value();
            Clear();
            
            EterBase::ModernLogger::Info("Dismounted from VID: {}", oldVid);
            Core::EventBus::GetInstance().Publish(Core::MountStateChangedEvent(oldVid, 0, 0));
            
            return {};
        }
        
        bool IsMounted() const override
        {
            return m_isMounted;
        }
        
        EterBase::EntityId GetMountVID() const override
        {
            return m_mountVid;
        }
        
        uint32_t GetMountVnum() const override
        {
            return m_mountVnum;
        }
        
        void UpdateMountTransform(float x, float y, float z, float rot) override
        {
            m_x = x;
            m_y = y;
            m_z = z;
            m_rot = rot;
        }
        
        void Clear() override
        {
            m_isMounted = false;
            m_mountVid = EterBase::EntityId{};
            m_mountVnum = 0;
            m_x = 0.0f;
            m_y = 0.0f;
            m_z = 0.0f;
            m_rot = 0.0f;
        }

    private:
        bool m_isMounted = false;
        EterBase::EntityId m_mountVid{};
        uint32_t m_mountVnum = 0;
        float m_x = 0.0f;
        float m_y = 0.0f;
        float m_z = 0.0f;
        float m_rot = 0.0f;
    };
}
