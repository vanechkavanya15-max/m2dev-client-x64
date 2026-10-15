---
task_id: "atlas_c05_04_terrain_height_sampler"
cluster: "WLD"
module_name: "TerrainHeightStorage - Dwuliniowy Probnik Wysokosci Z Terenu"
target_files:
- src/GameLib/Terrain/TerrainHeightStorage.cpp
- src/Client/World/TerrainHeightSampler.h
- src/GameLib/BilinearHeightInterpolator.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c05_04_terrain_height_sampler.md"
architecture_layer: "Swiat Gry, Teren, Kolizje i Nawigacja"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport AI: TerrainHeightStorage - Dwuliniowy Probnik Wysokosci Z Terenu

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
- **Funkcja modulu:** Podsystem odpowiada za wyliczanie i pobieranie dokladnej wysokosci fizycznej (o Z) terenu dla dowolnych wspolrzednych mapy (X, Y). Zapobiega to efektowi wpadania modeli pod mape, umozliwia poprawne odwzorowanie polozenia obiektow na nachylonym zboczu oraz sprawdza granice wody i kata nachylenia scian (kolizje poruszania).
- **Punkt wywolania (Game Loop):** Probnik wysokosci jest uzywany w fazie `OnUpdate` przez system fizyki i silnik ruchu gracza w celu wyznaczenia wlasciwej pozycji osi Z dla danego kafelka. Jest tez stosowany na potrzeby renderowania w `OnRender` przy obliczaniu cieniowania powierzchni, macierzy transformacji wektorow normalnych swiatla slonecznego na zboczach i pozycji efektow czasteczkowych uderzajacych o podloze.
- **Przeplyw danych (Data & Control Flow):** 
  1. Pamiec terenu jest wczytywana (poprzez fasade terenu) jako bloki mapy (chunks) do obiektu `TerrainHeightStorage`.
  2. Modul przyjmuje pozycje swiata i zamienia ja na koordynaty wewnatrzsektorowe.
  3. Klasy interpolacyjne i probkujace takie jak `BilinearHeightInterpolator` lub wewnetrzne mechanizmy dla `TerrainHeightStorage` lokalizuja dane wysokosci poszczegolnych wierzcholkow w pamieci podrecznej (czesto za pomoca bezpiecznych obiektow Span "zero-copy").
  4. Nastapi przeliczenie wysokosci lokalnej w trojkacie kafelka (komorka dzielona jest wg przekatnej w celu replikacji zachowania rzutowania Direct3D - `SampleHeightTriangular`).
  5. Wynik trafia do glownej fizyki lub modulu rzutowania wizualnego.
- **Cykl zycia:** Blok pamieci globalnej na teren inicjowany jest we wzorcu singleton-like/factory via `CreateTerrainHeightCache()`. Zwracany jest `std::unique_ptr`. Modul centralny `TerrainHeightStorage` zarzadza blokami `ChunkHeightData` w postaci instancji wylapywanych pod `std::shared_ptr`. Instancje poszczegolnych narzedzi pomniejszych np. interpolatora mozna tworzyc prosto na stosie z uzyciem istniejacego widoku pamieci `std::span`. Dealokacja w `Clear()`, ktora emituje `TerrainHeightCacheClearedEvent`.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound):**
  - Wywolywane przez watek logiki swiata, pozycjonowanie wierzchowcow i postaci, kalkulacje nawigacji A*, system odrzutu w walce oraz efekty wizualne (np. cienie na podlozu). Modul TerrainSubsystemFacade jako nadrzedny arbiter.
- **Zaleznosci wyjsciowe (Outbound):**
  - `<shared_mutex>` i wewnetrzna blokada modulu terenu do wielowatkowego odczytu i zapisu siatek punktow terenu.
  - EventBus do sygnalizowania wyczyszczenia cache'a terenu: `UserInterface::Core::EventBus::GetInstance()`.
  - Przestrzenie `std::expected` (oraz zdefiniowany error z `EterBase::NavigationError`) dla obslugi wyjatkow z bloku mapy. Logowanie z `EterBase::ModernLogger`.
- **Drzewo dyrektyw `#include`:**
  - `TerrainHeightStorage.cpp`: `../StdAfx.h`, `TerrainHeightStorage.h`, `TerrainEvents.h`, `../../EterBase/LogModern.h`, `<algorithm>`, `<cmath>`.
  - `TerrainHeightSampler.h`: `<cstdint>`, `<vector>`, `<expected>`.
  - `BilinearHeightInterpolator.h`: `<cstdint>`, `<span>`, `<expected>`, `<cmath>`, `"../EterBase/Result.h"`.
- **Model pamieciowy:** Wpelni inteligentny i nowoczesny sposob z zarzadzaniem czasem zycia po stronie wlasciciela. `TerrainHeightStorage` trzyma `std::unordered_map` korzystajacy ze zliczennych wskaznikow do powstalych blokow: `std::shared_ptr<ChunkHeightData>`. Probkowniki i interpolatory bezposrednio nie przejmuja rzadow uzywajac szybkich kopii albo widokow pamieci (`std::span` i stale referencje z API zero-copy).

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabela Klas i Struktur
- `GameLib::Terrain::ChunkHeightData`: Trzyma w sobie plaski wektor bufora kafelkow 131x131 w postaci 16-bitowych wskaznikow terenu (heights) oraz wektor wody. Wyciaga surowe Z dla wspolrzednych po uwzglednieniu skali i bazy wysokosci. Posiada kluczowa wewnetrzna metode `SampleHeightTriangular`. Watkowo obslugiwane przez locki globalnego storage'u.
- `GameLib::Terrain::TerrainHeightStorage`: Implementacja interfejsu `ITerrainHeightCache`. Cache terenu bedacy agregatem chunkow posiadajacy wielowatkowa bezpieczna implementacje dla dostepu do nich po ich lokalizacjach sektorowych (koordynatach SectorCoord). Posiada bufor `shared_mutex`.
- `GameLib::BilinearHeightInterpolator`: Czysto analityczny element mapujacy fizyczna dwuwymiarowa wielkosc komorki mapy ze siatka terenu i wysokoscia Z w sposob biliniowy. Interpolacja bazuje wylacznie na CPU. Oparty wylacznie na obslugiwanym `std::span` i parametrze 1D bufora.
- `Client::World::TerrainHeightSampler`: Odizolowany z logiki klas probnik wysokosci swiata umozliwiajacy rowniez uzyskanie wektora normalnego 3D (do pozycjonowania ujemnego nachylenia modeli i efektow czastek). Posiada `std::vector<float>` grida. Wlasciciel w swoim wlasnym zdefiniowanym procesie w zaleznosci od watku probkujacego.
- `Client::World::Vector3`: Klasyczny POD 12 bajtow (3 * 4 b float). Reprezentuje punkt Z, X i Y.
- Enum `Client::World::TerrainError`: OutOfBounds, InvalidGridSize, DataNotLoaded do zwracania w std::expected.

### Tabela Metod Publicznych
**TerrainHeightStorage:**
- `void InsertChunk(SectorCoord coord, ChunkHeightData data)`: Unikalna blokada watku (Write) alokujaca wezel nowymi danymi chunku. 
- `void RemoveChunk(SectorCoord coord)`: Usuwa dane konkretnego sektora. Blokada Write.
- `bool HasChunk(SectorCoord coord) const noexcept`: Metoda read-only, sprawdza po wezle wspolrzednych. 
- `float SampleHeight(float x, float y) const`: Podstawowy mostek po punkt mapy zwracajacy ostateczna i zinterpolowana wysokosc na plaszczyznie swiata. Obslugiwany watkowo.
- `float SampleWaterHeight(float x, float y) const`: Wylicza wartosc probkujaca wysokosci wody w swiecie, jezeli dotyczy. 
- `bool IsWalkableSlope(float x, float y, float maxSlopeAngle) const`: Sprawdza stopien zbliza zawezajac go po gradiencie trojkata i zwraca boolean zgodny z maxSlopeAngle. 
- `void BatchSampleHeight(const float* x, const float* y, float* outZ, size_t count) const`: Tablicowy pobor wektora probkowan, cache'ujacy ostatni uzyty wezel, podnoszacy niesamowicie optymalizacje dla probkowania wielu pkt (chociazby armia mobow).
- `void Clear()`: Reset pelnej puli map.
- `std::optional<float> SampleHeightExact(WorldPosition pos) const noexcept`: Operacja rdzeniowa probkowania wysokosci z predykcja optional jezeli brak mapy.
- `std::optional<std::tuple<float, float, float>> CalculateNormal(WorldPosition pos) const noexcept`: Wylicza wektor normalny ze spadkow X oraz Y dla danego zinterpolowanego trojkata swiata.

**BilinearHeightInterpolator:**
- `std::expected<float, EterBase::NavigationError> GetHeight(float worldX, float worldY) const`: Oczekiwana wartosc bezblednego wyliczenia interpolacji.

**TerrainHeightSampler:**
- `std::expected<void, TerrainError> Initialize(...)`: Ladowanie wektorow 3D siatki probkowania.
- `std::expected<float, TerrainError> GetHeight(float x, float y) const`: Przelicza siatki grida ze sprawdzeniem granic zwracajac poprawne dane interpolacyjne ze standardem expected.
- `std::expected<Vector3, TerrainError> GetNormal(float x, float y) const`: Podobnie, bazujac na sasiedztwie narzuconym przez granice grida podaje trojwymiarowy element.

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
- `Client::World::Vector3`: x (0x0), y (0x4), z (0x8), size: 12 bytes. Zgodny layout pod pamiec bufora GPU / DirectX / FFI i hookowanie.
- `ChunkHeightData`: W srodku znajdziemy standardowy offsetowy wektor 16-bit oraz byte'ow. Potem dodane atrybuty floats heightScale (offset m.in. 0x30 w zaleznosci od rozmiaru naglowka vectora platformy) oraz baseHeight.
- `TerrainHeightSampler`: `m_width` (0x0), `m_height` (0x4), `m_gridHeights` vector std dla floats (0x8).
- `BilinearHeightInterpolator`: Czyste referencje span z klasycznym layoutem 0x0 dla struktury view spana. Mamy tez po 4-bajty `width_`, `height_`, i `cellSize_`.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Pakiety Sieciowe:** Modul izolowany od warstwy pakietowej. Pakiety i wlasciwe opcode'y sa zarzadzane pietro wyzej np w Map Subsystem podczas ladowania nowej strefy serwerowej i koordynacji chunkow. Dla pakietu serwerowego moze to byc np. opcode aktualizujacy zasob cache'owy (Phase Game GC->Client).
- **Metody Pythona (`PyMethodDef`):** Modul jest silnikiem backendowym dla np. `CPythonBackground` ktore ma podpiecia ze skryptami. Na tym etapie, te pliki narzedzi nie eksponuja bezposrednio interfejsu dla API np `app.GetTerrainHeight`. Zeby to wdrozyc potrzebne jest sprobowanie dodania dedykowanych metod np w `PythonBackgroundModule.cpp` bazujacych na `Facade`.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Obiekt `TerrainHeightStorage` uzywa modyfikatora mutexa jako read/write lock (`std::shared_mutex`). Pobrania wysokosci poszczegolnego chunka oraz iteracyjne probkowanie z zablokowaniem bufora operuja glownie pod blokada do czytania co optymalizuje skoki threadowe. Brak bezposredniego narzucenia pracy w zdefiniowanym jednym watku - jest systemem gotowym do zrownoleglenia, lecz calosciowy dostep musi zostac otoczony blokadami udostepnianymi publicznie poprzez zarys klasy.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** Brak poprawnego float (`std::isnan` badz `!pos.IsFinite()`) moglby zniszczyc interpolacje - modul odpowiednio wychwytuje anomalie przy kazdym zapytaniu zwracajac puste opcje / zera badz -10000.0f (wartosc magiczna ignorowana na kliencie poza obrysem modulu dla braku wody).
- **Zarzadzanie zasobami (RAII):** Kod na tym poziomie w pelni egzekwuje std::expected jako error handling wylapujac z powrotem pamiec bezpiecznie po usunieciu struktury ze stosu. Alokacje blokow oparte na `std::shared_ptr`. Interpolatory wykorzystujace `std::span` eliminuja duplikowanie pamieci z siatek map i pozwalaja na zero-copy mapowanie grida na wektory obliczen procesora.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zdefiniuj nowa metode w `ITerrainHeightCache` i zaimplementuj w `TerrainHeightStorage`.
  2. Zamknij swoja metode odczytu wewnatrz `std::shared_lock lock(m_mutex);`.
  3. Wywolaj swoja wewnetrzna logike i w miare potrzeby zastosuj np nowa tablice / funkcje trygonometryczne.
  4. Nie rzucaj C++ throw, zamiast tego zastosuj typ rezultatu zgodny z reszta (np. `std::optional`).
- **Jak debugowac i logowac:** Przechwytywanie bledow za pomoca `EterBase::ModernLogger::Error` do srodowiska plikowego lub okna debugowania, zrzucanie danych dot. SectorCoord w postaci parametrow pod `{}` std::format. Zastosuj breakpoint w `SampleHeightTriangular` dla wychwycenia niewlasciwie poskladanej sciezki miedzy wierzcholkami na granicy dwoch kafelkow w grze na wysokosci.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Modul powstal idealnie w symbiozie pod testowanie jednostkowe (odsuniecie graficznego polaczenia). Napisz plik mocka symulujacego standardowa 131-punktowa siatke dla `ChunkHeightData`, zbuduj plik docelowy testu korzystajacy z izolowanej klasy z uzyciem Doctest wylapujac czy wysokosci zgadzaja sie analitycznie z np uzyciem parametru expected `TerrainError`. Pamietaj aby przed podawaniem zaleznosci objac wszelkie dyrektywy wykluczeniem np. #ifndef TEST_MODE_DISABLE_STDAFX w celu pominiecia plikow uniemozliwiajacych kompilacje headless w symulacji Linuxowej.
