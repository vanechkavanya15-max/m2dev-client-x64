#pragma once

#include <cstdint>
#include <optional>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::Services
{
    /**
     * @brief Informacja o pojedynczej umiejetnosci gracza.
     */
    struct PlayerSkillView
    {
        EterBase::SkillId skillId{0};
        uint8_t level{0};
        uint8_t masterType{0};
        float cooltimeRemaining{0.0f};
        float totalCooltime{0.0f};
        bool isActived{false};
    };

    /**
     * @brief Interfejs mikro-serwisu obslugi umiejetnosci gracza.
     */
    class ISkillService
    {
    public:
        virtual ~ISkillService() = default;

        virtual void SetSkillLevel(EterBase::SkillId id, uint8_t level, uint8_t masterType) = 0;
        virtual void SetSkillCooltime(EterBase::SkillId id, float duration) = 0;
        virtual bool IsSkillCooltime(EterBase::SkillId id) const = 0;
        virtual float GetSkillCooltimeRemaining(EterBase::SkillId id) const = 0;
        virtual std::optional<PlayerSkillView> GetSkill(EterBase::SkillId id) const = 0;
        virtual void UpdateCooltimes(float deltaTime) = 0;
        virtual void Clear() = 0;
    };
}
