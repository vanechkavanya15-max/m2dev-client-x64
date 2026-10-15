/**
 * @file SamplerAddressScope.h
 * @brief Defines the SamplerAddressScope RAII guard for DirectX 9 sampler address states.
 *
 * This file provides a modern C++23 mechanism to manage sampler address modes
 * (U, V, and W) in a scoped, exception-safe manner, eliminating the need for
 * manual state restoration and preventing leaked states.
 */

#pragma once

#ifndef ETERLIB_RENDER_SAMPLER_ADDRESS_SCOPE_H
#define ETERLIB_RENDER_SAMPLER_ADDRESS_SCOPE_H

#include <d3d9.h>
#include <cstdint>

// Include the StateManager to access the STATEMANAGER global macro or instance
// which handles Direct3D device state changes in an optimized caching layer.
#include "../StateManager.h"

namespace EterLib
{
namespace Render
{

/**
 * @class SamplerAddressScope
 * @brief An RAII class that temporarily changes the Direct3D sampler address modes and restores them upon destruction.
 *
 * The SamplerAddressScope manages the D3DSAMP_ADDRESSU, D3DSAMP_ADDRESSV,
 * and D3DSAMP_ADDRESSW states for a specified sampler stage. It saves the 
 * current states upon construction, sets the newly requested states, and 
 * guarantees that the original states are restored when the object goes out 
 * of scope, even in the event of an early exit.
 *
 * This architecture ensures that rendering pipelines do not inadvertently 
 * carry over addressing modes (such as Wrap or Clamp) into subsequent 
 * drawing operations, avoiding graphical artifacts.
 *
 * Usage example:
 * @code
 * {
 *     // Sets the sampler address modes for stage 0 to CLAMP.
 *     auto scope = EterLib::Render::SamplerAddressScope::Clamp(0);
 *     
 *     // Draw calls here will use CLAMP addressing...
 *     
 * } // Original addressing modes are automatically restored here.
 * @endcode
 */
class SamplerAddressScope
{
public:
    /**
     * @brief Constructs a new Sampler Address Scope and applies the specified address mode uniformly.
     * 
     * @param stage The sampler stage index (e.g., 0 for the first texture stage).
     * @param addressMode The D3DTEXTUREADDRESS mode to apply to U, V, and W axes.
     */
    SamplerAddressScope(uint32_t stage, D3DTEXTUREADDRESS addressMode)
        : m_stage(stage)
        , m_savedU(0)
        , m_savedV(0)
        , m_savedW(0)
        , m_isStateSaved(false)
    {
        SaveCurrentState();
        ApplyNewState(addressMode, addressMode, addressMode);
    }

    /**
     * @brief Constructs a new Sampler Address Scope and applies distinct address modes for each axis.
     * 
     * @param stage The sampler stage index.
     * @param addressU The D3DTEXTUREADDRESS mode for the U axis.
     * @param addressV The D3DTEXTUREADDRESS mode for the V axis.
     * @param addressW The D3DTEXTUREADDRESS mode for the W axis.
     */
    SamplerAddressScope(uint32_t stage, D3DTEXTUREADDRESS addressU, D3DTEXTUREADDRESS addressV, D3DTEXTUREADDRESS addressW)
        : m_stage(stage)
        , m_savedU(0)
        , m_savedV(0)
        , m_savedW(0)
        , m_isStateSaved(false)
    {
        SaveCurrentState();
        ApplyNewState(addressU, addressV, addressW);
    }

    /**
     * @brief Destroys the Sampler Address Scope, restoring the original address modes.
     *
     * This destructor ensures that the D3DSAMP_ADDRESSU, D3DSAMP_ADDRESSV, 
     * and D3DSAMP_ADDRESSW states are returned to the values they had prior 
     * to the instantiation of this scope.
     */
    ~SamplerAddressScope()
    {
        RestorePreviousState();
    }

    // Delete copy constructor and copy assignment operator to prevent state aliasing
    // and double-restoration of states upon destruction.
    SamplerAddressScope(const SamplerAddressScope&) = delete;
    SamplerAddressScope& operator=(const SamplerAddressScope&) = delete;

    // Delete move constructor and move assignment operator.
    // In C++17 onward, copy elision ensures we can return by value from factories
    // without invoking move or copy constructors.
    SamplerAddressScope(SamplerAddressScope&&) = delete;
    SamplerAddressScope& operator=(SamplerAddressScope&&) = delete;

    /**
     * @brief Factory method to create a scope with D3DTADDRESS_WRAP on all axes.
     * 
     * @param stage The sampler stage to apply the WRAP mode to.
     * @return A SamplerAddressScope instance managing the Wrap state.
     */
    [[nodiscard]] static SamplerAddressScope Wrap(uint32_t stage)
    {
        return SamplerAddressScope(stage, D3DTADDRESS_WRAP);
    }

    /**
     * @brief Factory method to create a scope with D3DTADDRESS_CLAMP on all axes.
     * 
     * @param stage The sampler stage to apply the CLAMP mode to.
     * @return A SamplerAddressScope instance managing the Clamp state.
     */
    [[nodiscard]] static SamplerAddressScope Clamp(uint32_t stage)
    {
        return SamplerAddressScope(stage, D3DTADDRESS_CLAMP);
    }

    /**
     * @brief Factory method to create a scope with D3DTADDRESS_BORDER on all axes.
     * 
     * @param stage The sampler stage to apply the BORDER mode to.
     * @return A SamplerAddressScope instance managing the Border state.
     */
    [[nodiscard]] static SamplerAddressScope Border(uint32_t stage)
    {
        return SamplerAddressScope(stage, D3DTADDRESS_BORDER);
    }

private:
    /**
     * @brief Saves the current sampler address states for U, V, and W axes.
     * 
     * Reads the current values from the STATEMANAGER and stores them
     * internally so they can be accurately restored later. This relies
     * on the efficiency of the underlying state manager's retrieval mechanisms.
     */
    void SaveCurrentState()
    {
        DWORD valU = 0;
        DWORD valV = 0;
        DWORD valW = 0;
        
        STATEMANAGER.GetSamplerState(m_stage, D3DSAMP_ADDRESSU, &valU);
        STATEMANAGER.GetSamplerState(m_stage, D3DSAMP_ADDRESSV, &valV);
        STATEMANAGER.GetSamplerState(m_stage, D3DSAMP_ADDRESSW, &valW);
        
        m_savedU = valU;
        m_savedV = valV;
        m_savedW = valW;
        
        m_isStateSaved = true;
    }

    /**
     * @brief Applies the new sampler address modes for each specific axis.
     * 
     * @param addressU The D3DTEXTUREADDRESS mode to set for the U axis.
     * @param addressV The D3DTEXTUREADDRESS mode to set for the V axis.
     * @param addressW The D3DTEXTUREADDRESS mode to set for the W axis.
     */
    void ApplyNewState(D3DTEXTUREADDRESS addressU, D3DTEXTUREADDRESS addressV, D3DTEXTUREADDRESS addressW) const
    {
        STATEMANAGER.SetSamplerState(m_stage, D3DSAMP_ADDRESSU, static_cast<DWORD>(addressU));
        STATEMANAGER.SetSamplerState(m_stage, D3DSAMP_ADDRESSV, static_cast<DWORD>(addressV));
        STATEMANAGER.SetSamplerState(m_stage, D3DSAMP_ADDRESSW, static_cast<DWORD>(addressW));
    }

    /**
     * @brief Restores the previously saved sampler address states.
     * 
     * Writes the stored values back to the STATEMANAGER for U, V, and W axes.
     * This operation is only performed if the states were successfully saved
     * during the construction of this scope.
     */
    void RestorePreviousState() const
    {
        if (m_isStateSaved)
        {
            STATEMANAGER.SetSamplerState(m_stage, D3DSAMP_ADDRESSU, m_savedU);
            STATEMANAGER.SetSamplerState(m_stage, D3DSAMP_ADDRESSV, m_savedV);
            STATEMANAGER.SetSamplerState(m_stage, D3DSAMP_ADDRESSW, m_savedW);
        }
    }

private:
    /// The sampler stage index this scope is actively managing.
    uint32_t m_stage;
    
    /// The saved internal state value of the D3DSAMP_ADDRESSU parameter.
    uint32_t m_savedU;
    
    /// The saved internal state value of the D3DSAMP_ADDRESSV parameter.
    uint32_t m_savedV;
    
    /// The saved internal state value of the D3DSAMP_ADDRESSW parameter.
    uint32_t m_savedW;
    
    /// A boolean flag indicating whether the original states have been saved.
    bool m_isStateSaved;
};

} // namespace Render
} // namespace EterLib

#endif // ETERLIB_RENDER_SAMPLER_ADDRESS_SCOPE_H
