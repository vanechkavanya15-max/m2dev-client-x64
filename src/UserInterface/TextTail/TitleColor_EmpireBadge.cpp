#include "../StdAfx.h"
#include "ITitleNameColorizer.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/ModernLogger.h"
#include "../Core/EventBus.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <format>
#include <expected>
#include <memory>

namespace UserInterface::TextTail
{
    /**
     * @brief Zdarzenie czyszczenia bufora / cache kolorow (np. zmiana mapy).
     */
    struct TitleColorClearedEvent
    {
    };

    /**
     * @class TitleColor_EmpireBadge
     * @brief Implementacja domeny TextTail odpowiedzialna za kolory krolestw i odznak.
     * 
     * Implementacja zasady SRP i Zero-Conflict, zawarta bezposrednio w pliku .cpp.
     */
    class TitleColor_EmpireBadge final : public ITitleNameColorizer
    {
    public:
        ~TitleColor_EmpireBadge() override = default;

        /**
         * @brief Pobiera kolor powiazany z krolestwem uzywajac nowoczesnego podejscia z std::expected.
         * @param empire ID krolestwa (1: Shinsoo, 2: Chunjo, 3: Jinno).
         * @return Wartosc ARGB koloru dla krolestwa.
         */
        [[nodiscard]] uint32_t GetEmpireColor(uint8_t empire) const override
        {
            auto result = CalculateEmpireColor(empire);
            
            if (result.has_value())
            {
                return result.value();
            }
            
            // W przypadku bledu logujemy jako blad, poniewaz to nieoczekiwana wartosc
            EterBase::ModernLogger::Error("Blad rozwiazywania koloru krolestwa: ID={} - {}", empire, result.error());
            return 0xFFFFFFFF; // Domyslnie bialy
        }

        [[nodiscard]] uint32_t GetAlignmentColor(int32_t alignment) const override
        {
            // Podstawowa logika, jezeli alignment >= 0 to na niebiesko, w przeciwnym razie czerwono. 
            // Poniewaz to tylko stub dla badge, zrobie prosty fallback.
            if (alignment >= 0)
                return 0xFF0000FF;
            return 0xFFFF0000;
        }

        [[nodiscard]] uint32_t GetLevelColor(int32_t playerLevel, int32_t mobLevel) const override
        {
            // Podstawowa implementacja, np. kolor na podstawie roznicy leveli
            int32_t diff = mobLevel - playerLevel;
            if (diff >= 10) return 0xFFFF0000; // Czerwony
            if (diff <= -10) return 0xFF00FF00; // Zielony
            return 0xFFFFFFFF; // Bialy
        }

        [[nodiscard]] std::string FormatGuildName(std::string_view guildName) const override
        {
            return std::format("[{}]", guildName);
        }

        [[nodiscard]] std::string FormatAlignmentTitle(int32_t alignment) const override
        {
            if (alignment >= 12000) return "Chivalric";
            if (alignment >= 8000) return "Noble";
            if (alignment >= 4000) return "Good";
            if (alignment >= 10) return "Friendly";
            if (alignment >= -3999) return "Aggressive";
            if (alignment >= -7999) return "Fraudulent";
            if (alignment >= -11999) return "Malicious";
            return "Cruel";
        }

        void Clear() override
        {
            EterBase::ModernLogger::Info("Czyszczenie danych TitleColor_EmpireBadge.");
            Core::EventBus::GetInstance().Publish(TitleColorClearedEvent{});
        }

    private:
        /**
         * @brief Pomocnicza metoda zwracajaca std::expected w celu bezpiecznej obslugi bledow C++23.
         * @param empire ID krolestwa
         * @return Zwraca kolor uint32_t (ARGB) lub komunikat bledu string_view.
         */
        [[nodiscard]] std::expected<uint32_t, std::string_view> CalculateEmpireColor(uint8_t empire) const
        {
            // Definicje kolorow krolestw Metin2:
            constexpr uint32_t EMPIRE_NONE    = 0xFFFFFFFF; // Bialy
            constexpr uint32_t EMPIRE_SHINSOO = 0xFFFF0000; // Czerwony
            constexpr uint32_t EMPIRE_CHUNJO  = 0xFFFFFF00; // Zolty
            constexpr uint32_t EMPIRE_JINNO   = 0xFF0000FF; // Niebieski

            switch (empire)
            {
                case 0: return EMPIRE_NONE;
                case 1: return EMPIRE_SHINSOO;
                case 2: return EMPIRE_CHUNJO;
                case 3: return EMPIRE_JINNO;
                default: 
                    return std::unexpected(std::string_view{"Nieznane ID krolestwa"});
            }
        }
    };
    
    /**
     * @brief Factory function do utworzenia instancji colorizera. Zapewnia dostep do logiki
     * bez naruszania zasady Zero-Conflict w naglowkach.
     */
    [[nodiscard]] std::unique_ptr<ITitleNameColorizer> CreateEmpireBadgeColorizer()
    {
        return std::make_unique<TitleColor_EmpireBadge>();
    }
}
