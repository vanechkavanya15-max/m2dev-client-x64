#include "../StdAfx.h"
#include "IRankTitleService.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include <unordered_map>
#include <mutex>

namespace UserInterface::Actors
{
    /**
     * @brief Implementacja uslugi zarzadzajacej ranga i tytulami postaci.
     * 
     * Przechowuje punkty rangi dla bytow. Kalkuluje grade na podstawie
     * punktow i rozglasza zdarzenie zmiany, jesli nowa wartosc
     * punktow badz grade ulegla zmianie.
     */
    class RankTitleService : public IRankTitleService
    {
    public:
        RankTitleService() = default;
        ~RankTitleService() override = default;

        EterBase::PacketResult<void> SetAlignment(EterBase::EntityId id, int32_t points) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            uint32_t newGrade = CalculateGrade(points);
            bool isNewOrChanged = true;

            auto it = m_alignments.find(id);
            if (it != m_alignments.end())
            {
                if (it->second.alignment == points && it->second.grade == newGrade)
                {
                    isNewOrChanged = false;
                }
                it->second.alignment = points;
                it->second.grade = newGrade;
            }
            else
            {
                m_alignments.emplace(id, RankData{points, newGrade});
            }

            if (isNewOrChanged)
            {
                EterBase::ModernLogger::Info("RankTitleService: SetAlignment(EntityId={}, points={}, grade={})", 
                    id.value(), points, newGrade);
                UserInterface::Core::EventBus::GetInstance().Publish(RankTitleChangedEvent(id, points, newGrade));
            }

            return {}; // Success
        }

        EterBase::Result<int32_t, EterBase::EntityError> GetAlignment(EterBase::EntityId id) const override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_alignments.find(id);
            if (it != m_alignments.end())
            {
                return it->second.alignment;
            }
            
            EterBase::ModernLogger::Debug("RankTitleService: GetAlignment(EntityId={}) - EntityError::NotFound", id.value());
            return EterBase::MakeError(EterBase::EntityError::NotFound);
        }

        EterBase::Result<uint32_t, EterBase::EntityError> GetGrade(EterBase::EntityId id) const override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_alignments.find(id);
            if (it != m_alignments.end())
            {
                return it->second.grade;
            }
            
            EterBase::ModernLogger::Debug("RankTitleService: GetGrade(EntityId={}) - EntityError::NotFound", id.value());
            return EterBase::MakeError(EterBase::EntityError::NotFound);
        }

    private:
        struct RankData {
            int32_t alignment;
            uint32_t grade;
        };

        mutable std::mutex m_mutex;
        std::unordered_map<EterBase::EntityId, RankData> m_alignments;

        /**
         * @brief Kalkuluje Grade na podstawie punktow rangi (Alignment).
         * @param points Punkty rangi.
         * @return Wyliczony Grade jako uint32_t.
         */
        uint32_t CalculateGrade(int32_t points) const
        {
            if (points >= 12000)
                return 0;
            else if (points >= 8000)
                return 1;
            else if (points >= 4000)
                return 2;
            else if (points >= 1000)
                return 3;
            else if (points >= 0)
                return 4;
            else if (points > -4000)
                return 5;
            else if (points > -8000)
                return 6;
            else if (points > -12000)
                return 7;
            
            return 8;
        }
    };
} // namespace UserInterface::Actors
