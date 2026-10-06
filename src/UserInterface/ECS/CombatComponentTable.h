#pragma once

#include <vector>
#include <cstdint>
#include <unordered_map>

namespace UserInterface::ECS
{
    /**
     * @brief Struktura SoA (Structure of Arrays) dla parametrow walki i stanu HP encji.
     */
    class CombatComponentTable
    {
    public:
        std::vector<uint32_t> entityIds;
        std::vector<uint32_t> currentHp;
        std::vector<uint32_t> maxHp;
        std::vector<uint8_t>  battleState;
        std::vector<uint8_t>  isDead;

        void Reserve(size_t capacity)
        {
            entityIds.reserve(capacity);
            currentHp.reserve(capacity);
            maxHp.reserve(capacity);
            battleState.reserve(capacity);
            isDead.reserve(capacity);
        }

        size_t Size() const noexcept { return entityIds.size(); }
        void Clear() noexcept
        {
            entityIds.clear();
            currentHp.clear();
            maxHp.clear();
            battleState.clear();
            isDead.clear();
            idToIndexMap_.clear();
        }

        void AddOrUpdate(uint32_t id, uint32_t hp, uint32_t maxHpVal, uint8_t state, uint8_t dead)
        {
            auto it = idToIndexMap_.find(id);
            if (it != idToIndexMap_.end())
            {
                size_t idx = it->second;
                currentHp[idx] = hp;
                maxHp[idx] = maxHpVal;
                battleState[idx] = state;
                isDead[idx] = dead;
                return;
            }

            size_t newIdx = entityIds.size();
            entityIds.push_back(id);
            currentHp.push_back(hp);
            maxHp.push_back(maxHpVal);
            battleState.push_back(state);
            isDead.push_back(dead);
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
                currentHp[idx] = currentHp[lastIdx];
                maxHp[idx] = maxHp[lastIdx];
                battleState[idx] = battleState[lastIdx];
                isDead[idx] = isDead[lastIdx];
                idToIndexMap_[lastId] = idx;
            }

            entityIds.pop_back();
            currentHp.pop_back();
            maxHp.pop_back();
            battleState.pop_back();
            isDead.pop_back();
            idToIndexMap_.erase(it);
        }

    private:
        std::unordered_map<uint32_t, size_t> idToIndexMap_;
    };
}
