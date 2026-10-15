#pragma once

#include <cstdint>
#include <expected>
#include <shared_mutex>
#include <mutex>
#include <unordered_map>
#include <optional>

// Zadeklarowanie istniejacej klasy CInstanceBase (legacy)
class CInstanceBase;

namespace Client::Actor {

// Typ bledu dla operacji na adapterze
enum class AdapterError {
    NotFound,
    AlreadyExists,
    InvalidPointer,
    InvalidId
};

// Struktura reprezentujaca unikalny identyfikator bytu
struct EntityID {
    uint32_t value{0};
    
    constexpr bool operator==(const EntityID& other) const noexcept {
        return value == other.value;
    }
    constexpr bool operator!=(const EntityID& other) const noexcept {
        return value != other.value;
    }
    constexpr bool IsValid() const noexcept {
        return value != 0;
    }
};

} // namespace Client::Actor

template <>
struct std::hash<Client::Actor::EntityID> {
    std::size_t operator()(const Client::Actor::EntityID& id) const noexcept {
        return std::hash<uint32_t>{}(id.value);
    }
};

namespace Client::Actor {

// Bezpieczny uchwyt (EntityHandle) uzywany zamiast surowego wskaznika
// Uchwyt nie posiada samego obiektu, sluzy tylko do weryfikacji i dostepu poprzez Adapter.
class EntityHandle {
public:
    constexpr EntityHandle() noexcept = default;
    constexpr explicit EntityHandle(EntityID id) noexcept : m_id(id) {}

    constexpr EntityID GetID() const noexcept { return m_id; }
    constexpr bool IsValid() const noexcept { return m_id.IsValid(); }

private:
    EntityID m_id;
};

// Klasa adaptera pelniaca role mostka pomiedzy starymi wskaznikami a bezpiecznymi uchwytami.
class LegacyInstanceAdapter {
public:
    LegacyInstanceAdapter() = default;
    ~LegacyInstanceAdapter() = default;

    // Blokada kopiowania i przenoszenia
    LegacyInstanceAdapter(const LegacyInstanceAdapter&) = delete;
    LegacyInstanceAdapter& operator=(const LegacyInstanceAdapter&) = delete;
    LegacyInstanceAdapter(LegacyInstanceAdapter&&) = delete;
    LegacyInstanceAdapter& operator=(LegacyInstanceAdapter&&) = delete;

    // Rejestruje istniejacy obiekt legacy i zwraca bezpieczny uchwyt
    std::expected<EntityHandle, AdapterError> RegisterInstance(EntityID id, CInstanceBase* instance) noexcept {
        if (!id.IsValid()) {
            return std::unexpected(AdapterError::InvalidId);
        }
        if (!instance) {
            return std::unexpected(AdapterError::InvalidPointer);
        }

        std::unique_lock lock(m_mutex);
        if (m_instances.contains(id)) {
            return std::unexpected(AdapterError::AlreadyExists);
        }

        m_instances[id] = instance;
        return EntityHandle{id};
    }

    // Pobiera surowy wskaznik na podstawie bezpiecznego uchwytu (zwraca blad jesli nie istnieje)
    std::expected<CInstanceBase*, AdapterError> GetInstance(EntityHandle handle) const noexcept {
        if (!handle.IsValid()) {
            return std::unexpected(AdapterError::InvalidId);
        }

        std::shared_lock lock(m_mutex);
        auto it = m_instances.find(handle.GetID());
        if (it != m_instances.end()) {
            return it->second;
        }

        return std::unexpected(AdapterError::NotFound);
    }

    // Usuwa instancje z rejestru na podstawie identyfikatora
    std::expected<void, AdapterError> UnregisterInstance(EntityID id) noexcept {
        if (!id.IsValid()) {
            return std::unexpected(AdapterError::InvalidId);
        }

        std::unique_lock lock(m_mutex);
        auto it = m_instances.find(id);
        if (it == m_instances.end()) {
            return std::unexpected(AdapterError::NotFound);
        }

        m_instances.erase(it);
        return {};
    }

    // Usuwa instancje bezposrednio ze wskaznika legacy (wymaga przeszukania mapy)
    std::expected<void, AdapterError> UnregisterByPointer(const CInstanceBase* instance) noexcept {
        if (!instance) {
            return std::unexpected(AdapterError::InvalidPointer);
        }

        std::unique_lock lock(m_mutex);
        for (auto it = m_instances.begin(); it != m_instances.end(); ++it) {
            if (it->second == instance) {
                m_instances.erase(it);
                return {};
            }
        }
        return std::unexpected(AdapterError::NotFound);
    }
    
    // Zwraca ilosc zarejestrowanych bytow
    size_t GetCount() const noexcept {
        std::shared_lock lock(m_mutex);
        return m_instances.size();
    }
    
    // Czysci caly rejestr
    void Clear() noexcept {
        std::unique_lock lock(m_mutex);
        m_instances.clear();
    }

private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<EntityID, CInstanceBase*> m_instances;
};

} // namespace Client::Actor
