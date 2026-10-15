#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <future>
#include <expected>
#include <cstdint>
#include <list>
#include <vector>

namespace Client::Concurrency
{
    // Kody bledow strumieniowania tekstur
    enum class TextureStreamingError
    {
        None,
        InvalidPath,
        FileNotFound,
        LoadFailed,
        OutOfMemory,
        CorruptedData
    };

    // Zmodyfikowana struktura tekstury dla potrzeb testow
    struct TextureResource
    {
        uint32_t width{0};
        uint32_t height{0};
        std::string path;
    };

    using TextureHandle = std::shared_ptr<TextureResource>;
    using TextureResult = std::expected<TextureHandle, TextureStreamingError>;
    using TextureFuture = std::shared_future<TextureResult>;

    // Interfejs fasady (Strangler Facade) sluzacy do wstrzykiwania zaleznosci
    class ITextureLoader
    {
    public:
        virtual ~ITextureLoader() = default;
        // Synchroniczne wczytywanie wywolywane w watku w tle
        virtual TextureResult LoadTexture(const std::string& path) = 0;
    };

    // Asynchroniczny system zarzadzania pamiecia tekstur w locie (LRU + Weak)
    class TextureStreamingCache
    {
    public:
        explicit TextureStreamingCache(std::shared_ptr<ITextureLoader> loader, size_t maxCacheCapacity = 100)
            : m_loader(std::move(loader)), m_capacity(maxCacheCapacity)
        {
        }

        ~TextureStreamingCache()
        {
            // Oczekiwanie na zakonczenie zadan asynchronicznych
            for (auto& task : m_backgroundTasks)
            {
                if (task.valid())
                {
                    task.wait();
                }
            }
        }

        // Zablokowane semantyki wartosci ze wzgledu na m.in. mutex
        TextureStreamingCache(const TextureStreamingCache&) = delete;
        TextureStreamingCache& operator=(const TextureStreamingCache&) = delete;
        TextureStreamingCache(TextureStreamingCache&&) = delete;
        TextureStreamingCache& operator=(TextureStreamingCache&&) = delete;

        // Zlecenie ladowania asynchronicznego z deduplikacja
        TextureFuture RequestTextureAsync(const std::string& path)
        {
            if (path.empty())
            {
                std::promise<TextureResult> p;
                p.set_value(std::unexpected(TextureStreamingError::InvalidPath));
                return p.get_future().share();
            }

            std::lock_guard<std::mutex> lock(m_mutex);

            CleanupExpired(); // Usuwa wygasle slabe wskazniki
            
            // 1. Sprawdz mapowanie (weak_ptr) - czy obiekt zyje w cache lub poza nim?
            auto it = m_weakCache.find(path);
            if (it != m_weakCache.end())
            {
                if (auto ptr = it->second.lock())
                {
                    // Odzyskano z weak_ptr. Zaktualizujmy LRU, aby podtrzymac zywotnosc w cache.
                    UpdateLru(path, ptr);
                    
                    std::promise<TextureResult> p;
                    p.set_value(ptr);
                    return p.get_future().share();
                }
                else
                {
                    // Wygasly slaby wskaznik
                    m_weakCache.erase(it);
                }
            }

            // 2. Sprawdz deduplikacje zadan (jesli inna czesc kodu juz to laduje)
            auto pendingIt = m_pendingRequests.find(path);
            if (pendingIt != m_pendingRequests.end())
            {
                return pendingIt->second;
            }

            // 3. Rozpocznij ladowanie asynchroniczne
            auto promise = std::make_shared<std::promise<TextureResult>>();
            TextureFuture future = promise->get_future().share();
            m_pendingRequests[path] = future;

            auto bgTask = std::async(std::launch::async, [this, path, promise]() {
                TextureResult result = m_loader->LoadTexture(path);
                
                std::lock_guard<std::mutex> innerLock(m_mutex);
                if (result.has_value())
                {
                    // Dodaj prawidlowa teksture do cache
                    AddTextureToCache(path, result.value());
                }
                
                promise->set_value(result);
                m_pendingRequests.erase(path);
            });

            m_backgroundTasks.push_back(std::move(bgTask));
            CleanupBackgroundTasks();

            return future;
        }

        size_t GetLruCacheSize() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_lruList.size();
        }

        size_t GetWeakCacheSize() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_weakCache.size();
        }
        
        void Clear()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_lruList.clear();
            m_lruMap.clear();
            m_weakCache.clear();
        }

    private:
        void UpdateLru(const std::string& path, const TextureHandle& handle)
        {
            auto it = m_lruMap.find(path);
            if (it != m_lruMap.end())
            {
                // Przesun na poczatek (most recently used)
                m_lruList.splice(m_lruList.begin(), m_lruList, it->second);
            }
            else
            {
                // Jesli nie bylo w LRU (byl tylko zywy na zewnatrz), dodaj go z powrotem.
                m_lruList.push_front({path, handle});
                m_lruMap[path] = m_lruList.begin();
                EnforceCapacity();
            }
        }

        void AddTextureToCache(const std::string& path, const TextureHandle& handle)
        {
            m_weakCache[path] = handle;

            m_lruList.push_front({path, handle});
            m_lruMap[path] = m_lruList.begin();
            EnforceCapacity();
        }

        void EnforceCapacity()
        {
            while (m_lruList.size() > m_capacity)
            {
                auto lastPath = m_lruList.back().first;
                m_lruMap.erase(lastPath);
                m_lruList.pop_back(); // Usuwa silna referencje (obiekt moze zostac zniszczony jesli nikt z zewnatrz go nie trzyma)
            }
        }

        void CleanupExpired()
        {
            for (auto it = m_weakCache.begin(); it != m_weakCache.end(); )
            {
                if (it->second.expired())
                {
                    it = m_weakCache.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        void CleanupBackgroundTasks()
        {
            for (auto it = m_backgroundTasks.begin(); it != m_backgroundTasks.end(); )
            {
                if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
                {
                    it = m_backgroundTasks.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        std::shared_ptr<ITextureLoader> m_loader;
        size_t m_capacity;

        mutable std::mutex m_mutex;
        
        // LRU Cache gwarantujacy, ze do capacity tekstur jest trzymanych nawet po zakonczeniu ich uzywania.
        std::list<std::pair<std::string, TextureHandle>> m_lruList;
        std::unordered_map<std::string, decltype(m_lruList)::iterator> m_lruMap;

        // Odtwarzanie wspoldzielenia juz zaladowanej lub wyrzuconej z LRU tekstury
        std::unordered_map<std::string, std::weak_ptr<TextureResource>> m_weakCache;

        // Trwajace ladowania deduplikowane
        std::unordered_map<std::string, TextureFuture> m_pendingRequests;
        std::vector<std::future<void>> m_backgroundTasks;
    };
}
