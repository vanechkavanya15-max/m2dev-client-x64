#pragma once

#include <vector>
#include <optional>
#include <expected>
#include <cstdint>
#include <concepts>
#include "EterBase/StrongTypes.h"

namespace Client::Actor {

enum class RegistryError {
    InvalidRef,
    ActorDespawned,
    GenerationMismatch,
    RegistryFull
};

/**
 * @brief Bezpieczny wskaznik na aktora, zapobiegajacy wiszacym referencjom
 * wykorzystujacy wzorzec Generational Index.
 */
class SafeActorRef {
public:
    constexpr SafeActorRef() noexcept = default;
    
    constexpr SafeActorRef(uint32_t index, uint32_t generation) noexcept
        : m_index(index), m_generation(generation) {}

    [[nodiscard]] constexpr uint32_t GetIndex() const noexcept { return m_index; }
    [[nodiscard]] constexpr uint32_t GetGeneration() const noexcept { return m_generation; }

    constexpr bool operator==(const SafeActorRef&) const noexcept = default;

private:
    uint32_t m_index = 0;
    uint32_t m_generation = 0;
};

/**
 * @brief Rejestr aktorow wykorzystujacy std::vector z std::optional do bezpiecznego
 * zarzadzania pamiecia bez narzutu na alokacje po stercie.
 */
template <typename TActor>
class ActorRegistry {
public:
    struct Slot {
        std::optional<TActor> actor;
        uint32_t generation = 1;
    };

    ActorRegistry() = default;

    ActorRegistry(const ActorRegistry&) = delete;
    ActorRegistry& operator=(const ActorRegistry&) = delete;

    ActorRegistry(ActorRegistry&&) noexcept = default;
    ActorRegistry& operator=(ActorRegistry&&) noexcept = default;

    /**
     * @brief Alokuje nowego aktora i zwraca bezpieczny wskaznik na niego
     */
    template <typename... Args>
    [[nodiscard]] std::expected<SafeActorRef, RegistryError> Spawn(Args&&... args) {
        uint32_t index = 0;
        
        if (!m_freeIndices.empty()) {
            index = m_freeIndices.back();
            m_freeIndices.pop_back();
        } else {
            if (m_slots.size() >= UINT32_MAX) {
                return std::unexpected(RegistryError::RegistryFull);
            }
            index = static_cast<uint32_t>(m_slots.size());
            m_slots.emplace_back();
        }

        auto& slot = m_slots[index];
        slot.actor.emplace(std::forward<Args>(args)...);
        
        return SafeActorRef{index, slot.generation};
    }

    /**
     * @brief Usuwa aktora i uniewaznia wszystkie wczesniejsze referencje
     */
    [[nodiscard]] std::expected<void, RegistryError> Despawn(SafeActorRef ref) {
        if (ref.GetIndex() >= m_slots.size()) {
            return std::unexpected(RegistryError::InvalidRef);
        }

        auto& slot = m_slots[ref.GetIndex()];
        
        if (slot.generation != ref.GetGeneration()) {
            return std::unexpected(RegistryError::GenerationMismatch);
        }

        if (!slot.actor.has_value()) {
            return std::unexpected(RegistryError::ActorDespawned);
        }

        slot.actor.reset();
        slot.generation++; // Kluczowe dla wzorca: uniewaznia istniejace SafeActorRef
        
        // Zabezpieczenie przed przekreceniem licznika
        if (slot.generation == 0) {
            slot.generation = 1;
        }

        m_freeIndices.push_back(ref.GetIndex());

        return {};
    }

    /**
     * @brief Zwraca wskaznik na instancje aktora jezeli referencja jest wciaz wazna
     */
    [[nodiscard]] std::expected<TActor*, RegistryError> Get(SafeActorRef ref) noexcept {
        if (ref.GetIndex() >= m_slots.size()) {
            return std::unexpected(RegistryError::InvalidRef);
        }

        auto& slot = m_slots[ref.GetIndex()];

        if (slot.generation != ref.GetGeneration()) {
            return std::unexpected(RegistryError::GenerationMismatch);
        }

        if (!slot.actor.has_value()) {
            return std::unexpected(RegistryError::ActorDespawned);
        }

        return &(*slot.actor);
    }

    [[nodiscard]] std::expected<const TActor*, RegistryError> Get(SafeActorRef ref) const noexcept {
        if (ref.GetIndex() >= m_slots.size()) {
            return std::unexpected(RegistryError::InvalidRef);
        }

        const auto& slot = m_slots[ref.GetIndex()];

        if (slot.generation != ref.GetGeneration()) {
            return std::unexpected(RegistryError::GenerationMismatch);
        }

        if (!slot.actor.has_value()) {
            return std::unexpected(RegistryError::ActorDespawned);
        }

        return &(*slot.actor);
    }
    
    /**
     * @brief Czysci caly rejestr z poprawna inkrementacja generacji
     */
    void Clear() noexcept {
        m_freeIndices.clear();
        for (uint32_t i = 0; i < m_slots.size(); ++i) {
            auto& slot = m_slots[i];
            if (slot.actor.has_value()) {
                slot.actor.reset();
                slot.generation++;
                if (slot.generation == 0) {
                    slot.generation = 1;
                }
            }
            m_freeIndices.push_back(i);
        }
    }

    /**
     * @brief Zwraca pojemnosc wektora
     */
    [[nodiscard]] size_t Capacity() const noexcept {
        return m_slots.size();
    }
    
    /**
     * @brief Zwraca ilosc aktywnych aktorow
     */
    [[nodiscard]] size_t ActiveCount() const noexcept {
        return m_slots.size() - m_freeIndices.size();
    }

private:
    std::vector<Slot> m_slots;
    std::vector<uint32_t> m_freeIndices;
};

} // namespace Client::Actor
