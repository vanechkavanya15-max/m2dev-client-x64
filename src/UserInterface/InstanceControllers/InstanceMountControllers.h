#pragma once

#include <cstdint>
#include <string_view>
#include <memory>
#include <expected>

#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "../Core/EventBus.h"
#include "InstanceAnimationControllers.h"

namespace UserInterface::Core
{
    struct MountCombatStateEvent : public IEvent
    {
        float attackRange;
        float rotationAngle;

        MountCombatStateEvent(float attackRange, float rotationAngle)
            : attackRange(attackRange), rotationAngle(rotationAngle) {}
    };
}

namespace UserInterface::InstanceControllers
{
    // --- Zdarzenia wierzchowca ---

    struct MountAnimSyncedEvent : public Core::IEvent
    {
        EterBase::EntityId riderId;
        uint32_t motionKey;

        MountAnimSyncedEvent(EterBase::EntityId rider, uint32_t motion)
            : riderId(rider), motionKey(motion) {}
    };

    struct RiderTransformSyncEvent : public Core::IEvent
    {
        EterBase::EntityId mountVid;
        float x, y, z, rot;

        RiderTransformSyncEvent(EterBase::EntityId id, float x, float y, float z, float rot)
            : mountVid(id), x(x), y(y), z(z), rot(rot) {}
    };

    /**
     * @brief Glowny interfejs kontrolera wierzchowca i konia dla instancji postaci.
     */
    class IInstanceMountHorseController
    {
    public:
        virtual ~IInstanceMountHorseController() = default;

        virtual EterBase::PacketResult<void> Mount(EterBase::EntityId mountVid, uint32_t mountVnum) = 0;
        virtual EterBase::PacketResult<void> Dismount() = 0;
        virtual bool IsMounted() const = 0;
        virtual EterBase::EntityId GetMountVID() const = 0;
        virtual uint32_t GetMountVnum() const = 0;
        virtual void UpdateMountTransform(float x, float y, float z, float rot) = 0;
        virtual void Clear() = 0;
    };

    /**
     * @brief Kontroler wsiadania i zsiadania z plynna interpolacja wysokosci siodla.
     */
    class InstanceMount_Board final : public IInstanceMountHorseController
    {
    public:
        InstanceMount_Board() = default;
        ~InstanceMount_Board() override = default;

        EterBase::PacketResult<void> Mount(EterBase::EntityId mountVid, uint32_t mountVnum) override;
        EterBase::PacketResult<void> Dismount() override;
        bool IsMounted() const override;
        EterBase::EntityId GetMountVID() const override;
        uint32_t GetMountVnum() const override;
        void UpdateMountTransform(float x, float y, float z, float rot) override;
        void Clear() override;

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

    /**
     * @brief Standardowy kontroler stanu wierzchowca dla instancji.
     */
    class InstanceMountHorseController : public IInstanceMountHorseController
    {
    public:
        ~InstanceMountHorseController() override = default;

        EterBase::PacketResult<void> Mount(EterBase::EntityId mountVid, uint32_t mountVnum) override;
        EterBase::PacketResult<void> Dismount() override;
        bool IsMounted() const override;
        EterBase::EntityId GetMountVID() const override;
        uint32_t GetMountVnum() const override;
        void UpdateMountTransform(float x, float y, float z, float rot) override;
        void Clear() override;

    private:
        bool m_isMounted{false};
        EterBase::EntityId m_mountVid{0};
        uint32_t m_mountVnum{0};
        float m_x{0.0f};
        float m_y{0.0f};
        float m_z{0.0f};
        float m_rot{0.0f};
    };

    /**
     * @brief Kontroler kalkulacji parametrów bojowych z siodla (zasieg ataku, kat obrotu).
     */
    class InstanceMountCombatController : public IInstanceMountHorseController
    {
    public:
        InstanceMountCombatController() = default;
        ~InstanceMountCombatController() override = default;

        EterBase::PacketResult<void> Mount(EterBase::EntityId mountVid, uint32_t mountVnum) override;
        EterBase::PacketResult<void> Dismount() override;
        bool IsMounted() const override;
        EterBase::EntityId GetMountVID() const override;
        uint32_t GetMountVnum() const override;
        void UpdateMountTransform(float x, float y, float z, float rot) override;
        void Clear() override;

    private:
        bool isMounted{false};
        EterBase::EntityId currentMountVid{0};
        uint32_t currentMountVnum{0};

        std::expected<float, std::string_view> CalculateCombatAttackRange(float x, float y, float z) const;
        std::expected<float, std::string_view> CalculateCombatRotation(float rot) const;
    };

    /**
     * @brief Kontroler synchronizacji polozenia i orientacji jezdzca wzgledem szkieletu konia.
     */
    class InstanceMountSyncController final : public IInstanceMountHorseController
    {
    public:
        InstanceMountSyncController();
        ~InstanceMountSyncController() override = default;

        EterBase::PacketResult<void> Mount(EterBase::EntityId mountVid, uint32_t mountVnum) override;
        EterBase::PacketResult<void> Dismount() override;
        bool IsMounted() const override;
        EterBase::EntityId GetMountVID() const override;
        uint32_t GetMountVnum() const override;
        void UpdateMountTransform(float x, float y, float z, float rot) override;
        void Clear() override;

    private:
        bool m_isMounted{false};
        EterBase::EntityId m_mountVid{0};
        EterBase::ItemVnum m_mountVnum{0};
        float m_x{0.0f};
        float m_y{0.0f};
        float m_z{0.0f};
        float m_rot{0.0f};
    };

    /**
     * @brief Klasa realizujaca wspolbiezne odtwarzanie animacji konia i ruchow tulowia jezdzca.
     */
    class MountAnimLinker
    {
    public:
        static EterBase::PacketResult<void> SyncAnimations(
            const IInstanceMountHorseController& mountController,
            IInstanceAnimationController& horseAnimCtrl,
            IInstanceAnimationController& riderAnimCtrl,
            uint32_t motionKey,
            float speedMultiplier = 1.0f);
    };

    // --- Fabryki ---
    std::unique_ptr<IInstanceMountHorseController> CreateMountBoardController();
    std::unique_ptr<IInstanceMountHorseController> CreateMountHorseController();
}
