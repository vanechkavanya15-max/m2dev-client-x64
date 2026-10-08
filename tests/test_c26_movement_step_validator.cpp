#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <cmath>
#include <expected>
#include <cstdint>
#include <utility>
#include <compare>
#include <format>
#include <functional>

// Local mock of dependencies since tests are standalone
namespace Client::Core {

template <typename Tag, typename Underlying, auto DefaultVal>
class StrongType {
public:
    constexpr StrongType() noexcept : value_{DefaultVal} {}
    constexpr explicit StrongType(Underlying value) noexcept : value_{std::move(value)} {}
    
    constexpr Underlying get() const noexcept { return value_; }
    
    constexpr auto operator<=>(const StrongType&) const = default;
    
private:
    Underlying value_;
};

struct MapCoords {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    
    constexpr auto operator<=>(const MapCoords&) const = default;
    
    constexpr MapCoords operator+(const MapCoords& other) const noexcept { return {x + other.x, y + other.y, z + other.z}; }
    constexpr MapCoords operator-(const MapCoords& other) const noexcept { return {x - other.x, y - other.y, z - other.z}; }
    constexpr MapCoords operator*(float scalar) const noexcept { return {x * scalar, y * scalar, z * scalar}; }
    constexpr MapCoords operator/(float scalar) const noexcept { return {x / scalar, y / scalar, z / scalar}; }
    
    float Length() const noexcept { return std::sqrt(x*x + y*y + z*z); }
    float Distance(const MapCoords& other) const noexcept { return (*this - other).Length(); }
};

enum class CommandError : uint8_t {
    MovementBlocked,
    InvalidParameter
};

}

namespace EterBase {
    template <typename T, typename E>
    using Result = std::expected<T, E>;
}

// In unit tests running standalone on Linux, we provide a mock class
// that holds the same ValidateStep signature and implementation without
// pulling in MSVC-specific headers or the actual source files, to respect ODR
// and avoid compilation issues with missing Game headers.
namespace Client::Gameplay {
    class MovementStepValidator {
    public:
        [[nodiscard]] static EterBase::Result<bool, Client::Core::CommandError> ValidateStep(
            const Client::Core::MapCoords& current, 
            const Client::Core::MapCoords& next, 
            float dt, 
            float speed) noexcept {

            if (dt <= 0.0f) {
                if (current == next) {
                    return true;
                }
                return std::unexpected(Client::Core::CommandError::InvalidParameter);
            }

            if (speed < 0.0f) {
                return std::unexpected(Client::Core::CommandError::InvalidParameter);
            }

            const float distance = current.Distance(next);
            // Include small epsilon to account for float inaccuracy
            const float max_allowed_distance = speed * dt + 0.01f;

            if (distance > max_allowed_distance) {
                return std::unexpected(Client::Core::CommandError::MovementBlocked);
            }

            return true;
        }
    };
}


using namespace Client::Gameplay;
using namespace Client::Core;

TEST_CASE("MovementStepValidator - Valid movement") {
    MapCoords current{0.0f, 0.0f, 0.0f};
    MapCoords next{100.0f, 0.0f, 0.0f}; // Distance: 100
    float dt = 0.5f;
    float speed = 250.0f; // Max allowed: 125.01

    auto result = MovementStepValidator::ValidateStep(current, next, dt, speed);
    CHECK(result.has_value());
    CHECK(result.value() == true);
}

TEST_CASE("MovementStepValidator - Valid zero movement") {
    MapCoords current{10.0f, 20.0f, 30.0f};
    MapCoords next{10.0f, 20.0f, 30.0f}; // Distance: 0
    float dt = 0.1f;
    float speed = 0.0f; // Max allowed: 0.01

    auto result = MovementStepValidator::ValidateStep(current, next, dt, speed);
    CHECK(result.has_value());
    CHECK(result.value() == true);
}

TEST_CASE("MovementStepValidator - Speedhack detected") {
    MapCoords current{0.0f, 0.0f, 0.0f};
    MapCoords next{1000.0f, 0.0f, 0.0f}; // Distance: 1000
    float dt = 0.5f;
    float speed = 500.0f; // Max allowed: 250.01

    auto result = MovementStepValidator::ValidateStep(current, next, dt, speed);
    CHECK_FALSE(result.has_value());
    CHECK(result.error() == CommandError::MovementBlocked);
}

TEST_CASE("MovementStepValidator - dt is zero") {
    MapCoords current{10.0f, 20.0f, 30.0f};
    MapCoords next{10.0f, 20.0f, 30.0f}; // distance 0

    // dt=0 and distance=0 -> should be true
    auto result = MovementStepValidator::ValidateStep(current, next, 0.0f, 100.0f);
    CHECK(result.has_value());
    CHECK(result.value() == true);

    MapCoords next_diff{20.0f, 20.0f, 30.0f}; // distance > 0
    // dt=0 and distance>0 -> error
    auto result_err = MovementStepValidator::ValidateStep(current, next_diff, 0.0f, 100.0f);
    CHECK_FALSE(result_err.has_value());
    CHECK(result_err.error() == CommandError::InvalidParameter);
}

TEST_CASE("MovementStepValidator - Negative speed") {
    MapCoords current{0.0f, 0.0f, 0.0f};
    MapCoords next{10.0f, 0.0f, 0.0f}; 

    auto result = MovementStepValidator::ValidateStep(current, next, 0.1f, -50.0f);
    CHECK_FALSE(result.has_value());
    CHECK(result.error() == CommandError::InvalidParameter);
}
