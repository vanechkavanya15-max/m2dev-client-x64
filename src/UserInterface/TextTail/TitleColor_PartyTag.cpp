#include "../StdAfx.h"
#include "ITitleNameColorizer.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include "../Domain/PartyContainerModel.h"

#include <expected>
#include <string>
#include <format>
#include <string_view>

namespace UserInterface::TextTail
{
    /**
     * @brief Singleton or standalone class to handle Party Tag colors and formatting.
     * Decouples Party roles from text tail UI rendering.
     */
    struct TextTailPartyTagUpdateEvent : public Core::IEvent
    {
        uint32_t entityId;
        std::string tag;
        uint32_t color;

        TextTailPartyTagUpdateEvent(uint32_t id, std::string t, uint32_t c)
            : entityId(id), tag(std::move(t)), color(c) {}
    };

    class PartyTagColorizer final : public ITitleNameColorizer
    {
    public:
        uint32_t GetAlignmentColor(int32_t alignment) const override { return 0xFFFFFFFF; }
        uint32_t GetEmpireColor(uint8_t empire) const override { return 0xFFFFFFFF; }
        uint32_t GetLevelColor(int32_t playerLevel, int32_t mobLevel) const override { return 0xFFFFFFFF; }
        std::string FormatGuildName(std::string_view guildName) const override { return std::string(guildName); }
        std::string FormatAlignmentTitle(int32_t alignment) const override { return ""; }
        void Clear() override {}

        uint32_t GetPartyRoleColor(uint8_t role) const
        {
            return GetRoleColor(static_cast<Domain::PartyRole>(role)).value_or(0xFFFFFFFF);
        }
        
        std::string FormatPartyRoleTag(uint8_t role) const
        {
            return FormatRoleTag(static_cast<Domain::PartyRole>(role)).value_or("");
        }

        /**
         * @brief Returns the color for a specific party role.
         * @param role The party role.
         * @return The color as a 32-bit ARGB value.
         */
        static std::expected<uint32_t, std::string_view> GetRoleColor(Domain::PartyRole role)
        {
            switch (role)
            {
                case Domain::PartyRole::Normal:
                    return 0xFFFFFFFF; // White
                case Domain::PartyRole::Leader:
                    return 0xFFFFD700; // Gold
                case Domain::PartyRole::Attacker:
                    return 0xFFFF4500; // OrangeRed
                case Domain::PartyRole::Defender:
                    return 0xFF4682B4; // SteelBlue
                case Domain::PartyRole::Buffer:
                    return 0xFF32CD32; // LimeGreen
                case Domain::PartyRole::SkillMaster:
                    return 0xFF9370DB; // MediumPurple
                default:
                    return std::unexpected("Unknown PartyRole");
            }
        }

        /**
         * @brief Formats the role tag string.
         * @param role The party role.
         * @return The formatted role tag (e.g. "[Leader]").
         */
        static std::expected<std::string, std::string_view> FormatRoleTag(Domain::PartyRole role)
        {
            switch (role)
            {
                case Domain::PartyRole::Normal:
                    return std::string("");
                case Domain::PartyRole::Leader:
                    return std::string("[Leader]");
                case Domain::PartyRole::Attacker:
                    return std::string("[Attacker]");
                case Domain::PartyRole::Defender:
                    return std::string("[Defender]");
                case Domain::PartyRole::Buffer:
                    return std::string("[Buffer]");
                case Domain::PartyRole::SkillMaster:
                    return std::string("[Skill Master]");
                default:
                    return std::unexpected("Unknown PartyRole");
            }
        }

        /**
         * @brief Notifies the UI layer that a text tail needs its party tag updated.
         * @param entityId The entity ID of the party member.
         * @param role The new role of the member.
         * @return PacketResult indicating success or failure.
         */
        static EterBase::PacketResult<void> NotifyTagUpdate(EterBase::EntityId entityId, Domain::PartyRole role)
        {
            auto colorResult = GetRoleColor(role);
            if (!colorResult.has_value()) {
                EterBase::ModernLogger::Error("Failed to get color for role: {}", colorResult.error());
                return std::unexpected(EterBase::PacketError::MalformedPayload);
            }

            auto tagResult = FormatRoleTag(role);
            if (!tagResult.has_value()) {
                EterBase::ModernLogger::Error("Failed to format tag for role: {}", tagResult.error());
                return std::unexpected(EterBase::PacketError::MalformedPayload);
            }

            Core::EventBus::GetInstance().Publish(TextTailPartyTagUpdateEvent(
                entityId.value(),
                tagResult.value(),
                colorResult.value()
            ));

            return {};
        }
    };

    ITitleNameColorizer* CreatePartyTagColorizer()
    {
        return new PartyTagColorizer();
    }
}
