#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <unordered_map>
#include <mutex>
#include <expected>
#include <cstdint>
#include <optional>

namespace Client::Bridge {

// Reprezentacja metryk dla pojedynczego zdarzenia Pythona (np. wywolania funkcji).
// Przechowuje czas trwania i liczbe wywolan w danej klatce.
struct PyEventMetrics {
    std::chrono::microseconds total_duration{0};
    uint64_t call_count{0};

    constexpr void AddMeasurement(std::chrono::microseconds duration) noexcept {
        total_duration += duration;
        ++call_count;
    }

    constexpr void Reset() noexcept {
        total_duration = std::chrono::microseconds{0};
        call_count = 0;
    }
};

// Bledy, ktore moga wystapic podczas pomiaru czasu.
enum class TelemetryError {
    EventAlreadyActive,
    NoActiveEvent,
    InvalidEventName
};

// Klasa telemetryczna zbierajaca dane o czasach wykonania skryptow Pythona.
// Zapewnia thread-safety uzywajac std::mutex.
class PyEventTelemetry {
public:
    static PyEventTelemetry& Instance() {
        static PyEventTelemetry instance;
        return instance;
    }

    // Rozpoczyna pomiar czasu dla danego zdarzenia. Zwraca std::expected.
    std::expected<void, TelemetryError> BeginEvent(std::string_view event_name) {
        if (event_name.empty()) {
            return std::unexpected(TelemetryError::InvalidEventName);
        }

        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_active_event_name.has_value()) {
            return std::unexpected(TelemetryError::EventAlreadyActive);
        }

        m_active_event_name = std::string(event_name);
        m_event_start_time = std::chrono::steady_clock::now();

        return {};
    }

    // Konczy pomiar czasu dla aktualnie mierzonego zdarzenia.
    std::expected<std::chrono::microseconds, TelemetryError> EndEvent() {
        auto end_time = std::chrono::steady_clock::now();

        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_active_event_name.has_value()) {
            return std::unexpected(TelemetryError::NoActiveEvent);
        }

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - m_event_start_time);
        m_metrics[m_active_event_name.value()].AddMeasurement(duration);

        m_active_event_name.reset();

        return duration;
    }

    // Pobiera statystyki z danej klatki.
    std::unordered_map<std::string, PyEventMetrics> GetMetrics() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_metrics;
    }

    // Resetuje statystyki, powinnoby byc wywolane po kazdej klatce.
    void ResetFrameMetrics() {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& [name, metrics] : m_metrics) {
            metrics.Reset();
        }
    }

    // Dodatkowa metoda na wymuszenie resetu calej klasy. 
    // Przydatne w testach.
    void ForceResetAllForTesting() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_metrics.clear();
        m_active_event_name.reset();
    }

    // Sprawdza czy jakies zdarzenie jest wlasnie mierzone.
    bool IsEventActive() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_active_event_name.has_value();
    }
    
    // Zwraca nazwe aktualnego zdarzenia
    std::optional<std::string> GetActiveEventName() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_active_event_name;
    }


private:
    PyEventTelemetry() = default;
    ~PyEventTelemetry() = default;

    // Usuwamy mozliwosc kopiowania
    PyEventTelemetry(const PyEventTelemetry&) = delete;
    PyEventTelemetry& operator=(const PyEventTelemetry&) = delete;

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, PyEventMetrics> m_metrics;
    std::optional<std::string> m_active_event_name;
    std::chrono::steady_clock::time_point m_event_start_time;
};

// RAII straznik sluzacy do latwego i bezpiecznego pomiaru czasu wykonania bloku kodu.
class ScopedEventTimer {
public:
    explicit ScopedEventTimer(std::string_view event_name) {
        auto result = PyEventTelemetry::Instance().BeginEvent(event_name);
        m_started = result.has_value();
    }

    ~ScopedEventTimer() {
        if (m_started) {
            // Ignorujemy blad w destruktorze.
            (void)PyEventTelemetry::Instance().EndEvent();
        }
    }

    // Blokujemy kopiowanie
    ScopedEventTimer(const ScopedEventTimer&) = delete;
    ScopedEventTimer& operator=(const ScopedEventTimer&) = delete;

private:
    bool m_started{false};
};

} // namespace Client::Bridge
