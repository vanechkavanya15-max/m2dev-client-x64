#include "../StdAfx.h"
#include "ITitleNameColorizer.h"
#include "../Core/EventBus.h"
#include "../../EterBase/LogModern.h"
#include <format>
#include <memory>

namespace UserInterface::TextTail
{
    struct AlignmentColorCalculatedEvent : public Core::IEvent
    {
        int32_t alignment;
        uint32_t color;

        AlignmentColorCalculatedEvent(int32_t align, uint32_t col)
            : alignment(align), color(col)
        {
        }
    };

    namespace
    {
        class TitleNameColorizer final : public ITitleNameColorizer
        {
        public:
            TitleNameColorizer() = default;
            ~TitleNameColorizer() override = default;

            uint32_t GetAlignmentColor(int32_t alignment) const override
            {
                uint32_t color = 0xFF808080; // Default: Gray (Neutralny)

                if (alignment >= 12000)
                {
                    color = 0xFFADD8E6; // Light Blue (Rycerski)
                }
                else if (alignment <= -12000)
                {
                    color = 0xFFDC143C; // Crimson (Okrutny)
                }

                EterBase::ModernLogger::Debug("GetAlignmentColor requested for alignment: {}, returning color: {:#010x}", alignment, color);

                AlignmentColorCalculatedEvent event(alignment, color);
                Core::EventBus::GetInstance().Publish(event);

                return color;
            }

            uint32_t GetEmpireColor(uint8_t empire) const override
            {
                return 0xFFFFFFFF; 
            }

            uint32_t GetLevelColor(int32_t playerLevel, int32_t mobLevel) const override
            {
                return 0xFFFFFFFF;
            }

            std::string FormatGuildName(std::string_view guildName) const override
            {
                return std::string(guildName);
            }

            std::string FormatAlignmentTitle(int32_t alignment) const override
            {
                if (alignment >= 12000) return "Rycerski";
                if (alignment <= -12000) return "Okrutny";
                return "Neutralny";
            }

            void Clear() override
            {
            }
        };
    } // namespace

    std::unique_ptr<ITitleNameColorizer> CreateTitleNameColorizer()
    {
        return std::make_unique<TitleNameColorizer>();
    }
} // namespace UserInterface::TextTail
