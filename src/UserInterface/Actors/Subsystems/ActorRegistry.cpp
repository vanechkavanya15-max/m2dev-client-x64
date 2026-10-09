#include "StdAfx.h"
#include "ActorRegistry.h"
#include "InstanceBase.h"
#include "Core/EventBus.h"
#include "EterBase/LogModern.h"

namespace UserInterface::Actors::Subsystems
{
    ActorRegistry::ActorRegistry(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept
        : m_pEventSink(pEventSink)
    {
    }

    ActorRegistry::~ActorRegistry()
    {
        ClearAll();
    }

    bool ActorRegistry::RegisterActor(DWORD dwVID, CInstanceBase* pInst)
    {
        if (dwVID == 0)
        {
            EterBase::ModernLogger::Error("ActorRegistry::RegisterActor: Proba rejestracji nieprawidlowego VID = 0.");
            return false;
        }

        if (pInst == nullptr)
        {
            EterBase::ModernLogger::Error("ActorRegistry::RegisterActor: Proba rejestracji wskaznika nullptr dla VID: {}.", dwVID);
            return false;
        }

        auto it = m_actors.find(dwVID);
        if (it != m_actors.end())
        {
            if (it->second != pInst)
            {
                EterBase::ModernLogger::Warn("ActorRegistry::RegisterActor: Nadpisanie istniejacej instancji aktora dla VID: {}.", dwVID);
                it->second = nullptr; // Uniewaznienie starego wskaznika (Dangling pointer safety)
                it->second = pInst;
            }
            return true;
        }

        m_actors.emplace(dwVID, pInst);
        return true;
    }

    bool ActorRegistry::UnregisterActor(DWORD dwVID)
    {
        if (dwVID == 0)
            return false;

        auto it = m_actors.find(dwVID);
        if (it == m_actors.end())
            return false;

        // DANGLING POINTER SAFETY:
        // Zerujemy wskaznik w komorce mapy przed fizycznym usunieciem wpisu.
        // Dzieki temu jakiekolwiek zagniezdzone odpytania nie otrzymaja nieaktualnego adresu.
        it->second = nullptr;
        m_actors.erase(it);

        // Powiadomienie interfejsu kontraktowego IGameEventSink
        if (m_pEventSink != nullptr)
        {
            UserInterface::Contracts::ActorDeadEvent deadEvt{ dwVID };
            m_pEventSink->OnActorDead(deadEvt);
        }

        // Opcjonalny callback zgodnosci wstecznej
        if (m_deadCallback)
        {
            m_deadCallback(dwVID);
        }

        // Rozgloszenie zdarzenia w szynie zdarzen EventBus
        UserInterface::Core::EventBus::GetInstance().Publish(Core::ActorDeadEvent(dwVID));

        EterBase::ModernLogger::Debug("ActorRegistry::UnregisterActor: Pomyslnie wyrejestrowano aktora VID: {}.", dwVID);
        return true;
    }

    CInstanceBase* ActorRegistry::FindActor(DWORD dwVID) const noexcept
    {
        if (dwVID == 0)
            return nullptr;

        auto it = m_actors.find(dwVID);
        if (it == m_actors.end())
            return nullptr;

        // Wskaznik moze byc nullptr jesli zostal uniewazniony
        return it->second;
    }

    void ActorRegistry::ClearAll() noexcept
    {
        for (auto& [vid, pActor] : m_actors)
        {
            pActor = nullptr; // Zerowanie wszystkich wskaznikow przed zwolnieniem pamieci mapy
        }
        m_actors.clear();
    }

    bool ActorRegistry::ContainsActor(DWORD dwVID) const noexcept
    {
        if (dwVID == 0)
            return false;

        auto it = m_actors.find(dwVID);
        return (it != m_actors.end() && it->second != nullptr);
    }

    size_t ActorRegistry::GetActorCount() const noexcept
    {
        return m_actors.size();
    }

    bool ActorRegistry::IsEmpty() const noexcept
    {
        return m_actors.empty();
    }

    void ActorRegistry::SetEventSink(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept
    {
        m_pEventSink = pEventSink;
    }

    UserInterface::Contracts::IGameEventSink* ActorRegistry::GetEventSink() const noexcept
    {
        return m_pEventSink;
    }

    void ActorRegistry::SetDeadCallback(DeadCallback callback) noexcept
    {
        m_deadCallback = std::move(callback);
    }

    const ActorRegistry::ActorMap& ActorRegistry::GetAllActors() const noexcept
    {
        return m_actors;
    }
}
