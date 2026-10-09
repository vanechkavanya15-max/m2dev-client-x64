---
task_id: "atlas_c08_10_minimap_atlas"
cluster: "UI"
module_name: "CPythonMiniMap - Radar, Minimapa i Pelna Mapa Atlasu"
target_files:
- src/UserInterface/PythonMiniMap.cpp
- src/UserInterface/PythonMiniMap.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c08_10_minimap_atlas.md"
architecture_layer: "Interfejs Uzytkownika, Okna i System Tekstu"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** `CPythonMiniMap` odpowiada za renderowanie zarowno mini-mapy (radaru w narozniku ekranu) jak i pelnej mapy (AtlasWindow). Wizualizuje teren, obiekty na mapie (graczy, NPC, potwory, punkty nawigacyjne, cele), gildie, oraz inne wazne lokacje, dzialajac na podstawie danych z `CPythonCharacterManager` oraz systemu terenu z `CPythonBackground`.
- **Miejsce wywolania (Game Loop):** Modul wywolywany jest glownie w dwoch momentach - `Update` w petli logiki (aktualizacja list podmiotow i pozycji) oraz `Render` w petli renderowania (rysowanie geometrii terenu mini-mapy, maskowanie jej na ksztalt kola, i rysowanie ikon punktow za pomoca Direct3D). Dla Atlasu istnieja osobne `UpdateAtlas` i `RenderAtlas`.
- **Przeplyw danych (Data Flow):** 
  - Pozycje bytowych (Entities) pobierane sa z `CPythonCharacterManager` i iterowane w `Update()`. Na podstawie filtra odleglosci (promien minimapy) ustalane sa ich docelowe koordynaty UI.
  - Modul korzysta z `CPythonBackground` do odczytywania map i wspolrzednych srodka (dla radaru lub kursora dla atlasu).
  - Skrypty Pythona poprzez Python API (`miniMap`) przekazuja komendy jak tworzenie celow (Targets), aktualizacje punktow nawigacyjnych czy otwieranie AtlasWindow.
- **Cykl zycia (Lifecycle):** Singleton tworzony przy starcie aplikacji (z reguly zintegrowany z `PythonApplication`). Posiada wlasne `Create()` (inicjalizacja buforow wierzcholkow i ladowanie tekstur ikon) i `Destroy()`. Tekstury terenu (m_lpMiniMapTexture) dla obszarow biezacych sa na biezaco aktualizowane poprzez `SetCenterPosition`.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** 
  - Wywolywany przez `PythonApplication` (inicjalizacja, render).
  - Skrypty Pythona (przez `initMiniMap()`, metody `miniMapShow`, `miniMapRender`, itp.).
  - Pakiety sieciowe (Network Handlers np. `NpcPositionHandler`, `ObserverMoveHandler`, oraz targety z faz gry).
- **Zaleznosci wyjsciowe (Outbound):** 
  - System renderingu EterLib (`CStateManager`, bufor wierzcholkow VBO, stany renderowania D3D, np. `D3DRS_TEXTUREFACTOR`).
  - System krajobrazu i terenu (`CPythonBackground`, `CMapOutdoor`).
  - System Encji/Postaci (`CPythonCharacterManager`, `CInstanceBase`).
- **Drzewo dyrektyw `#include`:** 
  - `StdAfx.h`, `PythonMiniMap.h`, `PythonBackground.h`, `PythonCharacterManager.h`, `PythonNonPlayer.h`, `PythonGuild.h`, `AbstractPlayer.h`
  - Z EterLib: `EterLib/StateManager.h`, `EterLib/GrpSubImage.h`, `EterLib/Camera.h`
  - Z PackLib: `PackLib/PackManager.h`
  - Ryzyka cyklicznych zaleznosci: Uzycie singletonow (CPythonCharacterManager::Instance) i zaleznosci miedzy wieloma elementami logiki gry ulatwia dostep, ale generuje gesty graf zaleznosci miedzy modulami UI a logika entity.
- **Model pamieciowy:** W C++ oparty glownie na strukturach standardowych (wektory pozycje bytow) oraz nagich wskaznikach C++ i referencjach z systemow singletonowych. Instancja Pythona przekazywana w `RegisterAtlasWindow` to nagi `PyObject*`, ktorym zarzadza UI okna.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabele Klas i Struktur:**
  - `CPythonMiniMap` - Singleton (wielkosc znaczna ze wzgledu na liczne kontenery STL, wskazniki do VRAM i instancje GraphicImage). Dziala wylacznie w glownym watku gry (Main Thread).
  - `TAtlasMarkInfo` - Struktura danych o znacznaczniku Atlasu. Zawiera wpolrzedne (mapy i ekranu), identyfikator instancji, typ i nazwe.
  - `TGuildAreaInfo` - Dane terytoriow gildyjnych (id, wymiary i recty renderingu).
  - `SObserver` - Klasa sluzaca do zarzadzania interpolacja ruchem punktow obserwowanych na minimapie.
  - `TMarkPosition` - Niesie x, y punktu na ekranie i indeks koloru (`UINT m_eNameColor`). Wektory takich struktur zastepuja wczesniejsze cache'owanie instancji zeby odciazyc render.
  - Enumatry (np. typy kropek): `TYPE_OPC`, `TYPE_OPCPVP`, `TYPE_NPC`, `TYPE_MONSTER`, `TYPE_WARP`, `TYPE_WAYPOINT`, `TYPE_TARGET`.

- **Tabela Metod Publicznych:**
  - `bool Create()` - inicjalizuje moduly graficzne. Zwraca bool oznaczajacy sukces.
  - `void Update(float fCenterX, float fCenterY)` - aktualizuje wektory obiektow (PC, Monster, NPC, Warp). Oblicza pozycje ekranowe krotek.
  - `void Render(float fScreenX, float fScreenY)` - rysuje krazek minimapy. Ustawia stany (StateManager) na blendowanie multi-texture i zglasza ikony na mape.
  - `bool LoadAtlas()` / `void UpdateAtlas()` / `void RenderAtlas(...)` - odpowiadaja pelnej mapie ukazywanej po nacisnieciu (M).
  - `void AddWayPoint(...)` / `void RemoveWayPoint(...)` - sterowanie customowymi punktami (np. cele questa).
  - `void AddObserver(...)` / `void MoveObserver(...)` / `void RemoveObserver(...)` - obsluga plynnosci ruchu dla znacznikow sledzonych jednostek (Observer mode).

- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  - Struktura klas dziedziczaca wielokrotnie (`CScreen`, `CSingleton<CPythonMiniMap>`). Klasa bazowa i pola maja znaczenie dla offsetow klas. 
  - Posiada bezposredni dostep do wektorow `m_PartyPCPositionVector`, `m_OtherPCPositionVector`, itd. Hooki odczytujace radary czesto przeszukuja wlasnie te bufory zamiast instancji gry. Posiada matryce transformacji D3D (`D3DXMATRIX m_matWorld`, `m_matMiniMapCover`).

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Modul bezposrednio jest karmiony z `PhaseGameTarget`, `PhaseGame`, `ObserverMoveHandler`, i `NpcPositionHandler` (C->G i G->C do zarzadzania zylami rudy, znacznikami gildi, czy sledzeniem podgladu na trybie obserwatora).
- **Metody Pythona (`PyMethodDef` - zaimplementowane w `PythonMiniMapModule.cpp`):**
  - `miniMap.SetScale`, `miniMap.ScaleUp`, `miniMap.ScaleDown` - kontrola przyblizenia.
  - `miniMap.Show`, `miniMap.Hide`, `miniMap.Render`, `miniMap.Update` - sterowanie i integracja z uiMiniMap.py
  - `miniMap.LoadAtlas`, `miniMap.ShowAtlas`, `miniMap.RenderAtlas`, `miniMap.GetAtlasInfo` - dostep do okna mapy (Atlasu).
  - `miniMap.RegisterAtlasWindow`, `miniMap.GetGuildAreaID` - narzedzia dla `uiMapNameShower.py` i interaktywnosci.
  - Stale eksportowane do UI: `miniMap.TYPE_OPC`, `miniMap.TYPE_OPCPVP`, `miniMap.TYPE_NPC`, itd.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Wszystkie wywolania tego modulu operuja w watku glownym Direct3D. Zmiana wektorow (np. `m_NPCPositionVector`) przez inny watek spowodowalaby race-condition z wywolaniem `Render()`.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** 
  - Podczas rysowania wektorow, zewnetrzny obiekt (np. tekstura) uzywana przez instancje (ImageInstance) musi istniec.
  - Obliczenia interpolacji trybu widza (`fPos`) w `Update()` nie chronia rygorystycznie mianownika przed wyjatkiem dzielenia przez 0, wiec w klasie `SObserver` zmusza do ustawienia `dwDstTime = dwSrcTime + 1000`.
  - Zarzadzanie wskaznikami PyObject (handler okna atlasu) wymaga by Python nie ubil instancji powiazanej podczas kiedy C++ gdzies ja wola (`m_poHandler`).
- **Zarzadzanie zasobami (RAII):** 
  - Obiekty typu Textury minimalizuja zonglowanie zasobami, jednak `D3DXMATRIX` modyfikuje pipeline transformacji. Po renderowaniu CPythonMiniMap upewnia sie w uzyciu funkcji `RestoreRenderState` ze tory stanow (ColorArg, AlphaArg, AddressU/V) wracaja do normy. Inaczej GUI psuje sie reszcie gry.
  - Nalezy uwazac na czyszczenie `m_kMap_dwVID_kObserver` aby uniknac puchniecia mapy (Memory Leak starych obserwatorow).

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zdefiniuj nowy staly enum `TYPE_...` w `CPythonMiniMap.h` i powiaz go w strukturze mapy ikon na poziomie C++ i Python (w `PythonMiniMapModule.cpp`).
  2. Przydziel/Zaladuj mu grafike (SubImage/ExpandedImageInstance).
  3. Skategoryzuj instancje graczy/celow w iteracji `CPythonCharacterManager` wewnatrz metody `CPythonMiniMap::Update`.
  4. Dodaj logike rysowania w bloku `CPythonMiniMap::Render` ze statycznym offsetem lub uzyciem funkcji sluzacych do mapowania (swiat->minimapa).
- **Jak debugowac i logowac:**
  - Dodaj logowanie via `TraceError/SysErr` w bloku iteracji `Update` i sprawdz jakie `VID` sa ignorowane przez fDistanceFromCenter (poza widocznoscia ekranu radaru). Sprawdz parametry zoomu `m_fScale`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  - Wyodrebnij mock na wektory encji (`TInstanceMarkPositionVector`). Utworz sztuczne wstrzykniecie pozycji globalnych, nastepnie przeprowadz assert na wynikowych wpolrzednych wewnatrz wektora np. `m_OtherPCPositionVector`. Obliczenia transformacji 3D->2D mozna badac matematycznie bez obaw o VRAM.
