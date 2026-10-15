#pragma once

#include <d3d9.h>
#include <EterBase/Stl.h>
#include <EterBase/LogModern.h>
#include <stdexcept>
#include <format>


namespace EterLib::Render
{
    /**
     * @class VertexBufferStreamScope
     * @brief A modern C++23 RAII scope guard for managing Direct3D 9 vertex buffer stream sources.
     *
     * This class adheres strictly to the zero-conflict rule by being an isolated header.
     * It safely manages the binding of a new vertex buffer to a specific stream index,
     * while remembering the previous buffer, offset, and stride. Upon destruction, it
     * automatically restores the previous state and correctly releases the reference to the
     * previous buffer using safe_release from EterBase/Stl.h.
     * 
     * Key features:
     * - C++23 design and RAII semantics.
     * - Automatic resource management via safe_release.
     * - Bounds checking on stream indices (must be between 0 and 15 inclusive).
     * - Logging of stream state changes using EterBase::ModernLogger.
     * - Exception safety and no raw `SAFE_RELEASE` macros.
     */
    class VertexBufferStreamScope
    {
    public:
        /**
         * @brief Constructs the scope and applies the new stream source.
         *
         * @param pDevice The Direct3D 9 device.
         * @param streamNumber The stream index (0 to 15).
         * @param pNewBuffer The new vertex buffer to bind.
         * @param offsetInBytes Offset from the start of the vertex buffer.
         * @param stride Stride of the vertex data.
         *
         * @throws std::invalid_argument if pDevice is null or streamNumber is out of bounds.
         */
        VertexBufferStreamScope(
            LPDIRECT3DDEVICE9EX pDevice,
            UINT streamNumber,
            LPDIRECT3DVERTEXBUFFER9 pNewBuffer,
            UINT offsetInBytes,
            UINT stride)
            : m_pDevice(pDevice),
              m_streamNumber(streamNumber),
              m_pOldBuffer(nullptr),
              m_oldOffset(0),
              m_oldStride(0),
              m_active(false)
        {
            if (!m_pDevice)
            {
                EterBase::ModernLogger::Error("VertexBufferStreamScope: Device is null.");
                throw std::invalid_argument("pDevice cannot be null");
            }

            if (m_streamNumber > 15)
            {
                EterBase::ModernLogger::Error(std::format("VertexBufferStreamScope: Invalid stream number {}.", m_streamNumber));
                throw std::invalid_argument("streamNumber must be between 0 and 15");
            }

            // Retrieve the current stream source to restore later
            HRESULT hr = m_pDevice->GetStreamSource(m_streamNumber, &m_pOldBuffer, &m_oldOffset, &m_oldStride);
            if (FAILED(hr))
            {
                EterBase::ModernLogger::Error(std::format("VertexBufferStreamScope: GetStreamSource failed for stream {}. HRESULT: {}.", m_streamNumber, static_cast<unsigned int>(hr)));
                // We don't throw here to allow partial degradation, but we log the error.
                m_pOldBuffer = nullptr;
                m_oldOffset = 0;
                m_oldStride = 0;
            }

            // Apply the new stream source
            hr = m_pDevice->SetStreamSource(m_streamNumber, pNewBuffer, offsetInBytes, stride);
            if (FAILED(hr))
            {
                EterBase::ModernLogger::Error(std::format("VertexBufferStreamScope: SetStreamSource failed for stream {}. HRESULT: {}.", m_streamNumber, static_cast<unsigned int>(hr)));
                m_active = false;
            }
            else
            {
                m_active = true;
            }
        }

        /**
         * @brief Destroys the scope, restoring the previous stream source.
         */
        ~VertexBufferStreamScope()
        {
            Restore();
        }

        // Prevent copying to maintain strict RAII semantics.
        VertexBufferStreamScope(const VertexBufferStreamScope&) = delete;
        VertexBufferStreamScope& operator=(const VertexBufferStreamScope&) = delete;

        // Allow move semantics
        VertexBufferStreamScope(VertexBufferStreamScope&& other) noexcept
            : m_pDevice(other.m_pDevice),
              m_streamNumber(other.m_streamNumber),
              m_pOldBuffer(other.m_pOldBuffer),
              m_oldOffset(other.m_oldOffset),
              m_oldStride(other.m_oldStride),
              m_active(other.m_active)
        {
            other.m_pDevice = nullptr;
            other.m_pOldBuffer = nullptr;
            other.m_active = false;
        }

        VertexBufferStreamScope& operator=(VertexBufferStreamScope&& other) noexcept
        {
            if (this != &other)
            {
                Restore(); // Restore current state before moving

                m_pDevice = other.m_pDevice;
                m_streamNumber = other.m_streamNumber;
                m_pOldBuffer = other.m_pOldBuffer;
                m_oldOffset = other.m_oldOffset;
                m_oldStride = other.m_oldStride;
                m_active = other.m_active;

                other.m_pDevice = nullptr;
                other.m_pOldBuffer = nullptr;
                other.m_active = false;
            }
            return *this;
        }

        /**
         * @brief Manually restores the previous stream source before destruction.
         */
        void Restore()
        {
            if (!m_active || !m_pDevice)
            {
                // Note: Even if not active, we MUST release the old buffer if we got it!
                if (m_pOldBuffer)
                {
                    safe_release(m_pOldBuffer);
                    m_pOldBuffer = nullptr;
                }
                return;
            }

            HRESULT hr = m_pDevice->SetStreamSource(m_streamNumber, m_pOldBuffer, m_oldOffset, m_oldStride);
            if (FAILED(hr))
            {
                EterBase::ModernLogger::Error(std::format("VertexBufferStreamScope: Failed to restore stream {}. HRESULT: {}.", m_streamNumber, static_cast<unsigned int>(hr)));
            }

            if (m_pOldBuffer)
            {
                // Release the reference obtained by GetStreamSource
                safe_release(m_pOldBuffer);
                m_pOldBuffer = nullptr;
            }

            m_active = false;
        }

        /**
         * @brief Checks if the scope is currently active and managing a state change.
         * @return True if active, false otherwise.
         */
        [[nodiscard]] bool IsActive() const noexcept
        {
            return m_active;
        }

        /**
         * @brief Retrieves the stored stream number.
         * @return The stream number being managed.
         */
        [[nodiscard]] UINT GetStreamNumber() const noexcept
        {
            return m_streamNumber;
        }

    private:
        LPDIRECT3DDEVICE9EX m_pDevice;          ///< The Direct3D 9 device.
        UINT m_streamNumber;                    ///< The stream index being managed.
        LPDIRECT3DVERTEXBUFFER9 m_pOldBuffer;   ///< The previous vertex buffer.
        UINT m_oldOffset;                       ///< The previous stream offset.
        UINT m_oldStride;                       ///< The previous stream stride.
        bool m_active;                          ///< Flag indicating if the scope is active.
    };

} // namespace EterLib::Render
