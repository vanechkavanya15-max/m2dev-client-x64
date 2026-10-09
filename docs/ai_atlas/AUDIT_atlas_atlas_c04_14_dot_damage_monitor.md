---
task_id: "atlas_c04_14_dot_damage_monitor"
cluster: "CBT"
module_name: "Monitor Efektow Periodycznych (DoT Damage & Buff Monitor)"
target_files:
- src/GameLib/DoTDamageCalculator.h
- src/GameLib/BuffDurationMonitor.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c04_14_dot_damage_monitor.md"
architecture_layer: "System Walki, Umiejetnosci i Fizyka Pociskow"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):

Analizowane moduly pelnia kluczowa role w systemie walki gry Metin2, odpowiadajac za bezstanowa kalkulacje obrazen periodycznych (DoT - Damage over Time) oraz monitorowanie czasu trwania wzmocnien (buffow) na postaci gracza.

*   `DoTDamageCalculator`: Jest to bezstanowa klasa narzedziowa (utility class) zawierajaca logike matematyczna do obliczania calkowitych obrazen z efektow takich jak otrucie (Poison) czy krwawienie (Bleeding). Zamiast bezposrednio modyfikowac stan obiektow gry czy interfejsu uzytkownika, kalkulator uzywa wzorca EventBus. Wyemitowanie zdarzenia `DoTDamageEvent` calkowicie dekupluje logike obliczeniowa od efektow wizualnych i logiki nakladania obrazen. Kod jest wykonany w nowoczesnym standardzie C++23, wysoce zalezy od std::optional oraz std::expected. Funkcja obliczeniowa `CalculateDamage` operuje synchronicznie na wywolanie z zewnetrznego zrodla (zazwyczaj w odpowiedzi na przetworzenie pakietu z serwera w Network Tick lub podczas OnUpdate).

*   `BuffDurationMonitor`: Jest to stanowosciowa klasa zajmujaca sie zarzadzaniem czasem trwania aktywnych wzmocnien (np. Aura Miecza, Silne Cialo, Berek). Posiada mechanizm monitorowania, ktory co ramke/tick zmniejsza pozostaly czas trwania o podana delte czasu (`deltaTimeMs`). Istotna czescia tego modulu jest system powiadomien (`BuffUpdateResult`), ktory wczesniej informuje (zwykle na 5 sekund przed koncem, definiowane przez `warningThresholdMs`) o zblizajacym sie zakonczeniu buffa. Umozliwia to interfejsowi uzytkownika rozpoczecie migania ikony buffa bez koniecznosci sprawdzania jego stany przez UI. Zapewnia pelen cykl zycia: dodanie/odswiezenie (`AddBuff`), usuniecie reczne (`RemoveBuff`), aktualizacje ze zmniejszeniem czasu (`Update`) az do automatycznego wygasniecia i usuniecia, i wyczyszczenie wszystkich (`ClearAllBuffs`).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):

*   **Zaleznosci wejsciowe (Inbound):**
    *   Wywolania sieciowe z glownej petli (PhaseGame / Combat handlers), przetwarzajace pakiety uderzen lub wlasciwosci (properties), gdzie efekty zostaly nalozone przez serwer.
    *   Glowna petla Update gry przekazujaca wartosc `deltaTimeMs` do `BuffDurationMonitor::Update`.
    *   System UI moze polegac na tych strukturach (np. `ActiveBuff`, `BuffType`) lub nasluchiwac na zblizajace sie wygasniecia poprzez uzycie monitora, nie bezposrednio klasy.

*   **Zaleznosci wyjsciowe (Outbound):**
    *   `Core::EventBus` (`"Core/EventBus.h"`): Uzywane przez `DoTDamageCalculator` do rozsylania zdarzen o obliczonych obrazeniach, tak by interfejs GUI oraz kontrolery statystyk mogly na nie zareagowac.
    *   `EterBase::ModernLogger` (`"../EterBase/ModernLogger.h"`): Do logowania informacji o zadanych obrazeniach (Logger bazuje np. na spdlog lub podobnej bibliotece formatujacej).
    *   `EterBase::EntityId` z `"../EterBase/StrongTypes.h"`.
    *   `EterBase::PacketResult` z `"../EterBase/PacketResult.h"`.
    *   Biblioteka standardowa: `<cstdint>`, `<vector>`, `<optional>`, `<string_view>`, `<expected>`, `<format>`.

*   **Drzewo dyrektyw `#include` i Ryzyka:**
    *   Brak widocznych zaleznosci cyklicznych w obrebie klas i plikow miedzy soba, sa dobrze odizolowane.
    *   Oba pliki naleza do roznych przestrzeni nazw (`GameLib::CombatMath` vs `Metin2::CombatMath`), co warto uporzadkowac na jedna.

*   **Model pamieciowy:**
    *   Kod w miare mozliwosci operuje na przekazywaniu danych przez wartosc (np. `BuffType`, `uint32_t`) oraz na bezpiecznych referencjach (np. wewnetrzne lapanie do wektora).
    *   `BuffDurationMonitor` zarzadza pamiecia buffow za pomoca dynamicznego wektora `std::vector<ActiveBuff>`.
    *   Pelen brak czystych wskaznikow C. Wszystko opiera sie na typach prostych oraz inteligentnych kolekcjach z biblioteki standardowej (RAII).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**GameLib::CombatMath (DoTDamageCalculator.h)**

*   **enum class DoTType : uint8_t**
    *   Rola: Reprezentuje typ zadawanego obrazenia periodycznego (Poison=0, Bleeding=1).

*   **struct DoTStats**
    *   Rola: Przechowuje statystyki obrazen.
    *   Pola:
        *   `uint32_t baseDamage` (offset 0): Bazowe obrazenia na tick.
        *   `uint32_t duration` (offset 4): Czas trwania efektu (ticki lub sekundy).
    *   Rozmiar: 8 bajtow.

*   **struct DoTDamageEvent** (dziedziczy po `Core::IEvent`)
    *   Rola: Zdarzenie emitowane przez kalkulator na magistrale.
    *   Pola:
        *   `EterBase::EntityId attackerId`
        *   `EterBase::EntityId victimId`
        *   `uint32_t damage`
        *   `DoTType type`
    *   Konstruktor: Wymaga wszystkich powyzszych pol do prawidlowej inicjalizacji.

*   **class DoTDamageCalculator**
    *   Rola: Typowa bezstanowa klasa (Static Method Holder) sluzaca do wyliczania obrazen. Wlasciciel watku - watek wywolujacy.
    *   Metody publiczne:
        *   `static EterBase::PacketResult<uint32_t> CalculateDamage(DoTType type, EterBase::EntityId attacker, EterBase::EntityId victim, std::optional<DoTStats> stats)`
        *   *Efekty uboczne:* Publikuje event w `Core::EventBus`, loguje przez `ModernLogger`.
        *   *Warunki wstepne:* `stats` nie moze byc `nullopt`, wyliczone mnozenie nie moze dac `0`. Wynik owiniety w monade.

**Metin2::CombatMath (BuffDurationMonitor.h)**

*   **enum class BuffType : uint8_t**
    *   Rola: Enumerator dostepnych w grze buffow (None=0, AuraMiecza=1, SilneCialo=2, Berek=3).

*   **struct ActiveBuff**
    *   Rola: Wiadomosc (struktura) dla pojedynczego aktywnego buffa.
    *   Pola:
        *   `BuffType type` (offset 0)
        *   `uint32_t remainingTimeMs` (offset 4)
        *   `bool warningTriggered` (offset 8)
    *   Rozmiar: 9-12 bajtow zaleznie od alignmentu kompilatora.

*   **struct BuffUpdateResult**
    *   Rola: Odpowiedz systemu po zaktualizowaniu czasu.
    *   Pola:
        *   `bool hasExpiringBuffs`
        *   `std::vector<BuffType> expiringBuffs`

*   **class BuffDurationMonitor**
    *   Rola: Instancja sledzaca bufy gracza/potwora. Zawiera stan pamieci na swoim wektorze, nalezy do watku ktory na niej operuje (nie posiada wlasnej synchronizacji `std::mutex`).
    *   Metody publiczne:
        *   `explicit BuffDurationMonitor(uint32_t warningThresholdMs = 5000)`: Inicjalizuje prog.
        *   `bool AddBuff(BuffType type, uint32_t durationMs)`: Zwraca `true` jesli ok, odrzuca typ `None`. Jesli buff jest juz aktywny, uaktualnia jego czas i wyzerowuje ostrzezenie jesli jest taka koniecznosc.
        *   `bool RemoveBuff(BuffType type)`: Zwraca `true` po pomyslnym usunieciu z listy.
        *   `BuffUpdateResult Update(uint32_t deltaTimeMs)`: Kluczowa metoda do wywolywania co klatke (OnUpdate). Automatycznie usuwa ze struktury buffy ktorych czas uplynal oraz powiadamia ktore wlasnie weszly w `warningThreshold`.
        *   `bool IsBuffActive(BuffType type) const`
        *   `std::optional<uint32_t> GetRemainingTime(BuffType type) const`
        *   `void ClearAllBuffs()`
        *   `void SetWarningThreshold(uint32_t thresholdMs)`
        *   `uint32_t GetWarningThreshold() const`
    *   Wewnetrzny stan:
        *   `uint32_t warningThresholdMs`
        *   `std::vector<ActiveBuff> activeBuffs`

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):

*   Obecnie obie implementacje sa czystym C++ i dzialaja w warstwie domeny. Mostek na UI / Python polega w tym wypadku wylacznie na zdarzeniach.
*   `DoTDamageCalculator` nie eksportuje do Pythona wlasnych metod C-API, lecz zamiast tego wysyla `DoTDamageEvent`. Prawdopodobnie inna instancja (np. PythonEventBridge) nasluchuje ich na EventBusie, aby wywolac w UI funkcje Pythona ukazujaca liczbe obrazen dookola ciala ofiary.
*   W pakietach `TPacketGCDamageInfo` moga znajdowac sie flagi czy dany dmg jest obrazeniami w czasie (Poison). Klasa prawdopodobnie przelicza obrazenia wedlug pakietow.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):

*   **Brak synchronizacji wielowatkowej:** Zarowno EventBus moze byc roznym srodowiskiem jak i `BuffDurationMonitor`. `BuffDurationMonitor` absolutnie nie powinien byc wywolywany z watku sieciowego (`Network`) dla metody `AddBuff` podczas gdy `Update` leci z glownego watku `MainD3D`. Jesli taka sytuacja zaszlaby w kodzie, wektor `activeBuffs` ulegnie korupcji, a program napotka `SEGFAULT`. W przypadku wstrzykiwania do roznych watkow bedzie wymagal muteksa.
*   **Aritmetyka na unsigned integers:** Modul wykorzystuje `uint32_t`. W procedurze zmniejszania trwania `it->remainingTimeMs -= deltaTimeMs;`, zastosowano wlasciwe i obowiazkowe sprawdzenie `if (it->remainingTimeMs <= deltaTimeMs)`. Jesli by nie bylo tego warunku (czeste bledy w modyfikacjach m2), pojawilby sie underflow do `4294967295`, dajac nieskonczony czas trwania buffa.
*   **Obliczanie mnoznika obrazen DoT:** W klasie `DoTDamageCalculator`, wymnazanie wartosci w lambdzie: `uint32_t calculatedDamage = s.baseDamage * s.duration;` posiada niebezpieczenstwo Integer Overflow. O ile w grze obrazenia bazowe to rzedy 500-2000, a duration np 10 to zagrozenia nie ma.
*   **Zarzadzanie Pamiacia:** Obiekt vector alokuje pamiac na bufy na stosie kopca (heap), ale zwalnia wszystko odpowiednio - pamiec jest trzymana poprawnie przez standardowe narzedzia jezyka.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):

*   **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
    Aby dodac nowy rodzaj efektu buffa:
    1. Otworz plik `src/GameLib/BuffDurationMonitor.h`.
    2. Dopisz kolejna pozycje w `enum class BuffType` (np. `OgnistyDuch = 4`).
    3. Zaktualizuj nazwe w switchu `GetBuffName`.
    Aby dodac nowy typ DoT:
    1. Otworz `src/GameLib/DoTDamageCalculator.h`.
    2. Dopisz wartosc np. `Fire = 2` w obrebie `enum class DoTType`.

*   **Jak debugowac i logowac:**
    *   Wszystkie bledne dane matematyczne z DoT odrzuca operator `std::unexpected`. Zawsze sprawdzaj typ zwrotny metoda `.has_value()`.
    *   Rejestrowane bledy i wartosci sa odnajdowane poprzez logera: `EterBase::ModernLogger::Info`.

*   **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
    *   Klasy stanowia zamknieta powloke dziedziny (Domain). Zaden interfejs graficzny D3D nie jest ladowany podczas ich alokacji.
    *   Mozna podpiac wlasny prosty mockowy system na bazie `doctest`.
    *   Nalezy zainicjalizowac recznie `Core::EventBus` z atrapa (dummy), aby metody `Publish` nie zrzucaly gry o brak podlaczonego event busa. W klasie `DoTDamageCalculator` metoda static bezposrednio dotyka globalnego EventBus (co jest typowym anti-patternem), dlatego przy unit testach EventBus musi zyc na czas testu.
    *   Dla testu monitora tworzysz `BuffDurationMonitor monitor(5000);`, przypinasz buffa `AddBuff(BuffType::AuraMiecza, 6000)` a nastepnie robisz assercje po wyslaniu `Update(1000)` by sprawdzic flagi.
