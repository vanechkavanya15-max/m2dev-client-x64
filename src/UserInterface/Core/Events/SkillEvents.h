#pragma once

#include <cstdint>
#include <string_view>
#include <expected>
#include <optional>
#include <format>
#include <string>

#include "EterBase/StrongTypes.h"
#include "../EventBus.h"

namespace UserInterface::Core::Events {

/**
 * @brief Zdarzenie emitowane, gdy stan umiejetnosci przelaczanej ulega zmianie.
 */
struct SkillToggleStateChanged : public IEvent {
    EterBase::SkillId skillId;
    bool isActived;

private:
    SkillToggleStateChanged(EterBase::SkillId id, bool active)
        : skillId(id), isActived(active) {}

public:
    [[nodiscard]] std::string ToString() const {
        return std::format("SkillToggleStateChanged: Skill [{}] is now {}", 
            skillId.value(), isActived ? "Active" : "Inactive");
    }

    static std::expected<SkillToggleStateChanged, std::string_view> Create(
        EterBase::SkillId id, bool active) 
    {
        if (!id) return std::unexpected("Invalid skill ID.");
        return SkillToggleStateChanged(id, active);
    }
};

} // namespace UserInterface::Core::Events
