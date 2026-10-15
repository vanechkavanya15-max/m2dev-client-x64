---
task_id: "atlas_c05_05_quadtree_culling"
cluster: "WLD"
module_name: "LinearSectorQuadtree - Drzewo Czworkowe i Culling Obiektow"
target_files:
- src/GameLib/Terrain/LinearSectorQuadtree.cpp
- src/GameLib/QuadTreeSpatialPartition.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c05_05_quadtree_culling.md"
architecture_layer: "Swiat Gry, Teren, Kolizje i Nawigacja"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Jaka jest dokladna funkcja tego modulu w architekturze klienta:**
  Ten modul odpowiada za optymalizacje renderowania swiata (culling) oraz szybkie zapytania przestrzenne (spatial queries). Sklada sie z dwoch glownych komponentow:
  1) `LinearSectorQuadtree`: Sluzy do statycznego odrzucania (cullingu) niewidocznych fragmentow (patchy) terenu dla danego sektora mapy wzgledem frustum kamery (view frustum). Uzywa plaskiej, bezalokacyjnej struktury o stalym rozmiarze (85 wezlow, 64 liscie), aby zmaksymalizowac wydajnosc w watku renderujacym.
  2) `QuadTreeSpatialPartition`: Dynamiczne drzewo czworkowe (quadtree) uzywane do zarzadzania jednostkami (`QuadTreeEntity`) i szybkiego wyszukiwania obiektow w zadanym promieniu (`SearchInRadius`).
- **W jakim momencie petli gry (OnUpdate / OnRender / Network Tick) ten kod jest wywolywany:**
  `LinearSectorQuadtree` (`CullTerrainPatches`) jest wywolywany w fazie OnRender (przed wyslaniem draw calli do D3D9) celem przefiltrowania widocznych patchow terenu.
  `QuadTreeSpatialPartition` moze byc wywolywane w OnUpdate (np. przy poruszaniu sie obiektow wywolujac `Update()`) oraz w trakcie przetwarzania logiki kolizji lub interakcji (`SearchInRadius`).
- **Pelny opis przeplywu danych (Control Flow & Data Flow) krok po kroku:**
  Dla `LinearSectorQuadtree`: Inicjalizacja odbywa sie przez `BuildQuadtree` lub `BuildWithHeights` (przy ladowaniu sektora), gdzie obliczane sa pudla otaczajace (AABB) dla kazdego z 85 wezlow ze znajomoscia siatki 131x131 wierzcholkow wysokosci. W trakcie renderowania podawane jest frustum kamery do `CullTerrainPatches`. Algorytm przechodzi wezly za pomoca iteracyjnego, wewnetrznego stosu o stalym rozmiarze (brak rekurencji), wykonujac testy przeciec AABB ze scianami frustum. Zwraca tablice widocznych patchId i emituje `TerrainCullCompletedEvent`.
  Dla `QuadTreeSpatialPartition`: Przy inicjalizacji podawany jest glowny `QuadTreeBoundingBox`. Obiekty moga byc wstawiane przez `Insert()` (jesli wezel przekroczy `NODE_CAPACITY`, nastapi `Subdivide()`). Podczas zmiany pozycji wysylany jest `Update()`, co powoduje usuniecie ze starego miejsca, dodanie do nowego i emisje `EntityMovedEvent` przez `EventBus`.
- **Cykl zycia obiektow (Lifecycle: alokacja, inicjalizacja, reset, dealokacja):**
  `LinearSectorQuadtree`: Cale drzewo alokowane z gory w `std::array`. Inicjalizacja zachodzi przy `BuildWithHeights`. Posiada mozliwosc resetowania przez `Clear()` do stanu z zerami.
  `QuadTreeSpatialPartition`: Dynamiczna struktura. Liscie i dzieci sa alokowane leniwie przez `std::make_unique` przy potrzebie podzialu (`Subdivide`). Wektory podlegaja realokacjom (`m_entities`).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** 
  `LinearSectorQuadtree` wolywany jest przez glownego zarzadce terenu (np. TerrainManager) do culling'u podczas renderingu w DirectX.
  `QuadTreeSpatialPartition` wolany przez menedzerow encji lub system kolizji podczas aktualizacji pozycji obiektow i wyszukiwania celow/npc w obszarze. `EventBus` powiadamia inne sub-systemy.
- **Zaleznosci wyjsciowe (Outbound):** 
  Korzysta z EventBus (`UserInterface::Core::EventBus`) do powiadamiania o zdarzeniach (`QuadtreeBuiltEvent`, `TerrainCullCompletedEvent`, `EntityMovedEvent`). Korzysta z `EterBase::LogModern` do logowania oraz `EterBase::Result` dla kontroli bledow domeny.
- **Drzewo dyrektyw `#include`:** 
  - `LinearSectorQuadtree.cpp`: `../StdAfx.h`, `LinearSectorQuadtree.h`, `TerrainEvents.h`, `../../EterBase/LogModern.h`, `<algorithm>`, `<cmath>`.
  - `QuadTreeSpatialPartition.h`: `<expected>`, `<vector>`, `<memory>`, `<optional>`, `<format>`, `<cmath>`, `<algorithm>`, `EterBase/StrongTypes.h`, `EterBase/Result.h`, `UserInterface/Core/EventBus.h`.
- **Model pamieciowy:** 
  `LinearSectorQuadtree`: Pamiec liniowa, obiekty leza blisko siebie (cache-friendly). Zadne dynamiczne alokacje nie sa wykonywane w runtime na render loop. Brak obslugi na stercie w petli hot-path.
  `QuadTreeSpatialPartition`: Uzywa inteligentnych wskaznikow `std::unique_ptr` dla zagniezdzonych galezi, co zmusza do podazania po stercie. Tablice jednostek przechowuja dane przez wartsoc (`std::vector<QuadTreeEntity>`). Wymaga operacji `std::move` przy realokacji.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur:**
  - `GameLib::QuadTreeBoundingBox`: Reprezentuje 2D AABB dla partycjonowania przestrzennego. 16 bajtow (4 floaty).
  - `GameLib::QuadTreeEntity`: Encja przechowywana w drzewie (`EterBase::EntityId id`, `float x`, `float y`).
  - `GameLib::EntityMovedEvent`: Event wysylany przez EventBus podczas przemieszczenia obiektu.
  - `GameLib::QuadTreeSpatialPartition`: Drzewo czworkowe do partycjonowania przestrzeni 2D dla dynamicznych zapytan o promieniu. Maksymalna glebokosc: 8, maksymalna ilosc elementow w wezle: 4.
  - `GameLib::Terrain::LinearQuadNode`: Wymuszone alignas(32) dla lepszego dopasowania do linii pamieci podrecznej. AABB z min/max dla XYZ. (32 bajty).
  - `GameLib::Terrain::LinearSectorQuadtree`: Klasa implementujaca interfejs `ITerrainQuadtreeCuller`. Bez-alokacyjne drzewo przechowujace dokladnie 85 wezlow.

- **Tabela Metod Publicznych:**
  Dla `QuadTreeSpatialPartition`:
  - `EterBase::Result<void, EterBase::EntityError> Insert(QuadTreeEntity entity)`: Wstawia element. W razie zapelnienia wezla wola prywatna funkcje `Subdivide()`.
  - `EterBase::Result<void, EterBase::EntityError> Remove(EterBase::EntityId id)`: Usuwa encje rekursywnie przy uzyciu ID z kontenera wektora iterujac algorytmem `std::remove_if`.
  - `EterBase::Result<void, EterBase::EntityError> Update(EterBase::EntityId id, float newX, float newY)`: Atomowo aktualizuje koordynaty - symuluje Remove i nastepnie Insert, w przypadku awarii przy Insert zapewnia rollback stanu.
  - `std::vector<EterBase::EntityId> SearchInRadius(float x, float y, float radius) const`: Zwraca identyfikatory obiektow uzywajac wczesnego cullingu wezlow bazujacego na prostokacie poszukiwan.
  
  Dla `LinearSectorQuadtree`:
  - `void BuildQuadtree(int32_t sectorX, int32_t sectorY)`: Inicjalizuje puste drzewo bez informacji o wysokosciach mapy.
  - `void BuildWithHeights(SectorCoord coord, std::span<const uint16_t> heights, float heightScale, float baseHeight) noexcept`: Oblicza minimalne i maksymalne wartosci minZ/maxZ wprost z plikow terenu i propaguje te dane w gore drzewa.
  - `size_t CullTerrainPatches(const float viewFrustum[6][4], uint32_t* outVisiblePatchIds)`: Iteracyjnie, wydajnie testuje P-vertex wezlow z wewnetrznym stosem na 64 wezly.
  - `uint8_t SelectLOD(float distance) const`: Zwraca LOD (0, 1 lub 2).

- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  Struktura `LinearQuadNode` z wymuszonym `alignas(32)`:
  - `Offset 0x00`: `float minX`
  - `Offset 0x04`: `float minY`
  - `Offset 0x08`: `float minZ`
  - `Offset 0x0C`: `float maxX`
  - `Offset 0x10`: `float maxY`
  - `Offset 0x14`: `float maxZ`
  - `Offset 0x18`: `uint16_t childBaseIndex`
  - `Offset 0x1A`: `uint8_t lodLevel`
  - `Offset 0x1B`: `uint8_t isLeaf`
  - `Offset 0x1C`: `uint32_t patchId`
  Rozmiar calkowity: Dokladnie 32 bajty. Brak niepotrzebnego wypelnienia paddingiem na koncu poza wyrownaniem z wlasciwosci `alignas`. Zoptymalizowane dla CPU cache-lines.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** 
  Modul jest w calosci oddzielony od warstwy sieciowej (Network Layer). Obsluga ruchu, np. pakietow serwerowych oznaczonych opcodem poruszania sie obiektow (`TPacketGCMove`), ostatecznie przekazuje wywolania do warstwy lokalnego silnika encji, zasilajac pozniej `QuadTreeSpatialPartition::Update()`.
- **Metody Pythona (`PyMethodDef`):** 
  Nie zidentyfikowano mostkow miedzy tym kodem, a systemem C-API Pythona. Modul culling'u jest odizolowany we wnetrzu wylacznie logiki natywnej C++.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** 
  `LinearSectorQuadtree` uruchamiany jest tylko w watku renderujacym, co czyni go bezpiecznym. Przebudowa za pomoca `BuildWithHeights` nie posiada wbudowanych lockow i jednoczesne cull-owanie stworzyloby data race, wiec synchronizacja lub przelaczenie stronki (double buffering) zalezy od wlasciciela (Terrain Managera).
  `QuadTreeSpatialPartition` korzysta ze struktur modyfikowalnych wektorowych bez mechanizmow `std::mutex`. Dlatego metody modyfikujace (Update, Insert, Remove) i odpytujace (SearchInRadius) musza byc wolane na tym samym watku logiki lub chronic dane we wlasnym zakresie.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** 
  W funkcji `CullTerrainPatches` uzyto stalej pre-alokowanej tablicy stosu wezlow `std::array<uint16_t, 64> nodeStack`. Przekroczenie limitu wielkosci mogloby grozic przepelnieniem stosu, dlatego zaimplementowano twardy test wielkosci (`if (stackTop + 1 < size)`), ktory w przypadku limitu ignoruje pozostale dzieci - to bezpieczne.
  Uzycie Null Pointer w parametrze funkcji cull zostalo zapobiezone checkiem w pierwszej linijce.
- **Zarzadzanie zasobami (RAII):** 
  `QuadTreeSpatialPartition` wykorzystuje unikalne wskazniki do dzieci, ktorymi automatycznie steruje cykl zycia przy niszczeniu menedzera. Liscie i pola `LinearSectorQuadtree` to bloki w pamieci liniowej, co jest z gruntu w pelni zwolnione od potencjalnych memory leakow.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** 
  Aby zmienic zasady cullingu lub uwzglednic nowe maski okluzji dla systemu Metin2:
  1. Otworz `src/GameLib/Terrain/LinearSectorQuadtree.h` i rozwaz dodanie dodatkowych zmiennych wezlowych w strukturze `LinearQuadNode`. PAMIETAJ o limitach 32 bajtow `alignas` aby nie popsuc wydajnosci na procesorze. 
  2. Implementuj nowy parametr logiczny we wnetrzu funkcji budujacej (`BuildWithHeights`).
  3. Zaimplementuj rozszerzenie bezposrednio w bez-rekursywnej petli `CullTerrainPatches`. Surowo zakazane jest modyfikowanie pamieci dynamicznej (`new`, `std::vector`) pod czas wyliczania render cull.
- **Jak debugowac i logowac:** 
  Zawsze wprowadzaj komunikaty przy wykorzystaniu mechanizmu `EterBase::ModernLogger::Info`. Z uwagi na wydajnosc w fazie renderingu logowanie klatek jest nieakceptowalne, nalezy to robic w metodach budowania takich jak `BuildWithHeights`. Wyniki mozna monitorowac za pomoca subskrypcji do podsystemu nasluchujacego powiadomien (`TerrainCullCompletedEvent`).
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** 
  Skonstruuj maly test na silniku Doctest ladujac mock-owana macierz z rzutem i frustum (`float viewFrustum[6][4]`). Przeslij puste tablice z moca dla wysokosci (`std::span<const uint16_t> heights`) oraz weryfikuj ile zwrotow widocznych id ladowanych pod `outVisiblePatchIds` generuje narzedzie. Podepnij srodowisko pod `UserInterface::Core::EventBus` by symulowac odbieranie danych o przebudowie czy zakonczeniu wyszukiwania, symulujac istnienie silnika D3D9.
