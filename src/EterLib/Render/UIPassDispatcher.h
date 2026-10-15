#pragma once

#include "../StdAfx.h"
#include <EterBase/LogModern.h>

namespace EterLib::Render {

/**
 * @brief Dispatcher for 2D UI Rendering Pass in Metin2 C++23.
 * 
 * Configures the Direct3D device to disable Z-buffer checks and writes,
 * disables culling and lighting, and enforces strict Z-order based on painter's algorithm.
 */
class UIPassDispatcher {
public:
    UIPassDispatcher() = default;
    ~UIPassDispatcher() = default;

    // Disallow copy/move
    UIPassDispatcher(const UIPassDispatcher&) = delete;
    UIPassDispatcher& operator=(const UIPassDispatcher&) = delete;

    /**
     * @brief Begins the UI rendering pass, overriding pipeline states.
     * @param dev Direct3D device (not strictly needed since we use CStateManager, but kept for signature compatibility)
     */
    void BeginPass(LPDIRECT3DDEVICE9 dev) noexcept;

    /**
     * @brief Ends the UI rendering pass, restoring the saved pipeline states.
     * @param dev Direct3D device
     */
    void EndPass(LPDIRECT3DDEVICE9 dev) noexcept;
};

} // namespace EterLib::Render

