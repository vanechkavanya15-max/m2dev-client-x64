---
task_id: "atlas_c05_15_water_dungeon_blocks"
cluster: "WLD"
module_name: "Strefy Wody i Bloki Geometryczne Dungeonow"
target_files:
- src/GameLib/WaterHazardMap.h
- src/GameLib/DungeonBlock.cpp
- src/GameLib/DungeonBlock.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c05_15_water_dungeon_blocks.md"
architecture_layer: "Swiat Gry, Teren, Kolizje i Nawigacja"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul jest odpowiedzialny za dwa glowne filary srodowiska swiata gry (WLD): zarzadzanie strefami wody ograniczajacymi ruch ladowy oraz zarzadzanie obiektami lochow (Dungeon Blocks), ktore sa specjalnymi modelami statycznymi.
W `WaterHazardMap` klasa zarzadza strefami wody dzialajacymi jako prostopadloscienne granice blokujace przemieszczanie sie jednostek; integracja ta opiera sie o wzorzec zdarzen (`UserInterface::Core::EventBus`). Klasy lochow (DungeonBlock) oparte sa o system wizualny Granny 3D (`CGraphicObjectInstance`, `CGrannyModelInstance`) oraz zarzadze renderowaniem blokow, obsluguja siatki deformatywne, uderzenia w sfery kraweziowe oraz renderuja cienie i oswietlenie na siatce lochow uzywajac stanu Direct3D.
Petla gry przetwarza te obiekty za pomoca standardowego cyklu: OnUpdate (dla `DungeonBlock` w celu transformacji modeli i interpolacji czasu), a nastepnie OnRender i OnRenderShadow podczas klatki renderowania. `WaterHazardMap` dziala jako sprawdzenie bezstanowe dla proby ruchu.
W obrebie cyklu zycia, `WaterHazardMap` alokuje strefy za posrednictwem `std::vector`, podczas gdy `CDungeonBlock` wczytuje zasoby przez `CResourceManager`, inicjuje modele instancji przyznajac po jednym na kazdy pod-model i recznie przydziela pamiec dynamiczna (`new CDungeonModelInstance`), ktore to musza byc wyczyszczone w funkcji `Destroy()` poprzez `stl_wipe`.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Kod `WaterHazardMap` jest powolywany w modulach nawigacji i sterowania (`ActorInstanceMotion` etc.). Z kolei `CDungeonBlock` ladowany jest podczas konstrukcji i ladowania map z lochami (`MapOutdoor`, `AreaLoaderThread`).
- **Zaleznosci wyjsciowe (Outbound):** 
  - `WaterHazardMap` zalezy od `EterBase/Result.h`, `EterBase/LogModern.h`, oraz `EventBus.h`.
  - `CDungeonBlock` powiazany jest z systemem EterLib: zasobami (`CResourceManager`), obiektami graficznymi (`CGraphicObjectInstance`), CGrannyModelInstance (z EterGrnLib), oraz narzedziami stanow D3D (`StateManager.h`).
- **Drzewo dyrektyw `#include`:** 
  - WaterHazardMap.h: `<vector>`, `<optional>`, `<format>`, `EterBase/Result.h`, `EterBase/StrongTypes.h`, `EterBase/LogModern.h`, `UserInterface/Core/EventBus.h`.
  - DungeonBlock.h: `EterLib/ResourceManager.h`, `EterLib/GrpObjectInstance.h`, `EterGrnLib/ModelInstance.h`, `EterGrnLib/Thing.h`.
  - DungeonBlock.cpp: `StdAfx.h`, `DungeonBlock.h`, `EterLib/StateManager.h`.
- **Model pamieciowy:** W `WaterHazardMap` wektor struktur trzymajacy statyczne `WaterZone`. W `CDungeonBlock` zastosowano klasyczny zestaw C++03 polegajacy na czystych wskaznikach, liscie iterowanej iteratorem, oraz surowych tablicach buforow (`LPDIRECT3DVERTEXBUFFER9`), a takze wykorzystano inteligentne delegaty systemow takich jak `CResourceManager`.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
| Nazwa | Rola | Wielkosc/Opis | Wlasciciel Watku |
|---|---|---|---|
| `GameLib::Navigation::WaterZone` | Prosta struktura blokujaca ruch po danym terenie. | 16B (4x int) | Watek Glowny |
| `GameLib::Navigation::WaterBlockEvent` | Event wysylany przy nieudanej probie ruchu przez wode. | Podstawa (IEvent) + EntityId + 2x int | Watek Glowny |
| `GameLib::Navigation::WaterHazardMap` | Modul zarzadzajacy `WaterZone` do ograniczania. | Zawiera `std::vector`. | Watek Glowny |
| `CDungeonBlock` | Blok geometrii z lochu. Oparty na Graphic Object Instance. | Sfera (16B), Wskazniki itp. | Watki Render i Glowny |
| `CDungeonModelInstance` | Obudowa na `CGrannyModelInstance` dla lochow | Wskaznik buforow w D3D | Render Thread |

**Tabela Metod Publicznych:**
- `WaterHazardMap::AddWaterZone(int startX, int startY, int endX, int endY) -> void`
- `WaterHazardMap::FindBlockingZone(int x, int y) -> std::optional<WaterZone>`
- `WaterHazardMap::CheckMovement(EterBase::EntityId entityId, int x, int y) -> EterBase::Result<void, EterBase::NavigationError>`
  - _Skutki uboczne:_ Emituje `WaterBlockEvent` w przypadku znalezienia wody; zapisuje do Loggera.
- `CDungeonBlock::Load(const char * c_szFileName) -> bool`
  - _Warunki wstepne:_ `CResourceManager` musi byc zainicjowany. Zwraca false gdy plik ma bledy struktury w CGraphicThing.
- `CDungeonBlock::BuildBoundingSphere() -> void`
  - _Skutki uboczne:_ Przelicza centralna wirtualna sfere oparta o min/max instancji, dolicza +150 jednostek marginesu.
- `CDungeonBlock::Intersect(float * pfu, float * pfv, float * pft) -> bool`

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
`CDungeonBlock` dziedziczy po `CGraphicObjectInstance`, wiec pamiec vtable oraz podstawowy matrix swiata znajduja sie na offsetach zerowych bazowego obiektu. Modyfikatory pamieci sfery zamykajacej wystepuja po czlonku wektora wskaznikow instancji z Granny. Zalezne od 32-bit (std::vector rozmiaru 12B/16B, vtable). 

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- Ten zestaw zrodel operuje glownie po stronie C++ swiata (WLD), nie definiuje wlasnego API dla Pythona i protokolow pakietowych. Zjawiska te integrowane sa jako moduly mapowe (`MapBase`). `WaterHazardMap` jest jedynie modulem fizyczno-logicznym uzywanym przez Network / ActorInstance; bezposrednio nie implementuje `PyMethodDef`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Obiekty z DirectX 9 i menadzera zasobow (`CDungeonBlock`) musza byc wczytywane (bufor, VRAM) we wlasciwym watku. Pule takie moga ulec destrukcji w nieoczekiwanych momentach, w ktorych uzywane jest `LPDIRECT3DVERTEXBUFFER9`. Renderowanie to proces liniowy, watek w D3D9 ma jednowatkowa optymalizacje.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** 
  - Klasa `CDungeonBlock` uzywa `stl_wipe(m_ModelInstanceContainer)`, wiec bezposredni dostep do pocietej tablicy spowoduje Use-After-Free jesli destrukcja nastapila za szybko. 
  - Interakcja ze zmiennym `fRadius` wywoluje bezwzgledne wazenie dla intersekcji kamery; zle wyliczone centrum uderzy w optymalizator frustum cullingu.
  - Ostrzezenie dotyczy wycinania elementow przy obsludze tablic renderowania Granny; struktury czesto nie posiadaja odpowiedniego mapowania D3D (`lpd3dRigidPNTVtxBuf`). Brak ladowania VRAM rzuci wyjatek.
- **Zarzadzanie zasobami (RAII):** `WaterHazardMap` prawidlowo uzywa RAII (std::vector), jednak `CDungeonBlock` opiera sie na C-style recznym wywolywaniu `new CDungeonModelInstance` i kasowaniu tego w destuktorze/`Destroy()`. Brak recznego wylaczenia bufora w `m_kDeformableVertexBuffer` przed `Create()` prowadzi do wyciekow pamieci VRAM w `Load()`.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji:** 
  1. Zmiana geometrii blokady powinna uwzgledniac event bus (dodac nowy format np. `LavaZoneEvent`)
  2. Implementacja dodawania `CDungeonBlock` wymaga modyfikacji w wektorach aktualizacji. 
  3. Podczas modyfikacji stref renderowania dungeonow nalezy uwazac na stany Direct3D (niezapomniec o `RestoreRenderState`), ktorego uzywa metoda `RenderDungeonBlockShadow()`.
- **Jak debugowac i logowac:** Do przesledzenia wody dodane sa logi ModernLogger (`Info` i `Debug` w WaterHazardMap). Logi dungeonow mozna wyciagnac wpinajac hook na TraceError z EterLib w `CDungeonBlock::Load()`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** 
  - `WaterHazardMap` jest gotowe na jednostkowe testowanie - mozna skompilowac tylko ten plik dla `doctest`, poniewaz odlacza on zaleznosci od DX i UI. `CDungeonBlock` nie nadaje sie do headless unit test bez duzej liczby mockow systemow D3D9 (zobacz wytyczne testowania MSVC/g++ dla makr D3D).
