#pragma once

#include <cstdint>
#include <expected>
#include <string_view>
#include <span>
#include <vector>
#include <unordered_map>
#include <cmath>

namespace Client::Domain {

enum class MovementError : uint8_t {
    None = 0,
    EntityNotFound,
    AlreadyMoving,
    InvalidDeltaTime,
    InvalidSpeed
};

[[nodiscard]] constexpr std::string_view ToString(MovementError err) noexcept {
    switch (err) {
        case MovementError::None: return "None";
        case MovementError::EntityNotFound: return "EntityNotFound";
        case MovementError::AlreadyMoving: return "AlreadyMoving";
        case MovementError::InvalidDeltaTime: return "InvalidDeltaTime";
        case MovementError::InvalidSpeed: return "InvalidSpeed";
    }
    return "UnknownMovementError";
}

template <typename T, typename E = MovementError>
using MovementResult = std::expected<T, E>;

struct Vector3 {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};

    constexpr bool operator==(const Vector3& other) const noexcept {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct MovementState {
    Vector3 position;
    Vector3 velocity;
    float rotation{0.0f};
    float speed{0.0f};
    bool isMoving{false};
};

class MovementDomainService {
public:
    using EntityId = uint32_t;

    MovementDomainService() noexcept = default;

    MovementResult<void> RegisterEntity(EntityId id, const MovementState& initialState) noexcept {
        m_states[id] = initialState;
        return {};
    }

    MovementResult<void> UnregisterEntity(EntityId id) noexcept {
        if (!m_states.contains(id)) {
            return std::unexpected(MovementError::EntityNotFound);
        }
        m_states.erase(id);
        return {};
    }

    MovementResult<void> SetVelocity(EntityId id, const Vector3& velocity, float speed) noexcept {
        if (!m_states.contains(id)) {
            return std::unexpected(MovementError::EntityNotFound);
        }
        if (speed < 0.0f) {
            return std::unexpected(MovementError::InvalidSpeed);
        }
        
        auto& state = m_states[id];
        state.velocity = velocity;
        state.speed = speed;
        state.isMoving = (speed > 0.0f && (velocity.x != 0.0f || velocity.y != 0.0f || velocity.z != 0.0f));
        
        return {};
    }

    MovementResult<void> SetRotation(EntityId id, float rotation) noexcept {
        if (!m_states.contains(id)) {
            return std::unexpected(MovementError::EntityNotFound);
        }
        m_states[id].rotation = rotation;
        return {};
    }

    MovementResult<void> UpdatePosition(EntityId id, float deltaTime) noexcept {
        if (!m_states.contains(id)) {
            return std::unexpected(MovementError::EntityNotFound);
        }
        if (deltaTime <= 0.0f) {
            return std::unexpected(MovementError::InvalidDeltaTime);
        }

        auto& state = m_states[id];
        if (!state.isMoving) {
            return {};
        }

        // Prosta interpolacja liniowa: pozycja = pozycja + wektor * predkosc * czas
        state.position.x += state.velocity.x * state.speed * deltaTime;
        state.position.y += state.velocity.y * state.speed * deltaTime;
        state.position.z += state.velocity.z * state.speed * deltaTime;

        return {};
    }

    MovementResult<void> StopEntities(std::span<const EntityId> ids) noexcept {
        bool anyNotFound = false;
        for (const auto& id : ids) {
            if (m_states.contains(id)) {
                auto& state = m_states[id];
                state.velocity = {0.0f, 0.0f, 0.0f};
                state.speed = 0.0f;
                state.isMoving = false;
            } else {
                anyNotFound = true;
            }
        }
        
        if (anyNotFound) {
            return std::unexpected(MovementError::EntityNotFound);
        }
        return {};
    }

    [[nodiscard]] std::optional<MovementState> GetState(EntityId id) const noexcept {
        auto it = m_states.find(id);
        if (it != m_states.end()) {
            return it->second;
        }
        return std::nullopt;
    }

private:
    std::unordered_map<EntityId, MovementState> m_states;
};

} // namespace Client::Domain
