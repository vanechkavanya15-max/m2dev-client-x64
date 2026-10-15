#pragma once

#include "../Core/EngineForwardDecls.h"
#include "../Contracts/IActorProvider.h"
#include "../Contracts/INetworkService.h"
#include <cstdint>
#include <map>

namespace UserInterface::PlayerControllers
{
    /**
     * @brief Egzekutor umiejetnosci gracza - weryfikacja cooldownow, zasiegu, szarzy i wysylanie pakietow.
     * Zgodny z guardrailami: ZERO MUTEXES, NEVER CACHE POINTERS, INETWORKSERVICE.
     */
    class PlayerSkillExecutor
    {
    public:
        static constexpr size_t MAX_SKILL_COUNT = 255;

        struct SSkillSlotData
        {
            DWORD dwIndex{0};
            int iType{0};
            int iGrade{0};
            int iLevel{0};
            float fcurEfficientPercentage{0.0f};
            float fnextEfficientPercentage{0.0f};
            BOOL isCoolTime{FALSE};
            float fCoolTime{0.0f};
            float fLastUsedTime{0.0f};
            BOOL bActive{FALSE};
        };

    public:
        PlayerSkillExecutor(
            Contracts::IActorProvider* pActorProvider = nullptr,
            Contracts::INetworkService* pNetworkService = nullptr);
        ~PlayerSkillExecutor() = default;

        // Rejestracja dostawcow uslug
        void SetActorProvider(Contracts::IActorProvider* pActorProvider) { m_pActorProvider = pActorProvider; }
        void SetNetworkService(Contracts::INetworkService* pNetworkService) { m_pNetworkService = pNetworkService; }

        // Konfiguracja slotow
        void SetSkillSlot(DWORD dwSlotIndex, DWORD dwSkillIndex, int iGrade = 0, int iLevel = 0, float fEfficiency = 0.0f);
        const SSkillSlotData* GetSkillSlotData(DWORD dwSlotIndex) const;
        SSkillSlotData* GetSkillSlotData(DWORD dwSlotIndex);
        void SetCurrentSkillSlotIndex(DWORD dwSlotIndex) { m_dwcurSkillSlotIndex = dwSlotIndex; }
        DWORD GetCurrentSkillSlotIndex() const { return m_dwcurSkillSlotIndex; }

        // Cooldowny
        bool CheckRestSkillCoolTime(DWORD dwSkillSlotIndex) const;
        void RunCoolTime(DWORD dwSkillSlotIndex, float fBaseCoolTime, int iCastingSpeed = 100);
        float GetSkillCoolTime(DWORD dwSkillSlotIndex) const;
        float GetSkillElapsedCoolTime(DWORD dwSkillSlotIndex) const;
        void ResetSkillCoolTimes();
        void ResetSkillCoolTimeForSlot(DWORD dwSkillSlotIndex);

        // Zasieg czarow
        DWORD GetSkillTargetRange(DWORD dwSkillBaseRange, DWORD dwBowDistance = 0) const;
        bool CheckSkillTargetRange(DWORD dwSkillSlotIndex, DWORD dwTargetVID, DWORD dwSkillBaseRange) const;
        bool ProcessEnemySkillTargetRange(DWORD dwSkillSlotIndex, DWORD dwTargetVID, float fTargetDistance, DWORD dwSkillRange, bool bIsChargeSkill);

        // Wykonywanie umiejetnosci i szarza
        bool UseSkill(DWORD dwSkillSlotIndex, DWORD dwTargetVID = 0);
        void UseCurrentSkill();
        void UseChargeSkill(DWORD dwSkillSlotIndex);
        bool IsUsingChargeSkill() const;

        // Rezerwacja umiejetnosci
        void ReserveUseSkill(DWORD dwTargetVID, DWORD dwSkillSlotIndex, DWORD dwRange);
        bool IsReservedUseSkill(DWORD dwSkillSlotIndex) const;
        void ClearReservedSkill();
        DWORD GetReservedSkillSlotIndex() const { return m_dwSkillSlotIndexReserved; }
        DWORD GetReservedSkillTargetVID() const { return m_dwSkillTargetVIDReserved; }
        DWORD GetReservedSkillRange() const { return m_dwSkillRangeReserved; }

        // Walidacja uzywalnosci
        bool CheckSkillUsable(DWORD dwSkillSlotIndex) const;

        // Mapowanie AffectIndex -> SkillIndex
        void RegisterAffectSkillMap(DWORD dwAffectIndex, DWORD dwSkillIndex);
        bool AffectIndexToSkillIndex(DWORD dwAffectIndex, DWORD* pdwSkillIndex) const;
        void ClearAffectSkillMap();

        // Wysylanie pakietow przez INetworkService
        bool SendUseSkillPacket(DWORD dwSkillIndex, DWORD dwTargetVID);

        // Aktualizacja stanu i cooldownow klatki
        void Update();

    private:
        Contracts::IActorProvider* m_pActorProvider{nullptr};
        Contracts::INetworkService* m_pNetworkService{nullptr};

        DWORD m_dwcurSkillSlotIndex{0};
        DWORD m_dwSkillSlotIndexReserved{0};
        DWORD m_dwSkillRangeReserved{0};
        DWORD m_dwSkillTargetVIDReserved{0};
        bool m_isSkillReserved{false};

        std::map<DWORD, DWORD> m_kMap_dwAffectIndexToSkillIndex;
        SSkillSlotData m_aSkill[MAX_SKILL_COUNT];
    };
}
