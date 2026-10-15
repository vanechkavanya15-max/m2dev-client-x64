#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <memory>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <deque>
#include <cstdint>
#include <atomic>
#include <stop_token>
#include "../Platform/VFSManager.h"
#include "../../EterBase/Result.h"

namespace Client::Concurrency {

/**
 * @brief Klasa EterPackAsyncReader sluzaca do asynchronicznego odczytu i dekompresji
 *        plikow z wirtualnego systemu plikow (VFS) w tle.
 *        Wspiera C++23, gwarantuje brak zawieszen watku glownego i bezpiecznie 
 *        zarzadza cyklem zycia jthread.
 */
class EterPackAsyncReader {
public:
    using ReadResult = EterBase::Result<std::vector<uint8_t>, Client::Platform::VFSError>;
    using RequestId = uint64_t;
    using VfsReaderFunc = std::function<ReadResult(std::string_view)>;

    struct CompletedRequest {
        RequestId id;
        std::string virtualPath;
        ReadResult result;
    };

    /**
     * @brief Konstruktor, uruchamia watki pracujace (I/O).
     * 
     * @param workerCount Liczba watkow pracujacych (domyslnie 2).
     * @param vfsFunc Opcjonalna funkcja wczytujaca z VFS (dla wstrzykiwania zaleznosci w testach).
     */
    explicit EterPackAsyncReader(size_t workerCount = 2, 
                                 VfsReaderFunc vfsFunc = nullptr) 
        : m_nextId(1), m_vfsFunc(std::move(vfsFunc)) {
        
        if (!m_vfsFunc) {
            m_vfsFunc = [](std::string_view path) {
                return Client::Platform::VFSManager::Instance().ReadFile(path);
            };
        }

        m_workers.reserve(workerCount);
        for (size_t i = 0; i < workerCount; ++i) {
            m_workers.emplace_back([this](std::stop_token stoken) { WorkerLoop(stoken); });
        }
    }

    /**
     * @brief Destruktor, uzywa domyslnej wlasciwosci std::jthread 
     *        do wyslania sygnalu przerwania (stop request).
     */
    ~EterPackAsyncReader() noexcept {
        for (auto& worker : m_workers) {
            worker.request_stop();
        }
        
        {
            std::scoped_lock lock(m_queueMutex);
            m_pendingRequests.clear(); // Oczyszczenie oczekujacych zadan
        }
        m_cv.notify_all();
    }

    // Blokada kopiowania i przenoszenia ze wzgledu na watki w tle
    EterPackAsyncReader(const EterPackAsyncReader&) = delete;
    EterPackAsyncReader& operator=(const EterPackAsyncReader&) = delete;
    EterPackAsyncReader(EterPackAsyncReader&&) = delete;
    EterPackAsyncReader& operator=(EterPackAsyncReader&&) = delete;

    /**
     * @brief Zleca asynchroniczne czytanie pliku.
     * 
     * @param virtualPath Sciezka w VFS.
     * @return RequestId Identyfikator zlecenia.
     */
    RequestId SubmitRead(std::string_view virtualPath) {
        RequestId id = m_nextId.fetch_add(1, std::memory_order_relaxed);
        {
            std::scoped_lock lock(m_queueMutex);
            m_pendingRequests.emplace_back(PendingRequest{id, std::string(virtualPath)});
        }
        m_cv.notify_one();
        return id;
    }

    /**
     * @brief Pobiera wszystkie ukonczone zlecenia do obslugi na watku glownym.
     *        Optymalizuje re-alokacje pamieci zachowujac capacity bazowego wektora.
     * 
     * @param completed Wektor do ktorego zostana przeniesione dane.
     */
    void PollCompleted(std::vector<CompletedRequest>& completed) noexcept {
        std::scoped_lock lock(m_completedMutex);
        if (m_completedRequests.empty()) {
            return;
        }
        
        completed.reserve(completed.size() + m_completedRequests.size());
        for (auto& req : m_completedRequests) {
            completed.emplace_back(std::move(req));
        }
        // Zachowuje pojemnosc wektora
        m_completedRequests.clear();
    }

    /**
     * @brief Przeciazenie dla latwiejszego zwracania wartosci (jesli caller nie buforuje).
     */
    std::vector<CompletedRequest> PollCompleted() noexcept {
        std::vector<CompletedRequest> completed;
        PollCompleted(completed);
        return completed;
    }

private:
    struct PendingRequest {
        RequestId id;
        std::string virtualPath;
    };

    void WorkerLoop(std::stop_token stoken) {
        while (!stoken.stop_requested()) {
            PendingRequest req;
            {
                std::unique_lock lock(m_queueMutex);
                m_cv.wait(lock, stoken, [this]() { return !m_pendingRequests.empty(); });

                if (stoken.stop_requested() || m_pendingRequests.empty()) {
                    break;
                }

                req = std::move(m_pendingRequests.front());
                m_pendingRequests.pop_front();
            }

            // Praca we/wy - wczytywanie pliku za pomoca wstrzyknietej funkcji VFS
            ReadResult result = m_vfsFunc(req.virtualPath);

            // Odkladanie na stos ukonczonych
            {
                std::scoped_lock lock(m_completedMutex);
                m_completedRequests.emplace_back(CompletedRequest{req.id, std::move(req.virtualPath), std::move(result)});
            }
        }
    }

    // Wazne: Kolejnosc zmiennych klasy ma znaczenie dla bezpiecznego niszczenia.
    // m_workers musi byc niszczony PRZED mutexami, dlatego deklarowany jest na samym dole.
    
    std::atomic<RequestId> m_nextId;
    VfsReaderFunc m_vfsFunc;
    
    std::mutex m_queueMutex;
    std::condition_variable_any m_cv;
    std::deque<PendingRequest> m_pendingRequests;

    std::mutex m_completedMutex;
    std::vector<CompletedRequest> m_completedRequests;

    std::vector<std::jthread> m_workers;
};

} // namespace Client::Concurrency
