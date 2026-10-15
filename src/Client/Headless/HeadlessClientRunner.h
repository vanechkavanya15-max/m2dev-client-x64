#pragma once

#include <cstdint>
#include <expected>
#include <string_view>
#include <optional>
#include <functional>
#include <utility>

namespace Client::Headless {

/**
 * @brief Enumy bledow dla HeadlessClientRunner.
 */
enum class RunnerError : uint8_t {
    None = 0,
    AlreadyInitialized,
    NotInitialized,
    RunFailed,
    TickFailed
};

/**
 * @brief Szablon Result oparty na std::expected dla Runnera.
 */
template <typename T>
using Result = std::expected<T, RunnerError>;

/**
 * @brief Funkcja zwracajaca czytelny opis bledu.
 */
[[nodiscard]] constexpr std::string_view ToString(RunnerError err) noexcept {
    switch (err) {
        case RunnerError::None: return "RunnerError::None";
        case RunnerError::AlreadyInitialized: return "RunnerError::AlreadyInitialized - Runner jest juz zainicjalizowany";
        case RunnerError::NotInitialized: return "RunnerError::NotInitialized - Runner nie zostal zainicjalizowany";
        case RunnerError::RunFailed: return "RunnerError::RunFailed - Blad podczas wykonywania petli";
        case RunnerError::TickFailed: return "RunnerError::TickFailed - Blad w trakcie pojedynczego ticku";
        default: return "RunnerError::Unknown";
    }
}

/**
 * @brief Konfiguracja dla HeadlessClientRunner.
 */
struct RunnerConfig {
    uint32_t targetFps = 60; //!< Docelowa liczba klatek na sekunde
    std::optional<uint64_t> maxTicks = std::nullopt; //!< Maksymalna liczba tickow przed zakonczeniem
    bool exitOnError = true; //!< Czy przerwac dzialanie w przypadku bledu
};

/**
 * @brief Zarzadca petli glownej gry w trybie Headless.
 * Wykonuje ticki bez potrzeby inicjalizacji okna Win32.
 */
class HeadlessClientRunner final {
public:
    HeadlessClientRunner() noexcept = default;
    ~HeadlessClientRunner() = default;

    // Usuniecie konstruktora kopiujacego i przypisania
    HeadlessClientRunner(const HeadlessClientRunner&) = delete;
    HeadlessClientRunner& operator=(const HeadlessClientRunner&) = delete;

    // Przenoszenie dozwolone
    HeadlessClientRunner(HeadlessClientRunner&&) noexcept = default;
    HeadlessClientRunner& operator=(HeadlessClientRunner&&) noexcept = default;

    /**
     * @brief Inicjalizuje runnera podana konfiguracja.
     * @param config Konfiguracja dla runnera.
     * @return Result<void> Zwraca sukces lub blad inicjalizacji.
     */
    Result<void> Initialize(RunnerConfig config) noexcept {
        if (m_initialized) {
            return std::unexpected(RunnerError::AlreadyInitialized);
        }
        m_config = std::move(config);
        m_initialized = true;
        m_exitRequested = false;
        m_currentTick = 0;
        return {};
    }

    /**
     * @brief Uruchamia glowna petle gry az do otrzymania zadania wyjscia lub osiagniecia maxTicks.
     * @return Result<void> Zwraca sukces lub blad petli.
     */
    Result<void> Run() noexcept {
        if (!m_initialized) {
            return std::unexpected(RunnerError::NotInitialized);
        }

        while (!m_exitRequested) {
            if (m_config.maxTicks.has_value() && m_currentTick >= m_config.maxTicks.value()) {
                break;
            }

            auto tickResult = Tick();
            if (!tickResult.has_value()) {
                if (m_config.exitOnError) {
                    return std::unexpected(tickResult.error());
                }
            }

            m_currentTick++;
            if (m_exitRequested) {
                break;
            }
        }

        return {};
    }

    /**
     * @brief Zglasza prosbe o zakonczenie petli w nastepnym ticku.
     */
    void RequestExit() noexcept {
        m_exitRequested = true;
    }

    /**
     * @brief Zwraca aktualna liczbe wykonanych tickow.
     */
    [[nodiscard]] uint64_t GetCurrentTick() const noexcept {
        return m_currentTick;
    }

    /**
     * @brief Rejestruje funkcje wywolywana podczas kazdego ticku.
     */
    void SetTickCallback(std::function<Result<void>()> callback) noexcept {
        m_tickCallback = std::move(callback);
    }

private:
    /**
     * @brief Wykonuje pojedynczy tick w petli.
     * @return Result<void> Sukces lub blad wykonania ticku.
     */
    Result<void> Tick() noexcept {
        if (m_tickCallback) {
            return m_tickCallback();
        }
        return {};
    }

    RunnerConfig m_config{};
    bool m_initialized = false;
    bool m_exitRequested = false;
    uint64_t m_currentTick = 0;
    std::function<Result<void>()> m_tickCallback = nullptr;
};

} // namespace Client::Headless
