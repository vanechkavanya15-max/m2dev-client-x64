#include "../StdAfx.h"
#include "ISkillService.h"
#include "../Packet.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include <span>
#include <unordered_map>
#include <mutex>
#include <algorithm>

namespace UserInterface::Services
{
    namespace PassiveSkillIds
    {
        constexpr uint32_t LEADERSHIP = 121;
        constexpr uint32_t MINING = 122;
        constexpr uint32_t LANGUAGE_1 = 126;
        constexpr uint32_t LANGUAGE_2 = 127;
        constexpr uint32_t LANGUAGE_3 = 128;
    }

    /**
     * @brief Zdarzenie emitowane po aktualizacji poziomu umiejetnosci pasywnej.
     * Decouples the service from the GUI.
     */
    struct PassiveSkillUpdatedEvent : public UserInterface::Core::IEvent
    {
        EterBase::SkillId skillId;
        uint8_t newLevel;
        uint8_t masterType;

        PassiveSkillUpdatedEvent(EterBase::SkillId id, uint8_t level, uint8_t master)
            : skillId(id), newLevel(level), masterType(master) {}
    };

    /**
     * @brief Implementacja serwisu dla umiejetnosci pasywnych (Jezyki, Gornictwo, Dowodzenie itp.).
     */
    class SkillService_Passives final : public ISkillService
    {
    public:
        SkillService_Passives() = default;
        ~SkillService_Passives() override = default;

        /**
         * @brief Przetwarza pakiet aktualizacji umiejetnosci.
         * 
         * @param payload Bufor z pakietem z serwera.
         * @return PacketResult (std::expected) wskazujacy na ewentualny blad domenowy.
         */
        [[nodiscard]] EterBase::PacketResult<void> ProcessSkillPacket(std::span<const uint8_t> payload)
        {
            if (payload.size() < sizeof(TPacketGCSkillLevelNew))
            {
                EterBase::ModernLogger::Error("SkillService_Passives: ProcessSkillPacket BufferUnderflow");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            const auto* packet = reinterpret_cast<const TPacketGCSkillLevelNew*>(payload.data());
            
            for (uint32_t i = 0; i < SKILL_MAX_NUM; ++i)
            {
                if (packet->skills[i].bLevel > 0)
                {
                    SetSkillLevel(EterBase::SkillId{i}, packet->skills[i].bLevel, packet->skills[i].bMasterType);
                }
            }

            return {};
        }

        /**
         * @brief Zwraca kalkulacje bonusu dla danej umiejetnosci pasywnej.
         * 
         * @param id Identyfikator umiejetnosci.
         * @return Result (std::expected) z wyliczonym bonusem, lub blad EntityError.
         */
        [[nodiscard]] EterBase::Result<int32_t, EterBase::EntityError> CalculatePassiveBonus(EterBase::SkillId id) const
        {
            return GetSkill(id)
                .transform([](const PlayerSkillView& view) -> EterBase::Result<int32_t, EterBase::EntityError> {
                    if (view.level == 0)
                    {
                        return EterBase::MakeError(EterBase::EntityError::NotFound);
                    }
                    
                    switch (view.skillId.value())
                    {
                        case PassiveSkillIds::LEADERSHIP:
                            // Przyklad kalkulacji: bazowy bonus 5 na poziom
                            return static_cast<int32_t>(view.level * 5);
                        case PassiveSkillIds::MINING:
                            // Zwiekszona szansa na wydobycie: +2% na poziom
                            return static_cast<int32_t>(view.level * 2);
                        case PassiveSkillIds::LANGUAGE_1:
                        case PassiveSkillIds::LANGUAGE_2:
                        case PassiveSkillIds::LANGUAGE_3:
                            // Procent zrozumienia jezyka: 10% na poziom, max 100%
                            return std::min<int32_t>(100, view.level * 10);
                        default:
                            return EterBase::MakeError(EterBase::EntityError::InvalidType);
                    }
                })
                .value_or(EterBase::MakeError(EterBase::EntityError::NotFound));
        }

        /**
         * @brief Ustawia poziom i master type dla podanej umiejetnosci.
         * 
         * @param id Identyfikator umiejetnosci.
         * @param level Nowy poziom umiejetnosci.
         * @param masterType Poziom mastera umiejetnosci.
         */
        void SetSkillLevel(EterBase::SkillId id, uint8_t level, uint8_t masterType) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            auto& skill = m_skills[id.value()];
            skill.skillId = id;
            skill.level = level;
            skill.masterType = masterType;

            EterBase::ModernLogger::Info("SkillService_Passives: Skill {0} updated to Level {1} (MasterType: {2})",
                                         id.value(), level, masterType);

            // Powiadom GUI przez EventBus (Decoupling)
            UserInterface::Core::EventBus::GetInstance().Publish(PassiveSkillUpdatedEvent(id, level, masterType));
        }

        /**
         * @brief Ustawia czas odnowienia dla umiejetnosci.
         * 
         * @param id Identyfikator umiejetnosci.
         * @param duration Czas odnowienia w sekundach.
         */
        void SetSkillCooltime(EterBase::SkillId id, float duration) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (auto it = m_skills.find(id.value()); it != m_skills.end())
            {
                it->second.cooltimeRemaining = std::max(0.0f, duration);
                it->second.totalCooltime = std::max(0.0f, duration);
                
                EterBase::ModernLogger::Debug("SkillService_Passives: Skill {0} cooldown set to {1}s", id.value(), duration);
            }
        }

        /**
         * @brief Sprawdza czy umiejetnosc jest aktualnie w trakcie odnawiania.
         * 
         * @param id Identyfikator umiejetnosci.
         * @return true jesli jest na cooldownie.
         */
        [[nodiscard]] bool IsSkillCooltime(EterBase::SkillId id) const override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (auto it = m_skills.find(id.value()); it != m_skills.end())
            {
                return it->second.cooltimeRemaining > 0.0f;
            }
            return false;
        }

        /**
         * @brief Pobiera pozostaly czas odnowienia dla umiejetnosci.
         * 
         * @param id Identyfikator umiejetnosci.
         * @return Pozostaly czas w sekundach.
         */
        [[nodiscard]] float GetSkillCooltimeRemaining(EterBase::SkillId id) const override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (auto it = m_skills.find(id.value()); it != m_skills.end())
            {
                return it->second.cooltimeRemaining;
            }
            return 0.0f;
        }

        /**
         * @brief Pobiera informacje o umiejetnosci jesli gracz ja posiada.
         * 
         * @param id Identyfikator umiejetnosci.
         * @return std::optional zawierajacy PlayerSkillView lub pusty jesli nie posiada.
         */
        [[nodiscard]] std::optional<PlayerSkillView> GetSkill(EterBase::SkillId id) const override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (auto it = m_skills.find(id.value()); it != m_skills.end())
            {
                return it->second;
            }
            return std::nullopt;
        }

        /**
         * @brief Aktualizuje czasy odnowienia umiejetnosci co klatke.
         * 
         * @param deltaTime Czas od ostatniej klatki w sekundach.
         */
        void UpdateCooltimes(float deltaTime) override
        {
            if (deltaTime <= 0.0f)
                return;

            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto& [key, skill] : m_skills)
            {
                if (skill.cooltimeRemaining > 0.0f)
                {
                    skill.cooltimeRemaining = std::max(0.0f, skill.cooltimeRemaining - deltaTime);
                }
            }
        }

        /**
         * @brief Czysci stan wszystkich umiejetnosci (np. przy wylogowaniu).
         */
        void Clear() override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_skills.clear();
            EterBase::ModernLogger::Info("SkillService_Passives: All skills cleared.");
        }

    private:
        mutable std::mutex m_mutex;
        std::unordered_map<uint32_t, PlayerSkillView> m_skills;
    };

    /**
     * @brief Zwraca nowa instancje serwisu dla umiejetnosci pasywnych.
     */
    std::unique_ptr<ISkillService> CreatePassiveSkillService()
    {
        return std::make_unique<SkillService_Passives>();
    }
}
