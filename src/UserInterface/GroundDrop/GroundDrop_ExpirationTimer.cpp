#include "../StdAfx.h"
#include <vector>
#include <algorithm>
#include <optional>
#include <format>
#include <memory>
#include "IGroundDropBatchRenderer.h"
#include "IGroundDropExpirationTimer.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"

namespace UserInterface::GroundDrop
{
    namespace
    {
        struct TimerData
        {
            EterBase::EntityId virtualId;
            float remainingSeconds;
        };
    }

    /**
     * @brief Modul odliczajacy czas zycia przedmiotow lezacych na ziemi,
     * wspolpracujacy bezposrednio z IGroundDropBatchRenderer poprzez EventBus,
     * aby zachowac separacje (Zero-Conflict).
     */
    class GroundDropExpirationTimer : public IGroundDropExpirationTimer
    {
    public:
        GroundDropExpirationTimer() = default;
        ~GroundDropExpirationTimer() override = default;

        /**
         * @brief Inicjuje odliczanie dla nowo dodanego przedmiotu.
         */
        void SetExpirationTime(EterBase::EntityId virtualId, float durationSeconds) override
        {
            if (durationSeconds <= 0.0f)
            {
                EterBase::ModernLogger::Warning("GroundDropExpirationTimer: SetExpirationTime - ujemny/zerowy czas {} dla vid {}", durationSeconds, virtualId.value());
                return;
            }

            auto it = std::ranges::find_if(m_timers, [virtualId](const TimerData& t) {
                return t.virtualId == virtualId;
            });

            if (it != m_timers.end())
            {
                it->remainingSeconds = durationSeconds;
                EterBase::ModernLogger::Trace("GroundDropExpirationTimer: Aktualizacja czasu dla vid {} na {}s", virtualId.value(), durationSeconds);
            }
            else
            {
                m_timers.push_back({virtualId, durationSeconds});
                EterBase::ModernLogger::Trace("GroundDropExpirationTimer: Dodano odliczanie dla vid {} na {}s", virtualId.value(), durationSeconds);
            }
        }

        /**
         * @brief Usuwa timer wylacznie wtedy, gdy przedmiot zostanie np. podniesiony przez gracza.
         */
        EterBase::PacketResult<void> RemoveTimer(EterBase::EntityId virtualId) override
        {
            auto it = std::ranges::find_if(m_timers, [virtualId](const TimerData& t) {
                return t.virtualId == virtualId;
            });

            if (it == m_timers.end())
            {
                return EterBase::MakeError(EterBase::PacketError::UnknownOpcode); // Uzywamy opcji z PacketError, bo modul ten dziala jako warstwa posrednia dla operacji sieciowych
            }

            m_timers.erase(it);
            EterBase::ModernLogger::Trace("GroundDropExpirationTimer: Usunieto odliczanie dla vid {}", virtualId.value());
            return {};
        }

        /**
         * @brief Zmniejsza liczniki dla wszystkich przedmiotow. Uplyniecie emituje GroundDropExpiredEvent.
         */
        void Update(float deltaTime) override
        {
            if (deltaTime <= 0.0f || m_timers.empty())
                return;

            std::vector<EterBase::EntityId> expiredIds;

            // Krok 1: Aktualizacja pozostalego czasu i zbieranie id wygaslych timerow
            for (auto& t : m_timers)
            {
                t.remainingSeconds -= deltaTime;
                if (t.remainingSeconds <= 0.0f)
                {
                    expiredIds.push_back(t.virtualId);
                }
            }

            if (expiredIds.empty())
                return;

            // Krok 2: Usuniecie wygaslych timerow z wektora (bezpiecznie, bez mutacji z wewnatrz lambdy usuwajacej)
            auto newEnd = std::remove_if(m_timers.begin(), m_timers.end(), [](const TimerData& t) {
                return t.remainingSeconds <= 0.0f;
            });
            m_timers.erase(newEnd, m_timers.end());

            // Krok 3: Publikacja zdarzen
            for (auto id : expiredIds)
            {
                Core::EventBus::GetInstance().Publish(GroundDropExpiredEvent{id});
                EterBase::ModernLogger::Debug("GroundDropExpirationTimer: Przedmiot vid {} wygasl (czas uplynal).", id.value());
            }
        }

        void ClearAll() override
        {
            m_timers.clear();
            EterBase::ModernLogger::Info("GroundDropExpirationTimer: Wyczyszczono wszystkie timery.");
        }

    private:
        std::vector<TimerData> m_timers;
    };

    std::unique_ptr<IGroundDropExpirationTimer> CreateExpirationTimer()
    {
        return std::make_unique<GroundDropExpirationTimer>();
    }
}
