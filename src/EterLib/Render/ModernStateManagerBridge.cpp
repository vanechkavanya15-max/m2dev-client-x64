#include "../StdAfx.h"
#include "ModernStateManagerBridge.h"
#include "../StateManager.h"

namespace EterLib::Render {

ModernStateManagerBridge& ModernStateManagerBridge::Instance() {
    static ModernStateManagerBridge s_instance;
    return s_instance;
}

void ModernStateManagerBridge::BeginFrame() {
    m_stateSwitches = 0;
}

void ModernStateManagerBridge::EndFrame() {
    // End of frame logic, e.g. verifying no state leaks
}

void ModernStateManagerBridge::SetRenderState(D3DRENDERSTATETYPE state, DWORD value) {
    if (static_cast<size_t>(state) >= 256) {
        STATEMANAGER.SetRenderState(state, value);
        return;
    }

    DWORD currentValue = 0;
    STATEMANAGER.GetRenderState(state, &currentValue);
    
    if (currentValue != value) {
        STATEMANAGER.SetRenderState(state, value);
        m_stateSwitches++;
    }
}

void ModernStateManagerBridge::SaveRenderState(D3DRENDERSTATETYPE state, DWORD value) {
    if (static_cast<size_t>(state) >= 256) {
        STATEMANAGER.SaveRenderState(state, value);
        return;
    }

    DWORD currentValue = 0;
    STATEMANAGER.GetRenderState(state, &currentValue);

    if (m_stacks[state].Push(currentValue)) {
        m_activeStates.Set(state);
        SetRenderState(state, value);
    } else {
        STATEMANAGER.SaveRenderState(state, value);
    }
}

void ModernStateManagerBridge::RestoreRenderState(D3DRENDERSTATETYPE state) {
    if (static_cast<size_t>(state) >= 256) {
        STATEMANAGER.RestoreRenderState(state);
        return;
    }

    auto poppedValue = m_stacks[state].Pop();
    if (poppedValue.has_value()) {
        SetRenderState(state, poppedValue.value());
        
        if (m_stacks[state].IsEmpty()) {
            m_activeStates.Clear(state);
        }
    } else {
        STATEMANAGER.RestoreRenderState(state);
    }
}

size_t ModernStateManagerBridge::GetStateSwitchCount() const noexcept {
    return m_stateSwitches;
}

size_t ModernStateManagerBridge::GetActiveStateCount() const noexcept {
    return m_activeStates.Count();
}

bool ModernStateManagerBridge::IsStateActive(D3DRENDERSTATETYPE state) const noexcept {
    if (static_cast<size_t>(state) >= 256) {
        return false;
    }
    return m_activeStates.Test(state);
}

void ModernStateManagerBridge::Reset() {
    for (auto& stack : m_stacks) {
        stack.Clear();
    }
    m_activeStates.ClearAll();
    m_stateSwitches = 0;
}

} // namespace EterLib::Render
