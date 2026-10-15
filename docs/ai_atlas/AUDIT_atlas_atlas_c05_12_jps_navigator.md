---
task_id: "atlas_c05_12_jps_navigator"
cluster: "WLD"
module_name: "Jump Point Search (JPS) - Ultra Szybki Nawigator Siatkowy"
target_files:
- src/GameLib/JumpPointSearchNavigator.cpp
- src/GameLib/JumpPointSearchNavigator.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c05_12_jps_navigator.md"
architecture_layer: "Swiat Gry, Teren, Kolizje i Nawigacja"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja w architekturze**: Modul realizuje algorytm Jump Point Search (JPS) do ultraszybkiego wyszukiwania sciezek (pathfinding) na siatce kolizji (CollisionGrid) reprezentujacej teren gry. Zastepuje on lub optymalizuje klasyczny algorytm A*, znaczaco przyspieszajac znajdowanie sciezki poprzez omijanie symetrycznie zbednych wezlow (tzw. jump points).
- **Moment wywolania**: Modul jest wywolywany na zadanie w ramach glownej petli gry (prawdopodobnie w fazie OnUpdate lub wyzwolony przez akcje gracza/sieci), gdy obiekt (Entity) potrzebuje wyznaczyc trase z punktu A do punktu B.
- **Przeplyw danych (Control Flow & Data Flow)**:
  1. Wywolanie funkcji `FindPath` z parametrami: siatka kolizji, punkt poczatkowy, punkt docelowy oraz identyfikator encji.
  2. Sprawdzenie, czy punkty poczatkowy i koncowy sa na obszarach dopuszczalnych do chodzenia (IsWalkable). W razie blokady, szybki powrot z bledem.
  3. Inicjalizacja kolejki priorytetowej otwartych wezlow oraz mapy kosztow i tras (A* z heurystyka octile).
  4. Sukcesywne wywolywanie metody `Jump` w dozwolonych kierunkach w poszukiwaniu znaczacych wezlow siatki, omijanie powtarzalnych prostych linii.
  5. Zwrocenie sciezki: po dotarciu do celu wektory sa odtwarzane od konca do poczatku, wektor jest odwracany i nastepuje publikacja zdarzenia `PathFoundEvent` na szynie `EventBus`. Zwracany jest takze rezultat uzywajac typu `EterBase::Result`.
- **Cykl zycia obiektow**: Klasa `JumpPointSearchNavigator` jest bezstanowa w kontekscie wyszukiwania - operuje tylko na dostarczonych argumentach oraz stertach lokalnych dla danej iteracji petli szukania. Nie przechowuje globalnego stanu pomiedzy wywolaniami (co umozliwia korzystanie z pojedynczej instancji nawigatora do obslugi wielu encji). Alokacja pamieci operacyjnej na sciezki (`std::priority_queue`, `std::unordered_map`) jest czyszczona natychmiastowo po zakonczeniu wyszukiwania.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound)**: Komponenty logiczne wlasciciela (kontrolery jednostek, systemy obslugi nawigacji kliknieciem), ktore wywoluja nawigator do odnalezienia poprawnej drogi.
- **Zaleznosci wyjsciowe (Outbound)**: 
  - `GameLib::CollisionGrid` - weryfikacja przeszkod i wezlow terenu.
  - `UserInterface::Core::EventBus` - rozglaszanie wiadomosci o znalezionych sciezkach (`PathFoundEvent`).
  - `EterBase::ModernLogger` - uzywane do komunikacji o przebiegu algorytmu lub jego awariach.
  - `EterBase::Result` i typy podstawowe.
- **Drzewo dyrektyw `#include`**: 
  - `<vector>`, `<optional>`, `<cstdint>`, `<queue>`, `<unordered_map>`, `<cmath>`, `<algorithm>` - naglowki standardowej biblioteki C++.
  - `"../EterBase/Result.h"`, `"../EterBase/StrongTypes.h"`, `"CollisionGrid.h"` - naglowki w `JumpPointSearchNavigator.h`. Brak zauwazalnego ryzyka zaleznosci cyklicznych dzieki prawidlowemu rozdzieleniu odpowiedzialnosci i braku naglowkow wzajemnych.
  - `"JumpPointSearchNavigator.h"`, `"../UserInterface/Core/EventBus.h"`, `"../EterBase/LogModern.h"` - naglowki w `JumpPointSearchNavigator.cpp`.
- **Model pamieciowy**:
  - `FindPath` operuje na referencjach do siatki stalych obiektow, eliminujac koszty kopiowania (`const CollisionGrid&`, `const Point&`).
  - Wykorzystanie RAII dla kolekcji stl podczas obliczen, bez uzywania surowych wskaznikow C. Przekazywanie wektorow wyjsciowych za pomoca `std::move`.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur**:
  - `GameLib::Point`: Prosta struktura wspolrzednych dla 2D. Pola: `uint32_t x`, `uint32_t y` (rozmiar 8 bajtow). Zapewnia operator rownosci i funkcje haszujaca `std::hash`. Obiekt przenoszony jest swobodnie poprzez kopie.
  - `GameLib::JumpPointSearchNavigator`: Glowny system nawigacji. Klasa calkowicie bezstanowa, mozna uzywac metod jako const. Nie jest bezposrednio wlascicielem zadnego watku.
  - `GameLib::PathFoundEvent`: Dziedziczy po `UserInterface::Core::IEvent`. Pola: `EterBase::EntityId entityId`, `std::vector<Point> path`. Kapsulkuje wynik wyliczania trasy dla innych czesci systemu (rozmiar zmienny w zaleznosci od wektora).
  - `GameLib::Node`: Lokalna struktura uzywana wewnatrz metody algorytmu do kolejki. Pola: `Point p`, `double gCost`, `double fCost`.

- **Tabela Metod Publicznych**:
  - `FindPath(const CollisionGrid& grid, const Point& start, const Point& end, EterBase::EntityId entityId) const` -> `EterBase::Result<std::vector<Point>, EterBase::NavigationError>`
    - Warunki wstepne: Siatka terenu (CollisionGrid) musi zostac poprawnie zainicjalizowana przez serwer/gre. Startowe i koncowe `Point` nie moga wchodzic w kolizje z `IsObstacle`.
    - Skutki uboczne: Asynchroniczna publikacja `PathFoundEvent` po szynie eventow. Zapis do logow `ModernLogger`.

- **Pamieciowy Layout Struktur (Memory Layout & Offsets)**:
  - `GameLib::Point`: offset `x` = 0x00, offset `y` = 0x04. Bardzo proste podpiecie pod narzedzia zewnetrzne (hooking).
  - `GameLib::PathFoundEvent`: VTable eventu + offsety na klase bazowa, dalej `entityId` oraz kontener `std::vector<Point>`.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- Modul dziala calkowicie po stronie logiki silnika w C++.
- **Pakiety Sieciowe:** Bezposrednio brak zaleznosci do pakietow, jednak na podstawie zdarzenia `PathFoundEvent` inne systemy moga wygenerowac pakiety ruchu np. CG (Client->Game). Kod nie operuje na kodach opcodow, polegajac na hermetyzacji zadania nawigacji.
- **Metody Pythona (`PyMethodDef`):** Funkcjonalnosc ta nie posiada bezposredniego wiazania (bindingu) API na jezyk Python w obrebie tego pliku zrodlowego.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Funkcja algorytmiczna jest const i thread-safe sama z siebie, pod warunkiem ze interfejs wywolan `CollisionGrid::IsObstacle` (oraz loggery/eventbusy) moga byc obslugiwane przez wiele watkow sekwencyjnie. Sam algorytm zaklada, ze w trakcie wyliczania drogi siatka i punkty sie nie zmienia.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Zamknieta sciezka lub ogromna otwarta struktura powoduje zwielokrotnione przeszukiwanie (zaleta JPS to znaczna redukcja wzgledem A* jednak pesymistyczny wariant i tak zwieksza utylizacje rdzeni procesora).
  - Wektory po przeksztalceniu moga byc puste, co system prawidlowo weryfikuje wysylajac zwroty bledu (np. przypadek gdy punkt konca jest przeszkoda loguje `EterBase::NavigationError::BlockedTerrain`).
  - Uzywanie typow `int` w metodzie `Jump` na ukladzie uint32_t zostalo odpowiednio obsluzone asercja logiczna z rzutowaniem upewniajacym (`IsWalkable` zabezpiecza ujemne wartosci koordynat, by na powrot przekazac uint32_t bez przepelnienia calkowitego pod `grid.IsObstacle`).
- **Zarzadzanie zasobami (RAII):** Kod operuje scisle i wylacznie wedlug poprawnych standardow zarzadzania pamiecia w C++ co redukuje szanse wystapienia wyciekow pamieci na mapach w trakcie przejsc uzytkownika/botow do zera. Brak manualnej alokacji/deakolacji `new`/`delete`.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Gdy planujesz uwzglednic nowy typ poruszania, dodaj dedykowane parametry `moveType` do `FindPath` umozliwiajace sterowanie typem komorki docelowej w `CollisionGrid`.
  2. Zmodyfikuj lokalna lambde `calculateHeuristic`, jesli w grze pojawia sie zmienne koszty ruchu po siatce (w JPS to utrudnione, warto zbadac D* / HPA* w takiej sytuacji, ale mozna wprowadzic wagi).
  3. Po ewentualnej zmianie `PathFoundEvent` (np. obsluga asynchroniczna z ID biletu), konieczna bedzie modyfikacja naglowka i subskrybentow w modulu obslugi AI i PlayerMovement.
- **Jak debugowac i logowac:**
  - Wlacz odpowiedni tryb `EterBase::ModernLogger::Debug` aby sprawdzic dokladna dlugosc odnalezionej sciezki.
  - Breakpointy powinno ustawiac sie na samym starcie `FindPath` oraz w okolicach tworzenia sciezki wstecznej w petli while (gdzie odbudowywana jest lista punktow).
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  - Modul nie ma zadnych obostrzen GUI (w przeciwienstwie np. do menedzerow okien czy directX). Do sprawdzenia algorytmu wystarczy zaimplementowac na boku maly mock interfejsu `CollisionGrid` (np. statyczna tablica dwuwymiarowa z boolami) z nadpisanym interfejsem dla potrzeb testow z doctest (pamiatajac by korzystac z dyrektyw zapobiegajacych podwojnym kompilacjom i nadpisywaniom).
  - Przechwytuj wyniki jako test jednostkowy by udowodnic prawidlowe rozdzielanie sciezki dla symetrycznych korytarzy.
