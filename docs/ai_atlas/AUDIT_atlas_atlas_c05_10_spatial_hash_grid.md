---
task_id: "atlas_c05_10_spatial_hash_grid"
cluster: "WLD"
module_name: "Siatka Przestrzenna (Spatial Hash Grid) Bytow Dynamicznych"
target_files:
- src/Client/World/SpatialHashGrid.h
- src/GameLib/SpatialEntityGrid.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c05_10_spatial_hash_grid.md"
architecture_layer: "Swiat Gry, Teren, Kolizje i Nawigacja"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

**Funkcja modulu:**
Modul implementuje struktury danych (siatki przestrzenne oparte na haszowaniu) przeznaczone do bardzo szybkiego (O(1)) lokalizowania bytow (postaci, potworow, przedmiotow, obiektow) w danym promieniu na mapie 2D. 
Dzieki podzialowi przestrzeni na rownolegle komorki, unika sie kosztownego sprawdzania kazdego bytu w swiecie (zlozonosc O(N)). 

**Zastosowanie w petli gry:**
Struktury te sa aktualizowane najczesciej w fazie `OnUpdate` (kiedy byty zmieniaja swoje koordynaty) oraz wykorzystywane zarowno w warstwie logiki gry, jak i renderingu (np. do odrzucania obiektow poza zasiegiem widzenia kamery - culling - lub wyszukiwania wrogow w zasiegu skilla/ataku). Moga byc rowniez aktualizowane podczas synchronizacji z serwerem (Network Tick).

**Przeplyw danych i cykl zycia:**
1. **Inicjalizacja (Alokacja):** Obiekt siatki tworzony jest z podanym rozmiarem komorki (`cellSize`), np. 1024.0f lub 100.0f.
2. **Dodawanie (Insert):** Nowo utworzony byt zostaje zgloszony do siatki. Jego pozycja (x, y) sluzy do wyliczenia unikalnego identyfikatora komorki (CellCoords / uint64_t hash). Identyfikator bytu laduje w kontenerze odpowiadajacym tej komorce.
3. **Aktualizacja (Update):** Jesli byt sie poruszy (zmiana starej pozycji na nowa), przeliczane sa koordynaty komorki. Jesli byt przekroczyl granice komorek, jest usuwany ze starej (`Remove`) i dodawany do nowej (`Insert`).
4. **Wyszukiwanie (QueryRadius / FindNearby):** Przekazujac centrum zapytania oraz promien, algorytm okresla min/max koordynaty siatki w obszarze zapytania i odpytuje tylko te komorki, drastycznie ograniczajac pule sprawdzanych bytow.
5. **Usuwanie i Zwalnianie (Remove / Clear):** Przy smierci bytu lub wyjsciu z zasiegu - byt jest usuwany. Puste komorki moga byc dealokowane z mapy w celu zaoszczedzenia pamieci. W momencie usuniecia siatki (zniszczenia instancji), cala struktura jest zwalniana.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
Siatki przestrzenne wywolywane sa na ogol przez glowne menedzery bytow, takie jak `CInstanceBase`, `CPythonCharacterManager` lub system dropu na ziemi (np. `GroundItemModel`). Zaleznie od implementacji, moga byc integrowane w celu przekazywania bytow do renderingu lub decydowania o interakcjach z innymi postaciami/przedmiotami.

**Zaleznosci wyjsciowe (Outbound):**
Klasy sa w duzym stopniu samodzielne i operuja wylacznie na wbudowanych strukturach STL oraz identyfikatorach.
`Client::World::SpatialHashGrid` wykorzystuje `EterBase::EntityId` z `../../EterBase/StrongTypes.h`.

**Drzewo dyrektyw `#include`:**
- `src/Client/World/SpatialHashGrid.h`:
  - `<vector>`, `<unordered_map>`, `<unordered_set>`, `<cstdint>`, `<cmath>`, `<shared_mutex>`, `<optional>`
  - `"../../EterBase/StrongTypes.h"`
- `src/GameLib/SpatialEntityGrid.h`:
  - `<cstdint>`, `<unordered_map>`, `<unordered_set>`, `<vector>`, `<cmath>`

**Ryzyka zaleznosci cyklicznych:**
Brak. Pliki zawieraja czysta implementacje struktur danych i unikaja wlaczania zewnetrznych elementow klienta, polegajac wylacznie na silnym typowaniu poprzez ID (szablon `Entity` lub `EterBase::EntityId`).

**Model pamieciowy:**
Zastosowano nowoczesne, bezpieczne podejscie. Dane identyfikatorow przetrzymywane sa w standardowych kontenerach (w pamieci na stercie poprzez wewnetrzne alokacje np. `std::vector` i `std::unordered_map`). Klasa `SpatialHashGrid` jest bezpieczna w uzyciu wielowatkowym za sprawa muteksu (`std::shared_mutex`). Obiekty identyfikujace to identyfikatory numeryczne (brak jawnych i surowych wskaznikow co zwieksza bezpieczenstwo).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur:**

1. **`Client::World::SpatialHashGrid`**
   - **Rola:** Przechowuje identyfikatory obiektow umozliwiajac bezpieczny watkowo dostep do odpytywania srodowiska o bliskie obiekty.
   - **Wlasciciel:** Prawdopodobnie menedzer swiata (glowny watek logiki gry, lecz dostepny wielowatkowo).

   **Metody Publiczne:**
   - `explicit SpatialHashGrid(float cellSize = 1024.0f)`: Konstruktor z domyslnym rozmiarem siatki.
   - `void Insert(EterBase::EntityId id, float x, float y)`: Dodaje nowy byt, bezpieczne watkowo.
   - `void Update(EterBase::EntityId id, float x, float y)`: Aktualizuje pozycje, zmienia komorke jesli trzeba.
   - `void Remove(EterBase::EntityId id)`: Usuwa byt z siatki i buforu pozycji.
   - `void Clear()`: Resetuje cale dane instancji.
   - `[[nodiscard]] size_t Count() const`: Zwraca liczbe przechowywanych bytow.
   - `[[nodiscard]] std::vector<EterBase::EntityId> QueryRadius(float center_x, float center_y, float radius) const`: Wyszukuje pozycje zgrubnie promieniem (Bounding Box radius).
   - `[[nodiscard]] std::optional<EterBase::EntityId> QueryNearest(float center_x, float center_y, float maxRadius, std::optional<EterBase::EntityId> ignoreId) const`: Szuka najblizszego bytu w zadanym promieniu z opcjonalnym ignorowaniem wskazanego ID.

2. **`SpatialEntityGrid<Entity>` (Szablon)**
   - **Rola:** Otwarta na szablon siatka do wyodrebniania bytow znajdujacych sie na zadanej odleglosci (szybki bounding-box), najczesciej logiki postaci.
   - **Wlasciciel:** Watek logiki. Z braku wlasnego `mutexu` operacje nie sa natywnie bezpieczne wielowatkowo (wymaga zewnetrznej synchronizacji).

   **Metody Publiczne:**
   - `explicit SpatialEntityGrid(float cellSize)`: Inicjalizuje z zadanym rozmiarem siatki, z zabezpieczeniem domyslnym > 0.
   - `void Insert(const Entity& entity, float x, float y)`: Oblicza Hash 64-bit komorki i umieszcza w niej byt.
   - `void Remove(const Entity& entity, float x, float y)`: Usuwa byt i kasuje cala komorke jesli jest po tym pusta (oszczednosc pamieci).
   - `void Update(const Entity& entity, float oldX, float oldY, float newX, float newY)`: Optymalizacja przepiecia pomiedzy cellami w przypadku przejscia granicy miedzy komorkami.
   - `void Clear()`: Czysci podlegle byty i komorki.
   - `[[nodiscard]] std::vector<Entity> FindNearby(float x, float y, float radius) const`: Wyszukuje wg obwiedni bounding box bazujacej na grid coord/min/max.
   - `template <typename Predicate> [[nodiscard]] std::vector<Entity> FindNearbyExact(float x, float y, float radius, Predicate filter) const`: Zwraca byty odfiltrowujac po wlasnej domenie, np. uzywane do dystansu euklidesowego.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
Brak offsetow do wpiecia bezposredniego (FFI) przez C-Type na zewnatrz ze wzgledu na uzycie kontenerow `std::unordered_map` oraz `std::shared_mutex`, ktore wplywaja na rozmiar i dynamiczne adresowanie pamieci zaleznie od wersji stl i platformy ABI.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

**Pakiety Sieciowe:**
Same pliki narzedziowe (SpatialHashGrid) nie mapuja z opcodami ani danymi GC/CG bezposrednio. Reaguja jednie wtorznie z sieci podczas synchronizacji (np. z pakietow TCP takich jak `GC::CHARACTER_UPDATE` informujacy o przeniesieniu na (X,Y) i zglaszajacy `Update` dla identyfikatora instancji).

**Metody Pythona (`PyMethodDef`):**
Moduly nie eksponuja wprost API do srodowiska Pythona. Dzialaja w warstwie C++ pod spodem globalnego zarzadzania instancjami/swiatem. Python moze byc interfejsem posrednio uzywajac interfejsu (na przyklad API do zapytania `chrmgr.GetVIDByPosition`).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

**Zasady wielowatkowosci:**
1. `Client::World::SpatialHashGrid` wykorzystuje muteks modyfikatora `std::shared_mutex` i obsluguje rownolegla prace (zapytania "Odczyt - Reader lock" a edycje przez `Insert`/`Update` jako "Zapis - Writer lock"). Nalezy ostroznie uzywac metod, by uniknac deadlockow (jesli by jakas petla zapetlana chciala jednoczesnie odczytywac grid w watku iteracji oraz z wnetrza odczytu mutowac `SpatialHashGrid`).
2. `SpatialEntityGrid` (szablon) z kolei NIE Posiada muteksa ochronnego. Uzycie go musi zostac uprzednio zsynchronizowane lockiem, jesli instancja modulu jest uzywana z kilku watkow (np. watek renderingu (DirectX) proboje odczytac pozycje bytow dla shadow prepass, podczas gdy watek gry wykonuje logike na serwerowych `TPacketGCCharacterUpdate`). 

**Potencjalne punkty awarii (Crash Points & Edge Cases):**
1. **Ostrzezenie Floating-Point:** Przy wyliczaniu `GetGridCoord(float coord)` z uzyciem zmiennoprzecinkowych z `std::floor`. Bardzo dalekie (kosmiczne) wartosci koordynatow na mapie, uzywajace precyzji rzedu ogromnego (np. float > 16mln), moga miec problemy z plynna krotkosc i dokladnoscia - obnizajac jakosc spatial hasha, albo powodowac bledy przy zrzutowaniu powrotnym na `int32_t`.
2. **Gubienie referencji (Desynchronizacja):** Wywolanie metody `Update()` na bycie w strukturze przestrzennej powinno sie zdarzac zawsze z dokladnymi poprzednimi (oldX, oldY), jezeli dostarczy sie zly poprzedni offset (dla SpatialEntityGrid), struktura moze byc zakorodowana "duchem" nieusunietego na starym miejscu w mapie bytu! Nowy `SpatialHashGrid` zalatwia to pamietajac stare w `m_entityPositions`.
3. **Brak plynnej uszczelki (Memory Leak na grid mapie):** W `SpatialEntityGrid::Remove` zaimplementowano sprawdzanie `if (it->second.empty()) cells.erase(it)`. Odzysk jest zapewniony na pustej komorce.

**Zarzadzanie zasobami (RAII):**
Obie klasy obsluguja pamiec dynamiczna poprzez zarzadzanie natywne w kontenerach stl `std::unordered_map` - po likwidacji niszczy same siebie automatycznie, nie ma koniecznosci uzywania delete.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
Chcac zaimplementowac na przyklad funkcje znalezienia promienia obcinajacego przestrzen wzdluz danego okregu kierunkowego (tzw. "Cone Query" lub "FOV Query" na potrzeby zasiegu widzenia kamery dla culling'u/stozkowego ataku potwora):
1. Dopisz metode publiczna `FindNearbyCone` w `SpatialHashGrid.h`.
2. Okresl najpierw zgrubnie `Bounding Box` promieniem uzywajac petli po komorkach minX/minY i uzyskujac liste wstepnych obiektow, podobnie jak `QueryRadius`.
3. Odfiltruj uzyskane z obwiedni obiekty weryfikujac punkt w przestrzeni przeciwko obliczeniom trygonometrycznym lub wbudowanym Dot Product'em uzywajac wektorow 3D w `Vector3` by stwierdzic czy obiekty te zawieraja sie w zadanym zakresie stozka/katow.

**Jak debugowac i logowac:**
Z uwagi, ze to wydajne narzedzie nie wolno wpisywac do srodka petli wyszukiwania rzedu wielkosci N operacji loggera (`EterBase::ModernLogger`). Zamiast tego zrob unit test, by zaobserwowac co daje `FindNearby` na sztucznym gridzie 2D. W produkcyjnej grze lepiej debugowac liczac zewnetrznie wartosc `Count()` albo w punkcie wejscia w `Insert()`. 

**Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
Stworz wewnatrz glownego pliku z frameworku `doctest` nowy obszar testowy w `tests/test_main.cpp` obejmujacy wstepne alokacje np:
```cpp
#include "doctest/doctest.h"
#include "../src/GameLib/SpatialEntityGrid.h"

TEST_CASE("SpatialEntityGrid Proximity") {
    SpatialEntityGrid<uint32_t> grid(100.0f);
    grid.Insert(1001, 150.0f, 150.0f);
    grid.Insert(1002, 210.0f, 210.0f);
    
    // Zapytanie radius w [150,150]
    auto result = grid.FindNearby(150.0f, 150.0f, 10.0f);
    CHECK(result.size() == 1);
    CHECK(result[0] == 1001);
}
```
Z uwagi na brak powiazan z renderem DX9, mozliwa jest bezpieczna manualna kompilacja poleceniem z linii terminala.
