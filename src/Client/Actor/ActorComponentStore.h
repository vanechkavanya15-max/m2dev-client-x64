#pragma once

#include <EterBase/StrongTypes.h>
#include <EterBase/Result.h>
#include <vector>
#include <algorithm>
#include <optional>
#include <span>

namespace Client::Actor {

// ============================================================================
// Komponenty Aktorow
// ============================================================================

struct TransformComponent {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float rotation = 0.0f;
};

struct StatsComponent {
    uint32_t hp = 0;
    uint32_t maxHp = 0;
    uint32_t sp = 0;
    uint32_t maxSp = 0;
    uint32_t level = 1;
};

struct EquipmentComponent {
    uint32_t weaponVnum = 0;
    uint32_t armorVnum = 0;
};

struct StateComponent {
    bool isDead = false;
    bool isMoving = false;
    bool isCombat = false;
};

// ============================================================================
// Bezpieczny, Cache-Friendly Magazyn Komponentow
// ============================================================================

template <typename T>
class ComponentStore {
public:
    struct Element {
        EterBase::EntityId entityId;
        T component;

        constexpr bool operator<(const Element& other) const noexcept {
            return entityId < other.entityId;
        }

        constexpr bool operator<(const EterBase::EntityId& id) const noexcept {
            return entityId < id;
        }
    };

    constexpr ComponentStore() noexcept = default;

    [[nodiscard]] constexpr EterBase::Result<void, EterBase::EntityError> Add(EterBase::EntityId id, T component) {
        auto it = std::lower_bound(m_elements.begin(), m_elements.end(), id);
        if (it != m_elements.end() && it->entityId == id) {
            return EterBase::MakeError(EterBase::EntityError::AlreadyExists);
        }
        m_elements.insert(it, Element{id, std::move(component)});
        return {};
    }

    [[nodiscard]] constexpr EterBase::Result<T*, EterBase::EntityError> Get(EterBase::EntityId id) noexcept {
        auto it = std::lower_bound(m_elements.begin(), m_elements.end(), id);
        if (it != m_elements.end() && it->entityId == id) {
            return &it->component;
        }
        return EterBase::MakeError(EterBase::EntityError::NotFound);
    }

    [[nodiscard]] constexpr EterBase::Result<const T*, EterBase::EntityError> Get(EterBase::EntityId id) const noexcept {
        auto it = std::lower_bound(m_elements.begin(), m_elements.end(), id);
        if (it != m_elements.end() && it->entityId == id) {
            return &it->component;
        }
        return EterBase::MakeError(EterBase::EntityError::NotFound);
    }

    [[nodiscard]] constexpr EterBase::Result<void, EterBase::EntityError> Remove(EterBase::EntityId id) noexcept {
        auto it = std::lower_bound(m_elements.begin(), m_elements.end(), id);
        if (it != m_elements.end() && it->entityId == id) {
            m_elements.erase(it);
            return {};
        }
        return EterBase::MakeError(EterBase::EntityError::NotFound);
    }

    constexpr void RemoveAllForEntity(EterBase::EntityId id) noexcept {
        (void)Remove(id);
    }

    constexpr void Clear() noexcept {
        m_elements.clear();
    }

    [[nodiscard]] constexpr std::span<const Element> GetAll() const noexcept {
        return m_elements;
    }

private:
    std::vector<Element> m_elements;
};

// ============================================================================
// Fasada Magazynow
// ============================================================================

class ActorComponentStore {
public:
    constexpr ActorComponentStore() noexcept = default;

    // Transform
    [[nodiscard]] constexpr EterBase::Result<void, EterBase::EntityError> AddTransform(EterBase::EntityId id, TransformComponent comp) {
        return m_transforms.Add(id, comp);
    }
    [[nodiscard]] constexpr EterBase::Result<TransformComponent*, EterBase::EntityError> GetTransform(EterBase::EntityId id) noexcept {
        return m_transforms.Get(id);
    }
    [[nodiscard]] constexpr EterBase::Result<const TransformComponent*, EterBase::EntityError> GetTransform(EterBase::EntityId id) const noexcept {
        return m_transforms.Get(id);
    }
    [[nodiscard]] constexpr EterBase::Result<void, EterBase::EntityError> RemoveTransform(EterBase::EntityId id) noexcept {
        return m_transforms.Remove(id);
    }

    // Stats
    [[nodiscard]] constexpr EterBase::Result<void, EterBase::EntityError> AddStats(EterBase::EntityId id, StatsComponent comp) {
        return m_stats.Add(id, comp);
    }
    [[nodiscard]] constexpr EterBase::Result<StatsComponent*, EterBase::EntityError> GetStats(EterBase::EntityId id) noexcept {
        return m_stats.Get(id);
    }
    [[nodiscard]] constexpr EterBase::Result<const StatsComponent*, EterBase::EntityError> GetStats(EterBase::EntityId id) const noexcept {
        return m_stats.Get(id);
    }
    [[nodiscard]] constexpr EterBase::Result<void, EterBase::EntityError> RemoveStats(EterBase::EntityId id) noexcept {
        return m_stats.Remove(id);
    }

    // Equipment
    [[nodiscard]] constexpr EterBase::Result<void, EterBase::EntityError> AddEquipment(EterBase::EntityId id, EquipmentComponent comp) {
        return m_equipments.Add(id, comp);
    }
    [[nodiscard]] constexpr EterBase::Result<EquipmentComponent*, EterBase::EntityError> GetEquipment(EterBase::EntityId id) noexcept {
        return m_equipments.Get(id);
    }
    [[nodiscard]] constexpr EterBase::Result<const EquipmentComponent*, EterBase::EntityError> GetEquipment(EterBase::EntityId id) const noexcept {
        return m_equipments.Get(id);
    }
    [[nodiscard]] constexpr EterBase::Result<void, EterBase::EntityError> RemoveEquipment(EterBase::EntityId id) noexcept {
        return m_equipments.Remove(id);
    }

    // State
    [[nodiscard]] constexpr EterBase::Result<void, EterBase::EntityError> AddState(EterBase::EntityId id, StateComponent comp) {
        return m_states.Add(id, comp);
    }
    [[nodiscard]] constexpr EterBase::Result<StateComponent*, EterBase::EntityError> GetState(EterBase::EntityId id) noexcept {
        return m_states.Get(id);
    }
    [[nodiscard]] constexpr EterBase::Result<const StateComponent*, EterBase::EntityError> GetState(EterBase::EntityId id) const noexcept {
        return m_states.Get(id);
    }
    [[nodiscard]] constexpr EterBase::Result<void, EterBase::EntityError> RemoveState(EterBase::EntityId id) noexcept {
        return m_states.Remove(id);
    }

    // General
    constexpr void RemoveAllComponents(EterBase::EntityId id) noexcept {
        m_transforms.RemoveAllForEntity(id);
        m_stats.RemoveAllForEntity(id);
        m_equipments.RemoveAllForEntity(id);
        m_states.RemoveAllForEntity(id);
    }

    constexpr void Clear() noexcept {
        m_transforms.Clear();
        m_stats.Clear();
        m_equipments.Clear();
        m_states.Clear();
    }

private:
    ComponentStore<TransformComponent> m_transforms;
    ComponentStore<StatsComponent> m_stats;
    ComponentStore<EquipmentComponent> m_equipments;
    ComponentStore<StateComponent> m_states;
};

} // namespace Client::Actor
