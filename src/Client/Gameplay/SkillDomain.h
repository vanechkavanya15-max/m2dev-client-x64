#pragma once

#include <unordered_map>
#include <chrono>
#include <string>
#include <shared_mutex>
#include "SkillCooldownTracker.h"

namespace Client::Gameplay {

    /**
     * @brief Definiuje typ umiejętności w systemie gry.
     * 
     * Typ umiejętności determinuje sposób jej aktywacji oraz to,
     * czy umiejętność zużywa punkty many (SP) jednorazowo podczas
     * wywołania, czy może w czasie rzeczywistym w formie przełącznika.
     */
    enum class SkillType : uint8_t {
        Passive, ///< Pasywna umiejętność, aktywna stale, brak kosztu użycia.
        Active,  ///< Aktywna umiejętność, wymaga wyzwolenia, ma koszt SP i czas odnawiania.
        Toggle   ///< Umiejętność przełączalna (np. Aura Miecza, Silne Ciało), konsumująca SP w czasie, gdy jest aktywna.
    };

    /**
     * @brief Zdefiniowane rodzaje celów dla umiejętności postaci.
     * 
     * Różne umiejętności wymagają różnych celów, od rzucania czarów
     * na wroga, aż po wzmacnianie sojuszników w grupie.
     */
    enum class SkillTarget : uint8_t {
        None,    ///< Brak specyficznego celu (np. umiejętności obszarowe rzucane wokoło siebie).
        Self,    ///< Umiejętność działa wyłącznie na postać, która ją wywołuje (np. buff na samego siebie).
        Enemy,   ///< Umiejętność wymaga wrogo nastawionego celu.
        Party    ///< Umiejętność działa na członków grupy znajdujących się w pobliżu.
    };

    /**
     * @brief Reprezentacja poziomu umiejętności.
     */
    using SkillLevel = uint8_t;

    /**
     * @brief Unikalny identyfikator umiejętności w grze.
     */
    using SkillId = uint32_t;
    
    /**
     * @brief Definiuje stopien zaawansowania (mistrzostwa) umiejetnosci.
     */
    enum class SkillMasterType : uint8_t {
        Normal = 0,         ///< Zwykly (poziomy 1-19)
        Master = 1,         ///< Master (M1 - M10, poziomy 20-29)
        GrandMaster = 2,    ///< Grand Master (G1 - G10, poziomy 30-39)
        PerfectMaster = 3   ///< Perfect Master (P, poziom 40+)
    };

    /**
     * @brief Przechowuje aktualny stan i metadane danej umiejetnosci u gracza.
     * 
     * Kazda umiejetnosc posiadana przez gracza ma swoj unikalny stan, 
     * okreslajacy poziom zaawansowania, stopien mistrzostwa oraz czasy odnawiania.
     */
    struct SkillState {
        SkillLevel level{0};                                  ///< Obecny calkowity poziom zaawansowania umiejetnosci (np. 1-40).
        uint8_t masterType{0};                                ///< Jawne pole typu mistrzostwa: 0=Normal, 1=Master, 2=GrandMaster, 3=PerfectMaster.
        uint8_t masteryLevel{0};                              ///< Poziom w ramach obecnego mistrzostwa (np. 1-10 dla M/G/P).
        std::chrono::steady_clock::time_point cooldownEndTime; ///< Absolutny czas, w ktorym zakonczy sie odnawianie.
        bool isCoolingDown{false};                            ///< Flaga wskazujaca, czy umiejetnosc jest aktualnie odnawiana.
        bool isToggledOn{false};                              ///< Flaga okreslajaca, czy umiejetnosc typu Toggle jest wlaczona.
    };

    /**
     * @brief Struktura opisująca bazowe, statyczne właściwości umiejętności.
     * 
     * Definiuje podstawowe zachowania umiejętności, z których menedżer
     * może następnie wyliczać wartości zależne od poziomu zaawansowania.
     */
    struct SkillData {
        SkillId id{0};                         ///< Unikalny identyfikator reprezentujący umiejętność.
        SkillType type{SkillType::Active};     ///< Typ danej umiejętności (Pasywny, Aktywny, Przełączalny).
        SkillTarget target{SkillTarget::Self}; ///< Wymagany cel do poprawnego aktywowania umiejętności.
        uint32_t baseSPCost{0};                ///< Podstawowy koszt użycia w punktach many (SP).
        float spMultiplier{1.0f};              ///< Mnożnik określający przyrost kosztu SP na każdy kolejny poziom umiejętności.
        std::string name{"Unknown"};           ///< Lokalizowana lub systemowa nazwa umiejętności.
    };

    /**
     * @class SkillDomain
     * @brief Zarządza zbiorem umiejętności, ich poziomami zaawansowania, 
     *        czasami odnawiania oraz wyliczaniem zapotrzebowania na manę.
     * 
     * Menedżer (Domain) grupujący całą logikę biznesową dotyczącą drzewka
     * umiejętności postaci. Umożliwia rejestrację poszczególnych skilli, 
     * aktualizowanie ich czasów odnawiania (cooldowns), a także sprawdza,
     * czy konkretna umiejętność może zostać wyzwolona w danym momencie.
     * 
     * @note Klasa wspiera mechaniki z podziałem na klasy mistrzostwa (Normal, Master, GrandMaster, PerfectMaster),
     * co znacząco wpływa m.in. na koszty many oraz ewentualne efekty uboczne.
     */
    class SkillDomain {
    public:
        /**
         * @brief Domyślny konstruktor menedżera umiejętności.
         */
        SkillDomain() = default;

        /**
         * @brief Domyślny destruktor menedżera umiejętności.
         */
        ~SkillDomain() = default;

        /**
         * @brief Definiuje nową umiejętność w centralnym słowniku metadanych menedżera.
         * 
         * @param id Unikalny identyfikator przypisywanej umiejętności.
         * @param data Pełna konfiguracja zawierająca koszty, typ i nazwę.
         */
        void DefineSkill(SkillId id, const SkillData& data);

        /**
         * @brief Rejestruje umiejętność bezpośrednio na profilu gracza z określonym poziomem.
         * 
         * Jeśli umiejętność nie była wcześniej u gracza znana, zostanie mu przypisana.
         * 
         * @param skillId Identyfikator wybranej umiejętności.
         * @param initialLevel Początkowy poziom (0 oznacza brak znajomości/aktywności).
         */
        void RegisterSkill(SkillId skillId, SkillLevel initialLevel = 0);
        void RegisterSkill(SkillId skillId, SkillLevel level, uint8_t masterType, uint8_t masteryLevel);
        
        /**
         * @brief Weryfikuje, czy gracz posiada zadaną umiejętność w swoich zasobach.
         * 
         * @param skillId Identyfikator weryfikowanej umiejętności.
         * @return true Jeśli umiejętność istnieje u gracza.
         * @return false W przeciwnym razie.
         */
        bool HasSkill(SkillId skillId) const;

        /**
         * @brief Pobiera aktualny stopień zaawansowania dla wskazanej umiejętności gracza.
         * 
         * @param skillId Identyfikator umiejętności.
         * @return SkillLevel Zwraca poziom od 0 (brak) do 40+ (Perfect Master).
         */
        SkillLevel GetSkillLevel(SkillId skillId) const;

        /**
         * @brief Modyfikuje bezpośrednio poziom zaawansowania wskazanej umiejętności gracza.
         * 
         * @param skillId Identyfikator umiejętności.
         * @param level Nowy pożądany poziom umiejętności.
         */
        void SetSkillLevel(SkillId skillId, SkillLevel level);

        /**
         * @brief Modyfikuje bezpośrednio stopien mistrzostwa oraz poziom w mistrzostwie.
         * 
         * @param skillId Identyfikator umiejetnosci.
         * @param masterType Typ mistrzostwa (0=Normal, 1=Master, 2=GrandMaster, 3=PerfectMaster).
         * @param masteryLevel Poziom w ramach danego mistrzostwa (np. 1-10).
         */
        void SetSkillMastery(SkillId skillId, uint8_t masterType, uint8_t masteryLevel);

        /**
         * @brief Zwraca typ mistrzostwa dla danej umiejetnosci.
         */
        uint8_t GetSkillMasterType(SkillId skillId) const;

        /**
         * @brief Zwraca poziom w ramach mistrzostwa dla danej umiejetnosci.
         */
        uint8_t GetSkillMasteryLevel(SkillId skillId) const;

        /**
         * @brief Pobiera wskaznik na pelny stan umiejetnosci gracza lub nullptr w przypadku braku.
         */
        const SkillState* GetSkillState(SkillId skillId) const;

        /**
         * @brief Uruchamia zegar odnawiania (cooldown) dla podanej umiejętności.
         * 
         * Gdy umiejętność jest odnawiana, nie może zostać ponownie aktywowana
         * aż do upłynięcia przypisanego czasu w milisekundach.
         * 
         * @param skillId Identyfikator odnawianej umiejętności.
         * @param duration Długość trwania odnawiania w milisekundach.
         */
        void StartCooldown(SkillId skillId, std::chrono::milliseconds duration);
        void StartCooldown(SkillId skillId, uint32_t durationMs);

        /**
         * @brief Weryfikuje gotowość danej umiejętności do aktywacji na podstawie jej cooldownu.
         * 
         * @param skillId Identyfikator weryfikowanej umiejętności.
         * @return true Jeśli umiejętność nie znajduje się aktualnie w fazie odnawiania.
         */
        bool IsSkillReady(SkillId skillId) const;

        /**
         * @brief Metoda odpowiedzialna za periodyczną synchronizację i wygaszanie czasów odnawiania.
         * 
         * Powinna być regularnie wywoływana z poziomu głównej pętli gry.
         */
        void UpdateCooldowns();

        /**
         * @brief Zwraca szacunkowy pozostały czas niezbędny do zakończenia odnawiania umiejętności.
         * 
         * @param skillId Identyfikator sprawdzanej umiejętności.
         * @return std::chrono::milliseconds Pozostały czas lub 0, jeśli umiejętność jest gotowa.
         */
        std::chrono::milliseconds GetRemainingCooldown(SkillId skillId) const;
        uint32_t GetRemainingCooldownMs(SkillId skillId) const;

        /**
         * @brief Zwraca postep odnawiania w zakresie [0.0f, 1.0f] (1.0f oznacza pelna gotowosc).
         */
        float GetCooldownProgress(SkillId skillId) const;

        /**
         * @brief Natychmiastowo zdejmuje obciążenie odnawiania dla danej umiejętności.
         * 
         * @param skillId Identyfikator odnawianej umiejętności.
         */
        void ResetCooldown(SkillId skillId);

        /**
         * @brief Resetuje czasy odnawiania dla wszystkich umiejetnosci gracza.
         */
        void ResetAllCooldowns();

        [[nodiscard]] const SkillCooldownTracker& GetCooldownTracker() const noexcept { return m_cooldownTracker; }
        [[nodiscard]] SkillCooldownTracker& GetCooldownTracker() noexcept { return m_cooldownTracker; }

        /**
         * @brief Główny algorytm wyliczający koszt użycia umiejętności w punktach SP.
         * 
         * Algorytm na podstawie bazy zapotrzebowania, obecnego poziomu zaawansowania 
         * oraz określonego mnożnika mistrzostwa (Master, GrandMaster, PerfectMaster), 
         * precyzyjnie definiuje rzeczywisty koszt pojedynczego rzucenia czaru
         * lub jednego tyknięcia w umiejętnościach podtrzymywanych (Toggle).
         * 
         * @param skillId Identyfikator umiejętności.
         * @return uint32_t Rzeczywisty, zaokrąglony w górę koszt w punktach Many (SP).
         */
        uint32_t CalculateSPCost(SkillId skillId) const;

        /**
         * @brief Przełącza tryb aktywacji dla umiejętności typu Toggle.
         * 
         * @param skillId Identyfikator przełączanej umiejętności.
         * @param state Nowy pożądany stan (włączona / wyłączona).
         */
        void ToggleSkill(SkillId skillId, bool state);

        /**
         * @brief Odpytuje system o bieżący tryb aktywacji umiejętności typu Toggle.
         * 
         * @param skillId Identyfikator sprawdzanej umiejętności.
         * @return true Jeśli umiejętność typu Toggle jest obecnie wprawiona w ruch.
         */
        bool IsSkillToggledOn(SkillId skillId) const;

        /**
         * @brief Zwraca stałą instancję metadanych o konfiguracji wskazanej umiejętności.
         * 
         * @param skillId Identyfikator poszukiwanej umiejętności.
         * @return const SkillData* Wskaźnik na definicję lub nullptr w przypadku braku.
         */
        const SkillData* GetSkillData(SkillId skillId) const;

    private:
        mutable std::shared_mutex m_mutex;
        /**
         * @brief Globalne metadane definiujące parametry brzegowe poszczególnych skilli.
         */
        std::unordered_map<SkillId, SkillData> m_skillDefinitions;

        /**
         * @brief Stany poszczególnych umiejętności gracza. Zawierają poziom i aktualny cooldown.
         */
        std::unordered_map<SkillId, SkillState> m_skills;

        /**
         * @brief Wbudowany tracker zarzadzania cooldownami umiejetnosci.
         */
        SkillCooldownTracker m_cooldownTracker;
    };

} // namespace Client::Gameplay
