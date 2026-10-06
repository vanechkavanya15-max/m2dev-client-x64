#include "../StdAfx.h"
#include "IInstanceTitleRankController.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"
#include <string>
#include <memory>
#include <format>
#include <cstdint>

namespace UserInterface::Core {
    struct AlignmentChangedEvent : public IEvent {
        int32_t newAlignment;
        std::string rankName;
        explicit AlignmentChangedEvent(int32_t alignment, std::string rank) 
            : newAlignment(alignment), rankName(std::move(rank)) {}
    };

    struct PKModeChangedEvent : public IEvent {
        uint8_t pkMode;
        explicit PKModeChangedEvent(uint8_t mode) : pkMode(mode) {}
    };

    struct GuildChangedEvent : public IEvent {
        uint32_t guildId;
        std::string guildName;
        explicit GuildChangedEvent(uint32_t id, std::string name) 
            : guildId(id), guildName(std::move(name)) {}
    };

    struct EmpireChangedEvent : public IEvent {
        uint8_t empire;
        explicit EmpireChangedEvent(uint8_t emp) : empire(emp) {}
    };
}

namespace UserInterface::InstanceControllers {

    class InstanceTitleRankController : public IInstanceTitleRankController {
    public:
        ~InstanceTitleRankController() override = default;

        void SetAlignment(int32_t alignment) override {
            m_alignment = alignment;
            std::string rankName = CalculateRank(alignment);
            EterBase::ModernLogger::Info("Alignment updated: {} (Rank: {})", alignment, rankName);
            
            Core::EventBus::GetInstance().Publish(Core::AlignmentChangedEvent{alignment, rankName});
        }

        int32_t GetAlignment() const override {
            return m_alignment;
        }

        void SetPKMode(uint8_t pkMode) override {
            m_pkMode = pkMode;
            EterBase::ModernLogger::Info("PK Mode updated: {}", pkMode);
            Core::EventBus::GetInstance().Publish(Core::PKModeChangedEvent{pkMode});
        }

        uint8_t GetPKMode() const override {
            return m_pkMode;
        }

        void SetGuild(uint32_t guildId, std::string_view guildName) override {
            m_guildId = guildId;
            m_guildName = std::string(guildName);
            EterBase::ModernLogger::Info("Guild updated: {} ({})", guildId, m_guildName);
            Core::EventBus::GetInstance().Publish(Core::GuildChangedEvent{guildId, m_guildName});
        }

        uint32_t GetGuildId() const override {
            return m_guildId;
        }

        void SetEmpire(uint8_t empire) override {
            m_empire = empire;
            EterBase::ModernLogger::Info("Empire updated: {}", empire);
            Core::EventBus::GetInstance().Publish(Core::EmpireChangedEvent{empire});
        }

        uint8_t GetEmpire() const override {
            return m_empire;
        }

        void SetCustomTitle(std::string_view title, uint32_t color) override {
            m_customTitle = std::string(title);
            m_customTitleColor = color;
            EterBase::ModernLogger::Info("Custom title updated: {}", m_customTitle);
            Core::EventBus::GetInstance().Publish(Core::CustomTitleChangedEvent{m_customTitle, color});
        }

        void Clear() override {
            m_alignment = 0;
            m_pkMode = 0;
            m_guildId = 0;
            m_guildName.clear();
            m_empire = 0;
            m_customTitle.clear();
            m_customTitleColor = 0;
            EterBase::ModernLogger::Info("InstanceTitleRankController cleared.");
        }

    private:
        int32_t m_alignment{0};
        uint8_t m_pkMode{0};
        uint32_t m_guildId{0};
        std::string m_guildName;
        uint8_t m_empire{0};
        std::string m_customTitle;
        uint32_t m_customTitleColor{0};

        std::string CalculateRank(int32_t alignment) const {
            if (alignment >= 12000) return "Rycerski";
            if (alignment >= 8000) return "Szlachetny";
            if (alignment >= 4000) return "Dobry";
            if (alignment >= 10) return "Przyjazny";
            if (alignment >= 0) return "Neutralny";
            if (alignment >= -3999) return "Agresywny";
            if (alignment >= -7999) return "Zlosliwy";
            if (alignment >= -11999) return "Okrutny";
            return "Morderczy";
        }
    };

    std::unique_ptr<IInstanceTitleRankController> CreateInstanceTitleRankController() {
        return std::make_unique<InstanceTitleRankController>();
    }
}
