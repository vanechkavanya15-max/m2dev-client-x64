#pragma once

#include <vector>
#include <algorithm>
#include <functional>
#include "InstanceBase.h"

namespace UserInterface {

/**
 * @brief Wydzielony menedzer sceny graficznej instancji postaci.
 * 
 * Odpowiada za zarzadzanie instancjami na scenie (zywe i martwe),
 * sortowanie wedlug odleglosci od kamery (dla przezroczystosci i minimalizacji overdraw),
 * renderowanie cieni oraz geometrii kolizji.
 */
class InstanceSceneManager
{
public:
    InstanceSceneManager() = default;
    ~InstanceSceneManager() = default;

    InstanceSceneManager(const InstanceSceneManager&) = delete;
    InstanceSceneManager& operator=(const InstanceSceneManager&) = delete;

    // Zarzadzanie instancjami w scenie
    void AddAliveInstance(CInstanceBase* pkInst);
    void RemoveAliveInstance(CInstanceBase* pkInst);
    void AddDeadInstance(CInstanceBase* pkInst);
    void RemoveDeadInstance(CInstanceBase* pkInst);
    void MoveToDead(CInstanceBase* pkInst);

    void Clear();
    void ClearAlive();
    void ClearDead(std::function<void(DWORD)> onInstanceDeleted = nullptr);

    // Petle renderowania i przejscia graficzne
    void Render();
    void RenderShadowMainInstance(CInstanceBase* pkInstMain);
    void RenderShadowAllInstances();
    void RenderCollision();
    void Deform();
    void UpdateDeleting(std::function<void(DWORD)> onInstanceDeleted = nullptr);

    // Sortowanie instancji pod katem kamery
    void SortAliveInstances(const D3DXVECTOR3& cameraEye);
    void SortDeadInstances(const D3DXVECTOR3& cameraEye);

    // Gettery i zapytania
    [[nodiscard]] size_t GetAliveCount() const noexcept { return m_aliveInstances.size(); }
    [[nodiscard]] size_t GetDeadCount() const noexcept { return m_deadInstances.size(); }
    [[nodiscard]] const std::vector<CInstanceBase*>& GetAliveInstances() const noexcept { return m_aliveInstances; }
    [[nodiscard]] const std::vector<CInstanceBase*>& GetDeadInstances() const noexcept { return m_deadInstances; }
    [[nodiscard]] bool IsDead(DWORD vid) const;
    [[nodiscard]] bool IsAlive(DWORD vid) const;

private:
    void RenderSortedAliveActorList();
    void RenderSortedDeadActorList();

private:
    std::vector<CInstanceBase*> m_aliveInstances;
    std::vector<CInstanceBase*> m_deadInstances;
};

} // namespace UserInterface

using InstanceSceneManager = UserInterface::InstanceSceneManager;
using CInstanceSceneManager = UserInterface::InstanceSceneManager;
