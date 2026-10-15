---
task_id: "atlas_c04_13_defense_reduction"
cluster: "CBT"
module_name: "Redukcje Obrazen i Kalkulator Odpornosci Elementarnych"
target_files:
- src/GameLib/DefenseReductionFormula.h
- src/GameLib/ElementalDefenseCalculator.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c04_13_defense_reduction.md"
architecture_layer: "System Walki, Umiejetnosci i Fizyka Pociskow"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul "Redukcje Obrazen i Kalkulator Odpornosci Elementarnych" odpowiada za matematyczne i bezstanowe kalkulacje obrazen w systemie walki gry Metin2, koncentrujac sie na dwoch glownych aspektach:
1. Obliczanie fizycznych obrazen otrzymywanych przez cel z uwzglednieniem podstawowej obrony (Armor) i redukcji procentowych (Resistance) np. odpornosc na miecze czy sztylety (w `DefenseReductionFormula`).
2. Obliczanie obrazen elementarnych z uwzglednieniem atakow zywiolowych (Ogien, Lod, Blyskawica, Wiatr, Ziemia, Mrok) i odpornosci celu na dany zywiol (w `ElementalDefenseCalculator`).

Z punktu widzenia architektonicznego, kod zaimplementowany zostal w przestrzeniach nazw (namespace `BattleCalculator` i `GameLib`) jako zbior metod statycznych. Nie posiadaja one stanu, opieraja sie w calosci na otrzymanych parametrach wejsciowych, dzieki czemu nie wystepuja w nich wspoldzielone mutowalne zasoby.

- **Faza wykonania w petli gry:** Funkcje te powolane sa do zycia podczas predykcji obrazen na kliencie (w module Gameplay/CombatDamagePredictor) a takze w obsludze asynchronicznych komunikatow/komend obrazen naplywajacych z serwera, umozliwiajac prawidlowa synchronizacje statystyk miedzy graczem, a klientem gry bez opoznien wizualnych (Zero-conflict design).
- **Przeplyw danych (Data Flow):** Parametry oparte sa o nowoczesne konstrukcje z C++23. W wartosciach uzywane sa unikalne obiekty tozsamosci `EterBase::EntityId` dla identyfikacji agresora i celu. Parametry statystyk przekazywane sa w kontenerze `std::optional`, wspierajac implementacje tzw. "monadic operations" (`and_then`, `transform`), gdzie bledy i braki atrybutow propagowane sa nizej do rezultatu bez koniecznosci stosowania wielopoziomowych instrukcji warunkowych IF. Wynik z obu funkcji kalkulacyjnych zwracany jest nie jako tradycyjny typ lub pointer w "out-parameter", ale jako obiekt rezultatu wyjsciowego `std::expected` lub `EterBase::Result`, co gwarantuje scisla obsluge kazdego przypadku, chroniac pamiec klienta. Na samym koncu procesu asynchronicznie emitowane sa zdarzenia powiadamiajace inne podsystemy (EventBus) i obsluge np. interfejsu GUI - takie jak `DefenseReductionAppliedEvent` lub `ElementalDamageCalculatedEvent`.
- **Cykl zycia obiektow (Lifecycle):** Kalkulatory i funkcje matematyczne nie wymagaja inicjalizacji i dealokacji - wykorzystuja zaleznosci efemeryczne i zmienne lokalne (przechodza na tzw. stos funkcji/stack memory) unikajac calkowicie wycieku pamieci (Zero-Allocation-Overhead dla wywolan w GameLoop).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** 
Obecnie zaden inny system w bezposrednim drzewie projektu nie implementuje tego modulu - brak `include` z tych plikow zrodlowych co swiadczy o tym, ze moduly te pelnia role narzedzia gotowego do integracji (np. przez pakiety `TPacketGCAttack` czy system statystyk). Czeka to na podpiecie m.in. przez EventBus.
- **Zaleznosci wyjsciowe (Outbound):** 
Oba pliki silnie zaleza od wewnetrznych modulow EterBase oraz systemu powiadomien w modelu asynchronicznym. 
- EterBase: `EterBase/StrongTypes.h` (dla identyfikatorow), `EterBase/Result.h` (zastepstwo std::expected dla zgodnosci z API), `EterBase/LogModern.h` (Logowanie i inspekcje debug).
- Podsystem GUI / Event System: `UserInterface/Core/EventBus.h`.
- **Drzewo dyrektyw `#include`:** 
`DefenseReductionFormula.h`:
  - `<optional>`
  - `<expected>`
  - `<algorithm>`
  - `"EterBase/StrongTypes.h"`
  - `"EterBase/Result.h"`
  - `"EterBase/LogModern.h"`
  - `"UserInterface/Core/EventBus.h"`
`ElementalDefenseCalculator.h`:
  - `<expected>`
  - `<optional>`
  - `<cstdint>`
  - `<format>`
  - `<span>`
  - `"EterBase/StrongTypes.h"`
  - `"EterBase/LogModern.h"`
  - `"UserInterface/Core/EventBus.h"`
- **Model pamieciowy:** Wymienione metody operuja niemal tylko i wylacznie bezstanowo uzywajac typow prostych, silnych tozsamosci (id/typu), struktur statystyk (kopiowanych/referencje) oraz wartosci opcjonalnych z biblioteki standardowej (stack memory). W ogole nie sa uzywane tradycyjne wkazniki `*` w C++ by chronic calosc przed dereferencja niewaznej pamieci - zgodnie z zasada "No Naked Pointers".

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Modul 1: `BattleCalculator::DefenseReductionFormula` (`src/GameLib/DefenseReductionFormula.h`)**
| Nazwa (Klasa/Struktura) | Rola | Wielkosc w bajtach | Wlasciciel watku |
| :--- | :--- | :--- | :--- |
| `DefenseReductionAppliedEvent` (dziedziczy po `UserInterface::Core::IEvent`) | Zdarzenie emitowane po kalkulacji obrazen fizycznych dla odseparowanego modulu UI. | min 16 bajtow | Przekazywane watkiem wykonawczym do EventBus (Watek glownego update). |
| `DefenseReductionStats` | Definicja statystyk uzywana do redukcji. Zawiera pole bazy defensywy i poziom postaci oraz resistans np na dany atak. | ok 9 bajtow (z alignment 12 bajtow) | Zalezy od calera, przetrzymywana w optional przez funkcje. |
| `DefenseReductionFormula` | Funkcje statyczne matematycznego silnika redukcji. | 0 (klasa bezstanowa z metodami statycznymi) | Dowolny watek. |

**Metody Publiczne (DefenseReductionFormula):**
- **Sygnatura:** `static EterBase::Result<float, EterBase::CombatError> CalculatePhysicalReduction(EterBase::EntityId attackerId, EterBase::EntityId defenderId, float rawDamage, const std::optional<DefenseReductionStats>& defenderStats)`
- **Typy argumentow:** `EterBase::EntityId`, `EterBase::EntityId`, `float`, `const std::optional<DefenseReductionStats>&`.
- **Wartosc zwracana:** Zwraca strukture wynikow i bledow (`EterBase::Result`). W przypadku braku bledu bedzie to `float` oznaczajacy koncowe obrazenia po mitygacji.
- **Warunki wstepne (pre-conditions):** ID atakujaego i broniacego sie gracza/moba nie moga byc niepoprawne lub puste (wymaga np poprawnych id z wartoscia wyzsza/rowna domyslnej `EntityId`). Minimalne obrazenia wejsciowe (`rawDamage`) musza byc dodatnie, by cokolwiek moglo ulegac pomniejszeniu.
- **Skutki uboczne:** Brak. Obliczenia sa bezstanowe i wywylaja jedynie polecenia powiadamiajace na logger oraz EventBus na zewnatrz modulu.

**Modul 2: `GameLib::ElementalDefenseCalculator` (`src/GameLib/ElementalDefenseCalculator.h`)**
| Nazwa (Klasa/Struktura) | Rola | Wielkosc w bajtach | Wlasciciel watku |
| :--- | :--- | :--- | :--- |
| `ElementalType` (enum class : uint8_t) | Typ zywiolu w walce (np. Ogien, Woda/Lod). | 1 bajt | Typ wyliczeniowy (kopiowany na stos). |
| `ElementalCalculateError` (enum class : uint8_t) | Okresla typ bledu wystepujacy np przy podaniu invalid parametrow przy liczeniu odpornosci. | 1 bajt | Typ wyliczeniowy. |
| `ElementalDamageCalculatedEvent` (dziedziczy po `UserInterface::Core::IEvent`) | Zdarzenie przekazujace przeliczony elementarny damage celu po asynchronicznej kalkulacji. | min 24 bajty | Przekazywany przez watek do EventBus. |
| `ElementalDefenseCalculator` | Stateless klasa posiadajaca tylko static function by obliczyc koncowy damage. | 0 (Klasa bezstanowa z metodami statycznymi) | Dowolny watek. |

**Metody Publiczne (ElementalDefenseCalculator):**
- **Sygnatura:** `static std::expected<int32_t, ElementalCalculateError> CalculateDamage(EterBase::EntityId attacker, EterBase::EntityId victim, ElementalType element, std::optional<int32_t> baseDamage, std::optional<int32_t> elementalDefense)`
- **Typy argumentow:** `EterBase::EntityId`, `EterBase::EntityId`, `ElementalType`, `std::optional<int32_t>`, `std::optional<int32_t>`.
- **Wartosc zwracana:** Oczekiwany pomyslny wynik calkowitego damage'a jako `int32_t` lub enum o bledzie `ElementalCalculateError` owiniete razem do `std::expected`.
- **Warunki wstepne (pre-conditions):** Brak mozliwosci wprowadzania ujemnego obrony zywiolu i wartosci negatywnych dla raw damage (sa sprawdzane w obsludze optional `and_then` wewnatrz funckcji i ew zwracany jest wtedy zly wynik w expected).
- **Skutki uboczne:** Logowanie dzialan w ModernLogger i submisja informacji w EventBus na poziomie calkowitej warstwy logicznej GUI do obslugi animacji czy cyferek z obraznieniami zadanych (Damage Numbers).

**Pamieciowy Layout Struktur (Memory Layout & Offsets) (Szacowane na platforme 64-bit Windows/Linux):**
- `DefenseReductionStats` offsety w przyblizeniu: `baseDefense` = 0x00, `level` = 0x04, `physicalResist` = 0x08.
- `ElementalDamageCalculatedEvent` base_class `IEvent` dziedziczony po virtualu, wiec zaczyna sie od tabeli wirtualnej na +0x0, a nastepnie pola sa rozmieszczane jako offsety na bazie alignment w systemie (64-bit).

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Kod nie zawiera obecnie sztywnych zalaczonych bindow bezposrednio z `Packet.h` (zero-conflict), aczkolwiek asynchroniczne odpalanie `EventBus` sugeruje, ze nasluchiwanie w podsystemie sieci to odbior wiadomosci o id np pakietow takich jak `HEADER_GC_DAMAGE_INFO` i mapowane sa do interfejsu (dane z tych pakietow GC bywaja parowane by uzywac `CalculatePhysicalReduction` dla predykcji po stronie klienta co widac uderzajac np z miecza, aby cyferki pokazaly sie szybciej bez opoznien serwera). Zaleznosc po API przewiduje uzycie identyfikatorow z serwera (`EntityId` zamias VID).
- **Metody Pythona (`PyMethodDef`):** Modul nie posiada bezposrednich wrapperow ani metod typu fastcall czy varargs z Python C-API (jak `PyMethodDef`). Interakcja ze srodowiskiem Pythona ma tu odbywac sie poprzez `EventBus` (system event-driven), co oznacza ze inna klasa (na przyklad warstwa widoku PythonApplication) subskrybuje sie do zdarzen wyemitowanych po skonczeniu dzialania np `DefenseReductionAppliedEvent`. To pozwala na minimalizacje uzycia Pythona poza jego domyslnym flow.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Poniewaz klasy `DefenseReductionFormula` oraz `ElementalDefenseCalculator` sa calkowicie pozbawione stanu z metodami stricte statycznymi, dzialajac na przekazanych kopiach wartosci, mozna je uznac za "Thread-Safe" przy dzialaniu. Jedynym niebezpiecznym puntem styku jest publikacja do `EventBus`. Musi on uzywac bezpiecznych zamkow (Mutexow / Spinlockow) na metodzie `Publish()` by zapobiec zablokowaniu jezeli np. UI bedzie iterowac po eventach we wlasnym watku (glownym GameLoop).
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** 
  1. Istnieje mechanizm dla `CalculatePhysicalReduction` ktory zapobiega spadku pomniejszonego obrazenia ponizej liczy "1.0f" (wbudowane instrukcje `if (mitigated < 1.0f) mitigated = 1.0f;`) co unika wystapienia redukcji ujemnych dajacych w efekcie "leczenie sie poprzez cios". Zabezpiecza rowniez przed podaniem ujemnego damagu przez sprawdzenie `if (rawDamage <= 0.0f) return 0.0f;`.
  2. Modul uzywa operacji na monadzie `.transform().value_or(rawDamage);`. Jezeli dane statystyk (optional) beda `std::nullopt` - zwrocone zostana surowe, niemodyfikowane obazenia - trzeba uwazac by to zachowanie brac pod uwage przy braku statystyk przy pobieraniu obrazen w innych modulach.
  3. Modul elementalny uzywa podobnego systemu "minimum 0", chroni to przed wystepowaniem problemow ale przy obsludze zdarzen uzytkownik nie powinien oczekiwac ze wynik nie bedzie ponizej domyslnego obrazenia, wiec jesli na serwerze nie ma obslugi, tu jest zabezpieczenie klienta.
- **Zarzadzanie zasobami (RAII):** Nie dokonuje bezposrednich zrzutow ze sterte (zero slow `new` lub `delete`). Parametry dla struktur i eventow przechodza na stosie i niszcza sie bez obciazenia dla "Garbage Collectora" albo systemu referencji we wspoldzielonych pamiatkach. Jest to zoptymalizowane specjalnie pod szybkie ilosc wywolan w np. predykcji lotu i strzalow od wielu lucznikow pod rzad na sekunde.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** 
  1. Zidentyfikuj co musisz policzyc (np "Odpornosc na Magie w procencie"). Dodaj pole takie do `DefenseReductionStats` lub w razie nowej grupy logiki stworz odrebny enum w `ElementalDefenseCalculator.h`.
  2. Zmodyfikuj mechanizm wewnetrznej kalkulacji lambda by uwzglednic to nowe pole w matematycznych przelicznikach, utrzymujac je w `std::max / std::min` dla minimalizowania obciazen i zapobiegania "underflow".
  3. Emituj dodatkowe pole z nowej kalkulacji badz zmodyfikuj interfejs klasy emituujacej powiadomienie do widoku gracza, upewniajac sie w systemie logiki UI (gdzies indziej zadeklarowanym w subskrybcji do `EventBus`), ze zdarzenie wykoerzystuje te zmiane bezposrednio.
- **Jak debugowac i logowac:** Modul uzywa `EterBase::ModernLogger::Log`. Najwazniejsze dane podgladamy poslugujac sie np `ModernLogger::LogLevel::Debug` lub sprawdzajac je w systemie rejestrow klienta gry na standardowych wyjsciach.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** W tym przypadku, jako ze modul jest bezstanowy, mozemy wykorzystac bardzo prosty Unit Test piszac maly program w `g++ -std=c++23` bez podpinania sie do Direct3D. Wymagane jest tylko wrzucenie odpowiednich stubow/mockow dla klasy EventBus i zalaczenie glownego naglowka. Podajemy przykladowe struktury do wejscia statycznej funcji `CalculatePhysicalReduction` lub `CalculateDamage` i assercjonujemy `std::expected` ze zawiera spodziewana wartosc dla konkretnych testow z matematyki klienta gry.
