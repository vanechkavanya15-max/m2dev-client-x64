#include "../StdAfx.h"
#include "IInstanceTitleRankController.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/ModernLogger.h"
#include "../Core/EventBus.h"

#include <string>

namespace UserInterface::InstanceControllers
{
    /**
     * @brief Implementation of IInstanceTitleRankController managing title, rank, and guild marks.
     */
    class InstanceRank_GuildMark final : public IInstanceTitleRankController
    {
    public:
        /**
         * @brief Constructs the controller for a specific entity.
         * @param ownerId The ID of the entity that owns this controller.
         */
        explicit InstanceRank_GuildMark(EterBase::EntityId ownerId = EterBase::EntityId{0})
            : m_ownerId(ownerId)
        {
        }

        /**
         * @brief Sets the alignment rank points.
         * @param alignment The new alignment value.
         */
        void SetAlignment(int32_t alignment) override
        {
            m_alignment = alignment;
        }

        /**
         * @brief Gets the current alignment rank points.
         * @return int32_t The current alignment.
         */
        int32_t GetAlignment() const override
        {
            return m_alignment;
        }

        /**
         * @brief Sets the player PK (Player Kill) mode.
         * @param pkMode The new PK mode.
         */
        void SetPKMode(uint8_t pkMode) override
        {
            m_pkMode = pkMode;
        }

        /**
         * @brief Gets the player PK mode.
         * @return uint8_t The current PK mode.
         */
        uint8_t GetPKMode() const override
        {
            return m_pkMode;
        }

        /**
         * @brief Sets the guild information for this instance.
         * @param guildId The unique ID of the guild.
         * @param guildName The name of the guild.
         */
        void SetGuild(uint32_t guildId, std::string_view guildName) override
        {
            m_guildId = EterBase::GuildId(guildId);
            m_guildName = guildName;

            EterBase::ModernLogger::Info("Guild assigned: {} (ID: {}) for Entity: {}", guildName, guildId, m_ownerId.value());

            // Notify UI about target refresh via EventBus
            Core::EventBus::GetInstance().Publish(Core::TargetBoardRefreshEvent(m_ownerId.value()));
        }

        /**
         * @brief Gets the current guild ID.
         * @return uint32_t The current guild ID.
         */
        uint32_t GetGuildId() const override
        {
            return m_guildId.value();
        }

        /**
         * @brief Sets the empire the instance belongs to.
         * @param empire The empire ID.
         */
        void SetEmpire(uint8_t empire) override
        {
            m_empire = empire;
        }

        /**
         * @brief Gets the current empire.
         * @return uint8_t The current empire.
         */
        uint8_t GetEmpire() const override
        {
            return m_empire;
        }

        /**
         * @brief Sets a custom title and its color.
         * @param title The custom title string.
         * @param color The color of the custom title.
         */
        void SetCustomTitle(std::string_view title, uint32_t color) override
        {
            m_customTitle = title;
            m_customTitleColor = color;
        }

        /**
         * @brief Clears all title, rank, and guild information.
         */
        void Clear() override
        {
            m_alignment = 0;
            m_pkMode = 0;
            m_guildId = EterBase::GuildId(0);
            m_guildName.clear();
            m_empire = 0;
            m_customTitle.clear();
            m_customTitleColor = 0;
        }

    private:
        EterBase::EntityId m_ownerId{0};      ///< The owner entity ID
        int32_t m_alignment{0};               ///< Current alignment points
        uint8_t m_pkMode{0};                  ///< Current PK mode
        EterBase::GuildId m_guildId{0};       ///< The guild ID
        std::string m_guildName;              ///< The guild name
        uint8_t m_empire{0};                  ///< The empire ID
        std::string m_customTitle;            ///< The custom title text
        uint32_t m_customTitleColor{0};       ///< The color of the custom title
    };
}
