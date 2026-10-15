#pragma once

#include <expected>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <string>
#include <optional>
#include <span>
#include <cstdint>
#include <functional>
#include <stop_token>

namespace Client::Concurrency {

// Enum reprezentujacy mozliwe bledy podczas ladowania
enum class LoaderError {
    QueueFull,
    InvalidInput,
    Cancelled,
    UnknownError
};

// Struktura reprezentujaca dane wejsciowe do zaladowania
struct ChunkRequest {
    int32_t chunkX{0};
    int32_t chunkY{0};
    std::string path{};
};

// Struktura reprezentujaca zaladowany sektor
struct ChunkData {
    int32_t chunkX{0};
    int32_t chunkY{0};
    std::vector<uint8_t> geometryData{};
};

// Funkcja odpowiedzialna za ladowanie danych (np. symulacja z dysku)
using LoadFunction = std::function<std::expected<ChunkData, LoaderError>(const ChunkRequest&)>;

// Klasa ladujaca sektory w tle
class TerrainChunkAsyncLoader {
public:
    // Konstruktor inicjujacy watek ladujacy i ustawiajacy maksymalny rozmiar kolejki
    explicit TerrainChunkAsyncLoader(size_t maxQueueSize, LoadFunction loadFunc)
        : m_maxQueueSize(maxQueueSize), m_loadFunc(std::move(loadFunc)) {
        m_workerThread = std::jthread([this](std::stop_token stoken) {
            this->WorkerLoop(stoken);
        });
    }

    // Blokujemy kopiowanie i przenoszenie (zgodnosc z zasadami)
    TerrainChunkAsyncLoader(const TerrainChunkAsyncLoader&) = delete;
    TerrainChunkAsyncLoader& operator=(const TerrainChunkAsyncLoader&) = delete;
    TerrainChunkAsyncLoader(TerrainChunkAsyncLoader&&) = delete;
    TerrainChunkAsyncLoader& operator=(TerrainChunkAsyncLoader&&) = delete;

    ~TerrainChunkAsyncLoader() {
        m_workerThread.request_stop();
        m_queueCV.notify_all();
    }

    // Zglaszanie zadania ladowania.
    // Zwraca std::expected ze statusem operacji. Brak rzucania wyjatkow.
    [[nodiscard]] std::expected<void, LoaderError> Enqueue(const ChunkRequest& request) noexcept {
        if (request.path.empty()) {
            return std::unexpected(LoaderError::InvalidInput);
        }

        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_pendingRequests.size() >= m_maxQueueSize) {
            return std::unexpected(LoaderError::QueueFull);
        }

        m_pendingRequests.push(request);
        lock.unlock();
        m_queueCV.notify_one();

        return {};
    }

    // Odbieranie zaladowanych danych bez blokowania watku glownego.
    [[nodiscard]] std::optional<std::expected<ChunkData, LoaderError>> Poll() noexcept {
        std::lock_guard<std::mutex> lock(m_resultMutex);
        if (m_results.empty()) {
            return std::nullopt;
        }

        auto result = std::move(m_results.front());
        m_results.pop();
        return result;
    }

private:
    void WorkerLoop(std::stop_token stoken) noexcept {
        while (!stoken.stop_requested()) {
            ChunkRequest req;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_queueCV.wait(lock, stoken, [this]() {
                    return !m_pendingRequests.empty();
                });

                if (stoken.stop_requested()) {
                    break;
                }

                if (m_pendingRequests.empty()) {
                    continue;
                }

                req = std::move(m_pendingRequests.front());
                m_pendingRequests.pop();
            }

            // Realizacja ladowania zewnetrznej funkcji.
            auto result = m_loadFunc(req);

            {
                std::lock_guard<std::mutex> lock(m_resultMutex);
                m_results.push(std::move(result));
            }
        }
    }

    size_t m_maxQueueSize{0};
    LoadFunction m_loadFunc{};

    std::queue<ChunkRequest> m_pendingRequests{};
    std::queue<std::expected<ChunkData, LoaderError>> m_results{};

    std::mutex m_mutex{};
    std::mutex m_resultMutex{};
    std::condition_variable_any m_queueCV{};
    std::jthread m_workerThread{};
};

} // namespace Client::Concurrency
