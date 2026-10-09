---
task_id: "atlas_c05_02_map_outdoor_streaming"
cluster: "WLD"
module_name: "CMapOutdoor - Asynchroniczny Silnik Sektorow Terenu"
target_files:
- src/GameLib/MapOutdoor.cpp
- src/GameLib/MapOutdoor.h
- src/GameLib/AreaLoaderThread.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c05_02_map_outdoor_streaming.md"
architecture_layer: "Swiat Gry, Teren, Kolizje i Nawigacja"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

**Rola modulu:**
Modul `CMapOutdoor` wraz z podsystemem `TEMP_CAreaLoaderThread` to fundament asynchronicznego silnika terenu (Outdoors) w kliencie gry Metin2. Odpowiada za plynne ladowanie, wyswietlanie i zarzadzanie kafelkami (sektorami) mapy o rozmiarach 3x3 w miare przemieszczania sie postaci gracza w otwartym swiecie. Technika ta (Terrain Streaming) zapobiega calkowitym przestojom (zamrozeniom ekranu) podczas ladowania nowych tekstur, kolizji, zrodel wody i obiektow zewnetrznych.

**Miejsce w petli gry (Execution Context):**
Mechanizm ladowania jest aktywowany okresowo w glownej petli logiki - podzas wykonywania metody `CMapOutdoor::Update()`. Ustalana jest nowa pozycja kamery/gracza (wspolrzedne x, y, z), a nastepnie wyliczana jest potrzeba zmiany kafelka na podstawie gridu `CTerrainImpl`.

**Przeplyw Danych i Cykl Zycia (Control Flow & Lifecycle):**
1. **Wykrycie przesuniecia:** Z kazda klatka metoda `Update()` przetwarza pozycje gracza na globalne indeksy siatki `sCoordX` i `sCoordY`. Jesli gracz przeszedl za granice `LOAD_SIZE_WIDTH`, system rozpoczyna ladowanie.
2. **Kolejkowanie zadan (Dispatch):** Klasa deleguje zadania typu background loading (Teren: height.raw, wodny water.wtr, splat mapy itd. oraz Obszary (Area): obiekty budynkow, roslinnosc) do watku pobocznego przez lambdy wrzucane do centralnej puli `CGameThreadPool`. Odbywa sie to przy pomocy klasy pomocniczej `TEMP_CAreaLoaderThread`.
3. **Przetwarzanie Tla (Background Parsing):** W watku roboczym pobierane i przetwarzane sa pliki (czesto przez PackManager/VFS) a dane surowe zrzucane do wejsciowych wlasciwosci terenu. Zapis z wykorzystaniem strumieni na dysk / z plikow VFS nie blokuje renderowania. 
4. **Zwieranie i synchronizacja (Fetch):** Watek glowny (okresowo lub wymuszonym oczekiwaniem) dokonuje inspekcji z wykorzystaniem `Fetch()` z obiektow deki `m_pTerrainCompleteDeque`, chronionych przez `std::mutex`.
5. **Garbage Collection (Dealokacja):** Przestarzale i opuszczone przez gracza kafelki 3x3 laduja w buforze `m_TerrainDeleteVector` oraz `m_AreaDeleteVector`. W celu zabezpieczenia FPS, ich usuwanie odbywa sie stopniowo przy uzyciu ograniczen czasowych i pollingu w metodzie `__UpdateGarvage()`.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
- **Metody wejsciowe z `CPythonBackground` (API aplikacji C++)**, przekazujace zdarzenia przesuniecia z modulu glownego i sieci. Pozycjonowanie odbywa sie ze statycznych systemow postaci lub z wolnej kamery.

**Zaleznosci wyjsciowe (Outbound):**
- **Wielowatkowosc:** `EterLib/GameThreadPool` (Pula watkow C++11 dla zadan w tle), `<mutex>`, `<deque>`.
- **System Plikow:** `PackLib/PackManager`, `EterLib/ResourceManager` (dostep do surowych binarow takich jak `.raw`, `.wtr`, `.atr`).
- **Renderowanie:** Modele drzew `CSpeedTreeForestDirectX`, Direct3D9 w kontekscie cieni, tekstur i siatki patchow (`MapOutdoorLoad.cpp`).
- **Zarzadzanie pamiecia:** Wewnetrzne struktury pool (np. `CDynamicPool<CMonsterAreaInfo>`, `CArea::ms_kPool`).

**Drzewo dyrektyw `#include` (Potencjalne ryzyka):**
- Glowny naglowek `MapOutdoor.h` implementuje wiele zewnetrznych bibliotek: tereny `CTerrain`, obszary `CArea`, kwadry `TerrainQuadtree.h`. Trzeba bacznie uwazac na cykliczne referencje ze struturami zwiastujacymi eventy - naglowki powinny korzystac glownie z deklaracji wprzod (forward declarations - `class CTerrain; class CArea;`).
- Dolaczenie `#include "AreaLoaderThread.h"` jest bezpieczne, implementacja wyodrebnila obiekty STL do pliku CPP z uzyciem wskaznikow na zewnatrz.

**Model pamieciowy:**
- Kod historycznie bazuje na klasycznych wskaznikach `*` przypisanych m.in. w kontenerach `std::vector<CTerrain*> m_TerrainVector`. Obiekty te posiadaja wlasne handlery referencji alokowane recznie przez `CTerrain::New()` i usuwane przez `CTerrain::Delete()`. Brakuje zarzadzania przez nowoczesne RAII (`std::unique_ptr`).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

#### Klasa: `CMapOutdoor`
- **Rola:** Glowny koordynator otwartego swiata terenu, renderowania cieni kafelkowych i wektorow obszarow (Teren, Atrybuty i Drzewa).
- **Rozmiar:** Bardzo masywny obiekt uzywajacy dziesiatek kontenerow STL i buforow graficznych.
- **Wlasciciel Watku:** Watek glowny klastra renderujacego (Main Render Thread). Wymaga uzywania bezposrednio w OnUpdate / OnRender.
- **Kluczowe metody publiczne:**
  - `bool Update(float fX, float fY, float fZ)`: Sprawdza zaleznosci koordynatowe 3x3 na podstawie kamery. Jesli prog zostanie przekroczony (przekroczenie obszaru kafelka) -> generuje wektory zadan `LoadTerrain`/`LoadArea`.
  - `bool Load(float x, float y, float z)`: Bootstrapping terenu, ustawienie pozycji zerowej, pobranie ustawien z `Setting.txt`. Zwraca boolean dla bezpieczenstwa.
  - `bool GetHeight(float x, float y)`: Zwraca z bufora przestrzennego wyliczona wysokosc mapy z-axis, bez zaglebiania sie w pule watkow (wartosc z cachu terenowego). Uzywane dla AI mobow.
  - `bool GetAttr(float fX, float fY, BYTE * pbyAttr)`: Sprawdzenie stanu komorki (woda, mur, pvp, bez-PK) mapowane na siatke atrybutow bezposrednio z `CTerrain`.

#### Klasa: `TEMP_CAreaLoaderThread`
- **Rola:** Zarzadca i proxy zadan pomiedzy glownym watkiem mapy, a pula watkow w tle. Odbiera zgloszenia i pobiera zasoby IO z plikow gry do bufora ramu (nie renderuje!).
- **Kluczowe Pola:** `std::deque<CTerrain*> m_pTerrainCompleteDeque`, `std::mutex m_TerrainCompleteMutex`.
- **Kluczowe Metody:**
  - `void Request(CTerrain* pTerrain)`: Rejestruje lambde wykonujaca proces ladowania IO. Funkcja dziala takze jako Fallback Fall-Trough synchronicznie na watku glownym, jezeli Pula ThreadPool nagle zniknie.
  - `bool Fetch(CTerrain** ppTerrian)`: Wyluskujaca lockowana kolejke na zakonczenie asynchronicznego joba (wywolywana wylacznie na MainThread).
  - `void ProcessTerrain(CTerrain * pTerrain)`: (Prywatna) - Czytanie wlasciwosci tekstowych i bezposrednio binarnego zrzutu `height.raw`, `water.wtr`, `attr.atr` do ramu. Wykonywana Wylacznie w Background.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

**Pakiety sieciowe i protokoly (Protokol Gry):**
Ten modul (Outdoors) jest czysto klientskim silnikiem obslugi perspektywy terenowej i nie konsumuje w ogole narzutu op-code pakietow. Nie posiada zadnych pakietow GC czy CG na szczeblu klas bezposrednich, reaguje posrednio, gdy siec wywola update polozenia postaci (`GC::CHARACTER_ADD` czy `GC::CHARACTER_POSITION`), a warstwa instancji zsynchronizuje te offsety z kafelkami mapy.

**Metody Pythona (Python C-API):**
Sam w sobie `MapOutdoor` nie eksponuje globalnego bindingu. Jest kontrolowany i instancjonowany ukradkiem przez `CPythonBackground` (zewnetrznie modul `background`). Mostki Pythona korzystaja z jego silnika w skryptach m.in jako: `background.LoadMap(map_name, x, y, z)`. C-API jest schowane i niedostepne na poziomie zrodel modulu Outdoors, zeby zadowolic wytyczne Zero-Python. 

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

**1. Zero-DirectX na Watkach Pobocznych:**
Absolutnie zadnych metod tworzacych obiekty interfejsu (jak `IDirect3DTexture9`, czy `Lock() / Unlock()`) wewnatrz `TEMP_CAreaLoaderThread::ProcessTerrain` lub `ProcessArea`. Direct3D API wspiera z zalozenia w tym kliencie tylko watek glowny renderujacy, wiec uzycie funkcji jak `pTerrain->LoadShadowTexture` doczytuje jedynie bufor bajtow z dysku, wlasciwa tekstura na VRAM wgrywana jest w OnUpdate z watku glownego.

**2. Garbage Collection vs Frame Stuttering:**
Metoda `__UpdateGarvage()` korzysta ze stalej wartosci `dwTerrainEraseInterval = 1000 * 60` jako timera pomiedzy wymuszeniami kasowania kafelkow. Jest to wymog celowy (stalling/throttle), aby silnik klienta nie zaliczyl spaku ilosci klatek (FPS Drop) w przypadku gdy gracz szybko wskakuje i wyskakuje z sektora kafelka za pomoca skilli (np. Ninja Szarza). Modul unika masowego `Delete()` wektorow i robi to powolnie.

**3. Kolejka zadan Fallback:**
Wazna funkcja architektoniczna: jesli instancja `CGameThreadPool` nie zostala poprawnie skonfigurowana, klasa `TEMP_CAreaLoaderThread` nie rzuca awarii ani `TraceError`, a uzywa Fallback w ramach bloku `else { ProcessTerrain(pTerrain); }` realizujac proces synchronicznie na Glownym Watku, co oznacza, ze nie bedzie zawieszenia klienta, a jedynie zauwazalny lag (Freeze).

**4. Zarzadzanie zasobami (Pamiec RAW):**
Modul nie chroni sie za pomoca RAII. Kolekcje zaleza od funkcji `pTerrain->Clear()` oraz recznego poolowania np. `CArea::ms_kPool.FreeAll()`. AI budujace nowe warstwy musi pilnowac czystosci kontenerow i czyszczenia zawartosci przy zmianie instancji (np. teleport do innego Dungeonu).

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Jak wprowadzic nowe obiekty kafelkow i atrybutow (Extension Guide):**
Gdy zajdzie potrzeba (np. zadanie na nowa mape z teksturami HD PBR lub mapy swiatecznej pogody na kafelek):
1. Dopisz nowe parametry odczytu (wypelnij token parser w `ProcessTerrain` i zmapuj na wejsciowy parametr `szNewParamFile`).
2. Nie uruchamiaj parsowania po stronie `CTerrain` i grafiki z `ProcessTerrain`. Zamien to tak, by w watku powstal jedynie parser tekstowy wrzucajacy dane na obiekty POSD (Plain Old Data Structures).
3. Przypisz te parametry w glownym ticku: w CMapOutdoor na zakonczeniu Update, zrob obrot petli obslugujacej zaladowany nowy patch, inicjalizujac shadery dla nowego zrodelka danych w RAM.
4. Zamodeluj usuniecie pliku z VRAM (jesli byl zaladowany na GPU) przy przejsciu cyklu `__UpdateGarvage()`. Nalezy wstawic kod usuwania.

**Testowanie na Linux i srodowiska CI (Headless Test Harness):**
Jezeli piszesz doctesty tego kodu, nie musisz mockowac zadnych watkow systemowych. Obiekty `TEMP_CAreaLoaderThread` zostaly napisane w taki sposob (z `if (pThreadPool)`), ze w izolowanym systemie testowym pod Linuxem automatycznie przejda na Single Thread mode. Mozna zatem sprawdzic walidacje logiki asynchronicznej wejscia i zrzuconych parametrow, odczytujac wyjscia kolejek synchronicznie. Brak uzaleznienia od Windowsowego `CreateThread` to ogromny plus. Opoznienia logowane sa przy wykorzystaniu `ELTimer_GetMSec()`. Do testow upewnij sie, ze zdefiniowano atrape dla funkcji `_snprintf`.
