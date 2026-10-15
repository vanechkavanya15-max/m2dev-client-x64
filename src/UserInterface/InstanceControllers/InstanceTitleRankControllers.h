#pragma once

#include <cstdint>
#include <string_view>
#include <string>
#include <memory>
#include <expected>

#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "../Core/EventBus.h"

namespace UserInterface::Core
{
    struct AlignmentChangedEvent : public IEvent
    {
        int32_t newAlignment;
        std::string rankName;
        AlignmentChangedEvent(int32_t alignment, std::string rank)
            : newAlignment(alignment), rankName(std::move(rank)) {}
    };

    struct PKModeChangedEvent : public IEvent
    {
        uint8_t pkMode;
        explicit PKModeChangedEvent(uint8_t mode) : pkMode(mode) {}
    };

    struct GuildChangedEvent : public IEvent
    {
        uint32_t guildId;
        std::string guildName;
        GuildChangedEvent(uint32_t id, std::string name)
            : guildId(id), guildName(std::move(name)) {}
    };

    struct EmpireChangedEvent : public IEvent
    {
        uint8_t empire;
        explicit EmpireChangedEvent(uint8_t emp) : empire(emp) {}
    };
}

namespace UserInterface::InstanceControllers
{
    struct EmpireColorResolvedEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;
        uint32_t colorArgb;

        EmpireColorResolvedEvent(EterBase::EntityId id, uint32_t color)
            : entityId(id), colorArgb(color) {}
    };

    /**
     * @brief Glowny interfejs kontrolera tytulow, rangi, krolestwa i gildii.
     */
    class IInstanceTitleRankController
    {
    public:
        virtual ~IInstanceTitleRankController() = default;

        virtual void SetAlignment(int32_t alignment) = 0;
        virtual int32_t GetAlignment() const = 0;
        virtual void SetPKMode(uint8_t pkMode) = 0;
        virtual uint8_t GetPKMode() const = 0;
        virtual void SetGuild(uint32_t guildId, std::string_view guildName) = 0;
        virtual uint32_t GetGuildId() const = 0;
        virtual void SetEmpire(uint8_t empire) = 0;
        virtual uint8_t GetEmpire() const = 0;
        virtual void SetCustomTitle(std::string_view title, uint32_t color) = 0;
        virtual void Clear() = 0;
    };

    /**
     * @brief Pelna implementacja kontrolera tytulu, rangi etycznej i przynaleznosci.
     */
    class InstanceTitleRankController : public IInstanceTitleRankController
    {
    public:
        ~InstanceTitleRankController() override = default;

        void SetAlignment(int32_t alignment) override;
        int32_t GetAlignment() const override;
        void SetPKMode(uint8_t pkMode) override;
        uint8_t GetPKMode() const override;
        void SetGuild(uint32_t guildId, std::string_view guildName) override;
        uint32_t GetGuildId() const override;
        void SetEmpire(uint8_t empire) override;
        uint8_t GetEmpire() const override;
        void SetCustomTitle(std::string_view title, uint32_t color) override;
        void Clear() override;

    private:
        std::string CalculateRank(int32_t alignment) const;

        int32_t m_alignment{0};
        uint8_t m_pkMode{0};
        uint32_t m_guildId{0};
        std::string m_guildName;
        uint8_t m_empire{0};
        std::string m_customTitle;
        uint32_t m_customTitleColor{0};
    };

    /**
     * @brief Kontroler wyspecjalizowany w obsludze wlasnych tytulow gracza.
     */
    class InstanceRankCustomTitleController final : public IInstanceTitleRankController
    {
    public:
        void SetAlignment(int32_t) override {}
        int32_t GetAlignment() const override { return 0; }
        void SetPKMode(uint8_t) override {}
        uint8_t GetPKMode() const override { return 0; }
        void SetGuild(uint32_t, std::string_view) override {}
        uint32_t GetGuildId() const override { return 0; }
        void SetEmpire(uint8_t) override {}
        uint8_t GetEmpire() const override { return 0; }
        void Clear() override {}

        void SetCustomTitle(std::string_view title, uint32_t color) override;
    };

    /**
     * @brief Kontroler zarzadzajacy oznaczeniami gildyjnymi instancji.
     */
    class InstanceRank_GuildMark final : public IInstanceTitleRankController
    {
    public:
        explicit InstanceRank_GuildMark(EterBase::EntityId ownerId = EterBase::EntityId{0});
        ~InstanceRank_GuildMark() override = default;

        void SetAlignment(int32_t alignment) override;
        int32_t GetAlignment() const override;
        void SetPKMode(uint8_t pkMode) override;
        uint8_t GetPKMode() const override;
        void SetGuild(uint32_t guildId, std::string_view guildName) override;
        uint32_t GetGuildId() const override;
        void SetEmpire(uint8_t empire) override;
        uint8_t GetEmpire() const override;
        void SetCustomTitle(std::string_view title, uint32_t color) override;
        void Clear() override;

    private:
        EterBase::EntityId m_ownerId{0};
        int32_t m_alignment{0};
        uint8_t m_pkMode{0};
        EterBase::GuildId m_guildId{0};
        std::string m_guildName;
        uint8_t m_empire{0};
        std::string m_customTitle;
        uint32_t m_customTitleColor{0};
    };

    // --- Pomocnicze funkcje i fabryki ---
    EterBase::PacketResult<void> ResolveAndPublishEmpireColor(
        EterBase::EntityId entityId,
        const IInstanceTitleRankController& controller);

    std::unique_ptr<IInstanceTitleRankController> CreateInstanceTitleRankController();
    std::unique_ptr<IInstanceTitleRankController> CreateInstanceRankCustomTitleController();
}
