---
task_id: "atlas_c05_09_collision_grid"
cluster: "WLD"
module_name: "Siatka Kolizyjna i Testowanie Przenikania Postaci ze Swiatem"
target_files:
- src/GameLib/CollisionGrid.h
- src/Client/World/CollisionDetector.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c05_09_collision_grid.md"
architecture_layer: "Swiat Gry, Teren, Kolizje i Nawigacja"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Modul zapewnia fundamentalne mechanizmy do sprawdzania kolizji 2D i 3D miedzy aktorami (graczami, potworami) a swiatem gry. `CollisionGrid` odpowiada za efektywne pamieciowo przechowywanie siatki przeszkod terenu w postaci splaszczonego wektora bitow. Z kolei `CollisionDetector` dostarcza algorytmy detekcji kolizji (Cylinder-Cylinder, Cylinder-AABB) oraz logike wyliczania optymalnego przesuniecia - rozwiazywania ruchu, w tym "wall sliding" (slizganie sie po scianach).
- **Punkt wywolania w petli gry:** Funkcje detekcji sa wywolywane glownie w petli aktualizacji stanu gry (np. `OnUpdate` na instancji postaci), gdy nastepuje proba przemieszczenia aktora o dany wektor predkosci, jak i w narzedziach nawigacyjnych (np. A* lub Jump Point Search).
- **Przeplyw danych (Control Flow & Data Flow):** Kiedy podmiot zglasza wektor intencji ruchu, jest wywolywana funkcja `ResolveMovement`. Kod ten przetwarza predkosc w petli (maksymalnie 3 iteracje) aby umozliwic bezkolizyjne zsuwanie sie po przeszkodach. Kod najpierw sprawdza przeciecia ze srodowiskiem (`AABB`), a dopiero potem z innymi jednostkami (`Cylinder`). W przypadku trafienia modyfikowany jest wektor intencji za pomoca `CalculateSlideVector`.
- **Cykl zycia:** Instancje klasy `CollisionGrid` sa alokowane i wypelniane danymi na etapie ladowania fragmentow mapy, poprzez metody takie jak `LoadFromBuffer`. Sa niszczone (dealokowane) wraz z ukryciem mapy. Z kolei `CollisionDetector` dziala bezstanowo, wiec opiera sie po prostu na danych przekazywanych w argumencie bez uzycia globalnego cyklu zycia.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Do kodu odwoluje sie aparat wyszukiwania drogi (`AStarNavigator`, `JumpPointSearchNavigator`), a takze instancje kontrolujace zachowanie aktorow (takie jak `CActorInstance`), ktore wolaja testy detekcji. Z memory contextu widac, ze uzywa to takze `JumpPointSearchNavigator`.
- **Zaleznosci wyjsciowe (Outbound):** Kod odzwierciedla zelazna zasade ZERO-DIRECTX & ZERO-PYTHON. Implementacja uzywa wylacznie standardowej biblioteki C++23.
- **Drzewo dyrektyw `#include`:** W obu plikach uzyto miedzy innymi: `<cstdint>`, `<vector>`, `<span>`, `<stdexcept>`, `<string>`, `<string_view>`, `<algorithm>`, `<cmath>` oraz `<optional>`. Obie czesci modulu unikaja referencji w te i wewte, zapewniajac czysty DAG zaleznosci.
- **Model pamieciowy:** W `CollisionGrid` wlasciwe bity trzymane sa we wbudowanym `std::vector<uint8_t> data`. Algorytmy przestrzenne (`CollisionDetector`) silnie bazuja na typach przekazywanych prez wartosc lub const referencje (`const Cylinder&`, `const Vector3&`), calkowicie unikajac alokacji heapu i surowych wskaznikow C.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
| Nazwa | Rola | Wielkosc w bajtach | Wlasciciel Watku |
|-------|------|--------------------|------------------|
| `CollisionGrid` | Klasa dostepu do 2D bitow przeszkod, C++23 index | `sizeof(string) + sizeof(vector) + 8` | Watek Glowny |
| `Client::World::Vector3` | Struktura pozycji w przestrzeni | 12 bajtow | Watek Glowny |
| `Client::World::Cylinder` | Bryla ograczniczajaca aktora | 20 bajtow | Watek Glowny |
| `Client::World::AABB` | Prostopadloscian srodowiska (np. budynek) | 24 bajty | Watek Glowny |
| `Client::World::CollisionResult`| Struktura z normalna uderzenia i wektorem zagebienia | 20 bajtow | Watek Glowny |
| `Client::World::CollisionDetector`| Pojemnik dla statycznych funkcji API logiki testow | N/A (Klasa statyczna) | Bezstanowa |

**Tabela Metod Publicznych:**
- `CollisionGrid::IsObstacle(uint32_t x, uint32_t y) const noexcept -> bool`: Weryfikacja przeszkody na kordach x, y. Zwraca `true` dla pol w siatce oznaczonych bitem 1, lub jesli pozycja wykracza poza rozmiar siatki.
- `CollisionGrid::operator[](uint32_t x, uint32_t y) const noexcept -> bool`: Przeciazenie nowego operatora dostepu wielowymiarowego C++23, dziala identycznie jak `IsObstacle`.
- `CollisionGrid::LoadFromBuffer(std::span<const uint8_t> buffer)`: Inicjuje mape kolizji wpisujac dostarczone surowe bajty z powloki zewnetrznej do zoptymalizowanego wektora bitowego. Rzuca `std::invalid_argument` jesli span nie matchuje bitom.
- `Client::World::CollisionDetector::ResolveMovement(...) -> Vector3`: Przelicza ostateczny wektor ruchu postaci w oparciu o iteracyjne zderzenia i zlizgi. Oczekuje cylindra aktora, jego wektora szybkosci i zestawu klockow kolizyjnych na mapie.
- `Client::World::CollisionDetector::TestCylinderAABB(const Cylinder& cyl, const AABB& aabb) -> CollisionResult`: Wykrywa czy punkt bryly dotyka kwadratu w 2D osiach X i Y, z dodatkowym badaniem pokrycia wysokosci osi Z.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
`Client::World::Vector3` asercja mowi `static_assert(sizeof(Vector3) == 12)`:
`+0x00`: `float x`
`+0x04`: `float y`
`+0x08`: `float z`
Bez paddingu (alignment = 4). Swietny cel do mapowania pamieci z Rust FFI i pisania modow nawigacyjnych.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Powyzszy kod nie serializuje struktur pakietowych explicite. Jest jednak integralnym zapleczem pod logike wysylania `TPacketCGMove` i upewniania sie, ze synchronizacja sieciowa w oparciu o `TPacketGCMove` umieszcza gracza w odpowiednich bezkolizyjnych miejscach, chroniac przed zablokowaniem w budynku po stronie Game Servera.
- **Metody Pythona:** Nie ma tu wyeksponowanych funkcji C-API z modulu `PyMethodDef`. Dzialania takie jak zmiana biezacego wektora (poprzez klawiature w UI Python) w ostatecznym rezultacie tlumaczone sa w obiekcie klienta i rzutowane na kalkulacje w `ResolveMovement`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Wszystkie obliczenia kolizji musza byc przeprowadzone w watku, z ktorego pochodza struktury przestrzenne AABB. Aktualizowanie (modyfikowanie) elementow siatki (`CollisionGrid::SetObstacle`) asynchronicznie od jej odpytywania wywola wyscig, wiec modyfikacje robimy w watku wczytywania, lub zabezpieczamy muteksem jesli robione asynchronicznie (tutaj klasa ich nie posiada!).
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Uzycie poza bounadami na `SetObstacle` generuje panike wyjatku `std::out_of_range`. 
  - Bezpieczenstwo wewnetrzne `Vector3::Normalized`: Zabezpieczono przed dzieleniem przez zero; wektory o zerowej badz bliskiej zeru dlugosci (`LengthSq`) wroca jako obiekt inicjalny `{0.0f, 0.0f, 0.0f}` bez wystepowania bledu NaN.
  - Wyjscia kordow w `IsObstacle` naturalnie zwracaja `true` uzywajac granicy siatki jako ogromnej sciany i uniemozliwiajac wypadniecie aktora ze swiata.
- **Zarzadzanie zasobami (RAII):** Kod bardzo poprawnie uzywa stdlib. Uzytkowanie spanow (`std::span`) zamiast wskaznikow w `LoadFromBuffer` usuwa mozliwosc obsluzenia zerowego pointera do odczytu atrybutow.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** W razie rozbudowy przestrzeni logicznej (np. Raycasting dla kamer), nalezy:
  1. Zdefiniowac promien np. `struct Ray { Vector3 origin, direction; }` w pliku naglowkowym.
  2. Dodac funkcje API `CollisionResult TestRayAABB(const Ray&, const AABB&)` i umiescic ja jako nowa deklaracje statyczna w `CollisionDetector`.
  3. Wywolac powyzsza z petli klienta w poszukiwaniu Line-Of-Sight np. dla potworow.
- **Jak debugowac i logowac:** Wyniki poszczegolnych kolizji z wall slidingu w `ResolveMovement` najlepiej podgladac przez prosty dump loger: logowanie po kazdej iteracji z wypisaniem dlugosci wektora po `CalculateSlideVector`, jesli dochodzi do zakleszczenia postaci.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** W 100% umozliwiono tworzenie testow zewnetrznych. Podepnij te 2 naglowki do biblioteki `doctest`. Zaalokuj grid: `CollisionGrid mapGrid(10, 10); mapGrid.SetObstacle(5, 5, true);` po czym upewnij sie skladnia C++23 ze zadzialalo: `CHECK(mapGrid[5, 5] == true);`. Dzieki braku DirectX i Pythona odpada koniecznosc mockowania srodowiska.
