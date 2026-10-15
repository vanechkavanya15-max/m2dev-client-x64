#pragma once

#include <cstdint>
#include <memory>
#include <functional>
#include <d3dx9.h>

#include "GameLib/RaceData.h"
#include "GameLib/ActorInstance.h"
#include "GameLib/ItemData.h"

namespace UserInterface::InstanceComponents {

/**
 * @brief Komponent zarzadzajacy stanem wierzchowca/konia, aktorem wierzchowca,
 * animacjami dosiadu, poziomem wierzchowca oraz uprawnieniami do walki i skilli.
 *
 * Wydzielony z monolitu CInstanceBase (wczesniej zagniezdzone SHORSE w InstanceBaseMount.cpp).
 */
class InstanceMountComponent {
public:
    InstanceMountComponent();
    ~InstanceMountComponent();

    InstanceMountComponent(const InstanceMountComponent&) = delete;
    InstanceMountComponent& operator=(const InstanceMountComponent&) = delete;

    InstanceMountComponent(InstanceMountComponent&& other) noexcept;
    InstanceMountComponent& operator=(InstanceMountComponent&& other) noexcept;

    // Tworzenie i niszczenie instancji wierzchowca
    void Create(const TPixelPosition& pos, uint32_t mountRace, uint32_t hitEffect);
    bool Mount(CActorInstance& riderActor, const TPixelPosition& pos, uint32_t mountRace, uint32_t hitEffect);
    void Dismount(CActorInstance& riderActor);
    void Destroy();
    void Clear();

    // Rysowanie i deformacja
    void Deform();
    void Render();
    void UpdateMotion();

    // Siodlo (przyczepianie/odczepianie modelu do glownego aktora)
    void AttachSaddle(CActorInstance& riderActor);
    void DetachSaddle(CActorInstance& riderActor);

    // Predkosci
    void SetAttackSpeed(uint32_t atkSpd);
    void SetMoveSpeed(uint32_t movSpd);

    // Stan i uprawnienia
    [[nodiscard]] bool IsMounting() const noexcept { return m_isMounting; }
    [[nodiscard]] uint32_t GetMountRace() const noexcept { return m_mountRace; }
    [[nodiscard]] uint32_t GetLevel() const noexcept;
    [[nodiscard]] bool IsNewMount() const noexcept;
    [[nodiscard]] bool CanAttack() const noexcept;
    [[nodiscard]] bool CanUseSkill() const noexcept;

    // Dostep do instancji aktora wierzchowca
    [[nodiscard]] CActorInstance* GetActorPtr() noexcept { return m_pkActor; }
    [[nodiscard]] const CActorInstance* GetActorPtr() const noexcept { return m_pkActor; }
    [[nodiscard]] CActorInstance& GetActorRef();
    [[nodiscard]] const CActorInstance& GetActorRef() const;

    // Czyste reguly domenowe (funkcje statyczne bez efektow ubocznych)
    [[nodiscard]] static uint32_t CalculateMountLevel(uint32_t mountRace) noexcept;
    [[nodiscard]] static bool IsNewMountRace(uint32_t mountRace) noexcept;
    [[nodiscard]] static int GetHorseMotionMode(uint8_t weaponSubType) noexcept;

    // Wstrzykiwalny sink zdarzen (decoupling od singletonu EventBus)
    using MountStateCallback = std::function<void(uint32_t mountRace, uint8_t state)>;
    void SetStateCallback(MountStateCallback cb) { m_stateCallback = std::move(cb); }

private:
    void Initialize();

    bool m_isMounting{false};
    uint32_t m_mountRace{0};
    CActorInstance* m_pkActor{nullptr};
    MountStateCallback m_stateCallback;
};

} // namespace UserInterface::InstanceComponents
