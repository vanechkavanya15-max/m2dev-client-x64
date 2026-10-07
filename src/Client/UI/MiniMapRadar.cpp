#include <unordered_map>
#include "MiniMapRadar.h"
#include <algorithm>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d9.h>
#include "../../EterLib/StateManager.h"
#else
// Mock for Linux testing environment
typedef unsigned long DWORD;
enum D3DRENDERSTATETYPE {
    D3DRS_TEXTUREFACTOR = 60,
    D3DRS_ALPHABLENDENABLE = 27
};
class CStateManager {
public:
    void SaveRenderState(D3DRENDERSTATETYPE type, DWORD val) {
        m_states[type] = val;
    }
    void RestoreRenderState(D3DRENDERSTATETYPE type) { }
    void SetRenderState(D3DRENDERSTATETYPE type, DWORD val) { m_states[type] = val; }
private:
    std::unordered_map<int, DWORD> m_states;
};
#endif

namespace Client::UI {

MiniMapRadar::MiniMapRadar() = default;

void MiniMapRadar::SetCenterPosition(float x, float y) noexcept {
    m_centerX = x;
    m_centerY = y;
}

void MiniMapRadar::SetRadius(float radius) noexcept {
    m_radius = std::max(0.0f, radius);
}

void MiniMapRadar::SetScale(float scale) noexcept {
    m_scale = std::max(0.001f, scale); // Avoid division by zero
}

bool MiniMapRadar::ProjectToRadar(float worldX, float worldY, float& outLocalX, float& outLocalY) const noexcept {
    float diffX = (worldX - m_centerX) / m_scale;
    float diffY = (worldY - m_centerY) / m_scale;
    
    float distanceSq = diffX * diffX + diffY * diffY;
    float radiusSq = m_radius * m_radius;
    
    if (distanceSq <= radiusSq) {
        outLocalX = diffX;
        outLocalY = diffY;
        return true; // Wewnatrz okregu
    }
    
    // Punkt lezy poza okregiem, rzutujemy na krawedz
    float distance = std::sqrt(distanceSq);
    if (distance > 0.0f) {
        float ratio = m_radius / distance;
        outLocalX = diffX * ratio;
        outLocalY = diffY * ratio;
    } else {
        outLocalX = 0.0f;
        outLocalY = 0.0f;
    }
    
    return false; // Przyciety do krawedzi
}

void MiniMapRadar::AddMark(const RadarMark& mark) {
    m_marks.push_back(mark);
}

void MiniMapRadar::ClearMarks() noexcept {
    m_marks.clear();
}

void MiniMapRadar::RenderMarks(CStateManager* stateManager, float screenX, float screenY) const {
    if (!stateManager) {
        return;
    }

#ifdef _WIN32
    // Zabezpieczenie przed brudnymi stanami D3DRS_TEXTUREFACTOR
    stateManager->SaveRenderState(D3DRS_TEXTUREFACTOR, 0xFFFFFFFF);
    stateManager->SaveRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
#endif

    for (const auto& mark : m_marks) {
        float localX = 0.0f;
        float localY = 0.0f;
        
        ProjectToRadar(mark.worldX, mark.worldY, localX, localY);
        
        uint32_t color = GetMarkColor(mark.type);
        
        // Obliczenie pozycji na ekranie
        float finalRenderX = screenX + localX;
        float finalRenderY = screenY + localY;
        
        // Zabezpiecz kolor
#ifdef _WIN32
        stateManager->SetRenderState(D3DRS_TEXTUREFACTOR, color);
        // [TUTAJ RENDEROWANIE MARKA] e.g., RenderImage(...)
        // W tym zadaniu symulujemy zabezpieczenie stanow.
#endif
    }

#ifdef _WIN32
    stateManager->RestoreRenderState(D3DRS_TEXTUREFACTOR);
    stateManager->RestoreRenderState(D3DRS_ALPHABLENDENABLE);
#endif
}

} // namespace Client::UI
