#include "../StdAfx.h"
#include "../Packet.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../../EterLib/GrpColor.h"
#include "../Core/EventBus.h"

namespace UserInterface::Actors
{
    /**
     * @brief Klasa odpowiedzialna za zarzadzanie ranga i obliczanie koloru poziomu gracza/celu.
     * Wykorzystuje nowoczesne C++23 std::expected, EterBase::StrongTypes.
     */
    class RankTitleService
    {
    public:
        /**
         * @brief Oblicza kolor dla renderowania poziomu (Level) w zaleznosci od celu wzgledem gracza.
         * 
         * @param playerLevel Aktualny poziom postaci glownej.
         * @param targetLevel Poziom celu.
         * @return EterBase::Result<CGraphicColor> Obliczony kolor.
         */
        static EterBase::Result<CGraphicColor> GetTargetLevelColor(
            EterBase::PlayerLevel playerLevel, 
            EterBase::PlayerLevel targetLevel)
        {
            if (playerLevel.get() == 0 || targetLevel.get() == 0)
            {
                EterBase::ModernLogger::Warn("RankTitleService::GetTargetLevelColor - Invalid levels: playerLevel={}, targetLevel={}", 
                    playerLevel.get(), targetLevel.get());
                return EterBase::MakeError("Level cannot be 0");
            }

            int32_t diff = static_cast<int32_t>(targetLevel.get()) - static_cast<int32_t>(playerLevel.get());

            if (diff >= 15)
            {
                // Znacznie wyzszy poziom (Czerwony)
                return CGraphicColor(255.0f/255.0f, 51.0f/255.0f, 51.0f/255.0f, 1.0f);
            }
            else if (diff > 0)
            {
                // Wyzszy poziom (Zolty)
                return CGraphicColor(255.0f/255.0f, 204.0f/255.0f, 51.0f/255.0f, 1.0f);
            }
            else if (diff >= -15)
            {
                // Podobny lub nieco nizszy poziom (Zielony)
                return CGraphicColor(152.0f/255.0f, 255.0f/255.0f, 51.0f/255.0f, 1.0f);
            }
            else
            {
                // Znacznie nizszy poziom (Szary)
                return CGraphicColor(153.0f/255.0f, 153.0f/255.0f, 153.0f/255.0f, 1.0f);
            }
        }
    };
}
