---
task_id: "atlas_c04_02_combat_target_validator"
cluster: "CBT"
module_name: "Walidator Celu Ataku i Geometria Zasiegu Broni"
target_files:
- src/Client/Gameplay/CombatTargetValidator.h
- src/Client/Gameplay/CombatRangeCalculator.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c04_02_combat_target_validator.md"
architecture_layer: "System Walki, Umiejetnosci i Fizyka Pociskow"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Ten modul stanowi rdzen systemu weryfikacji celow oraz obliczania dystansu uderzen w systemie walki klienta Metin2 (w architekturze C++23).
- **Funkcja w architekturze:** `CombatTargetValidator` jest bezstanowym komponentem uslugowym weryfikujacym czy atak moze byc zainicjowany (sprawdza czy to ten sam byt, czy jeden z bytow jest w strefie bezpiecznej, czy cel jest niewrazliwy/NPC oraz waliduje logike trybow PvP). `CombatRangeCalculator` oblicza maksymalny zasieg bicia dla roznych typow broni (miecze, luki, sztylety) oraz sprawdza zasieg miedzy atakujacym a celem.
- **Moment wywolania:** Wywolywane z warstwy UI (np. przy kliknieciu myszka na cel z zamiarem ataku) lub w petli gry (OnUpdate) instancji postaci zanim wyslany zostanie pakiet z proba ataku na serwer. 
- **Przeplyw danych (Control & Data Flow):** Przyjmuje stan obu podmiotow w postaci `CombatEntityState` (ktore sa zwykle alokowane na stosie/zbudowane tymczasowo na podstawie instancji `CInstanceBase`). Po przeprowadzeniu matematycznych lub logicznych kalkulacji zwracany jest wynik typu `Core::Result<bool, Core::CommandError>` (czyli `std::expected`), ktory zglasza sukces `true` lub konkretny kod bledu z enum `Core::CommandError` (np. InvalidTarget, OutOfRange).
- **Cykl zycia:** Klasy `CombatTargetValidator` i `CombatRangeCalculator` nie sa instancjonowane - posiadaja wylacznie bezstanowe statyczne metody przyjmujace parametry z zewnatrz i oddajace wyliczone wartosci. Klasy sa bezstanowe (brak stanow), nie ma koniecznosci zwolnien pamieci (dealokacji).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound):** Moduly wyzszego rzedu, takie jak `InstanceCombatEngine` albo skrypty Pythona sterujace atakowaniem i autohuntem zglaszaja tu prosby.
- **Zaleznosci wyjsciowe (Outbound):** Posiadaja niewiele zaleznosci (tylko stany w obrebie modulu i logike C++). Zwracaja `std::expected` z biblioteki standardowej (lub jego odpowiednik `Core::Result`), obsluguja proste struktury DTO (np. `CombatEntityState`, `Core::MapCoords`). Brak powiazania z Direct3D, ani Granny - to czysty biznesowy back-end klienta.
- **Drzewo dyrektyw `#include`:** Wewnatrz naglowkow mamy:
  - `../Core/Result.h`
  - `../Core/DomainCommands.h`
  - `../Core/StrongTypes.h`
  - `<cstdint>`
  Brak ryzyka cykli dzieki prostej i mocnej separacji.
- **Model pamieciowy:** Wymienia dane calkowicie przez wartosc lub referencje (`const CombatEntityState&`, `const Core::MapCoords&`). Zmienne sa w wiekszosci na stosie (stack-allocated structures) - brak wskaznikow (raw pointers).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur:**
- `EntityType`: (enum class, 1 bajt) - klasyfikacja bytow (Unknown, Player, Monster, NPC, Pet, Mount).
- `WeaponType`: (enum class, 1 bajt) - klasyfikacja orieza (Sword, Dagger, Bow, TwoHanded, Bell, Fan, Arrow, None). W `CombatRangeCalculator.h`.
- `CombatEntityState`: (struktura C++, na stosie) - enkapsulacja atrybutow jednostki do walki:
  - `Core::EntityVid vid`
  - `Core::RaceVnum race`
  - `EntityType type`
  - `uint8_t level`
  - `uint8_t empire`
  - `bool isInvincible`
  - `bool inSafeZone`
  - `bool isDuelActive`
  - `bool isPvPModeActive`
- `CombatTargetValidator`: statyczna klasa utility, bezstanowa (0 bajtow obj).
- `CombatRangeCalculator`: statyczna klasa utility, bezstanowa (0 bajtow obj).

**Tabela Metod Publicznych:**
- W `CombatTargetValidator`:
  - `static Core::Result<bool, Core::CommandError> ValidateTarget(const CombatEntityState& attacker, const CombatEntityState& target) noexcept`
  Warunki wstepne: brak. Skutki uboczne: brak. Sprawdza poprawnosc celu.
- W `CombatRangeCalculator`:
  - `static float GetBaseRange(WeaponType weaponType) noexcept`
  Zwraca float w zaleznosci od typu orieza.
  - `static Core::Result<bool, Core::CommandError> IsTargetInRange(const Core::MapCoords& attackerPos, const Core::MapCoords& targetPos, WeaponType weaponType) noexcept`
  Warunki wstepne: zadeklarowane kordy przestrzenne. Skutki uboczne: brak. Sprawdza zasieg z prosta weryfikacja Pitagorasa za pomoca funkcji dystansu.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
`CombatEntityState` uzywa silnie typowanych domen (`EntityVid`, `RaceVnum` z `EterBase::StrongType`), cala paczka zajmie okolo 16 bajtow (jesli `vid` i `race` maja po 4 bajty), plus wyrownanie dla flag logicznych. Uzywa bezposredniego ukladu bez unikalnych wyrownan, swietnie nadaje sie pod ewentualne podpiecie C-API FFI z Rust lub Python (ale nalezy pamietac o wyrownaniu pamieci (alignment) zaleznym od kompilatora x64).

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Pakiety Sieciowe:** Powiazane posrednio. Prawidlowa weryfikacja w tym module skutkuje zwykle w module wyzszego rzedu wywolaniem pakietu `CG_ATTACK` (wymagajacy wyslania do serwera ataku/umiejetnosci, ktory moze wygenerowac `GC_ATTACK`). Brak bezposredniego mapowania kodow operacji na poziomie tych dwoch plikow naglowkowych.
- **Metody Pythona (`PyMethodDef`):** Modul jest zbytnio zaglebiony w warstwie Gameplay by byc prosto powiazany z PythonNetworkStream, jednak klient Pythonowy (UI) bazuje na poprawnosci tych walidacji zasiegu by podejmowac decyzje (jesli blad InvalidTarget lub OutOfRange wystapi w logice, odpowiednia informacja uzywana jest by np. postac automatycznie podeszla blizej celu podczas uzywania autohuntu).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Funkcje sa oznaczone `noexcept`, uzywaja stalych referencji read-only (`const &`), i sa kompletnie bezstanowe, wiec sa w pelni Thread-Safe (bezpieczne wielowatkowo). Moga byc wywolywane na dowolnym watku (np. glownym albo network worker).
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** 
  - Roznice w interpretacji wartosci bezpiecznej strefy miedzy klientem a serwerem moga skutkowac atakami bez efektu (desynchronizacja i ghost hity). 
  - Brak dedykowanego sprawdzenia konkretnej flagi trybu PvP w strukturze `CombatEntityState` (jest tylko jedna zmienna `isPvPModeActive`), co powoduje ze logika dla wielu roznych trybow zalezy od wlasciwego utworzenia tej struktury gdzies indziej w kodzie.
- **Zarzadzanie zasobami (RAII):** 100 procentowe automatyczne zarzadzanie pamiecia na stosie. Zwracaja rezultat przez `std::expected` (typ `Core::Result`), co zapobiega stosowaniu wyjatkow (throw) i zapewnia wydajnosc poprzez obsluge kodow z enum klasy bledow.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zaprojektuj nowy warunek logiczny (np. blokada ataku ze wzgledu na roznice poziomow wieksza niz 30 w pewnych strefach).
  2. Zaktualizuj enkapsulacje pol w `CombatEntityState` wewnatrz `CombatTargetValidator.h` (jesli potrzebujesz dodac nowa dana stanowa, jak np. identyfikator instancji gildii).
  3. Zmodyfikuj cialo funkcji w `CombatTargetValidator.cpp` poprzez dodanie nowej intrukcji warunkowej wewnatrz `ValidateTarget`, zwracajacej `std::unexpected(Core::CommandError::NowyBlad)`.
- **Jak debugowac i logowac:** Jesli rejestrowanie aktywnosci lub ostrzezen bedzie wymagane, uzywaj wylacznie bezposredniego wezwania statycznych metod `EterBase::ModernLogger::Debug(...)` zgodnie ze struktura loggera dla nowoczesnego C++. Zastepuj znaki formatowania argumentami bezposrednimi `{}` w stylu fmt.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** W zwiazku z calkowitym brakiem stanov pobocznych mozliwe jest natychmiastowe utworzenie zestawu testow jednostkowych w srodowisku CI/CD (np. uzywajac bibliotek klasy Google Test). Stworz mockowe obiekty strukturalne `CombatEntityState`, przeslij je przez statyczna funkcje `ValidateTarget` lub `IsTargetInRange` i przeprowadz prosta weryfikacje kodu z oczekiwanymi zwrotami przez interfejs `expected`.
