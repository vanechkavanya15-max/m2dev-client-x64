#pragma once

#include <vector>
#include <cstdint>
#include <span>
#include <expected>
#include <optional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <stdexcept>

namespace Client::Concurrency
{

enum class AssetError
{
    InvalidInput,
    QueueFull,
    SystemShutdown,
    AllocationFailed,
    DecompressionFailed
};

template<typename T>
using AsyncResult = std::expected<T, AssetError>;

enum class AssetType
{
    Texture,
    Mesh
};

struct AssetRequest
{
    uint32_t id;
    AssetType type;
    std::vector<uint8_t> data;
};

struct AssetResponse
{
    uint32_t id;
    AssetType type;
    AsyncResult<std::vector<uint8_t>> result;
};

class AsyncAssetLoader
{
public:
    explicit AsyncAssetLoader(size_t workerCount, size_t maxQueueSize) noexcept
        : m_maxQueueSize(maxQueueSize), m_shutdown(false)
    {
        try
        {
            for (size_t i = 0; i < workerCount; ++i)
            {
                m_workers.emplace_back([this](std::stop_token st) { WorkerLoop(st); });
            }
        }
        catch (...)
        {
            Shutdown();
        }
    }

    ~AsyncAssetLoader() noexcept
    {
        Shutdown();
    }

    AsyncAssetLoader(const AsyncAssetLoader&) = delete;
    AsyncAssetLoader& operator=(const AsyncAssetLoader&) = delete;
    AsyncAssetLoader(AsyncAssetLoader&&) = delete;
    AsyncAssetLoader& operator=(AsyncAssetLoader&&) = delete;

    AsyncResult<void> EnqueueAsset(uint32_t id, AssetType type, std::span<const uint8_t> compressedData) noexcept
    {
        if (compressedData.empty())
        {
            return std::unexpected(AssetError::InvalidInput);
        }

        try
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_shutdown.load(std::memory_order_relaxed))
            {
                return std::unexpected(AssetError::SystemShutdown);
            }

            if (m_requestQueue.size() >= m_maxQueueSize)
            {
                return std::unexpected(AssetError::QueueFull);
            }

            AssetRequest req{
                id,
                type,
                std::vector<uint8_t>(compressedData.begin(), compressedData.end())
            };

            m_requestQueue.push(std::move(req));
            lock.unlock();
            m_cv.notify_one();

            return {};
        }
        catch (const std::bad_alloc&)
        {
            return std::unexpected(AssetError::AllocationFailed);
        }
        catch (...)
        {
            return std::unexpected(AssetError::InvalidInput);
        }
    }

    std::optional<AssetResponse> PollResult() noexcept
    {
        try
        {
            std::lock_guard<std::mutex> lock(m_resultMutex);
            if (m_resultQueue.empty())
            {
                return std::nullopt;
            }

            AssetResponse res = std::move(m_resultQueue.front());
            m_resultQueue.pop();
            return res;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    void Shutdown() noexcept
    {
        bool expected = false;
        if (!m_shutdown.compare_exchange_strong(expected, true, std::memory_order_relaxed))
        {
            return;
        }

        m_cv.notify_all();

        for (auto& worker : m_workers)
        {
            if (worker.joinable())
            {
                worker.request_stop();
                try
                {
                    worker.join();
                }
                catch (...) {}
            }
        }
        m_workers.clear();
    }

private:
    void WorkerLoop(std::stop_token st) noexcept
    {
        while (!st.stop_requested())
        {
            std::optional<AssetRequest> requestOpt;

            try
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, st, [this] { return !m_requestQueue.empty() || m_shutdown.load(std::memory_order_relaxed); });

                if (st.stop_requested() || (m_shutdown.load(std::memory_order_relaxed) && m_requestQueue.empty()))
                {
                    break;
                }

                if (!m_requestQueue.empty())
                {
                    requestOpt = std::move(m_requestQueue.front());
                    m_requestQueue.pop();
                }
            }
            catch (...)
            {
                // W przypadku bledu blokady przerywamy petle
                break;
            }

            if (requestOpt)
            {
                ProcessRequest(std::move(*requestOpt));
            }
        }
    }

    void ProcessRequest(AssetRequest&& req) noexcept
    {
        AssetResponse response{req.id, req.type, std::unexpected(AssetError::DecompressionFailed)};

        try
        {
            // Symulacja dekompresji - odwrocenie kolejnosci bajtow na potrzeby weryfikacji
            std::vector<uint8_t> resultData;
            resultData.reserve(req.data.size());
            
            for (auto it = req.data.rbegin(); it != req.data.rend(); ++it)
            {
                resultData.push_back(*it);
            }

            response.result = std::move(resultData);
        }
        catch (const std::bad_alloc&)
        {
            response.result = std::unexpected(AssetError::AllocationFailed);
        }
        catch (...)
        {
            response.result = std::unexpected(AssetError::DecompressionFailed);
        }

        try
        {
            std::lock_guard<std::mutex> lock(m_resultMutex);
            m_resultQueue.push(std::move(response));
        }
        catch (...)
        {
            // Ignorujemy bledy wrzucania do kolejki wynikowej (np. bad_alloc)
        }
    }

    size_t m_maxQueueSize;
    std::atomic<bool> m_shutdown;
    std::vector<std::jthread> m_workers;
    
    std::mutex m_mutex;
    std::condition_variable_any m_cv;
    std::queue<AssetRequest> m_requestQueue;

    std::mutex m_resultMutex;
    std::queue<AssetResponse> m_resultQueue;
};

} // namespace Client::Concurrency
