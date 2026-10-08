#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <d3dx9.h>

#include "GameLib/RaceData.h"
#include "GameLib/ActorInstance.h"

namespace UserInterface::InstanceComponents {

/**
 * @brief Komponent zarzadzajacy wygladem, czesciami ciala, bronia, ksztaltem, fryzura i efektami.
 */
class InstanceVisualComponent {
public:
    explicit InstanceVisualComponent(CActorInstance& actorInstance);
    ~InstanceVisualComponent() = default;

    InstanceVisualComponent(const InstanceVisualComponent&) = delete;
    InstanceVisualComponent& operator=(const InstanceVisualComponent&) = delete;

    // Czesci ciala i modele
    bool SetArmor(uint32_t shapeIndex, float specular = 0.0f);
    bool SetWeapon(uint32_t weaponVnum);
    bool SetHair(uint32_t hairVnum);
    void SetShape(uint32_t shapeIndex, float specular = 0.0f);

    [[nodiscard]] uint32_t GetPart(uint32_t partIndex) const noexcept;
    [[nodiscard]] uint32_t GetArmor() const noexcept { return m_armorShape; }
    [[nodiscard]] uint32_t GetWeapon() const noexcept { return m_weaponVnum; }
    [[nodiscard]] uint32_t GetHair() const noexcept { return m_hairVnum; }
    [[nodiscard]] uint32_t GetShape() const noexcept { return m_armorShape; }

    // Efekty specjalne i czasteczkowe
    uint32_t AttachSpecialEffect(uint32_t effectIndex);
    void DetachSpecialEffect(uint32_t effectId);
    void ClearAllEffects();

    // TextTail (etykiety imienia, gildii i rangi)
    void AttachTextTail(uint32_t guildId, uint32_t vid, float height = 10.0f);
    void DetachTextTail(uint32_t vid);
    void RefreshTextTail();
    void UpdateTextTailLevel(uint32_t vid, std::string_view levelText);

    // Przezroczystosc i widocznosc
    void SetAlpha(float alpha);
    [[nodiscard]] float GetAlpha() const noexcept { return m_alpha; }
    void SetVisible(bool isVisible);
    [[nodiscard]] bool IsVisible() const noexcept { return m_isVisible; }

    void Clear();

private:
    CActorInstance& m_actorInstance;

    uint32_t m_armorShape{0};
    uint32_t m_weaponVnum{0};
    uint32_t m_hairVnum{0};

    float m_alpha{1.0f};
    bool m_isVisible{true};
    bool m_hasTextTail{false};
    uint32_t m_textTailVid{0};
};

} // namespace UserInterface::InstanceComponents
