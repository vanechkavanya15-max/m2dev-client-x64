#include "../StdAfx.h"
#include "IPlayerStatsService.h"
#include "../Packet.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Domain/PlayerStatsModel.h"
#include "../Core/EventBus.h"

namespace UserInterface::Services
{
    /**
     * @brief Implementacja serwisu do zarzadzania statystykami gracza.
     * Most przekierowujacy wywolania punktow statusu z pakietow i systemow.
     */
    class PlayerFacadeStatsBridge : public IPlayerStatsService
    {
    public:
        PlayerFacadeStatsBridge() = default;
        ~PlayerFacadeStatsBridge() override = default;

        /**
         * @brief Ustawia wartosc konkretnego punktu statystyki.
         * @param type Typ statystyki zdefiniowany w Packet.h (np. POINT_HP).
         * @param value Nowa wartosc dla punktu.
         */
        void SetPoint(uint32_t type, int64_t value) override
        {
            bool updated = false;

            switch (type)
            {
                case POINT_HP:
                    m_points.hp = static_cast<uint32_t>(value);
                    updated = true;
                    break;
                case POINT_MAX_HP:
                    m_points.maxHp = static_cast<uint32_t>(value);
                    updated = true;
                    break;
                case POINT_SP:
                    m_points.sp = static_cast<uint32_t>(value);
                    updated = true;
                    break;
                case POINT_MAX_SP:
                    m_points.maxSp = static_cast<uint32_t>(value);
                    updated = true;
                    break;
                case POINT_STAMINA:
                    m_points.stamina = static_cast<uint32_t>(value);
                    updated = true;
                    break;
                case POINT_MAX_STAMINA:
                    m_points.maxStamina = static_cast<uint32_t>(value);
                    updated = true;
                    break;
                case POINT_EXP:
                    m_points.exp = static_cast<uint64_t>(value);
                    updated = true;
                    break;
                case POINT_NEXT_EXP:
                    m_points.nextExp = static_cast<uint64_t>(value);
                    updated = true;
                    break;
                case POINT_GOLD:
                    m_points.gold = value;
                    updated = true;
                    break;
                case POINT_LEVEL:
                    m_points.level = static_cast<uint8_t>(value);
                    updated = true;
                    break;
                case POINT_STAT:
                    m_points.statPoints = static_cast<uint16_t>(value);
                    updated = true;
                    break;
                case POINT_SKILL:
                    m_points.skillPoints = static_cast<uint16_t>(value);
                    updated = true;
                    break;
                default:
                    EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "PlayerFacadeStatsBridge::SetPoint - Nieobslugiwany typ punktu: {}", type);
                    break;
            }

            if (updated)
            {
                EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "PlayerFacadeStatsBridge::SetPoint - Zaktualizowano punkt {} na {}", type, value);
                
                // Powiadomienie szyny zdarzen, co pozwala na odseparowanie logiki GUI
                UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Domain::PlayerStatsUpdatedEvent{EterBase::EntityId{0}});
            }
        }

        /**
         * @brief Pobiera aktualna wartosc zadanego punktu statystyki.
         * @param type Typ statystyki.
         * @return Wartosc punktu.
         */
        int64_t GetPoint(uint32_t type) const override
        {
            switch (type)
            {
                case POINT_HP:          return m_points.hp;
                case POINT_MAX_HP:      return m_points.maxHp;
                case POINT_SP:          return m_points.sp;
                case POINT_MAX_SP:      return m_points.maxSp;
                case POINT_STAMINA:     return m_points.stamina;
                case POINT_MAX_STAMINA: return m_points.maxStamina;
                case POINT_EXP:         return static_cast<int64_t>(m_points.exp);
                case POINT_NEXT_EXP:    return static_cast<int64_t>(m_points.nextExp);
                case POINT_GOLD:        return m_points.gold;
                case POINT_LEVEL:       return m_points.level;
                case POINT_STAT:        return m_points.statPoints;
                case POINT_SKILL:       return m_points.skillPoints;
                default:
                    return 0;
            }
        }

        /**
         * @brief Zwraca strukture tylko-do-odczytu ze wszystkimi statystykami.
         * @return Referencja do PlayerPointsView.
         */
        const PlayerPointsView& GetPoints() const override
        {
            return m_points;
        }

        /**
         * @brief Czysci wszystkie statystyki do stanow domyslnych.
         */
        void Clear() override
        {
            m_points = PlayerPointsView{};
            EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "PlayerFacadeStatsBridge::Clear - Wyczyszczono statystyki gracza.");
        }

    private:
        PlayerPointsView m_points{}; ///< Wewnetrzny stan punktow gracza.
    };
} // namespace UserInterface::Services
