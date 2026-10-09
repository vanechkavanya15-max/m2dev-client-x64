#include "JobSystem.h"

namespace Client::Concurrency
{
    JobSystem::JobSystem(std::size_t numThreads)
    {
        if (numThreads == 0)
        {
            numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0)
            {
                numThreads = 2; // Bezpieczny fallback
            }
        }

        m_queues.resize(numThreads);
        m_mutexes = std::make_unique<std::mutex[]>(numThreads);

        m_threads.reserve(numThreads);
        for (std::size_t i = 0; i < numThreads; ++i)
        {
            m_threads.emplace_back([this, i]() {
                WorkerThread(i);
            });
        }
    }

    JobSystem::~JobSystem() noexcept
    {
        m_stop.store(true, std::memory_order_release);
        m_cv.notify_all();

        // std::jthread automatycznie wykonuje join w destruktorze
        m_threads.clear();
    }

    void JobSystem::PushJob(Job job)
    {
        if (m_threads.empty() || m_stop.load(std::memory_order_relaxed))
        {
            return;
        }

        m_queuedJobs.fetch_add(1, std::memory_order_release);

        // Wybieramy kolejke o najmniejszym obciazeniu (round-robin lub proste haszowanie)
        static std::atomic<std::size_t> roundRobinIdx{0};
        const std::size_t targetQueue = roundRobinIdx.fetch_add(1, std::memory_order_relaxed) % m_queues.size();

        {
            std::lock_guard<std::mutex> lock(m_mutexes[targetQueue]);
            m_queues[targetQueue].push_back(std::move(job));
        }

        m_cv.notify_one();
    }

    void JobSystem::WorkerThread(std::size_t workerId)
    {
        while (!m_stop.load(std::memory_order_relaxed))
        {
            Job currentJob;
            bool foundJob = false;

            // 1. Probujemy pobrac zadanie z wlasnej kolejki
            {
                std::unique_lock<std::mutex> lock(m_mutexes[workerId]);
                if (!m_queues[workerId].empty())
                {
                    currentJob = std::move(m_queues[workerId].front());
                    m_queues[workerId].pop_front();
                    foundJob = true;
                }
            }

            // 2. Jesli wlasna kolejka jest pusta, kradniemy z innej (work stealing)
            if (!foundJob)
            {
                foundJob = TrySteal(currentJob, workerId);
            }

            // 3. Wykonanie zadania
            if (foundJob)
            {
                m_queuedJobs.fetch_sub(1, std::memory_order_relaxed);
                m_activeJobs.fetch_add(1, std::memory_order_relaxed);

                if (currentJob)
                {
                    currentJob();
                }

                m_activeJobs.fetch_sub(1, std::memory_order_release);

                // Powiadom ewentualne WaitAll
                if (m_queuedJobs.load(std::memory_order_acquire) == 0 &&
                    m_activeJobs.load(std::memory_order_acquire) == 0)
                {
                    m_waitCv.notify_all();
                }
            }
            else
            {
                // Czekamy na notyfikacje o nowych zadaniach
                std::unique_lock<std::mutex> lock(m_cvMutex);
                m_cv.wait_for(lock, std::chrono::milliseconds(5), [this]() {
                    return m_stop.load(std::memory_order_relaxed) || m_queuedJobs.load(std::memory_order_relaxed) > 0;
                });
            }
        }
    }

    bool JobSystem::TrySteal(Job& job, std::size_t workerId)
    {
        const std::size_t numQueues = m_queues.size();
        for (std::size_t i = 0; i < numQueues; ++i)
        {
            const std::size_t victimId = (workerId + 1 + i) % numQueues;
            if (victimId == workerId) continue;

            std::unique_lock<std::mutex> lock(m_mutexes[victimId], std::try_to_lock);
            if (lock.owns_lock() && !m_queues[victimId].empty())
            {
                // Kradniemy z konca kolejki (LIFO) dla optymalizacji cache
                job = std::move(m_queues[victimId].back());
                m_queues[victimId].pop_back();
                return true;
            }
        }
        return false;
    }

    void JobSystem::WaitAll()
    {
        std::unique_lock<std::mutex> lock(m_waitMutex);
        m_waitCv.wait(lock, [this]() {
            return m_queuedJobs.load(std::memory_order_acquire) == 0 &&
                   m_activeJobs.load(std::memory_order_acquire) == 0;
        });
    }
}
