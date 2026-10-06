#include "../StdAfx.h"
#include "ICharacterAffectService.h"
#include "../PythonCharacterManager.h"
#include "../PythonPlayer.h"
#include "../InstanceBase.h"
#include "../Packet.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../Core/EventBus.h"

#include <mutex>
#include <expected>
#include <format>

namespace UserInterface::Actors
{
    /**
     * @brief Zdarzenie publikowane po wyczyszczeniu afektow konkretnego aktora.
     */
    struct CharacterAffectsClearedEvent : public Core::IEvent
    {
        EterBase::EntityId targetId;

        explicit CharacterAffectsClearedEvent(EterBase::EntityId id) : targetId(id) {}
    };

    /**
     * @brief Zdarzenie publikowane po wyczyszczeniu afektow wszystkich aktorow na mapie.
     */
    struct AllCharactersAffectsClearedEvent : public Core::IEvent
    {
        AllCharactersAffectsClearedEvent() = default;
    };

    /**
     * @brief Implementacja serwisu afektow obslugujaca proces ich pelnego czyszczenia (ClearAll).
     *
     * Klasa zaimplementowana w duchu Single Responsibility Principle. Obsluguje
     * jedynie czyszczenie stanow afektow i powiadamia przez system zdarzen.
     */
    class CharacterAffectService final : public ICharacterAffectService
    {
    public:
        CharacterAffectService() = default;
        ~CharacterAffectService() override = default;

        /**
         * @brief Zaslepka dla SetAffect (SRP: ten plik obsluguje wylacznie ClearAll).
         */
        void SetAffect(EterBase::EntityId id, uint32_t affectIndex, bool enabled) override
        {
            // Ignorowane - implementacja w innym module zgodnie z zasada Zero-Conflict.
        }

        /**
         * @brief Zaslepka dla HasAffect.
         */
        bool HasAffect(EterBase::EntityId id, uint32_t affectIndex) const override
        {
            return false;
        }

        /**
         * @brief Czysci wszystkie afekty dla konkretnego aktora.
         *
         * @param id Identyfikator (VID) aktora.
         */
        void ClearAffects(EterBase::EntityId id) override
        {
            // Czyszczenie wizualnych efektow aktora
            CInstanceBase* pInstance = CPythonCharacterManager::Instance().GetInstancePtr(id.value());
            if (pInstance)
            {
                pInstance->__ClearAffects();
                pInstance->__ClearAffectFlagContainer();
                EterBase::ModernLogger::Debug("Cleared visual affects for entity [{}]", id.value());
            }

            // Sprawdzanie czy podany identyfikator nalezy do lokalnego gracza
            CInstanceBase* pMainInstance = CPythonCharacterManager::Instance().GetMainInstancePtr();
            if (pMainInstance && pMainInstance->GetVirtualID() == id.value())
            {
                CPythonPlayer::Instance().ClearAffects();
                EterBase::ModernLogger::Debug("Cleared logical affects for local player [{}]", id.value());
            }

            // Publikacja zdarzenia domenowego do GUI (decoupling)
            Core::EventBus::GetInstance().Publish(CharacterAffectsClearedEvent{id});
            EterBase::ModernLogger::Info("Affects cleared for entity [{}] and event published.", id.value());
        }

        /**
         * @brief Czysci wszystkie afekty dla wszystkich aktorow.
         */
        void Clear() override
        {
            // Czyszczenie wizualnych efektow dla wszystkich aktorow
            auto it = CPythonCharacterManager::Instance().CharacterInstanceBegin();
            auto end = CPythonCharacterManager::Instance().CharacterInstanceEnd();

            size_t clearedCount = 0;
            for (; it != end; ++it)
            {
                CInstanceBase* pInstance = *it;
                if (pInstance)
                {
                    pInstance->__ClearAffects();
                    pInstance->__ClearAffectFlagContainer();
                    ++clearedCount;
                }
            }

            // Czyszczenie stanow dla lokalnego gracza
            CPythonPlayer::Instance().ClearAffects();

            // Publikacja zdarzenia domenowego
            Core::EventBus::GetInstance().Publish(AllCharactersAffectsClearedEvent{});
            
            EterBase::ModernLogger::Info("All affects cleared for {} entities.", clearedCount);
        }
    };
}
