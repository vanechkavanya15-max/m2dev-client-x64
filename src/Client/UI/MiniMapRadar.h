#pragma once

#include <cstdint>
#include <vector>
#include <cmath>
#include <optional>

// Forward declaration for D3D StateManager (to avoid full inclusion)
class CStateManager;

namespace Client::UI {

enum class RadarMarkType : uint8_t {
    NPC = 0,
    Party,
    Warp,
    Mob,
    Waypoint,
    Count
};

struct RadarMark {
    float worldX;
    float worldY;
    RadarMarkType type;
    uint32_t id;
};

class MiniMapRadar {
public:
    MiniMapRadar();
    ~MiniMapRadar() = default;

    /**
     * @brief Zwraca statycznie gwarantowany kolor (alfa = 1.0f / 0xFF) dla znacznikow.
     * Zabezpiecza przed niewidocznymi kropkami.
     */
    static constexpr uint32_t GetMarkColor(RadarMarkType type) noexcept {
        switch (type) {
            case RadarMarkType::NPC:      return 0xFF7AE75D; // NPC = 0xFF7AE75D
            case RadarMarkType::Party:    return 0xFF3388FF;
            case RadarMarkType::Warp:     return 0xFFCC33CC;
            case RadarMarkType::Mob:      return 0xFFFF3333;
            case RadarMarkType::Waypoint: return 0xFFFFFF00;
            default:                      return 0xFFFFFFFF;
        }
    }

    void SetCenterPosition(float x, float y) noexcept;
    void SetRadius(float radius) noexcept;
    void SetScale(float scale) noexcept;
    
    [[nodiscard]] float GetRadius() const noexcept { return m_radius; }
    [[nodiscard]] float GetScale() const noexcept { return m_scale; }
    [[nodiscard]] float GetCenterX() const noexcept { return m_centerX; }
    [[nodiscard]] float GetCenterY() const noexcept { return m_centerY; }

    /**
     * @brief Projekcja wspolrzednych swiata na okrag radarowy.
     * Zwraca lokalne (wzgledem srodka) pozycje X i Y.
     * Jezeli punkt lezy poza okregiem, zostanie rzutowany na krawedz.
     * Zwraca true jesli punkt byl wewnatrz, false jesli na krawedzi (zostal przyciety).
     */
    bool ProjectToRadar(float worldX, float worldY, float& outLocalX, float& outLocalY) const noexcept;

    void AddMark(const RadarMark& mark);
    void ClearMarks() noexcept;

    const std::vector<RadarMark>& GetMarks() const noexcept { return m_marks; }

    /**
     * @brief Renderuje znaczniki radarowe.
     * Zabezpiecza stany D3D przed brudnymi ustawieniami TEXTUREFACTOR.
     */
    void RenderMarks(CStateManager* stateManager, float screenX, float screenY) const;

private:
    float m_centerX{0.0f};
    float m_centerY{0.0f};
    float m_radius{100.0f};
    float m_scale{1.0f};
    std::vector<RadarMark> m_marks;
};

} // namespace Client::UI
