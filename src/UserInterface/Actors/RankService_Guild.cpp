// #include "../StdAfx.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <string_view>
#include <expected>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"
#include "../Packet.h" // Obowiązkowo kontrakt

namespace UserInterface::Actors
{
    /**
     * @brief Zdarzenie aktualizacji nazwy i tytulu (rangi) gildii dla danego Entity.
     * Publikowane przez EventBus po zaktualizowaniu danych rangi w serwisie.
     */
    struct GuildTitleUpdateEvent : public Core::IEvent
    {
        uint32_t entityId;
        std::string guildName;
        std::string gradeName; // np. "Lider", "Członek"
        
        GuildTitleUpdateEvent(uint32_t id, std::string name, std::string grade)
            : entityId(id), guildName(std::move(name)), gradeName(std::move(grade)) {}
    };

    /**
     * @brief Serwis przypisujacy nazwe gildii oraz lidera/range nad glowa postaci (TextTail).
     * Zgodny z domena RankTitleService (Zero-Conflict, brak naglowka).
     */
    class RankGuildTitleService
    {
    public:
        RankGuildTitleService()
        {
            EterBase::ModernLogger::Info("RankGuildTitleService initialized.");
        }

        /**
         * @brief Aktualizuje i przypisuje nazwe gildii oraz range aktora.
         * Może zostać wywołana przez parser (PacketHandler) z payloadem.
         * 
         * @param entityId ID postaci (EntityId)
         * @param guildId ID gildii (GuildId)
         * @param guildName Nazwa gildii (std::string_view)
         * @param gradeName Ranga w gildii, np. "Lider" (std::string_view)
         * @return EterBase::PacketResult<void>
         */
        EterBase::PacketResult<void> UpdateCharacterGuildRank(
            EterBase::EntityId entityId, 
            EterBase::GuildId guildId, 
            std::string_view guildName, 
            std::string_view gradeName)
        {
            if (!entityId)
            {
                EterBase::ModernLogger::Error("RankGuildTitleService: EntityId is null.");
                return std::unexpected(EterBase::PacketError::MalformedPayload);
            }

            // Oznaczamy dane gildii
            CharacterRankData data;
            data.guildId = guildId;
            data.guildName = std::string(guildName);
            data.gradeName = std::string(gradeName);

            m_rankData[entityId] = data;

            EterBase::ModernLogger::Info("RankGuildTitleService: Updated rank for Entity {} - Guild [{}] Grade: {}", 
                                         entityId.value(), data.guildName, data.gradeName);

            // Publikujemy zdarzenie na szynie eventow (oddzielenie od GUI)
            Core::EventBus::GetInstance().Publish(
                GuildTitleUpdateEvent(entityId.value(), data.guildName, data.gradeName)
            );

            return {};
        }

        /**
         * @brief Przetwarza bezposrednio payload sieciowy TPacketGCGuildSubGrade.
         */
        EterBase::PacketResult<void> HandleGuildGradePacket(std::span<const uint8_t> payload, EterBase::EntityId entityId, EterBase::GuildId guildId, std::string_view guildName)
        {
            if (payload.size() < sizeof(TPacketGCGuildSubGrade))
            {
                EterBase::ModernLogger::Error("RankGuildTitleService: Guild grade packet underflow.");
                return std::unexpected(EterBase::PacketError::BufferUnderflow);
            }

            const auto* packet = reinterpret_cast<const TPacketGCGuildSubGrade*>(payload.data());
            
            // Limit string size using strnlen to avoid out-of-bounds reading
            std::string gradeName(packet->grade_name, strnlen(packet->grade_name, sizeof(packet->grade_name)));
            
            return UpdateCharacterGuildRank(entityId, guildId, guildName, gradeName);
        }

        /**
         * @brief Pobiera aktualne dane o randze gildyjnej postaci.
         * 
         * @param entityId ID postaci.
         * @return std::expected<CharacterRankData, EterBase::EntityError> 
         */
        auto GetRankData(EterBase::EntityId entityId) const
        {
            auto it = m_rankData.find(entityId);
            if (it == m_rankData.end())
            {
                return std::expected<CharacterRankData, EterBase::EntityError>(std::unexpected(EterBase::EntityError::NotFound));
            }
            return std::expected<CharacterRankData, EterBase::EntityError>(it->second);
        }

        /**
         * @brief Usuwa dane o randze dla wylogowanej postaci.
         */
        void RemoveCharacter(EterBase::EntityId entityId)
        {
            m_rankData.erase(entityId);
        }

        /**
         * @brief Czysci caly stan.
         */
        void Clear()
        {
            m_rankData.clear();
        }

    private:
        struct CharacterRankData
        {
            EterBase::GuildId guildId;
            std::string guildName;
            std::string gradeName;
        };

        std::unordered_map<EterBase::EntityId, CharacterRankData> m_rankData;
    };

    /**
     * @brief Singleton dostępowy lokalnie (anonimowa przestrzeń w celu ukrycia).
     */
    namespace
    {
        RankGuildTitleService& GetRankGuildTitleServiceInstance()
        {
            static RankGuildTitleService instance;
            return instance;
        }
    }

    /**
     * @brief API wystawiane na zewnatrz (bez deklaracji w naglowkach).
     * Może byc wywoływane przez Handlery sieciowe przychodzące.
     */
    extern "C" {
        __attribute__((visibility("default"))) 
        bool RankService_ProcessGuildGrade(const uint8_t* data, size_t size, uint32_t vid, uint32_t guildId, const char* guildName)
        {
            if (!data || size == 0 || !guildName) return false;

            std::span<const uint8_t> payload(data, size);
            auto result = GetRankGuildTitleServiceInstance().HandleGuildGradePacket(
                payload, 
                EterBase::EntityId(vid), 
                EterBase::GuildId(guildId), 
                std::string_view(guildName)
            );
            
            return result.has_value();
        }
    }
}
