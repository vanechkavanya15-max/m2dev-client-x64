#pragma once

#include <cstdint>
#include <string>

namespace UserInterface::Contracts
{
    using VID = uint32_t;
    using ItemID = uint32_t;

    struct ItemReceivedEvent
    {
        uint32_t dwCell;
        uint32_t dwVnum;
        uint32_t dwCount;
    };

    struct AttackExecutedEvent
    {
        VID dwAttackerVID;
        VID dwVictimVID;
        uint8_t byMotionType;
    };

    struct DamageInfoEvent
    {
        VID dwVictimVID;
        VID dwAttackerVID;
        uint32_t dwDamage;
        uint8_t byDamageFlag;
    };

    struct ActorDeadEvent
    {
        VID dwVID;
    };

    struct ActorMovedEvent
    {
        VID dwVID;
        int32_t lX;
        int32_t lY;
        float fRot;
        uint32_t dwTime;
    };

    struct TargetChangedEvent
    {
        VID dwVID;
    };

    struct SkillCooldownStartedEvent
    {
        uint32_t dwSkillIndex;
        float fDuration;
    };

    struct StaminaChangedEvent
    {
        float fCurrentStamina;
        float fMaxStamina;
    };

    class IGameEventSink
    {
    public:
        virtual ~IGameEventSink() = default;

        virtual void OnItemReceived(const ItemReceivedEvent& event) {}
        virtual void OnAttackExecuted(const AttackExecutedEvent& event) {}
        virtual void OnDamageInfo(const DamageInfoEvent& event) {}
        virtual void OnActorDead(const ActorDeadEvent& event) {}
        virtual void OnActorMoved(const ActorMovedEvent& event) {}
        virtual void OnTargetChanged(const TargetChangedEvent& event) {}
        virtual void OnSkillCooldownStarted(const SkillCooldownStartedEvent& event) {}
        virtual void OnStaminaChanged(const StaminaChangedEvent& event) {}
    };
}
