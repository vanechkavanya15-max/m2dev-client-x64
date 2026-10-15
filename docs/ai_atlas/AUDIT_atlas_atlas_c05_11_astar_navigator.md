---
task_id: "atlas_c05_11_astar_navigator"
cluster: "WLD"
module_name: "A* Nawigator - Wyznaczanie Sciezki w Swiecie Gry"
target_files:
- src/GameLib/AStarNavigator.cpp
- src/GameLib/PathNode.h
- src/GameLib/PathSplineSmoother.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c05_11_astar_navigator.md"
architecture_layer: "Swiat Gry, Teren, Kolizje i Nawigacja"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Modul implementuje podstawowy algorytm nawigacji i wyszukiwania sciezki w grze przy uzyciu algorytmu A* (A-Star). Wspolpracuje z interfejsem CollisionGrid, umozliwiajac omijanie przeszkod na mapie. Z kolei `PathSplineSmoother` wygladza odnaleziona, kanciasta sciezke przy uzyciu splajnow Catmull-Rom.
- **Moment wywolania:** Wywolywany prawdopodobnie w glownej petli gry w obsludze zdarzen poruszania sie (OnUpdate/Input) gdy uzytkownik wyznacza nowy cel podrozy (klikniecie na mape) lub z serwera przychodzi zadanie zmiany pozycji obiektu.
- **Przeplyw danych:** 
  1. Wejscie: `Point start`, `Point end`, referencja do `CollisionGrid`.
  2. A* iteruje po kafelkach 2D uzywajac `std::priority_queue` i ocenia koszt z uzyciem heurystyki dystansu ukosnego (Diagonal distance). 
  3. Po osiagnieciu celu odtwarzana jest wektorowa trasa `std::vector<Point>`.
  4. Nastepnie kanciasta trasa moze byc przekonwertowana na Waypointy 3D i przepuszczona przez `PathSplineSmoother::SmoothPath`, ktory zwraca gesta siatke punktow `std::vector<Waypoint>` za posrednictwem `std::expected`. Emituje takze zdarzenie `PathSmoothedEvent` do `Core::EventBus`.
- **Cykl zycia obiektow:**
  - `AStarNavigator` jest bezstanowy, moze byc traktowany jak utility/singleton lub lokalnie alokowany obiekt na stosie. Nie utrzymuje stanu po wyjsciu z `FindPath`.
  - `PathNode` alokowany lokalnie podczas operacji A*, uzywany tylko jako prosta struktura C-podobna.
  - `PathSplineSmoother` tez bezstanowy z delete na kopiowanie (zabezpieczenie na przyszlosc). Publikuje eventy po sukcesie. 

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Inne systemy (np. klienta, PythonPlayer, podsystem klikniec myszka) wywoluja metody instancji `AStarNavigator` oraz `PathSplineSmoother` do wyznaczania trasy do poruszania z punktu A do punktu B.
- **Zaleznosci wyjsciowe (Outbound):** 
  - `CollisionGrid` (interfejs polimorficzny pozwalajacy sprawdzac teren).
  - `Core::EventBus` zalezny od UserInterface (dla eventu wygladzenia sciezki w `PathSplineSmoother`).
  - `EterBase::EntityId`, `EterBase::ModernLogger`, `EterBase::StrongTypes`.
  - `Navigation::WaypointTracker` (dla bazowych typow, jak np. `Waypoint`).
- **Drzewo dyrektyw `#include`:** 
  - AStarNavigator: `<cstdint>`, `<vector>`, `<queue>`, `<unordered_map>`, `<cmath>`, `<algorithm>`, `<stdexcept>`. (Brak cyklicznych zaleznosci, czyste biblioteki standardowe).
  - PathNode: `<cstdint>`.
  - PathSplineSmoother: `<vector>`, `<expected>`, `<span>`, `<optional>`, `<format>`, `<cmath>`, `<cstdint>`, `"WaypointTracker.h"`, `"../EterBase/StrongTypes.h"`, `"../EterBase/LogModern.h"`. Deklaracja zapowiadajaca `Core::EventBus` zeby zapobiec zaleznosciom wstecznym.
- **Model pamieciowy:** Brak trwalych alokacji na stercie wewnatrz tych klas (poza strukturami lokalnymi np. std::vector, std::unordered_map w trakcie wykonywania obliczen). Przewaga alokacji na stosie i uzycie `std::span` dla unikniecia kopii buforow w `PathSplineSmoother`. Wyniki sa w zwracane jako instancje `std::vector` z zarzadzaniem RAII. Zwracanie przez `std::expected`.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur:**
  - `GameLib::Point`: 2D wektor int32_t, 8 bajtow. Czyste dane, x i y.
  - `GameLib::CollisionGrid`: interfejs z funkcja `IsBlocked`. Polimorficzny.
  - `GameLib::AStarNavigator`: nawigator, brak stanow class fields.
  - `PathNode`: struct x, y, gScore, hScore, fScore, pointer na parent. Rozmiar: 4+4+4+4+4+8 = 28 bajtow (z paddingiem pewnie 32 bajty w 64-bit).
  - `Navigation::SplineError`: enum class bledow (EmptyPath, InsufficientPoints, itp.).
  - `Navigation::PathSmoothedEvent`: 24 bajty (EntityId 8B, size_t 8B, size_t 8B). Event wysylany przez EventBus.
  - `Navigation::PathSplineSmoother`: Wygladzacz tras Catmull-Rom. Bezstanowy.
- **Tabela Metod Publicznych:**
  - `AStarNavigator::FindPath(const CollisionGrid& grid, const Point& start, const Point& end)` -> `std::vector<Point>`: Szuka sciezki. Warunki wstepne: `grid.IsBlocked(start) == false`. Zwraca pusta wektor jak nie znajdzie.
  - `PathNode::UpdateCosts()` -> `bool`: aktualizuje `fScore` = `gScore` + `hScore`.
  - `PathSplineSmoother::SmoothPath(EterBase::EntityId entityId, std::span<const Waypoint> inputPath, uint32_t subdivisions)` -> `std::expected<std::vector<Waypoint>, SplineError>`: Dodaje punkty za posrednictwem Catmull-Rom. Emituje zdarzenia. Nie toleruje pustych sciezek (zwroci SplineError::EmptyPath).
- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  - `Point`: `[0x0] uint32_t x`, `[0x4] uint32_t y`.
  - `PathNode`: `[0x0] int32_t x`, `[0x4] int32_t y`, `[0x8] float gScore`, `[0xC] float hScore`, `[0x10] float fScore`, `[0x18] PathNode* parent`. Offsety przydatne pod FFI. (Zakladajac align 8 ze wzgledu na pointer).
  - `PathSmoothedEvent`: `[0x0] EntityId entityId`, `[0x8] size_t originalPointCount`, `[0x10] size_t smoothedPointCount`.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Kod nie obsluguje bezposrednio opcodow pakietow, ale uzywa narzedzi przygotowujacych trajektorie (np. dla pakietu `CG_MOVE` lub powiazanych z wysylaniem lokalizacji paczek od klienta do serwera np. co okreslony dystans czy kat ruchu).
- **Metody Pythona (`PyMethodDef`):** Modul nie posiada bezposredniego powiazania z modulem pythonowym. Operacje C++ wywolywane z warstw interfejsowych prawdopodobnie mapowane na event klikniecia na ziemie `player.SetTargetDestination`. Zadne API Pythona nie wystepuje w zrodle samych analizowanych plikow.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Funkcje powiazane z A* i wygladzaniem moga pochlonac duzo cykli procesora (A* dziala z powolna `std::unordered_map`). Klasy nie sa synchronizowane wewnetrznie mutexami, powinnysmy wiec traktowac je jako wywolywane w glownym watku klienta lub watku dedykowanym Pathfindingowi z osobnymi (kopiowanymi) instancjami wejscia/wyjscia.
- **Potencjalne punkty awarii:** 
  - Wywolanie `AStarNavigator` w obrebie otwartego swiata bez limitu glebokosci przeszukiwania: W przypadku bardzo zlozonych, zabarykadowanych map lub braku dostepu do celu, A* zacznie analizowac gigantyczna liczbe kafelkow, co grozi przycieciem klatek animacji klienta.
  - Wyciek w algorytmie A* nie grozi, obiekty `std::unordered_map` radza sobie same, ale czeste zapytania dla duzych sciezek w grze moga powodowac problemy z OOM lub gc mapy hash.
- **Zarzadzanie zasobami (RAII):** Kod bardzo ladnie radzi sobie z pamiecia dzieki `std::vector` oraz `std::unordered_map`. Zero czystych `new` w `AStarNavigator`. Splajner zabezpieczony przed podaniem ilosci podzialow (subdivisions) = 0 co spowodowaloby krasza wynikajacego z dzielenia przez zero.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** 
  - Dodaj limit glebokosci poszukiwan (np. maksymalny `fScore` lub licznik operacji) do while loop w `FindPath`, zeby uniknac lagow (timeout algorithm). 
  - Zamien uzycie std::unordered_map na pre-alokowany grid w celu optymalizacji i unikniecia alokacji na stercie w czasie A*.
- **Jak debugowac i logowac:** Do modyfikacji A* nalezy wstrzyknac instancje logowania, ewentualnie dodac warunkowe logi preprocesora podczas obrotow while dla poszczegolnych krokow. `PathSplineSmoother` posiada juz mechanizmy `EterBase::ModernLogger`, do ktorych mozna podpiac sink.
- **Jak testowac bez interfejsu graficznego:** Modul dziala calkowicie bez stanu 3D. Wystarczy napisac prostego mokowanego implementatora klienta `CollisionGrid` i odpalic bezposrednio przez framework Doctest w C++ z komend: np. symulujac labirynt 10x10.
