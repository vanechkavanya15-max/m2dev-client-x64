#pragma once

#include <vector>
#include <mutex>
#include <string_view>
#include <d3d9.h>
#include "EterBase/Result.h"

namespace Graphics
{
    /**
     * @brief Window modes supported by the rendering context.
     */
    enum class WindowMode
    {
        Windowed,
        Fullscreen,
        Borderless
    };

    /**
     * @brief Configuration block for initializing the rendering context.
     */
    struct RenderContextConfig
    {
        HWND hWindow;
        int width;
        int height;
        WindowMode mode;
        bool vSync;
        bool useSoftwareVertexProcessing;
    };

    /**
     * @brief Interface for receiving device loss and reset notifications.
     */
    class IDeviceResetListener
    {
    public:
        virtual ~IDeviceResetListener() = default;

        /**
         * @brief Called when the Direct3D device is lost, prior to reset.
         * Resources in D3DPOOL_DEFAULT should be released here.
         */
        virtual void OnDeviceLost() = 0;

        /**
         * @brief Called after the Direct3D device has been successfully reset.
         * Resources in D3DPOOL_DEFAULT should be recreated here.
         */
        virtual void OnDeviceReset() = 0;
    };

    /**
     * @brief Manages the Direct3D9 device lifecycle, presentation, and recovery state machine.
     */
    class RenderContext
    {
    public:
        RenderContext();
        ~RenderContext();

        /**
         * @brief Initializes the Direct3D device.
         * @param config The rendering configuration.
         * @return EterBase::VoidResult<> on success, or an error.
         */
        EterBase::VoidResult<std::string_view> Initialize(const RenderContextConfig& config);

        /**
         * @brief Presents the rendered image to the screen. Handles device loss recovery state machine.
         * @return true if successful or recovering, false on unrecoverable failure.
         */
        bool Present();

        /**
         * @brief Registers a listener for device lost/reset events.
         * @param pListener The listener to register.
         */
        void RegisterListener(IDeviceResetListener* pListener);

        /**
         * @brief Unregisters a listener.
         * @param pListener The listener to unregister.
         */
        void UnregisterListener(IDeviceResetListener* pListener);

        /**
         * @brief Gets the initialized Direct3D device.
         * @return The IDirect3DDevice9 pointer.
         */
        IDirect3DDevice9* GetDevice() const { return m_pDevice; }

    private:
        /**
         * @brief Builds the presentation parameters based on the current config.
         * @param outParams The parameters to populate.
         */
        void BuildPresentParameters(D3DPRESENT_PARAMETERS& outParams) const;

        /**
         * @brief Handles the device lost recovery state machine.
         * @return true if recovery is complete, false if still lost or failed.
         */
        bool HandleDeviceLost();

        IDirect3D9* m_pD3D;
        IDirect3DDevice9* m_pDevice;
        RenderContextConfig m_config;
        bool m_deviceLost;

        std::vector<IDeviceResetListener*> m_listeners;
        std::mutex m_listenerMutex;
    };
}
