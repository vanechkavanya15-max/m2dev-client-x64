---
task_id: "atlas_c05_01_map_manager_core"
cluster: "WLD"
module_name: "CMapManager - Glowny Koordynator Mapy Swiata"
target_files:
- src/GameLib/MapManager.cpp
- src/GameLib/MapManager.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c05_01_map_manager_core.md"
architecture_layer: "Swiat Gry, Teren, Kolizje i Nawigacja"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** `CMapManager` dziala jako centralny menedzer dla calego swiata gry po stronie klienta. Zarzadza ladowaniem mapy (`CMapOutdoor`), parsowaniem metadanych z `AtlasInfo.txt`, obsluga srodowiska (oswietlenie, mgla za pomoca struktury `TEnvironmentData`) oraz koordynacja zapytan o wlasciwosci terenu (np. wysokosc z `GetHeight`, kolizje z `isAttrOn`). Reprezentuje glowny wezel (singletonopodobny w uzyciu, choc instancjonowany zewnetrznie) spinajacy logike mapy z podsystemem renderowania (DirectX/EterLib) oraz drzewami predkosci (`CSpeedTreeForestDirectX`).
- **Moment wywolania:** Wywolywany glownie podczas inicjalizacji (ladowania nowej mapy przez `LoadMap`), a takze w glownej petli gry podczas aktualizacji logiki i widoku (przez `UpdateMap` i `UpdateAroundAmbience`). Rowniez system renderowania odpytuje `CMapManager` o parametry terenu (mgla, srodowisko) przed rysowaniem w fazie `OnRender` (posrednio np. przez `BeginEnvironment`/`EndEnvironment`).
- **Przeplyw danych (Data Flow):** 
  1. Przy wejsciu do gry wczytywane jest `AtlasInfo.txt` (`__LoadMapInfoVector`), uzupelniajac wektor `m_kVct_kMapInfo` nazwami map i ich rozmiarami/koordynatami.
  2. Nastepuje wywolanie `LoadMap`, co skutkuje inicjalizacja lub przypisaniem glownego obiekty terenu (`CMapOutdoor`), wyliczaniem lokalizacji i przekazaniem kontroli ladowania terenu.
  3. Modul srodowiska pyta `CMapManager` o `TEnvironmentData` by zaaplikowac fog (mgla) czy swiatla (kierunkowe).
  4. Zapytania np. z fizyki lub entity o kolizje trafiaja do `isPhysicalCollision` i dalej w dol do `CMapOutdoor`.
- **Cykl zycia (Lifecycle):** Instancja jest zazwyczaj zarzadzana przez aplikacje (np. CPythonBackground pod spodem). Alokacja w `Create()` (`AllocMap()`), inicjalizacja podstaw, a przy przejsciu przez mapy: `LoadMap` -> dzialanie -> `UnloadMap` (badz `Clear`). Zwalnianie przez `Destroy()` na sam koniec zamkniecia klienta. Obiekty `TEnvironmentData` alokowane i dealokowane dynamicznie w zaleznosci od zarejestrowanych map srodowisk (`RegisterEnvironmentData`).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Wywolywany przez logike tla (`CPythonBackground`, moduly ladowania), silnik fizyki graczy/mobow (potrzebujacy sprawdzic `GetTerrainHeight`, `isPhysicalCollision` poprzez interfejs `IPhysicsWorld`), narzedzia Python/C API.
- **Zaleznosci wyjsciowe (Outbound):** 
  - `CMapOutdoor` (glowne opakowanie renderingu mapy i pod-sektorow/obszarow - `CArea`).
  - `TEnvironmentData` (struktury definiujace oswietlenie i mgle - pliki `.msenv` lub `.mse`).
  - DirectX 9 (`STATEMANAGER` / statemanager do `SetRenderState`, `D3D` parametry fog).
  - EterPack / Virtual File System (`CPackManager`, `TPackFile`) do ladowania pliku `AtlasInfo.txt`.
  - `CSpeedTreeForestDirectX` (wsparcie renderowania lasow z drzew speedtree - konfiguracja mgly/transparencji drzew).
  - `CMemoryTextFileLoader` (prosty parser ASCII txt).
- **Drzewo dyrektyw `#include`:**
  - `MapOutdoor.h` (bezposrednia zaleznosc obiektu terenu, w kompozycji rzutowana w `m_pkMap`).
  - `PropertyManager.h` (`m_PropertyManager` - zarzadzanie metadanymi wlasciwosci).
  - `PhysicsObject.h` (dla kolizji, interfejs `IPhysicsWorld`).
  - Posrednie zaleznosci od `StdAfx.h` (lub ogolnego setupu w CMapManager.cpp, np. wbudowane makra DirectX, `STATEMANAGER`, `TraceError`).
- **Model pamieciowy:** W C++03/98 styl: czyste wskazniki (`CMapOutdoor * m_pkMap`, iteratory map srodowiska przechowujace czyste wskazniki `TEnvironmentData *`). Silny nacisk na zarzadzanie wskaznikami przez `new`/`delete` (np. `DeleteEnvironmentData`).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur:**
  - `CMapManager`: Dziedziczy po `CScreen` (prawdopodobnie obsluga GUI/wyswietlania z EterLib) i `IPhysicsWorld` (obsluga kolizji terenu). Wielkosc zalezy od wbudowanych wektorow i map (`std::vector`, `std::map`, interfejsow). Posiada `CMapOutdoor* m_pkMap` (wlasnosc glownego swiata) oraz `CSpeedTreeForestDirectX m_Forest` jako agregacje lub wsparcie dla instancji drzew.
  - `TMapInfo`: Struktura POD (z `std::string`) przechowujaca: nazwa, baseX, baseY, sizeX, sizeY, endX, endY. Informacje z `AtlasInfo.txt` wczytane na potrzeby lokalizacji gracza na globalnej siatce.
  - `FFindMapName`: Funktor (`struct`) do wyszukiwania z kaskadowym opuszczaniem wielkosci liter (case-insensitive `stl_lowers`), uzywany podczas walidacji / wyszukiwania ladowanej mapy.
- **Tabela Metod Publicznych (wybrane kluczowe):**
  - `bool LoadMap(const std::string&, float x, float y, float z)`: Laduje nowa lokacje.
  - `bool UpdateMap(float fx, float fy, float fz)`: Odswieza widocznosc sektorow wzgledem pozycji (fx, fy, fz).
  - `void BeginEnvironment() / EndEnvironment()`: Push / Pop stanow renderowania DirectX (Fog, Swiatlo) na podstawie przypisanego wczesniej `TEnvironmentData`.
  - `float GetTerrainHeight(float fx, float fy)`: Zwraca fizyczna wysokosc Z terenu na x,y. Pre-condition: Mapa zaladowana (`IsMapReady()`).
  - `bool isPhysicalCollision(const D3DXVECTOR3& c_rvCheckPosition)`: Override z `IPhysicsWorld` (albo custom) weryfikujacy `ATTRIBUTE_BLOCK` w koordynatach gry (-Y uwzgledniane).
- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  - `CMapManager` utrzymuje wektor wartosci (`m_kVct_kMapInfo`) sluzacy do bisekcji/iteracji. Mapy srodowiskowe w zbalansowanym drzewie (`std::map<DWORD, TEnvironmentData*> m_EnvironmentDataMap`). Posiada wlasna instancje lasu. Wymagane pilnowanie cyklu zycia wskaznikow z mapy srodowiskowej pod katem wyciekow.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- Modul sam z siebie nie wysyla ani nie odbiera pakietow. 
- Jest punktem styku zewnetrznym dla bindow Pythona posrednio poprzez `CPythonBackground` (zatem funkcje map managera sa eksponowane w `PythonBackgroundManager` jako moduly scriptowe do np. ustawiania srodowiska - day/night switch).
- Wywolania logowania i wczytywania (np. poziom mgly) polegaja na instancji `CPythonSystem::Instance().GetFogLevel()`, co wiaze wczytywanie mapy bezposrednio z konfiguracja Pythona.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Funkcje renderujace (`BeginEnvironment`) i modyfikujace mape (`UpdateMap`) musza byc wylacznie w glownym watku DirectX. Operacje na plikach (np. `__LoadMapInfoVector`) obecnie wydaja sie byc w pelni synchroniczne (bez async IO), co moze powodowac blokady na klatkach podczas ladowania terenu.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - `mc_pcurEnvironmentData` musi byc poprawnie aktualizowany. Resetowanie w trakcie renderowania lub dostep na nullu (dlatego uzywany jest `TraceError`/`assert`). Zauwaz w `BeginEnvironment`: jesli `mc_pcurEnvironmentData == nullptr`, konczy sie wczesnym returnem, ale jesli gdzies ponizej jest zakladany jako nienull, spowoduje crash.
  - Zwolnienie pamieci iteracyjne: `m_EnvironmentDataMap` recznie zarzadza pamiecia przez `DeleteEnvironmentData(f->second);`. Moze to spowodowac wycieki jesli mapa nie jest poprawnie czyszczona na zniszczeniu CMapManager.
  - Rzutowanie wskaznikow i stanow (`STATEMANAGER.SetRenderState(D3DRS_FOGDENSITY, *((DWORD *) &fDensity));` - naruszenie strict aliasing, typowe w dx9 C++98 API).
- **Zarzadzanie zasobami (RAII):** Silnie oparte na `Initialize()` / `Destroy()`. Zrezygnuj z tworzenia obkietu na stosie jesli cykl alokacji (stworzenie wezla renderingu terenu) zaklada globalne wiazania. Brak C++11 smart pointerow (np. std::unique_ptr) dla wlasnosci mapy `m_pkMap`.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zidentyfikuj, czy funkcja to cecha renderowania, czy kolizji. Jesli to modyfikacja terenu globalnego (np. nowa flaga strefy), dodaj nowa tablice/wczytywanie w `CMapManager::LoadMap/LoadMapInfo` oraz w delegacji do `CMapOutdoor`.
  2. Deklaruj nowa sygnature w `MapManager.h`, unikaj wstrzykiwania bezposrednio nowych include'ow, faworyzuj forward declarations.
  3. Wywolania render state DX9 dodawaj/modyfikuj ostroznie, pilnuj by zaraz po uzyciu pop-owac state w `EndEnvironment`.
- **Jak debugowac i logowac:**
  - W C++ w uzyciu jest system powiazany z `TraceError` (dla null checkow przy srodowisku) oraz typowe asercje. 
  - Do sprawdzania siatki koordynatow, zwroc uwage, ze koordynaty sa czesto przeliczane pomiedzy wielkosciami komorek/chunkow.
- **Jak testowac bez interfejsu graficznego:**
  - Jako ze jest mocno sprzezony z DirectX (`STATEMANAGER`, uzycia `D3DFOG_EXP`), testy headless wymagaja mocka klas `STATEMANAGER` i `CMapOutdoor`, ewentualnie obejscia dyrektyw z `CSpeedTreeForestDirectX`. Uruchamiaj logike w trybie, w ktorym wskaznik na interfejs DX9 moze byc zastepowany atrapa i wylacz rysowanie (`IsSoftwareTilingEnable()`).
