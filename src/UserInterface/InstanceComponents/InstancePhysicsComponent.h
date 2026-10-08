#pragma once

#include <cstdint>
#include <d3dx9.h>

#include "GameLib/ActorInstance.h"
#include "EterBase/Result.h"

namespace UserInterface::InstanceComponents {

/**
 * @brief Komponent odpowiedzialny za polozenie, rotacje, interpolacje ruchu i kolizje instancji.
 * 
 * Wydzielony z CInstanceBase w celu odciecia mechaniki transformacji i fizyki od logiki D3D.
 */
class InstancePhysicsComponent {
public:
    explicit InstancePhysicsComponent(CActorInstance& actorInstance);
    ~InstancePhysicsComponent() = default;

    InstancePhysicsComponent(const InstancePhysicsComponent&) = delete;
    InstancePhysicsComponent& operator=(const InstancePhysicsComponent&) = delete;

    // Pozycja i przemieszczenie
    void SetPixelPosition(const TPixelPosition& pos);
    [[nodiscard]] const TPixelPosition& GetPixelPosition() const noexcept;
    [[nodiscard]] TPixelPosition& GetPixelPositionRef() noexcept;

    // Rotacja i kierunek
    void SetRotation(float rotation);
    [[nodiscard]] float GetRotation() const noexcept { return m_rotation; }
    void SetTargetRotation(float targetRotation);
    [[nodiscard]] float GetTargetRotation() const noexcept { return m_targetRotation; }
    void SetRotationSpeed(float speed) noexcept { m_rotationSpeed = speed; }
    [[nodiscard]] float GetRotationSpeed() const noexcept { return m_rotationSpeed; }

    // Predkosc ruchu
    void SetMoveSpeed(uint32_t speed) noexcept { m_moveSpeed = speed; }
    [[nodiscard]] uint32_t GetMoveSpeed() const noexcept { return m_moveSpeed; }

    // Obliczenia dystansu
    [[nodiscard]] float CalculateDistanceSq3d(const TPixelPosition& targetPos) const noexcept;
    [[nodiscard]] float CalculateDistance2d(const TPixelPosition& targetPos) const noexcept;

    // Aktualizacja klatkowa interpolacji
    void UpdateMovement();

    // Reset stanu fizycznego
    void Clear();

private:
    CActorInstance& m_actorInstance;

    TPixelPosition m_pixelPosition{0.0f, 0.0f, 0.0f};
    float m_rotation{0.0f};
    float m_targetRotation{0.0f};
    float m_rotationSpeed{24.0f};
    uint32_t m_moveSpeed{100};
};

} // namespace UserInterface::InstanceComponents
