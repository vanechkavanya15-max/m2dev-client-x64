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

    struct ShopEvent
    {
        uint8_t bySubHeader;
    };

    struct ShopSignEvent
    {
        VID dwVID;
        std::string szSign;
    };

    struct ExchangeEvent
    {
        uint8_t bySubHeader;
        bool bIsMe;
        uint32_t dwArg1;
        uint32_t dwArg2;
        uint32_t dwArg3;
    };

    struct QuestInfoEvent
    {
        uint16_t wIndex;
        uint8_t byFlag;
    };

    struct QuestConfirmEvent
    {
        std::string szMsg;
        int32_t lTimeout;
        uint32_t dwRequestPID;
    };

    struct PartyAddEvent
    {
        uint32_t dwPID;
        std::string szName;
    };

    struct PartyUpdateEvent
    {
        uint32_t dwPID;
        uint8_t byState;
        uint8_t byPercentHP;
    };

    struct PartyRemoveEvent
    {
        uint32_t dwPID;
    };

    struct GuildEvent
    {
        uint8_t bySubHeader;
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

        virtual void OnShop(const ShopEvent& event) {}
        virtual void OnShopSign(const ShopSignEvent& event) {}
        virtual void OnExchange(const ExchangeEvent& event) {}
        virtual void OnQuestInfo(const QuestInfoEvent& event) {}
        virtual void OnQuestConfirm(const QuestConfirmEvent& event) {}
        virtual void OnPartyAdd(const PartyAddEvent& event) {}
        virtual void OnPartyUpdate(const PartyUpdateEvent& event) {}
        virtual void OnPartyRemove(const PartyRemoveEvent& event) {}
        virtual void OnGuild(const GuildEvent& event) {}
    };
}
