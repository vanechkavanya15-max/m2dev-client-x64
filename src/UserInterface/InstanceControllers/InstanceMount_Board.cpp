#include "../StdAfx.h"
#include "IInstanceMountHorseController.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "../Core/EventBus.h"

#include <cmath>
#include <expected>
#include <memory>

namespace UserInterface::InstanceControllers
{
    /**
     * @brief Implementation of mount and horse boarding mechanics utilizing modern C++23.
     * Follows Single Responsibility Principle (SRP) to manage instance state and emit events.
     */
    class InstanceMount_Board final : public IInstanceMountHorseController
    {
    public:
        InstanceMount_Board() = default;
        ~InstanceMount_Board() override = default;

        /**
         * @brief Initiates boarding of a mount and computes interpolation for smooth UI updates.
         * @param mountVid The Entity ID of the mount instance.
         * @param mountVnum The VNUM identifier of the mount.
         * @return PacketResult containing void on success, or an error code.
         */
        EterBase::PacketResult<void> Mount(EterBase::EntityId mountVid, uint32_t mountVnum) override
        {
            if (m_isMounted)
            {
                EterBase::ModernLogger::Error("InstanceMount_Board::Mount - Already mounted on VID: {}", static_cast<uint32_t>(m_mountVid));
                return std::unexpected(EterBase::PacketError::SequenceMismatch);
            }

            m_mountVid = mountVid;
            m_mountVnum = mountVnum;
            m_isMounted = true;

            // Initialize smooth mount boarding interpolation
            m_isInterpolating = true;
            m_interpolationStartHeight = m_z;
            m_interpolationTargetHeight = m_z + 100.0f; // Typical mount elevation height
            m_interpolationProgress = 0.0f;
            
            EterBase::ModernLogger::Info("InstanceMount_Board::Mount - Boarding mount VNUM {}", mountVnum);

            // Emit decoupled event for subsystems & UI
            ::UserInterface::Core::EventBus::GetInstance().Publish(
                ::UserInterface::Core::MountStateChangedEvent(static_cast<uint32_t>(m_mountVid), m_mountVnum, 1)
            );

            return {};
        }

        /**
         * @brief Dismounts the entity from the current mount.
         * @return PacketResult containing void on success, or an error code.
         */
        EterBase::PacketResult<void> Dismount() override
        {
            if (!m_isMounted)
            {
                EterBase::ModernLogger::Error("InstanceMount_Board::Dismount - Not mounted.");
                return std::unexpected(EterBase::PacketError::SequenceMismatch);
            }

            EterBase::ModernLogger::Info("InstanceMount_Board::Dismount - Dismounted from VNUM {}", m_mountVnum);

            auto previousVid = static_cast<uint32_t>(m_mountVid);
            Clear();

            // Emit decoupled event for subsystems & UI
            ::UserInterface::Core::EventBus::GetInstance().Publish(
                ::UserInterface::Core::MountStateChangedEvent(previousVid, 0, 0)
            );

            return {};
        }

        /**
         * @brief Checks if the entity is currently mounted.
         * @return true if mounted, false otherwise.
         */
        bool IsMounted() const override
        {
            return m_isMounted;
        }

        /**
         * @brief Gets the VID of the current mount.
         * @return EntityId representing the mount.
         */
        EterBase::EntityId GetMountVID() const override
        {
            return m_mountVid;
        }

        /**
         * @brief Gets the VNUM of the current mount.
         * @return VNUM identifier as uint32_t.
         */
        uint32_t GetMountVnum() const override
        {
            return m_mountVnum;
        }

        /**
         * @brief Updates the positional transform of the mount.
         * @param x X coordinate.
         * @param y Y coordinate.
         * @param z Z coordinate.
         * @param rot Rotation angle.
         */
        void UpdateMountTransform(float x, float y, float z, float rot) override
        {
            if (!m_isMounted)
            {
                return;
            }

            m_x = x;
            m_y = y;
            m_rot = rot;

            if (m_isInterpolating)
            {
                // In a real game loop, we'd use actual delta time. We simulate a tick increment for now.
                // Assuming this is called per frame, e.g. 60fps (~0.016s delta). 
                // A fixed interpolation step of 0.03f gives ~33 ticks (0.5 seconds).
                m_interpolationProgress += 0.03f;
                
                if (m_interpolationProgress >= 1.0f)
                {
                    m_interpolationProgress = 1.0f;
                    m_isInterpolating = false;
                }
                
                m_z = std::lerp(m_interpolationStartHeight, m_interpolationTargetHeight, m_interpolationProgress);
            }
            else
            {
                m_z = z;
            }
        }

        /**
         * @brief Clears the internal state representing the mount.
         */
        void Clear() override
        {
            m_isMounted = false;
            m_isInterpolating = false;
            m_mountVid = EterBase::EntityId{0};
            m_mountVnum = 0;
            m_interpolationProgress = 0.0f;
        }

    private:
        bool m_isMounted{false};
        bool m_isInterpolating{false};
        EterBase::EntityId m_mountVid{0};
        uint32_t m_mountVnum{0};

        float m_x{0.0f};
        float m_y{0.0f};
        float m_z{0.0f};
        float m_rot{0.0f};

        float m_interpolationStartHeight{0.0f};
        float m_interpolationTargetHeight{0.0f};
        float m_interpolationProgress{0.0f};
    };

    std::unique_ptr<IInstanceMountHorseController> CreateMountBoardController()
    {
        return std::make_unique<InstanceMount_Board>();
    }
}
