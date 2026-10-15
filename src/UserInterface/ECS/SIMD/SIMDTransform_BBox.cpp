#include "../../StdAfx.h"
#include "ISIMDTransformEngine.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include <cmath>
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

namespace UserInterface::Core {

/**
 * @brief Event emitted when a SIMD batch transform is completed.
 * Defined locally in the .cpp to adhere to Zero-Conflict rules.
 */
struct SIMDTransformBatchCompletedEvent : public IEvent {
    size_t processedCount;
    
    explicit SIMDTransformBatchCompletedEvent(size_t count) : processedCount(count) {}
};

/**
 * @brief Event emitted when AABB transformations are completed.
 * Defined locally in the .cpp to adhere to Zero-Conflict rules.
 */
struct SIMDAABBTransformCompletedEvent : public IEvent {
    size_t processedCount;
    
    explicit SIMDAABBTransformCompletedEvent(size_t count) : processedCount(count) {}
};

} // namespace UserInterface::Core

namespace UserInterface::ECS::SIMD {

/**
 * @brief Implementation of ISIMDTransformEngine for bounding box transformations.
 */
class SIMDTransformEngine : public ISIMDTransformEngine {
public:
    SIMDTransformEngine() {
        EterBase::ModernLogger::Info("SIMDTransformEngine initialized.");
    }
    
    ~SIMDTransformEngine() override = default;

    void BatchVectorAddMul(
        const float* posX, const float* posY, const float* posZ,
        const float* velX, const float* velY, const float* velZ,
        float dt, float* outX, float* outY, float* outZ, size_t count) override 
    {
        if (!posX || !posY || !posZ || !velX || !velY || !velZ || !outX || !outY || !outZ) {
            EterBase::ModernLogger::Error("SIMDTransformEngine: Null pointer provided to BatchVectorAddMul.");
            return;
        }

        for (size_t i = 0; i < count; ++i) {
            outX[i] = posX[i] + (velX[i] * dt);
            outY[i] = posY[i] + (velY[i] * dt);
            outZ[i] = posZ[i] + (velZ[i] * dt);
        }
        
        EterBase::ModernLogger::Debug("SIMDTransformEngine: Processed {} vectors in BatchVectorAddMul.", count);
        
        Core::SIMDTransformBatchCompletedEvent event(count);
        Core::EventBus::GetInstance().Publish(event);
    }

    void BatchDistanceCheck(
        const float* x1, const float* y1,
        float targetX, float targetY,
        float* outDistances, size_t count) override 
    {
        if (!x1 || !y1 || !outDistances) {
            EterBase::ModernLogger::Error("SIMDTransformEngine: Null pointer provided to BatchDistanceCheck.");
            return;
        }

        for (size_t i = 0; i < count; ++i) {
            float dx = x1[i] - targetX;
            float dy = y1[i] - targetY;
            outDistances[i] = std::sqrt(dx * dx + dy * dy);
        }
        
        EterBase::ModernLogger::Debug("SIMDTransformEngine: Processed {} vectors in BatchDistanceCheck.", count);
    }

    void Clear() override {
        EterBase::ModernLogger::Debug("SIMDTransformEngine: Clear called.");
    }
};

/**
 * @brief Factory function to create an instance of SIMDTransformEngine.
 */
std::unique_ptr<ISIMDTransformEngine> CreateSIMDTransformEngine() {
    return std::make_unique<SIMDTransformEngine>();
}

/**
 * @brief Free function providing vectorized AABB transformations in world space.
 * 
 * Implemented as a free function to satisfy the Zero-Conflict rule (avoiding modification of the shared interface),
 * while keeping the logic globally accessible via forward declaration.
 */
std::expected<void, EterBase::EntityError> TransformAABBsWorldSpace(
    std::span<const EterBase::EntityId> entityIds,
    std::span<const float> minX, std::span<const float> minY, std::span<const float> minZ,
    std::span<const float> maxX, std::span<const float> maxY, std::span<const float> maxZ,
    std::span<const float> transX, std::span<const float> transY, std::span<const float> transZ,
    std::span<float> outMinX, std::span<float> outMinY, std::span<float> outMinZ,
    std::span<float> outMaxX, std::span<float> outMaxY, std::span<float> outMaxZ)
{
    size_t count = entityIds.size();

    if (minX.size() != count || minY.size() != count || minZ.size() != count ||
        maxX.size() != count || maxY.size() != count || maxZ.size() != count ||
        transX.size() != count || transY.size() != count || transZ.size() != count ||
        outMinX.size() != count || outMinY.size() != count || outMinZ.size() != count ||
        outMaxX.size() != count || outMaxY.size() != count || outMaxZ.size() != count) 
    {
        EterBase::ModernLogger::Error("TransformAABBsWorldSpace: Span size mismatch.");
        return std::unexpected(EterBase::EntityError::OutOfRange);
    }

    for (size_t i = 0; i < count; ++i) {
        outMinX[i] = minX[i] + transX[i];
        outMinY[i] = minY[i] + transY[i];
        outMinZ[i] = minZ[i] + transZ[i];

        outMaxX[i] = maxX[i] + transX[i];
        outMaxY[i] = maxY[i] + transY[i];
        outMaxZ[i] = maxZ[i] + transZ[i];
    }

    EterBase::ModernLogger::Debug("TransformAABBsWorldSpace: Translated {} AABBs.", count);
    
    Core::SIMDAABBTransformCompletedEvent event(count);
    Core::EventBus::GetInstance().Publish(event);

    return {};
}

} // namespace UserInterface::ECS::SIMD
