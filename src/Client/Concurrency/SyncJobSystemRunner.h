#pragma once

#include <functional>
#include <queue>
#include <expected>
#include <string_view>
#include <cstdint>
#include <cstddef>

namespace Client::Concurrency {

/**
 * @enum JobSystemError
 * @brief Kody bledow dla operacji SyncJobSystemRunner.
 */
enum class JobSystemError : uint8_t {
    None = 0,
    QueueEmpty,
    ExecutionFailed
};

/**
 * @brief Konwertuje kod bledu na tekst.
 */
[[nodiscard]] constexpr std::string_view ToString(JobSystemError err) noexcept {
    switch (err) {
        case JobSystemError::None: return "None";
        case JobSystemError::QueueEmpty: return "QueueEmpty";
        case JobSystemError::ExecutionFailed: return "ExecutionFailed";
    }
    return "UnknownJobSystemError";
}

/**
 * @brief Typ zwracany dla operacji SyncJobSystemRunner oparty na std::expected.
 */
template <typename T = void>
using JobResult = std::expected<T, JobSystemError>;

/**
 * @class SyncJobSystemRunner
 * @brief Synchroniczny wykonawca zadan umozliwiajacy powtarzalne testy bez wyscigow.
 *
 * Komponent sluzy do deterministycznego wykonywania zadan w glownym watku,
 * co pozwala na izolowane testowanie logiki gry bez narzutu wielowatkowosci
 * oraz asynchronicznych pul zadan. Wykorzystuje C++23 std::expected dla obslugi bledow.
 */
class SyncJobSystemRunner {
public:
    using JobType = std::function<void()>;

    SyncJobSystemRunner() = default;
    ~SyncJobSystemRunner() = default;

    // Komponent nie jest kopiowalny
    SyncJobSystemRunner(const SyncJobSystemRunner&) = delete;
    SyncJobSystemRunner& operator=(const SyncJobSystemRunner&) = delete;

    // Komponent jest przenosny
    SyncJobSystemRunner(SyncJobSystemRunner&&) noexcept = default;
    SyncJobSystemRunner& operator=(SyncJobSystemRunner&&) noexcept = default;

    /**
     * @brief Dodaje zadanie do kolejki do pozniejszego wykonania.
     * @param job Funkcja do wykonania. Puste zadania (np. nullptr w std::function) 
     *            sa ignorowane.
     */
    void Enqueue(JobType job) {
        if (job) {
            m_jobs.push(std::move(job));
        }
    }

    /**
     * @brief Wykonuje wszystkie zadania znajdujace sie obecnie w kolejce w kolejnosci FIFO.
     * @return JobResult<size_t> Liczba pomyslnie wykonanych zadan lub blad w przypadku wyjatku.
     *         W przypadku wyjatku podczas wykonywania zadania, przerywa wykonywanie 
     *         kolejnych zadan.
     */
    JobResult<size_t> ExecuteAll() noexcept {
        size_t executedCount = 0;
        while (!m_jobs.empty()) {
            auto job = std::move(m_jobs.front());
            m_jobs.pop();
            try {
                if (job) {
                    job();
                    executedCount++;
                }
            } catch (...) {
                return std::unexpected(JobSystemError::ExecutionFailed);
            }
        }
        return executedCount;
    }

    /**
     * @brief Wykonuje pojedyncze zadanie z poczatku kolejki (FIFO).
     * @return JobResult<void> Sukces jesli zadanie zostalo wykonane, lub blad 
     *         jesli kolejka byla pusta badz wystapil wyjatek.
     */
    JobResult<void> ExecuteNext() noexcept {
        if (m_jobs.empty()) {
            return std::unexpected(JobSystemError::QueueEmpty);
        }
        
        auto job = std::move(m_jobs.front());
        m_jobs.pop();
        
        try {
            if (job) {
                job();
            }
        } catch (...) {
            return std::unexpected(JobSystemError::ExecutionFailed);
        }
        
        return {};
    }

    /**
     * @brief Zwraca liczbe zadan aktualnie oczekujacych w kolejce.
     * @return size_t Liczba zadan.
     */
    [[nodiscard]] size_t GetPendingJobCount() const noexcept {
        return m_jobs.size();
    }

    /**
     * @brief Czysci wszystkie zadania z kolejki bez ich wykonywania.
     */
    void Clear() noexcept {
        std::queue<JobType> emptyQueue;
        m_jobs.swap(emptyQueue);
    }

private:
    std::queue<JobType> m_jobs;
};

} // namespace Client::Concurrency
