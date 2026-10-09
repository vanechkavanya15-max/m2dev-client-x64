#pragma once

#include <cstdint>
#include <compare>
#include <vector>
#include <optional>
#include <limits>
#include "../../EterBase/Result.h"

namespace Client::Actor {

/**
 * @brief Uchwyt bytu (EntityHandle) uzywajacy indeksu generacyjnego.
 * Chroni przed problemem ABA i Use-After-Free.
 */
class EntityHandle {
public:
    using IndexType = uint32_t;
    using GenerationType = uint32_t;

    static constexpr IndexType InvalidIndex = std::numeric_limits<IndexType>::max();

    constexpr EntityHandle() noexcept : m_index(InvalidIndex), m_generation(0) {}
    constexpr EntityHandle(IndexType index, GenerationType generation) noexcept
        : m_index(index), m_generation(generation) {}

    [[nodiscard]] constexpr bool IsValid() const noexcept { return m_index != InvalidIndex; }
    [[nodiscard]] constexpr IndexType GetIndex() const noexcept { return m_index; }
    [[nodiscard]] constexpr GenerationType GetGeneration() const noexcept { return m_generation; }

    constexpr auto operator<=>(const EntityHandle&) const noexcept = default;
    constexpr bool operator==(const EntityHandle&) const noexcept = default;

private:
    IndexType m_index;
    GenerationType m_generation;
};

/**
 * @brief Rejestr z indeksem generacyjnym O(1).
 * Zapewnia bezpieczne zarzadzanie pamiecia i odpornosc na kolizje (Use-After-Free).
 */
template <typename T>
class GenerationalRegistry {
private:
    struct Slot {
        std::optional<T> data;
        EntityHandle::GenerationType generation = 0;
        EntityHandle::IndexType nextFreeIndex = EntityHandle::InvalidIndex;
    };

    std::vector<Slot> m_slots;
    EntityHandle::IndexType m_firstFreeIndex = EntityHandle::InvalidIndex;
    size_t m_activeCount = 0;

public:
    GenerationalRegistry() = default;
    ~GenerationalRegistry() = default;

    GenerationalRegistry(const GenerationalRegistry&) = delete;
    GenerationalRegistry& operator=(const GenerationalRegistry&) = delete;

    GenerationalRegistry(GenerationalRegistry&&) noexcept = default;
    GenerationalRegistry& operator=(GenerationalRegistry&&) noexcept = default;

    /**
     * @brief Dodaje nowy obiekt do rejestru i zwraca jego uchwyt.
     */
    [[nodiscard]] EterBase::Result<EntityHandle, EterBase::EntityError> Insert(T value) {
        EntityHandle::IndexType index;
        EntityHandle::GenerationType generation;

        if (m_firstFreeIndex != EntityHandle::InvalidIndex) {
            index = m_firstFreeIndex;
            m_firstFreeIndex = m_slots[index].nextFreeIndex;
            generation = m_slots[index].generation;
            m_slots[index].data.emplace(std::move(value));
        } else {
            index = static_cast<EntityHandle::IndexType>(m_slots.size());
            generation = 0;
            m_slots.push_back(Slot{ std::make_optional(std::move(value)), generation, EntityHandle::InvalidIndex });
        }

        m_activeCount++;
        return EntityHandle(index, generation);
    }

    /**
     * @brief Pobiera obiekt z rejestru na podstawie uchwytu.
     * UWAGA: Zwracany wskaznik jest wazny tylko do nastepnego wywolania Insert().
     */
    [[nodiscard]] EterBase::Result<T*, EterBase::EntityError> Get(EntityHandle handle) noexcept {
        if (!handle.IsValid() || handle.GetIndex() >= m_slots.size()) {
            return EterBase::MakeError(EterBase::EntityError::NotFound);
        }

        Slot& slot = m_slots[handle.GetIndex()];

        if (slot.generation != handle.GetGeneration() || !slot.data.has_value()) {
            return EterBase::MakeError(EterBase::EntityError::Dead);
        }

        return &slot.data.value();
    }

    /**
     * @brief Pobiera obiekt z rejestru (wersja const).
     * UWAGA: Zwracany wskaznik jest wazny tylko do nastepnego wywolania Insert().
     */
    [[nodiscard]] EterBase::Result<const T*, EterBase::EntityError> Get(EntityHandle handle) const noexcept {
        if (!handle.IsValid() || handle.GetIndex() >= m_slots.size()) {
            return EterBase::MakeError(EterBase::EntityError::NotFound);
        }

        const Slot& slot = m_slots[handle.GetIndex()];

        if (slot.generation != handle.GetGeneration() || !slot.data.has_value()) {
            return EterBase::MakeError(EterBase::EntityError::Dead);
        }

        return &slot.data.value();
    }

    /**
     * @brief Usuwa obiekt z rejestru na podstawie uchwytu.
     */
    [[nodiscard]] EterBase::Result<void, EterBase::EntityError> Erase(EntityHandle handle) noexcept {
        if (!handle.IsValid() || handle.GetIndex() >= m_slots.size()) {
            return EterBase::MakeError(EterBase::EntityError::NotFound);
        }

        Slot& slot = m_slots[handle.GetIndex()];

        if (slot.generation != handle.GetGeneration() || !slot.data.has_value()) {
            return EterBase::MakeError(EterBase::EntityError::Dead);
        }

        slot.data.reset();
        slot.generation++; // Inkrementacja generacji uniewaznia wszystkie wczesniejsze uchwyty
        slot.nextFreeIndex = m_firstFreeIndex;
        m_firstFreeIndex = handle.GetIndex();

        m_activeCount--;
        return {};
    }

    [[nodiscard]] size_t Size() const noexcept { return m_activeCount; }
    [[nodiscard]] size_t Capacity() const noexcept { return m_slots.size(); }
    
    /**
     * @brief Czysci rejestr zachowujac historie generacji.
     */
    void Clear() noexcept {
        m_firstFreeIndex = EntityHandle::InvalidIndex;
        m_activeCount = 0;
        
        // Przebudowa free-listy od tylu (dla zachowania rosnacej kolejnosci przy wstawianiu),
        // resetowanie danych i podbijanie generacji.
        for (size_t i = m_slots.size(); i > 0; --i) {
            size_t idx = i - 1;
            Slot& slot = m_slots[idx];
            if (slot.data.has_value()) {
                slot.data.reset();
                slot.generation++;
            }
            slot.nextFreeIndex = m_firstFreeIndex;
            m_firstFreeIndex = static_cast<EntityHandle::IndexType>(idx);
        }
    }
};

} // namespace Client::Actor
