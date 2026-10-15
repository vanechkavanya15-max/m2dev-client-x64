#include "../StdAfx.h"
#include "ICharacterAffectService.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/LogModern.h"
#include "../PythonCharacterManager.h"
#include "../InstanceBase.h"
#include "../Core/EventBus.h"

namespace UserInterface::Actors
{
    /**
     * @brief Zdarzenie publikowane na magistrali po zmianie stanu ogluszenia.
     * Dziedziczy po IEvent zgodnie z zeroconflict i C++23.
     */
    struct StunAffectChangedEvent : public UserInterface::Core::IEvent
    {
        uint32_t targetId;
        bool isStunned;

        explicit StunAffectChangedEvent(uint32_t targetId, bool isStunned)
            : targetId(targetId), isStunned(isStunned)
        {}
    };

    /**
     * @brief Serwis do zarzadzania afektem Ogluszenia postaci (Stun).
     * Odpowiada za gwiazdki nad glowa i blokade ruchu.
     */
    class AffectService_Stun final : public ICharacterAffectService
    {
    public:
        AffectService_Stun() = default;
        ~AffectService_Stun() override = default;

        /**
         * @brief Ustawia stan ogluszenia dla danego EntityId.
         * 
         * @param id Identyfikator postaci (VID).
         * @param affectIndex Indeks buffa/debuffa (oczekiwany AFFECT_STUN lub NEW_AFFECT_STUN).
         * @param enabled Czy ogluszenie ma byc wlaczone czy wylaczone.
         */
        void SetAffect(EterBase::EntityId id, uint32_t affectIndex, bool enabled) override
        {
            if (affectIndex != CInstanceBase::AFFECT_STUN && affectIndex != CInstanceBase::NEW_AFFECT_STUN)
            {
                EterBase::ModernLogger::Debug("AffectService_Stun: Ignorowanie affectIndex {}, poniewaz to nie jest STUN.", affectIndex);
                return;
            }

            auto* pkInst = CPythonCharacterManager::Instance().GetInstancePtr(id.value());
            if (!pkInst)
            {
                EterBase::ModernLogger::Warn("AffectService_Stun: Nie znaleziono instancji postaci o VID {}.", id.value());
                return;
            }

            // Aplikowanie statusu w CInstanceBase
            pkInst->SCRIPT_SetAffect(affectIndex, enabled);

            // Aktywacja blokady ruchu oraz efektu wizualnego (gwiazdki) poprzez SetSleep
            auto* graphicInst = pkInst->GetGraphicThingInstancePtr();
            if (graphicInst)
            {
                graphicInst->SetSleep(enabled);
            }

            // Rozgloszenie zdarzenia domenowego do GUI bez bezposredniego powiazania UI
            UserInterface::Core::EventBus::GetInstance().Publish(StunAffectChangedEvent{id.value(), enabled});

            EterBase::ModernLogger::Info("AffectService_Stun: Stan ogluszenia zmieniony dla VID {}, stan: {}", id.value(), enabled);
        }

        /**
         * @brief Sprawdza czy dany aktor ma aktywny afekt ogluszenia.
         * 
         * @param id Identyfikator postaci (VID).
         * @param affectIndex Indeks buffa/debuffa.
         * @return true jesli posiada, false w przeciwnym wypadku.
         */
        bool HasAffect(EterBase::EntityId id, uint32_t affectIndex) const override
        {
            if (affectIndex != CInstanceBase::AFFECT_STUN && affectIndex != CInstanceBase::NEW_AFFECT_STUN)
            {
                return false;
            }

            auto* pkInst = CPythonCharacterManager::Instance().GetInstancePtr(id.value());
            if (!pkInst)
            {
                return false;
            }

            return pkInst->IsAffect(affectIndex);
        }

        /**
         * @brief Cysci afekty ogluszenia dla zadanego aktora.
         * 
         * @param id Identyfikator postaci (VID).
         */
        void ClearAffects(EterBase::EntityId id) override
        {
            auto* pkInst = CPythonCharacterManager::Instance().GetInstancePtr(id.value());
            if (!pkInst)
            {
                return;
            }

            // Wylaczamy oba znane stuny
            pkInst->SCRIPT_SetAffect(CInstanceBase::AFFECT_STUN, false);
            pkInst->SCRIPT_SetAffect(CInstanceBase::NEW_AFFECT_STUN, false);

            auto* graphicInst = pkInst->GetGraphicThingInstancePtr();
            if (graphicInst)
            {
                graphicInst->SetSleep(false);
            }

            UserInterface::Core::EventBus::GetInstance().Publish(StunAffectChangedEvent{id.value(), false});
            EterBase::ModernLogger::Info("AffectService_Stun: Usunieto wszystkie ogluszenia dla VID {}.", id.value());
        }

        /**
         * @brief Czysci globalny stan serwisu.
         */
        void Clear() override
        {
            // Poniewaz ten serwis polega na CInstanceBase dla stanu aktorow,
            // nie posiada wlasnych list w pamieci do czyszczenia.
            EterBase::ModernLogger::Info("AffectService_Stun: Wywolano Clear(). Zewnetrzne stany zostana wyczyszczone przez CPythonCharacterManager.");
        }
    };
}
