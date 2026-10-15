#include "../../StdAfx.h"
#include "ISIMDFrustumCuller.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"
#include <span>
#include <expected>

namespace UserInterface::ECS::SIMD
{
    /**
     * @brief Implementation of ISIMDFrustumCuller focusing on packing visible indices.
     */
    class SIMDCullerResultPackImpl final : public ISIMDFrustumCuller
    {
    public:
        SIMDCullerResultPackImpl() = default;
        ~SIMDCullerResultPackImpl() override = default;

        void SetFrustum(const FrustumPlanes& planes) override
        {
            EterBase::ModernLogger::Debug("SIMDCullerResultPackImpl: SetFrustum called (not implemented in ResultPack module)");
        }

        void BatchCullAABB(
            const float* minX, const float* minY, const float* minZ,
            const float* maxX, const float* maxY, const float* maxZ,
            uint8_t* outVisibleMask, size_t count) override
        {
            EterBase::ModernLogger::Debug("SIMDCullerResultPackImpl: BatchCullAABB called (not implemented in ResultPack module)");
        }

        /**
         * @brief Packs indices of visible entities into a linear buffer.
         * @param visibleMask Array of 8-bit masks indicating visibility (non-zero = visible).
         * @param count Total number of elements in the mask array.
         * @param outIndices Output buffer for the visible indices.
         * @return The number of visible entities.
         */
        size_t PackVisibleIndices(
            const uint8_t* visibleMask, size_t count, uint32_t* outIndices) override
        {
            auto result = PackVisibleIndicesSafe(visibleMask, count, outIndices);
            if (!result.has_value())
            {
                EterBase::ModernLogger::Error("SIMDCullerResultPackImpl: Pack failed with error");
                return 0;
            }
            return result.value();
        }

        void Clear() override
        {
            EterBase::ModernLogger::Debug("SIMDCullerResultPackImpl: Clear called");
        }

    private:
        std::expected<size_t, EterBase::EntityError> PackVisibleIndicesSafe(
            const uint8_t* visibleMask, size_t count, uint32_t* outIndices)
        {
            if (!visibleMask || !outIndices)
            {
                return std::unexpected(EterBase::EntityError::NotFound);
            }

            if (count == 0)
            {
                return 0;
            }

            std::span<const uint8_t> maskSpan(visibleMask, count);
            std::span<uint32_t> outSpan(outIndices, count);

            size_t visibleCount = 0;

            // Packing loop: Extract indices of non-zero mask entries.
            // This loop focuses strictly on math calculation and packing, maximizing auto-vectorization potential.
            for (size_t i = 0; i < maskSpan.size(); ++i)
            {
                if (maskSpan[i] != 0)
                {
                    outSpan[visibleCount++] = static_cast<uint32_t>(i);
                }
            }

            EterBase::ModernLogger::Info("SIMDCullerResultPackImpl: Packed {} visible entities out of {} total", visibleCount, count);

            // Notify other subsystems using the EventBus
            ::UserInterface::Core::EventBus::GetInstance().Publish(::UserInterface::Core::SIMDCullingCompletedEvent{count, visibleCount});

            return visibleCount;
        }
    };
    
    // According to standard C++ practices, provide a factory/exposure function so this file isn't dead code
    extern "C" ISIMDFrustumCuller* CreateSIMDCullerResultPack() {
        return new SIMDCullerResultPackImpl();
    }
}
