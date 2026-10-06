#include "../StdAfx.h"
#include "IInstanceMountHorseController.h"
#include "../Core/EventBus.h"
#include "EterBase/ModernLogger.h"
#include "EterBase/Result.h"

namespace UserInterface::InstanceControllers
{
    /**
     * @brief A concrete implementation of the IInstanceMountHorseController for managing mount events.
     */
    class InstanceMountHorseController final : public IInstanceMountHorseController
    {
    public:
        InstanceMountHorseController() = default;
        ~InstanceMountHorseController() override = default;

        /**
         * @brief Mounts the entity and broadcasts a state change event.
         * @param mountVid The unique identifier of the mount entity.
         * @param mountVnum The virtual number of the mount.
         * @return PacketResult<void> Returns empty success on completion.
         */
        EterBase::PacketResult<void> Mount(EterBase::EntityId mountVid, uint32_t mountVnum) override
        {
            if (isMounted_)
            {
                EterBase::ModernLogger::Debug("Mount attempted while already mounted.");
            }

            isMounted_ = true;
            mountVid_ = mountVid;
            mountVnum_ = mountVnum;

            // Character ID is passed as 0 since the instance controller does not manage the character ID natively here.
            Core::EventBus::GetInstance().Publish(Core::MountStateChangedEvent(0, mountVnum, 1));
            
            EterBase::ModernLogger::Info("Instance mounted: VID {}, Vnum {}", mountVid.value(), mountVnum);

            return {};
        }

        /**
         * @brief Dismounts the entity and broadcasts a state change event.
         * @return PacketResult<void> Returns empty success on completion.
         */
        EterBase::PacketResult<void> Dismount() override
        {
            if (!isMounted_)
            {
                return {};
            }

            EterBase::ModernLogger::Info("Instance dismounted: VID {}", mountVid_.value());

            isMounted_ = false;
            mountVid_ = EterBase::EntityId{0};
            mountVnum_ = 0;

            Core::EventBus::GetInstance().Publish(Core::MountStateChangedEvent(0, 0, 0));

            return {};
        }

        /**
         * @brief Checks if the instance is currently mounted.
         * @return True if mounted, false otherwise.
         */
        bool IsMounted() const override
        {
            return isMounted_;
        }

        /**
         * @brief Gets the current mount entity ID.
         * @return The mount entity ID.
         */
        EterBase::EntityId GetMountVID() const override
        {
            return mountVid_;
        }

        /**
         * @brief Gets the current mount Vnum.
         * @return The mount Vnum.
         */
        uint32_t GetMountVnum() const override
        {
            return mountVnum_;
        }

        /**
         * @brief Updates the transform of the mount.
         * @param x The X coordinate.
         * @param y The Y coordinate.
         * @param z The Z coordinate.
         * @param rot The rotation.
         */
        void UpdateMountTransform(float x, float y, float z, float rot) override
        {
            x_ = x;
            y_ = y;
            z_ = z;
            rot_ = rot;
        }

        /**
         * @brief Clears the mount state.
         */
        void Clear() override
        {
            (void)Dismount();
        }

    private:
        bool isMounted_{false};
        EterBase::EntityId mountVid_{0};
        uint32_t mountVnum_{0};
        
        float x_{0.0f};
        float y_{0.0f};
        float z_{0.0f};
        float rot_{0.0f};
    };
}
