#include "../StdAfx.h"
#include "ITitleNameColorizer.h"
#include "EterBase/LogModern.h"
#include "EterBase/StrongTypes.h"
#include "Core/EventBus.h"

#include <expected>
#include <format>
#include <string>
#include <cstdint>

namespace UserInterface::TextTail
{
    // Define an event for level badge colorization as required by architecture
    struct LevelBadgeColorizedEvent : public Core::IEvent
    {
        int32_t diff;
        uint32_t color;

        LevelBadgeColorizedEvent(int32_t diff, uint32_t color) : diff(diff), color(color) {}
    };

    class TitleColorLevelBadge final : public ITitleNameColorizer
    {
    public:
        TitleColorLevelBadge()
        {
            EterBase::ModernLogger::Info("TitleColorLevelBadge initialized");
        }

        ~TitleColorLevelBadge() override = default;

        [[nodiscard]] uint32_t GetAlignmentColor(int32_t alignment) const override
        {
            return 0xFFFFFFFF; // White
        }

        [[nodiscard]] uint32_t GetEmpireColor(uint8_t empire) const override
        {
            return 0xFFFFFFFF; // White
        }

        [[nodiscard]] uint32_t GetLevelColor(int32_t playerLevel, int32_t mobLevel) const override
        {
            const int32_t diff = playerLevel - mobLevel;
            uint32_t color = 0xFFFFFFFF; // Default

            if (diff >= 8)
                color = 0xFFCCCCCC; // Light Gray (Mob is much weaker)
            else if (diff >= 3)
                color = 0xFF00FF00; // Green (Mob is weaker)
            else if (diff >= -2)
                color = 0xFFFFFFFF; // White (Mob is equal)
            else if (diff >= -7)
                color = 0xFFFF8C00; // Dark Orange (Mob is stronger)
            else
                color = 0xFFFF0000; // Red (Mob is much stronger)

            // Publish event to decouple from GUI
            Core::EventBus::GetInstance().Publish(LevelBadgeColorizedEvent(diff, color));
            return color;
        }

        // Implementation of getting formatted level string using expected as required
        [[nodiscard]] std::expected<std::string, std::string> FormatLevelBadge(int32_t level) const
        {
            if (level <= 0)
            {
                return std::unexpected("Invalid level");
            }
            return std::format("[Lv. {}]", level);
        }

        [[nodiscard]] std::string FormatGuildName(std::string_view guildName) const override
        {
            return std::format("[{}]", guildName);
        }

        [[nodiscard]] std::string FormatAlignmentTitle(int32_t alignment) const override
        {
            return ""; 
        }

        void Clear() override
        {
            EterBase::ModernLogger::Debug("TitleColorLevelBadge cleared");
        }
    };

    // Factory function to create the instance 
    std::unique_ptr<ITitleNameColorizer> CreateTitleNameColorizer_LevelBadge()
    {
        return std::make_unique<TitleColorLevelBadge>();
    }
}
