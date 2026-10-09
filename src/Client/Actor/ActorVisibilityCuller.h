#pragma once

#include <array>
#include <expected>
#include <span>
#include <cstdint>

namespace Client::Actor {

    // Struct representing a 3D vector.
    struct Vec3 { 
        float x;
        float y;
        float z; 
    };

    // Struct representing a plane in 3D space.
    struct Plane { 
        float a;
        float b;
        float c;
        float d; 
    };

    // Enum representing visibility errors.
    enum class VisibilityError {
        InvalidData
    };

    // C++23 Zero-Conflict Implementation of ActorVisibilityCuller
    // Wstrzymywanie obliczen animacji szkieletowej dla bytow poza polem widzenia kamery.
    class ActorVisibilityCuller {
    public:
        ActorVisibilityCuller() noexcept = default;
        ~ActorVisibilityCuller() noexcept = default;

        // Disallow copy to enforce single instance per subsystem.
        // Move semantics are explicitly defaulted.
        ActorVisibilityCuller(const ActorVisibilityCuller&) = delete;
        ActorVisibilityCuller& operator=(const ActorVisibilityCuller&) = delete;
        ActorVisibilityCuller(ActorVisibilityCuller&&) noexcept = default;
        ActorVisibilityCuller& operator=(ActorVisibilityCuller&&) noexcept = default;

        // Aktualizuje plaszczyzny frustum
        void UpdateFrustum(std::span<const Plane, 6> frustumPlanes) noexcept {
            for (size_t i = 0; i < 6; ++i) {
                m_frustumPlanes[i] = frustumPlanes[i];
            }
            m_hasFrustum = true;
        }
        
        // Sprawdza czy aktor jest widoczny w oparciu o jego pozycje i promien
        [[nodiscard]] std::expected<bool, VisibilityError> IsVisible(const Vec3& position, float boundingRadius) const noexcept {
            if (!m_hasFrustum) {
                return std::unexpected(VisibilityError::InvalidData);
            }

            if (boundingRadius < 0.0f) {
                return std::unexpected(VisibilityError::InvalidData);
            }

            for (const auto& plane : m_frustumPlanes) {
                float distance = plane.a * position.x + plane.b * position.y + plane.c * position.z + plane.d;
                if (distance < -boundingRadius) {
                    return false; // Aktor poza frustum (niewidoczny)
                }
            }

            return true; // Aktor przecina sie lub jest w srodku frustum
        }

    private:
        std::array<Plane, 6> m_frustumPlanes{};
        bool m_hasFrustum{false};
    };

} // namespace Client::Actor
