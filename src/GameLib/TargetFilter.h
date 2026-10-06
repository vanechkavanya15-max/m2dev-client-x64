/**
 * @file TargetFilter.h
 * @brief Defines the target filtering logic for the Metin2 client.
 * 
 * This module provides the `Filter` class and related structures to filter a 
 * list of target actors based on various criteria (e.g., Closest, Lowest HP, Boss, Metin)
 * while maintaining decoupling from the GUI and higher-level application logic.
 */

#pragma once

#include <cstdint>
#include <span>
#include <functional>
#include <limits>
#include "ActorInstance.h"

namespace TargetFilter
{
    /**
     * @brief Represents the criteria used for filtering targets.
     */
    enum class FilterCriteria : uint8_t
    {
        Closest,    ///< The target closest to the source.
        LowestHP,   ///< The target with the lowest health points.
        Boss,       ///< The target which is a boss.
        Metin       ///< The target which is a metin stone.
    };

    /**
     * @brief Context structure for target filtering.
     * 
     * Since CActorInstance does not natively expose UI-level concepts like HP or rank,
     * this context allows callers (e.g., the UserInterface layer) to inject logic
     * via callbacks without creating circular dependencies.
     */
    struct FilterContext
    {
        /// @brief Callback to retrieve the current health points of a target.
        std::function<float(CActorInstance*)> getHpCallback;

        /// @brief Callback to determine if a target is considered a boss.
        std::function<bool(CActorInstance*)> isBossCallback;
    };

    /**
     * @brief A utility class for filtering a collection of CActorInstance objects based on specific criteria.
     */
    class Filter
    {
    public:
        /**
         * @brief Default constructor.
         */
        Filter() = default;

        /**
         * @brief Default destructor.
         */
        ~Filter() = default;

        /**
         * @brief Finds the best target among a list of targets based on the given criteria.
         *
         * @param source The actor instance initiating the filter.
         * @param targets A span of actor instances to filter from.
         * @param criteria The criteria to evaluate the targets.
         * @param context Additional context for retrieving external actor data (like HP, Boss status).
         * @return A pointer to the best CActorInstance, or nullptr if no valid target is found.
         */
        CActorInstance* FindTarget(CActorInstance* source, std::span<CActorInstance*> targets, FilterCriteria criteria, const FilterContext& context = {}) const
        {
            if (!source || targets.empty())
            {
                return nullptr;
            }

            switch (criteria)
            {
                case FilterCriteria::Closest:
                    return FindClosestTarget(source, targets);
                case FilterCriteria::LowestHP:
                    return FindLowestHPTarget(source, targets, context);
                case FilterCriteria::Boss:
                    return FindBossTarget(source, targets, context);
                case FilterCriteria::Metin:
                    return FindMetinTarget(source, targets);
                default:
                    return nullptr;
            }
        }

    private:
        /**
         * @brief Finds the closest target to the source.
         *
         * @param source The actor instance to calculate distance from.
         * @param targets A span of actor instances.
         * @return A pointer to the closest CActorInstance.
         */
        CActorInstance* FindClosestTarget(CActorInstance* source, std::span<CActorInstance*> targets) const
        {
            CActorInstance* closest = nullptr;
            float minDistanceSq = std::numeric_limits<float>::max();

            const auto& sourcePos = source->GetPositionVectorRef();

            for (auto* target : targets)
            {
                if (!target || target == source)
                    continue;

                const auto& targetPos = target->GetPositionVectorRef();
                float dx = sourcePos.x - targetPos.x;
                float dy = sourcePos.y - targetPos.y;
                float dz = sourcePos.z - targetPos.z;
                float distSq = dx * dx + dy * dy + dz * dz;

                if (distSq < minDistanceSq)
                {
                    minDistanceSq = distSq;
                    closest = target;
                }
            }
            return closest;
        }

        /**
         * @brief Finds the target with the lowest health points.
         *
         * @param source The source actor instance.
         * @param targets A span of actor instances.
         * @param context Context containing the callback to retrieve HP.
         * @return A pointer to the CActorInstance with the lowest HP.
         */
        CActorInstance* FindLowestHPTarget(CActorInstance* source, std::span<CActorInstance*> targets, const FilterContext& context) const
        {
            if (!context.getHpCallback)
            {
                return nullptr;
            }

            CActorInstance* bestTarget = nullptr;
            float minHp = std::numeric_limits<float>::max();

            for (auto* target : targets)
            {
                if (!target || target == source)
                    continue;

                float hp = context.getHpCallback(target);
                if (hp < minHp)
                {
                    minHp = hp;
                    bestTarget = target;
                }
            }

            return bestTarget;
        }

        /**
         * @brief Finds a boss target. If multiple bosses exist, returns the closest one.
         *
         * @param source The source actor instance.
         * @param targets A span of actor instances.
         * @param context Context containing the callback to check boss status.
         * @return A pointer to a boss CActorInstance.
         */
        CActorInstance* FindBossTarget(CActorInstance* source, std::span<CActorInstance*> targets, const FilterContext& context) const
        {
            if (!context.isBossCallback)
            {
                return nullptr;
            }

            CActorInstance* bestTarget = nullptr;
            float minDistanceSq = std::numeric_limits<float>::max();
            const auto& sourcePos = source->GetPositionVectorRef();

            for (auto* target : targets)
            {
                if (!target || target == source)
                    continue;

                if (context.isBossCallback(target))
                {
                    const auto& targetPos = target->GetPositionVectorRef();
                    float dx = sourcePos.x - targetPos.x;
                    float dy = sourcePos.y - targetPos.y;
                    float dz = sourcePos.z - targetPos.z;
                    float distSq = dx * dx + dy * dy + dz * dz;

                    if (distSq < minDistanceSq)
                    {
                        minDistanceSq = distSq;
                        bestTarget = target;
                    }
                }
            }

            return bestTarget;
        }

        /**
         * @brief Finds a metin stone target. If multiple metins exist, returns the closest one.
         *
         * @param source The source actor instance.
         * @param targets A span of actor instances.
         * @return A pointer to a metin CActorInstance.
         */
        CActorInstance* FindMetinTarget(CActorInstance* source, std::span<CActorInstance*> targets) const
        {
            CActorInstance* bestTarget = nullptr;
            float minDistanceSq = std::numeric_limits<float>::max();
            const auto& sourcePos = source->GetPositionVectorRef();

            for (auto* target : targets)
            {
                if (!target || target == source)
                    continue;

                // TYPE_STONE corresponds to Metin stones in CActorInstance::EType
                if (target->GetActorType() == CActorInstance::TYPE_STONE)
                {
                    const auto& targetPos = target->GetPositionVectorRef();
                    float dx = sourcePos.x - targetPos.x;
                    float dy = sourcePos.y - targetPos.y;
                    float dz = sourcePos.z - targetPos.z;
                    float distSq = dx * dx + dy * dy + dz * dz;

                    if (distSq < minDistanceSq)
                    {
                        minDistanceSq = distSq;
                        bestTarget = target;
                    }
                }
            }

            return bestTarget;
        }
    };
}
