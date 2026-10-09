---
task_id: "atlas_c04_15_mount_combat_bridge"
cluster: "CBT"
module_name: "Modyfikatory Walki z Konia i Wierzchowcow"
target_files:
- src/GameLib/MountCombatModifierCalculator.h
- src/Client/Gameplay/MountDomain.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c04_15_mount_combat_bridge.md"
architecture_layer: "System Walki, Umiejetnosci i Fizyka Pociskow"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Modul ten odpowiada za obliczanie pasywnych modyfikatorow bojowych (wartosc ataku, obrona, ich mnozniki) dla postaci poruszajacych sie na wierzchowcach oraz zarzadzanie stanem wierzchowcow. 
- **Architektura C++23:** Modul `MountCombatModifierCalculator` jest zestawem bezstanowych funkcji, wykorzystujacym `std::expected` oraz abstrakcje jak `EterBase::EntityId` do wyliczania wartosci. 
- **Zarzadzanie Domena:** Interfejs `IMountDomain` z modulu Gameplay odcina warstwe interfejsu UI i pythona od scislej mechaniki C++. Zapewnia to zachowanie zasady Zero-Conflict (brak bezposrednich powiazan modulu rozgrywki ze starym kodem wizualizacji).
- **Przeplyw Danych i Cykl Zycia:** Zamiast modyfikowac wlasciwosci wewnatrz glownej petli postaci, kalkulator wylicza parametry na zadanie (np. po wejsciu na konia), a wynik jest przekazywany w swiat poprzez szyne wiadomosci `UserInterface::Core::EventBus` za pomoca `MountModifierCalculatedEvent`. Zarzadzaniem obiektami mountow zajmuje sie glownie domena, utrzymujac cykl zycia uzywajac `std::unique_ptr`.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound):** Kod jest wywolywany najprawdopodobniej przy pakiecie C2G / G2C dotyczacym przesiadki (mount/dismount) lub podczas ataku/przeliczania statystyk klienta w instancji `InstanceCombatComponent`. Moduly UI nasluchuja `MountModifierCalculatedEvent`, by uaktualnic np. wartosci obrazen na karcie postaci.
- **Zaleznosci wyjsciowe (Outbound):** Zaleznosci na wlasne typy systemowe (`EterBase::EntityId`, `EterBase::PlayerLevel`), nowozytny system logow (`EterBase::ModernLogger`), C++23 `std::expected`, `std::optional`, `std::unique_ptr` oraz `UserInterface::Core::EventBus` do komunikacji zewnetrznej.
- **Drzewo dyrektyw `#include`:** 
  - `src/GameLib/MountCombatModifierCalculator.h` dolacza m.in. `<cstdint>`, `<optional>`, `<expected>`, `<format>`, `../EterBase/Result.h`, `../EterBase/StrongTypes.h`, `../EterBase/LogModern.h`, `../UserInterface/Core/EventBus.h`.
  - `src/Client/Gameplay/MountDomain.h` dolacza `<memory>`, `<expected>`, `<string>`, `<cstdint>`.
  - Brak widocznych ryzyko cyklicznych dolaczen ze wzgledu na brak polaczen do instancji samej postaci lub starych plikow pythona.
- **Model pamieciowy:** W pelni nowoczesny, uzywa `std::unique_ptr<IMountDomain>` oraz wartosci (by value) do zdarzen. Zero wskaznikow (raw pointers), kod nie zarzadza wprost alokacja nowej pamieci poza stosowaniem.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)
- **`GameLib::CombatMath::MountType` (enum class)**
  - Reprezentuje typ uzywanego wierzchowca. 
  - Rozmiar: bazowo `uint8_t` (1 bajt).
  - Wartosci: `None = 0`, `Horse = 1`, `SpecialMount = 2`.
- **`GameLib::CombatMath::MountModifiers` (struct)**
  - Przechowuje wartosci numeryczne modyfikatorow bojowych wierzchowca.
  - Pola: `uint32_t attackBonus`, `uint32_t defenseBonus`, `float attackMultiplier`, `float defenseMultiplier`.
- **`GameLib::CombatMath::MountModifierCalculatedEvent` (struct -> UserInterface::Core::IEvent)**
  - Obiekt zdarzenia rzucany na EventBus po kazdym przeliczeniu modyfikatorow.
  - Pola: `EterBase::EntityId entityId`, `MountModifiers modifiers`.
- **`GameLib::CombatMath::MountCombatModifierCalculator` (class)**
  - Klasa kalkulacyjna (bezstanowa matematyka bojowa).
  - Metody publiczne: `CalculateModifiers(EterBase::EntityId, MountType, std::optional<EterBase::PlayerLevel>, EterBase::PlayerLevel) -> std::expected<MountModifiers, EterBase::CombatError>`.
  - Logika: Mnozy bonus ataku, np. bazowo poziom wierzchowca x2. Jesli `playerLevel` jest ponizej 25 dla typu `Horse`, rzuca blad. Generuje tez event systemowy przed zwroceniem nowej struktury `MountModifiers`.
- **`Client::Gameplay::IMountDomain` (interfejs klasowy)**
  - Zarzadza jazda gracza ze strony domenowej. Interfejs do podpiecia w architekturze Zero-Conflict.
  - Metody publiczne wirtualne: `Mount(uint32_t)`, `Dismount()`, `IsMounting()`, `GetMountLevel()`, `GetSpeedBonus()`, `CanChangeMotionMode(uint32_t)`. Zwracaja nowozytne typy (`bool`, `uint32_t`, `std::expected<void, std::string>`).
- **`Client::Gameplay::CreateMountDomain()` (funkcja fabryczna)**
  - Konstruuje swieza instancje implementacji domeny na heapie.
  - Zwraca `std::unique_ptr<IMountDomain>`.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Pakiety Sieciowe:** Moduly bezposrednio nie przetwarzaja sieci. Sa posrednikami uzywanymi przez instancje (np. CInstanceBase) obslugujace pakiety `CG_MOUNT` / `GC_MOUNT` lub pakiety stanu ataku z wierzchowcow.
- **Metody Pythona (`PyMethodDef`):** Modul jest izolowany od Pythona i eksportuje jedynie zdarzenia. Stary system `CPythonCharacterManager` i API `player.SetMount()` prawdopodobnie ostatecznie uzywaja `IMountDomain::Mount()` pod spodem przez pomosty (PythonNetworkStream).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Obiekty matematyczne z `MountCombatModifierCalculator` i event rzucany przez `EventBus::Publish` musza dzialac thread-safe. Brak dodatkowych mutexow oznacza, iz glowna petla zaklada wywolywanie ich z jednego glownego watku domenowego logiki.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** Gdy wierzchowiec nie zostanie poprawnie zainicjalizowany jako np. SpecialMount a ma pusty poziom (nullopt), uzyto domyslnego poziomu "1" w `.value_or(1)`. Zbyt niski poziom do jazdy na koniu (< 25) skutkuje bledem o charakterze biznesowym i moze nie zostac przechwycony poprawnie przez UI jezeli implementacja pominie sprawdzenie wyniku `std::expected` powodujac uzycie wczesniejszego modyfikatora.
- **Zarzadzanie zasobami (RAII):** Kod powoluje nowe domeny fabryka `CreateMountDomain()`, przez co wlascicielem ma byc kontener posiadajacy odpowiedni `unique_ptr`. Brak manualnego "delete".

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** W celu dodania np. modyfikatora predkosci poruszania sie w wierzchowcach nalezy: 
  1. Dodac pole `float speedMultiplier` w strukturze `MountModifiers`.
  2. Uwzglednic zasade jego wyliczania w switchu funkcji `CalculateModifiers`.
  3. Zmodyfikowac nasluchujacy kod kliencki (subskrybent `MountModifierCalculatedEvent`), by przyjal ten nowy parametr.
  4. Skompilowac bez modyfikowania starych monolitycznych klas w `EterLib` na biezaco.
- **Jak debugowac i logowac:** Wszystko bazuje na `EterBase::ModernLogger` (korzystajac z poziomow `.Debug`, `.Warn`, `.Info`). Aby zweryfikowac dlaczego postac zadaje male obrazenia, postaw breakpoint w srodku `CalculateModifiers` lub odczytaj logi dla "Entity {} is level {} and cannot benefit from horse combat".
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** W przypadku wlasnego testu modyfikatorow bojowych, wywolaj wylacznie metode klasy kalkulatora `MountCombatModifierCalculator::CalculateModifiers(...)` przekazujac spreparowane `StrongTypes` (`EterBase::EntityId(1)`, `MountType::Horse`, `EterBase::PlayerLevel(30)`). Zwracany typ `std::expected` i zdarzenia mozesz latwo zmockowac w GTest.
