#include "../StdAfx.h"
#include "InstanceTitleRankControllers.h"
#include "../../EterBase/LogModern.h"

namespace UserInterface::InstanceControllers
{
    // =========================================================================
    // InstanceTitleRankController
    // =========================================================================

    void InstanceTitleRankController::SetAlignment(int32_t alignment)
    {
        m_alignment = alignment;
        std::string rankName = CalculateRank(alignment);
        EterBase::ModernLogger::Info("Alignment updated: {} (Rank: {})", alignment, rankName);

        Core::EventBus::GetInstance().Publish(Core::AlignmentChangedEvent{alignment, rankName});
    }

    int32_t InstanceTitleRankController::GetAlignment() const
    {
        return m_alignment;
    }

    void InstanceTitleRankController::SetPKMode(uint8_t pkMode)
    {
        m_pkMode = pkMode;
        EterBase::ModernLogger::Info("PK Mode updated: {}", pkMode);
        Core::EventBus::GetInstance().Publish(Core::PKModeChangedEvent{pkMode});
    }

    uint8_t InstanceTitleRankController::GetPKMode() const
    {
        return m_pkMode;
    }

    void InstanceTitleRankController::SetGuild(uint32_t guildId, std::string_view guildName)
    {
        m_guildId = guildId;
        m_guildName = std::string(guildName);
        EterBase::ModernLogger::Info("Guild updated: {} ({})", guildId, m_guildName);
        Core::EventBus::GetInstance().Publish(Core::GuildChangedEvent{guildId, m_guildName});
    }

    uint32_t InstanceTitleRankController::GetGuildId() const
    {
        return m_guildId;
    }

    void InstanceTitleRankController::SetEmpire(uint8_t empire)
    {
        m_empire = empire;
        EterBase::ModernLogger::Info("Empire updated: {}", empire);
        Core::EventBus::GetInstance().Publish(Core::EmpireChangedEvent{empire});
    }

    uint8_t InstanceTitleRankController::GetEmpire() const
    {
        return m_empire;
    }

    void InstanceTitleRankController::SetCustomTitle(std::string_view title, uint32_t color)
    {
        m_customTitle = std::string(title);
        m_customTitleColor = color;
        EterBase::ModernLogger::Info("Custom title updated: {}", m_customTitle);
        Core::EventBus::GetInstance().Publish(Core::CustomTitleChangedEvent{m_customTitle, color});
    }

    void InstanceTitleRankController::Clear()
    {
        m_alignment = 0;
        m_pkMode = 0;
        m_guildId = 0;
        m_guildName.clear();
        m_empire = 0;
        m_customTitle.clear();
        m_customTitleColor = 0;
        EterBase::ModernLogger::Info("InstanceTitleRankController cleared.");
    }

    std::string InstanceTitleRankController::CalculateRank(int32_t alignment) const
    {
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

    // =========================================================================
    // InstanceRankCustomTitleController
    // =========================================================================

    void InstanceRankCustomTitleController::SetCustomTitle(std::string_view title, uint32_t color)
    {
        if (title.empty())
        {
            EterBase::ModernLogger::Warning("InstanceRank_CustomTitle - Proba ustawienia pustego tytulu.");
            return;
        }

        EterBase::ModernLogger::Info("InstanceRank_CustomTitle - Ustawianie tytulu: {}, kolor: {}", title, color);
        Core::EventBus::GetInstance().Publish(Core::CustomTitleChangedEvent{title, color});
    }

    // =========================================================================
    // InstanceRank_GuildMark
    // =========================================================================

    InstanceRank_GuildMark::InstanceRank_GuildMark(EterBase::EntityId ownerId)
        : m_ownerId(ownerId)
    {
    }

    void InstanceRank_GuildMark::SetAlignment(int32_t alignment)
    {
        m_alignment = alignment;
    }

    int32_t InstanceRank_GuildMark::GetAlignment() const
    {
        return m_alignment;
    }

    void InstanceRank_GuildMark::SetPKMode(uint8_t pkMode)
    {
        m_pkMode = pkMode;
    }

    uint8_t InstanceRank_GuildMark::GetPKMode() const
    {
        return m_pkMode;
    }

    void InstanceRank_GuildMark::SetGuild(uint32_t guildId, std::string_view guildName)
    {
        m_guildId = EterBase::GuildId(guildId);
        m_guildName = guildName;

        EterBase::ModernLogger::Info("Guild assigned: {} (ID: {}) for Entity: {}", guildName, guildId, m_ownerId.value());
        Core::EventBus::GetInstance().Publish(Core::TargetBoardRefreshEvent(m_ownerId.value()));
    }

    uint32_t InstanceRank_GuildMark::GetGuildId() const
    {
        return m_guildId.value();
    }

    void InstanceRank_GuildMark::SetEmpire(uint8_t empire)
    {
        m_empire = empire;
    }

    uint8_t InstanceRank_GuildMark::GetEmpire() const
    {
        return m_empire;
    }

    void InstanceRank_GuildMark::SetCustomTitle(std::string_view title, uint32_t color)
    {
        m_customTitle = title;
        m_customTitleColor = color;
    }

    void InstanceRank_GuildMark::Clear()
    {
        m_alignment = 0;
        m_pkMode = 0;
        m_guildId = EterBase::GuildId(0);
        m_guildName.clear();
        m_empire = 0;
        m_customTitle.clear();
        m_customTitleColor = 0;
    }

    // =========================================================================
    // Funkcje pomocnicze i fabryki
    // =========================================================================

    EterBase::PacketResult<void> ResolveAndPublishEmpireColor(
        EterBase::EntityId entityId,
        const IInstanceTitleRankController& controller)
    {
        uint8_t empireId = controller.GetEmpire();
        uint32_t colorArgb = 0;

        switch (empireId)
        {
            case 1:
                colorArgb = 0xFFFF0000; // Shinsoo - Red
                EterBase::ModernLogger::Debug("Resolved empire color for entity {}: Shinsoo (Red)", entityId.value());
                break;
            case 2:
                colorArgb = 0xFFFFFF00; // Chunjo - Yellow
                EterBase::ModernLogger::Debug("Resolved empire color for entity {}: Chunjo (Yellow)", entityId.value());
                break;
            case 3:
                colorArgb = 0xFF0000FF; // Jinno - Blue
                EterBase::ModernLogger::Debug("Resolved empire color for entity {}: Jinno (Blue)", entityId.value());
                break;
            default:
                EterBase::ModernLogger::Warning("Unknown empire ID {} for entity {}", empireId, entityId.value());
                return std::unexpected(EterBase::PacketError::MalformedPayload);
        }

        Core::EventBus::GetInstance().Publish(EmpireColorResolvedEvent{entityId, colorArgb});
        return {};
    }

    std::unique_ptr<IInstanceTitleRankController> CreateInstanceTitleRankController()
    {
        return std::make_unique<InstanceTitleRankController>();
    }

    std::unique_ptr<IInstanceTitleRankController> CreateInstanceRankCustomTitleController()
    {
        return std::make_unique<InstanceRankCustomTitleController>();
    }
}
