#include "../StdAfx.h"
#include "InstanceMountControllers.h"
#include "../../EterBase/LogModern.h"

#include <cmath>

namespace UserInterface::InstanceControllers
{
    // =========================================================================
    // InstanceMount_Board
    // =========================================================================

    EterBase::PacketResult<void> InstanceMount_Board::Mount(EterBase::EntityId mountVid, uint32_t mountVnum)
    {
        if (m_isMounted)
        {
            EterBase::ModernLogger::Error("InstanceMount_Board::Mount - Already mounted on VID: {}", static_cast<uint32_t>(m_mountVid));
            return std::unexpected(EterBase::PacketError::SequenceMismatch);
        }

        m_mountVid = mountVid;
        m_mountVnum = mountVnum;
        m_isMounted = true;

        m_isInterpolating = true;
        m_interpolationStartHeight = m_z;
        m_interpolationTargetHeight = m_z + 100.0f;
        m_interpolationProgress = 0.0f;

        EterBase::ModernLogger::Info("InstanceMount_Board::Mount - Boarding mount VNUM {}", mountVnum);

        Core::EventBus::GetInstance().Publish(
            Core::MountStateChangedEvent(static_cast<uint32_t>(m_mountVid), m_mountVnum, 1)
        );

        return {};
    }

    EterBase::PacketResult<void> InstanceMount_Board::Dismount()
    {
        if (!m_isMounted)
        {
            EterBase::ModernLogger::Error("InstanceMount_Board::Dismount - Not mounted.");
            return std::unexpected(EterBase::PacketError::SequenceMismatch);
        }

        EterBase::ModernLogger::Info("InstanceMount_Board::Dismount - Dismounted from VNUM {}", m_mountVnum);

        auto previousVid = static_cast<uint32_t>(m_mountVid);
        Clear();

        Core::EventBus::GetInstance().Publish(
            Core::MountStateChangedEvent(previousVid, 0, 0)
        );

        return {};
    }

    bool InstanceMount_Board::IsMounted() const
    {
        return m_isMounted;
    }

    EterBase::EntityId InstanceMount_Board::GetMountVID() const
    {
        return m_mountVid;
    }

    uint32_t InstanceMount_Board::GetMountVnum() const
    {
        return m_mountVnum;
    }

    void InstanceMount_Board::UpdateMountTransform(float x, float y, float z, float rot)
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

    void InstanceMount_Board::Clear()
    {
        m_isMounted = false;
        m_isInterpolating = false;
        m_mountVid = EterBase::EntityId{0};
        m_mountVnum = 0;
        m_interpolationProgress = 0.0f;
    }

    // =========================================================================
    // InstanceMountHorseController
    // =========================================================================

    EterBase::PacketResult<void> InstanceMountHorseController::Mount(EterBase::EntityId mountVid, uint32_t mountVnum)
    {
        m_isMounted = true;
        m_mountVid = mountVid;
        m_mountVnum = mountVnum;

        EterBase::ModernLogger::Info("Mounted on VID: {}, Vnum: {}", m_mountVid.value(), m_mountVnum);
        Core::EventBus::GetInstance().Publish(Core::MountStateChangedEvent(m_mountVid.value(), m_mountVnum, 1));

        return {};
    }

    EterBase::PacketResult<void> InstanceMountHorseController::Dismount()
    {
        if (!m_isMounted)
        {
            EterBase::ModernLogger::Warning("Attempted to dismount, but not mounted.");
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }

        uint32_t oldVid = m_mountVid.value();
        Clear();

        EterBase::ModernLogger::Info("Dismounted from VID: {}", oldVid);
        Core::EventBus::GetInstance().Publish(Core::MountStateChangedEvent(oldVid, 0, 0));

        return {};
    }

    bool InstanceMountHorseController::IsMounted() const
    {
        return m_isMounted;
    }

    EterBase::EntityId InstanceMountHorseController::GetMountVID() const
    {
        return m_mountVid;
    }

    uint32_t InstanceMountHorseController::GetMountVnum() const
    {
        return m_mountVnum;
    }

    void InstanceMountHorseController::UpdateMountTransform(float x, float y, float z, float rot)
    {
        m_x = x;
        m_y = y;
        m_z = z;
        m_rot = rot;
    }

    void InstanceMountHorseController::Clear()
    {
        m_isMounted = false;
        m_mountVid = EterBase::EntityId{};
        m_mountVnum = 0;
        m_x = 0.0f;
        m_y = 0.0f;
        m_z = 0.0f;
        m_rot = 0.0f;
    }

    // =========================================================================
    // InstanceMountCombatController
    // =========================================================================

    EterBase::PacketResult<void> InstanceMountCombatController::Mount(EterBase::EntityId /*mountVid*/, uint32_t /*mountVnum*/)
    {
        EterBase::ModernLogger::Error("Mount method stubbed in combat controller.");
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    EterBase::PacketResult<void> InstanceMountCombatController::Dismount()
    {
        EterBase::ModernLogger::Error("Dismount method stubbed in combat controller.");
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    bool InstanceMountCombatController::IsMounted() const
    {
        return isMounted;
    }

    EterBase::EntityId InstanceMountCombatController::GetMountVID() const
    {
        return currentMountVid;
    }

    uint32_t InstanceMountCombatController::GetMountVnum() const
    {
        return currentMountVnum;
    }

    void InstanceMountCombatController::UpdateMountTransform(float x, float y, float z, float rot)
    {
        if (!isMounted) return;

        auto rangeResult = CalculateCombatAttackRange(x, y, z);
        auto rotationResult = CalculateCombatRotation(rot);

        float attackRange = rangeResult.value_or(0.0f);
        float rotationAngle = rotationResult.value_or(0.0f);

        UserInterface::Core::EventBus::GetInstance().Publish(
            UserInterface::Core::MountCombatStateEvent(attackRange, rotationAngle)
        );
    }

    void InstanceMountCombatController::Clear()
    {
        EterBase::ModernLogger::Error("Clear method stubbed in combat controller.");
    }

    std::expected<float, std::string_view> InstanceMountCombatController::CalculateCombatAttackRange(float x, float y, float z) const
    {
        float range = std::sqrt(x * x + y * y + z * z);
        return range;
    }

    std::expected<float, std::string_view> InstanceMountCombatController::CalculateCombatRotation(float rot) const
    {
        float newRot = std::fmod(rot + 15.0f, 360.0f);
        return newRot;
    }

    // =========================================================================
    // InstanceMountSyncController
    // =========================================================================

    InstanceMountSyncController::InstanceMountSyncController()
        : m_isMounted(false), m_mountVid(0), m_mountVnum(0), m_x(0.0f), m_y(0.0f), m_z(0.0f), m_rot(0.0f)
    {
    }

    EterBase::PacketResult<void> InstanceMountSyncController::Mount(EterBase::EntityId mountVid, uint32_t mountVnum)
    {
        if (m_isMounted)
        {
            EterBase::ModernLogger::Warning("InstanceMountSyncController::Mount: Already mounted on {}", m_mountVnum.get());
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }

        m_mountVid = mountVid;
        m_mountVnum = EterBase::ItemVnum{mountVnum};
        m_isMounted = true;

        EterBase::ModernLogger::Info("InstanceMountSyncController::Mount: Mounted on VID {}, VNUM {}", mountVid.get(), m_mountVnum.get());

        Core::EventBus::GetInstance().Publish(
            Core::MountStateChangedEvent{m_mountVid.get(), m_mountVnum.get(), 1});

        return EterBase::VoidResult<EterBase::PacketError>{};
    }

    EterBase::PacketResult<void> InstanceMountSyncController::Dismount()
    {
        if (!m_isMounted)
        {
            EterBase::ModernLogger::Warning("InstanceMountSyncController::Dismount: Not currently mounted.");
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }

        EterBase::ModernLogger::Info("InstanceMountSyncController::Dismount: Dismounted from VNUM {}", m_mountVnum.get());
        m_isMounted = false;

        Core::EventBus::GetInstance().Publish(
            Core::MountStateChangedEvent{m_mountVid.get(), 0, 0});

        m_mountVid = EterBase::EntityId{0};
        m_mountVnum = EterBase::ItemVnum{0};

        return EterBase::VoidResult<EterBase::PacketError>{};
    }

    bool InstanceMountSyncController::IsMounted() const
    {
        return m_isMounted;
    }

    EterBase::EntityId InstanceMountSyncController::GetMountVID() const
    {
        return m_mountVid;
    }

    uint32_t InstanceMountSyncController::GetMountVnum() const
    {
        return m_mountVnum.get();
    }

    void InstanceMountSyncController::UpdateMountTransform(float x, float y, float z, float rot)
    {
        if (!m_isMounted) return;

        m_x = x;
        m_y = y;
        m_z = z;
        m_rot = rot;

        EterBase::ModernLogger::Trace("InstanceMountSyncController::UpdateMountTransform: VID {} at ({}, {}, {}) rot {}",
            m_mountVid.get(), x, y, z, rot);

        Core::EventBus::GetInstance().Publish(RiderTransformSyncEvent{m_mountVid, x, y, z, rot});
    }

    void InstanceMountSyncController::Clear()
    {
        if (m_isMounted)
        {
            Dismount();
        }
    }

    // =========================================================================
    // MountAnimLinker
    // =========================================================================

    EterBase::PacketResult<void> MountAnimLinker::SyncAnimations(
        const IInstanceMountHorseController& mountController,
        IInstanceAnimationController& horseAnimCtrl,
        IInstanceAnimationController& riderAnimCtrl,
        uint32_t motionKey,
        float speedMultiplier)
    {
        if (!mountController.IsMounted())
        {
            EterBase::ModernLogger::Warning("MountAnimLinker: Player is not mounted, cannot sync animations.");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        EterBase::EntityId riderId = mountController.GetMountVID();

        MotionConfig horseConfig;
        horseConfig.motionKey = motionKey;
        horseConfig.blendTime = 0.15f;
        horseConfig.speedMultiplier = speedMultiplier;
        horseConfig.isLooping = true;

        MotionConfig riderConfig = horseConfig;

        auto horseResult = horseAnimCtrl.PlayMotion(horseConfig);
        if (!horseResult.has_value())
        {
            EterBase::ModernLogger::Error("MountAnimLinker: Failed to play motion on horse.");
            return horseResult;
        }

        auto riderResult = riderAnimCtrl.PlayMotion(riderConfig);
        if (!riderResult.has_value())
        {
            EterBase::ModernLogger::Error("MountAnimLinker: Failed to play synced motion on rider.");
            return riderResult;
        }

        EterBase::ModernLogger::Info("MountAnimLinker: Synced animation {} for rider {}", motionKey, riderId.get());

        Core::EventBus::GetInstance().Publish(MountAnimSyncedEvent(riderId, motionKey));
        return {};
    }

    // =========================================================================
    // Fabryki
    // =========================================================================

    std::unique_ptr<IInstanceMountHorseController> CreateMountBoardController()
    {
        return std::make_unique<InstanceMount_Board>();
    }

    std::unique_ptr<IInstanceMountHorseController> CreateMountHorseController()
    {
        return std::make_unique<InstanceMountHorseController>();
    }
}
