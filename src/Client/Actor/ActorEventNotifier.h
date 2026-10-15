#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>
#include <expected>
#include <algorithm>
#include <atomic>
#include <optional>

namespace Client::Actor {

    // Kody bledow dla notyfikatora zdarzen
    enum class NotifierError {
        InvalidCallback,
        SubscriptionNotFound
    };

    // Podstawowe typy zdarzen dla aktorow (zmiany parametrow)
    enum class ActorEventType {
        HealthChanged,
        ManaChanged,
        PositionChanged,
        StateChanged,
        LevelUp
    };

    // Typ identyfikatora subskrypcji
    using SubscriptionId = uint64_t;

    // Struktura reprezentujaca zdarzenie aktora
    struct ActorEvent {
        ActorEventType type;
        uint32_t actorId;
        int64_t value1;
        int64_t value2;
    };

    // Typ callbacku (delegata)
    using EventCallback = std::function<void(const ActorEvent&)>;

    // Klasa zarzadzajaca powiadomieniami dla aktorow (Event Bus / Observer)
    // Bezpieczna dla watkow, zero-conflict z istniejacym kodem
    class ActorEventNotifier {
    public:
        ActorEventNotifier() noexcept = default;
        ~ActorEventNotifier() noexcept = default;

        // Wylaczenie kopiowania i przenoszenia (zapewnia bezpieczenstwo dla lockow)
        ActorEventNotifier(const ActorEventNotifier&) = delete;
        ActorEventNotifier& operator=(const ActorEventNotifier&) = delete;
        ActorEventNotifier(ActorEventNotifier&&) = delete;
        ActorEventNotifier& operator=(ActorEventNotifier&&) = delete;

        // Rejestracja subskrypcji. Zwraca unikalne ID (SubscriptionId)
        std::expected<SubscriptionId, NotifierError> Subscribe(ActorEventType type, EventCallback callback) noexcept {
            if (!callback) {
                return std::unexpected(NotifierError::InvalidCallback);
            }

            SubscriptionId newId = ++m_nextSubscriptionId;
            
            std::lock_guard<std::mutex> lock(m_mutex);
            m_subscriptions.push_back({newId, type, std::move(callback)});
            
            return newId;
        }

        // Usuniecie subskrypcji uzywajac wczesniej otrzymanego ID
        std::expected<void, NotifierError> Unsubscribe(SubscriptionId id) noexcept {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = std::find_if(m_subscriptions.begin(), m_subscriptions.end(),
                [id](const SubscriptionRecord& rec) { return rec.id == id; });

            if (it == m_subscriptions.end()) {
                return std::unexpected(NotifierError::SubscriptionNotFound);
            }

            m_subscriptions.erase(it);
            return {};
        }

        // Publikacja zdarzenia do wszystkich subskrybentow nasluchujacych danego typu
        void Notify(const ActorEvent& event) noexcept {
            std::vector<EventCallback> callbacksToInvoke;
            
            // Minimalizujemy czas trzymania locka poprzez kopiowanie samych callbackow
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                for (const auto& sub : m_subscriptions) {
                    if (sub.eventType == event.type) {
                        callbacksToInvoke.push_back(sub.callback);
                    }
                }
            }

            // Wywolanie callbackow na zewnatrz sekcji krytycznej chroni przed zakleszczeniami,
            // jesli callback sam chcialby modyfikowac subskrypcje (np. Unsubscribe)
            for (const auto& cb : callbacksToInvoke) {
                if (cb) {
                    cb(event);
                }
            }
        }

        // Czyszczenie wszystkich subskrypcji
        void Clear() noexcept {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_subscriptions.clear();
        }

        // Pobranie liczby aktywnych subskrypcji
        [[nodiscard]] size_t GetSubscriptionCount() const noexcept {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_subscriptions.size();
        }

    private:
        struct SubscriptionRecord {
            SubscriptionId id;
            ActorEventType eventType;
            EventCallback callback;
        };

        mutable std::mutex m_mutex;
        std::vector<SubscriptionRecord> m_subscriptions;
        std::atomic<SubscriptionId> m_nextSubscriptionId{0};
    };

} // namespace Client::Actor
