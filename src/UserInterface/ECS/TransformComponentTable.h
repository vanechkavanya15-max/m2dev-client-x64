#pragma once

#include <vector>
#include <cstdint>
#include <unordered_map>
#include "EterBase/StrongTypes.h"

namespace UserInterface::ECS
{
    /**
     * @brief Struktura SoA (Structure of Arrays) dla pozycji i rotacji encji w swiecie gry.
     * Wyrownana do wektoryzacji SIMD / AVX2.
     */
    class TransformComponentTable
    {
    public:
        std::vector<uint32_t> entityIds;
        std::vector<float> posX;
        std::vector<float> posY;
        std::vector<float> posZ;
        std::vector<float> rotation;
        std::vector<float> targetX;
        std::vector<float> targetY;
        std::vector<float> velocity;

        void Reserve(size_t capacity)
        {
            entityIds.reserve(capacity);
            posX.reserve(capacity);
            posY.reserve(capacity);
            posZ.reserve(capacity);
            rotation.reserve(capacity);
            targetX.reserve(capacity);
            targetY.reserve(capacity);
            velocity.reserve(capacity);
        }

        size_t Size() const noexcept { return entityIds.size(); }
        void Clear() noexcept
        {
            entityIds.clear();
            posX.clear();
            posY.clear();
            posZ.clear();
            rotation.clear();
            targetX.clear();
            targetY.clear();
            velocity.clear();
            idToIndexMap_.clear();
        }

        void AddOrUpdate(uint32_t id, float x, float y, float z, float rot, float speed)
        {
            auto it = idToIndexMap_.find(id);
            if (it != idToIndexMap_.end())
            {
                size_t idx = it->second;
                posX[idx] = x;
                posY[idx] = y;
                posZ[idx] = z;
                rotation[idx] = rot;
                velocity[idx] = speed;
                return;
            }

            size_t newIdx = entityIds.size();
            entityIds.push_back(id);
            posX.push_back(x);
            posY.push_back(y);
            posZ.push_back(z);
            rotation.push_back(rot);
            targetX.push_back(x);
            targetY.push_back(y);
            velocity.push_back(speed);
            idToIndexMap_[id] = newIdx;
        }

        void Remove(uint32_t id)
        {
            auto it = idToIndexMap_.find(id);
            if (it == idToIndexMap_.end())
                return;

            size_t idx = it->second;
            size_t lastIdx = entityIds.size() - 1;

            if (idx != lastIdx)
            {
                uint32_t lastId = entityIds[lastIdx];
                entityIds[idx] = entityIds[lastIdx];
                posX[idx] = posX[lastIdx];
                posY[idx] = posY[lastIdx];
                posZ[idx] = posZ[lastIdx];
                rotation[idx] = rotation[lastIdx];
                targetX[idx] = targetX[lastIdx];
                targetY[idx] = targetY[lastIdx];
                velocity[idx] = velocity[lastIdx];
                idToIndexMap_[lastId] = idx;
            }

            entityIds.pop_back();
            posX.pop_back();
            posY.pop_back();
            posZ.pop_back();
            rotation.pop_back();
            targetX.pop_back();
            targetY.pop_back();
            velocity.pop_back();
            idToIndexMap_.erase(it);
        }

    private:
        std::unordered_map<uint32_t, size_t> idToIndexMap_;
    };
}
