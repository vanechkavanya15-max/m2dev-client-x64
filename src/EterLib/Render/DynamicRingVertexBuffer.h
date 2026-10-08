#pragma once

#include <d3d9.h>
#include <EterBase/Stl.h>
#include <EterBase/LogModern.h>
#include <cstdint>

namespace EterLib::Render
{
    /**
     * @class DynamicRingVertexBuffer
     * @brief A modern C++23 16MB ring buffer for dynamic vertices to avoid CPU-GPU locks.
     * 
     * Uses D3DLOCK_NOOVERWRITE when there is enough space and D3DLOCK_DISCARD when
     * a wrap-around is needed.
     */
    class DynamicRingVertexBuffer
    {
    public:
        DynamicRingVertexBuffer() = default;
        ~DynamicRingVertexBuffer();

        // Prevent copying
        DynamicRingVertexBuffer(const DynamicRingVertexBuffer&) = delete;
        DynamicRingVertexBuffer& operator=(const DynamicRingVertexBuffer&) = delete;

        // Allow moving
        DynamicRingVertexBuffer(DynamicRingVertexBuffer&& other) noexcept;
        DynamicRingVertexBuffer& operator=(DynamicRingVertexBuffer&& other) noexcept;

        /**
         * @brief Initializes the dynamic vertex buffer.
         * @param dev The Direct3D device.
         * @param sizeBytes The size of the ring buffer in bytes.
         * @return true on success, false on failure.
         */
        bool Initialize(LPDIRECT3DDEVICE9 dev, size_t sizeBytes);

        /**
         * @brief Allocates space in the ring buffer, copies the data, and returns the byte offset.
         * @param sizeBytes Size of the data to copy.
         * @param data Pointer to the vertex data.
         * @param stride Size of a single vertex, used for proper offset alignment.
         * @return The offset in bytes where the data was copied. Returns 0xFFFFFFFF on failure.
         */
        uint32_t Allocate(size_t sizeBytes, const void* data, uint32_t stride);

        /**
         * @brief Returns the underlying Direct3D vertex buffer.
         */
        [[nodiscard]] LPDIRECT3DVERTEXBUFFER9 GetBuffer() const noexcept;

        /**
         * @brief Resets the ring buffer state (e.g. on device lost).
         */
        void Reset() noexcept;

    private:
        LPDIRECT3DDEVICE9 m_device{nullptr};
        LPDIRECT3DVERTEXBUFFER9 m_buffer{nullptr};
        size_t m_sizeBytes{0};
        size_t m_currentOffset{0};
    };

} // namespace EterLib::Render

