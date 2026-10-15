---
task_id: "atlas_c04_03_combat_damage_predictor"
cluster: "CBT"
module_name: "Predyktor Obrazen i Analiza Formuly Bitewnej"
target_files:
- src/Client/Gameplay/CombatDamagePredictor.h
- src/GameLib/BattleCalculator.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c04_03_combat_damage_predictor.md"
architecture_layer: "System Walki, Umiejetnosci i Fizyka Pociskow"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Modul `CombatDamagePredictor` oraz powiazany namespace `BattleCalculator` realizuja kluczowa czesc logiki gry: predykcje obrazen fizycznych po stronie klienta (Client-Side Prediction).

*   **CombatDamagePredictor:** Klasa sluzaca glownie do rzutowania kosci (RNG) w celach sprawdzenia, czy atak bedzie krytyczny, przeszywajacy, czy zostanie zablokowany, badz unikniety, a nastepnie obliczenia wstepnych obrazen na bazie statystyk. Dzieki temu klient jest responsywny (obrazenia i efekty pojawiaja sie na ekranie przed odpowiedzia serwera). Zwraca wynik uzywajac narzedzi C++23: `Client::Core::Result` (`std::expected`), w przypadku bledu zwracajac typ `Client::Core::CommandError`.
*   **BattleCalculator:** Modul statyczny C++20 definiujacy wysoce zoptymalizowane dzialania matematyczne i statystyki niezbedne do wlasciwego ustalania obrazen w oparciu o modyfikatory zalezne od typu broni, poziomow atakujacego i broniacego, oraz odpornosci na magie i bron, kompletnie odizolowany od warstwy UI i sieci, wykonywany z predykcja stalych (constexpr).
*   **Architektura & Cykl Zycia:** `CombatDamagePredictor` jest stanowy i inicjalizuje wlasny silnik RNG (Mersenne Twister `std::mt19937`) w momencie konstrukcji instancji. Natomiast kod zawarty w `BattleCalculator` jest calkowicie bezstanowy i mozna go uzywac jako darmowych czystych matematycznych funkcji inline na dowolnym etapie (OnUpdate, OnRender, czy nawet Worker Threads), choc realnie odbywa sie to po odebraniu wejscia uzytkownika i przed wygenerowaniem sieciowych pakietow akcji `Command` ze `src/Client/Core/DomainCommands.h`.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

### Zaleznosci wejsciowe (Inbound):
*   Nie wskazano jawnie w plikach zrodlowych kodu wywolujacego, ale na podstawie naglowkow `DomainCommands.h` jest prawdopodobne, ze ten modul jest aktywowany przez system akcji (Player Actions), m.in. z komend atakowania celu (`AttackCommand`, `ShootCommand`, `UseSkillCommand`). Prawdopodobnie wywolywany na logice CInstanceBaseCombat lub odp. kontrolerze encji (np. w przestrzeni UI / Input). Zaleznosci logiczne wystepuja wzgledem `DomainCommands.h` oraz `Result.h`.

### Zaleznosci wyjsciowe (Outbound):
*   `src/Client/Gameplay/CombatDamagePredictor.h` dolacza standardowe naglowki `<cstdint>`, `<random>`, a w szczegolnosci uzywa `../Core/Result.h` (ktory z kolei dostarcza `std::expected` dla C++23) oraz `../Core/DomainCommands.h`.
*   `src/Client/Gameplay/CombatDamagePredictor.cpp` wola `CombatDamagePredictor.h`, `<algorithm>` i `<expected>`.
*   `src/GameLib/BattleCalculator.h` dolacza standardowe naglowki `<cstdint>` oraz `<algorithm>`. Nie polega na zadnych klasach bazowych ani systemach okien (calkowity brak zaleznosci do EterLib, Granny3D, ani UI Event Bus-a w samym naglowku pliku). 

### Drzewo dyrektyw `#include`:
```cpp
// CombatDamagePredictor.h
#include <cstdint>
#include <random>
#include "../Core/Result.h"      // -> #include <expected>, <string>, <string_view>, <format>, "DomainErrors.h"
#include "../Core/DomainCommands.h" // -> #include <optional>, "StrongTypes.h"

// CombatDamagePredictor.cpp
#include "CombatDamagePredictor.h"
#include <algorithm>
#include <expected>

// BattleCalculator.h
#include <cstdint>
#include <algorithm>
```
Nie wykryto cyklicznych zaleznosci miedzy wyzej wymienionymi naglowkami.

### Model pamieciowy:
*   Zarowno w predykcji po stronie klienta jak i w kalkulacjach matematycznych wszystko opiera sie na czystym przekazywaniu struktur bitowych statystyk poprzez stala referencje `const Type&` i rzutowaniu pod maska do prymitywow `uint32_t`, `float`, itd. Nie uzyto tu alokacji pamieci dynamicznej w sterowaniu przeplywem; obslugiwane sa jedynie obiekty na stosie, a jedyny stan to wewnetrzny generator `m_rng`. Brak uzycia inteligentnych wskaznikow lub czystych wskaznikow C.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabela Klas i Struktur
| Nazwa (Przestrzen) | Rola | Wielkosc/Layout | Typ |
| :--- | :--- | :--- | :--- |
| `Client::Gameplay::AttackerStats` | Pakiet statystyk ofensywnych atakujacego. | 12 bajtow | struct |
| `Client::Gameplay::TargetStats` | Pakiet statystyk defensywnych celu ataku. | 12 bajtow | struct |
| `Client::Gameplay::DamagePredictionResult` | Wynik symulacji w predyktorze. | 8 bajtow | struct (uint32_t + 4 bools) |
| `Client::Gameplay::CombatDamagePredictor` | Glowny generator przewidywanego wyniku uderzenia na bazie RNG. | M.in. size of mt19937 | class (stanowy) |
| `BattleCalculator::DamageType` | Zestaw wartosci wyliczeniowych (enum) okreslajacy rodzaj uzytej broni lub zrodla obrazen. | 1 bajt (uint8_t) | enum class |
| `BattleCalculator::AttackStats` | Rozszerzone statystyki potrzebne do obliczen redukcji. | 12 bajtow | struct |
| `BattleCalculator::DefenseStats` | Szczegolowe dane o pancerzu celu uzywane w BattleCalculator. | 12 bajtow | struct |

### Tabela Metod Publicznych
**`Client::Gameplay::CombatDamagePredictor::PredictDamage`**
*   **Sygnatura:** `Client::Core::Result<DamagePredictionResult, Client::Core::CommandError> PredictDamage(const AttackerStats& attacker, const TargetStats& defender);`
*   **Warunki wstepne:** Wartosci procentowe szans (criticalChance, penetrateChance, dodgeChance, blockChance) nie moga przekraczac `100`. Jesli sa wieksze, funkcja zwroci `std::unexpected(CommandError::InvalidParameter)`.
*   **Skutki uboczne:** Postepuje (mutuje) wewnetrzny stan `m_rng`.

**`BattleCalculator::CalculateDamage`**
*   **Sygnatura:** `constexpr uint32_t CalculateDamage(const AttackStats& attackerStats, const DefenseStats& defenderStats, DamageType type) noexcept;`
*   **Warunki wstepne:** Brak krytycznych zaleznosci; clampowanie zapewnia poprawne zachowanie poza zakresem wejsciowym, lecz odpornosci i kary wchodza od 0 do 100 procent.
*   **Skutki uboczne:** Brak (czysta funkcja constexpr bezstanowa).

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
*   **Pakiety Sieciowe:** Kod ten operuje pod interfejsem graficznym, czesto przed jego synchronizacja, stad posluguje sie glownie komendami i odpowiedziami wezlowymi z pliku `src/Client/Core/DomainCommands.h`. Nie obsluguje bezposrednio binarnego polaczenia sieciowego. Struktury w nim zdefiniowane prawdopodobnie trafiaja bezposrednio lub posrednio do takich komend jak np. `AttackCommand` (gdzie targetVid to cel uderzenia a attackType umozliwia przekazanie specyfiki).
*   **Mostki Python C-API:** Ten modul C++ jest elementem niskopoziomowym klienta i nie zawiera zadnych symboli typu `PyMethodDef`. By uzyskac dostep do tych wynikow, inna czesc kodu musialaby zarejestrowac go przez wrapper.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
*   **Zasady wielowatkowosci:** Obiekt klasy `CombatDamagePredictor` nie jest thread-safe, poniewaz kazde rzucenie z `std::uniform_int_distribution` bedzie mutowalo `std::mt19937 m_rng`. Jesli AI zechce wielowatkowo prognozowac bitwy, nalezy alokowac jeden predyktor na watek (thread-local), lub zamykac mutacje w `std::mutex`. Przestrzen `BattleCalculator` jest jednak wolna od blokad z racji uzycia funkcji typu constexpr.
*   **Potencjalne punkty awarii (Crash Points & Edge Cases):**
    *   W predyktorze w bloku sprawdzania krytycznych trafien brakuje jawnego sprawdzania przepelnienia calkowitego w linii `result.damage *= 2;`. To moze generowac UB (Overflow) przy ogromnych liczbach.
    *   Funkcja rzutowania bazowych obrazen obsluguje clamp i max pod maska by wykluczyc strzaly ujemne.
*   **Zarzadzanie zasobami (RAII):** Kod jest bezalokacyjny i zero-memcpy, omijajac ryzyko wyciekow pamieci RAM.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
*   **Instrukcja dodawania nowej funkcji:**
    1.  Jesli nowa zasada tyczy sie wspolczynnikow obrony badz pancerza, poszerz struct `BattleCalculator::DefenseStats` i zaktualizuj wewnetrzne `constexpr CalculateDamage`. Nie modyfikuj zaleznosci bezposrednio w srodku algorytmu, zamiast tego uzywaj systemow clamp z `<algorithm>`.
    2.  Jesli nowa mechanika bitewna to "Status nakladany z pewna doza szansy" (np. krwawienie, trucizna), modyfikuj strukture `TargetStats` na `TargetStats` dodajac nowa szanse obrony oraz dopisz rzut RNG w `CombatDamagePredictor::PredictDamage`. Zauwaz, by zaktualizowac rowniez poczatkowe walidacje `> 100`.
*   **Jak debugowac i logowac:** Do predyktora najlepiej nie zaciagac makr starego typu ani klas EterBase/LogModern w pliku h; preferowane uzycie nowoczesnego loggera `EterBase::ModernLogger::Log()` tak jak mozna zaobserwowac w systemie zdarzen na pliku pobocznym `DefenseReductionFormula.h`.
*   **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Wystarczy dopisac plik `#include "CombatDamagePredictor.h"` w pliku typu `doctest` by wytestowac losowosc pod katem dystrybucji na tysiacach zapytan. Ze wzgledu na decoupling klasa nie inicjalizuje serwera gry. Metody bezstanowe sa trywialne do zunit testowania poprzez assert na zwroconym wyniku (constexpr umozliwia assertowania i mockowanie w calosci na etapie kompilacji: `static_assert(BattleCalculator::CalculateDamage({...}, {...}, BattleCalculator::DamageType::Physical) == expected);`).
