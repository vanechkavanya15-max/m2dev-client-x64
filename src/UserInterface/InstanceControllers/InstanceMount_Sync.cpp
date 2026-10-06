#include "../StdAfx.h"


#include "IInstanceMountHorseController.h"
#include "../Core/EventBus.h"
#include "../../EterBase/ModernLogger.h"

namespace UserInterface::InstanceControllers
{
    // Define the event here if it's not in the shared EventBus header to ensure zero-conflict
    struct RiderTransformSyncEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId mountVid;
        float x, y, z, rot;
        
        RiderTransformSyncEvent(EterBase::EntityId id, float x, float y, float z, float rot)
            : mountVid(id), x(x), y(y), z(z), rot(rot) {}
    };

    class InstanceMountSyncController final : public IInstanceMountHorseController
    {
    public:
        InstanceMountSyncController() 
            : m_isMounted(false), m_mountVid(0), m_mountVnum(0), m_x(0.0f), m_y(0.0f), m_z(0.0f), m_rot(0.0f) 
        {
        }

        ~InstanceMountSyncController() override = default;

        EterBase::PacketResult<void> Mount(EterBase::EntityId mountVid, uint32_t mountVnum) override
        {
            if (m_isMounted)
            {
                EterBase::ModernLogger::Warning("InstanceMountSyncController::Mount: Already mounted on {}", m_mountVnum.get());
                return EterBase::MakeError(EterBase::PacketError::InvalidHeader); // Or a specific error
            }

            m_mountVid = mountVid;
            m_mountVnum = EterBase::ItemVnum{mountVnum};
            m_isMounted = true;

            EterBase::ModernLogger::Info("InstanceMountSyncController::Mount: Mounted on VID {}, VNUM {}", mountVid.get(), m_mountVnum.get());

            // The EventBus notifies GUI and other systems of the mount state change.
            UserInterface::Core::EventBus::GetInstance().Publish(
                UserInterface::Core::MountStateChangedEvent{m_mountVid.get(), m_mountVnum.get(), 1});

            return EterBase::VoidResult<EterBase::PacketError>{};
        }

        EterBase::PacketResult<void> Dismount() override
        {
            if (!m_isMounted)
            {
                EterBase::ModernLogger::Warning("InstanceMountSyncController::Dismount: Not currently mounted.");
                return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
            }

            EterBase::ModernLogger::Info("InstanceMountSyncController::Dismount: Dismounted from VNUM {}", m_mountVnum.get());

            m_isMounted = false;
            
            UserInterface::Core::EventBus::GetInstance().Publish(
                UserInterface::Core::MountStateChangedEvent{m_mountVid.get(), 0, 0});

            m_mountVid = EterBase::EntityId{0};
            m_mountVnum = EterBase::ItemVnum{0};
            
            return EterBase::VoidResult<EterBase::PacketError>{};
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
            return m_mountVnum.get();
        }

        void UpdateMountTransform(float x, float y, float z, float rot) override
        {
            if (!m_isMounted) return;

            // Synchronize the rider's transformation matrix with the horse/mount saddle skeleton.
            // Under zero-conflict rules we do not edit global monolithic InstanceBase, but emit an event
            // or perform mathematical sync. Since we don't have access to CGraphicThingInstance inside this 
            // zero-conflict class, we emit an event for matrix sync.
            
            m_x = x;
            m_y = y;
            m_z = z;
            m_rot = rot;

            // Log the matrix sync calculation
            EterBase::ModernLogger::Trace("InstanceMountSyncController::UpdateMountTransform: Rider transformation matrix sync to saddle skeleton. VID {} at ({}, {}, {}) rot {}", 
                m_mountVid.get(), x, y, z, rot);
                
            UserInterface::Core::EventBus::GetInstance().Publish(RiderTransformSyncEvent{m_mountVid, x, y, z, rot});
        }

        void Clear() override
        {
            if (m_isMounted)
            {
                Dismount();
            }
        }

    private:
        bool m_isMounted;
        EterBase::EntityId m_mountVid;
        EterBase::ItemVnum m_mountVnum;
        float m_x;
        float m_y;
        float m_z;
        float m_rot;
    };
}
