---
task_id: "atlas_c06_07_actor_instanced_batch"
cluster: "RND"
module_name: "ActorInstancedBatcher - Masowy Instancing Postaci i Mobow"
target_files:
- src/EterLib/Render/ActorInstancedBatcher.cpp
- src/EterLib/Render/ActorInstancedBatcher.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c06_07_actor_instanced_batch.md"
architecture_layer: "Renderowanie, Direct3D 9, Shading i Culling"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
**Cel Biznesowy:**
ActorInstancedBatcher to komponent renderowania sluzacy do masowego instancingu postaci i potworow (tzw. "mobow") w silniku gry. Przyspiesza on znaczaco rysowanie w zageszczonych obszarach (np. w zaludnionych miastach lub na spotach), pozwalajac na pogrupowanie setek identycznych modeli w pojedyncze wywolania instanced draw call. Zamiast wydawac polecenie rysowania dla kazdego z tych elementow osobno, silnik przekazuje jedno zadanie do GPU, minimalizujac narzut zwiazany z wywolywaniem funkcji Direct3D (tzw. CPU overhead).

**Miejsce w Petli Gry:**
Modul jest wykorzystywany w fazie `OnRender` (z reguly po fazie culling'u, w trakcie zbierania polecen rysowania). Zaimplementowano go w zmodernizowanym pipelinie renderowania C++23. Dodawanie instancji za pomoca `AddActorInstance` nastepuje w trakcie traversing'u sceny, natomiast wysylanie calych partii odbywa sie podczas wywolania `Flush`, co deleguje obiekty command (`ActorBatchCommand`) do glownej kolejki `RenderQueue`.

**Przeplyw Danych (Control Flow & Data Flow):**
1. System przechodzi przez widoczne encje w scenie (np. potwory na mapie).
2. Dla kazdej encji tego samego modelu, logika wola `AddActorInstance`, przekazujac ID modelu (`modelId`), macierz transformacji w swiecie (`world`) oraz odcien (`tint`).
3. Dane (`ActorInstanceData`) sa gromadzone w mapie hashujacej `std::unordered_map` grupowane wedlug `modelId`.
4. Wywolanie metody `Flush` przetwarza skompletowane dane instancji, kopiujac je do stalej pamieci ramki na dany kadr (za pomoca `LinearFrameAllocator`).
5. Tworzony jest `ActorBatchCommand`, ktory trafia do `RenderQueue` wraz z wygenerowanym 64-bitowym kluczem sortowania (`SortKeyBuilder::WithPass`, `WithShader`).
6. Kiedy kolejka jest oprozniana, nastepuje wlasciwe wywolanie `Execute`, konfigurujace podzial strumieni dla vertexow (`device->SetStreamSourceFreq`) oraz wlasciwe wyrysowanie geometrii w oparciu o zbuforowane dane.
7. Metoda `Clear` wyczyszcza bufory wektorow w `m_batches` gotowe na nastepna klatke.

**Cykl Zycia Obiektow:**
Obiekty `ActorBatchCommand` oraz ich bufory wejsciowe (`ActorInstanceData`) alokowane sa ramka po ramce za pomoca `LinearFrameAllocator`, ktory automatycznie dealokuje pamiec co kazdy kadr. Sam `ActorInstancedBatcher` jest elementem dlugozyjacym (zazwyczaj nalezacym do klasy renderera). Pojedyncze instancje przechowywane w wektorach wewnatrz tablicy asocjacyjnej resetowane sa za pomoca metody `clear()` wektorow standardowych (nie usuwa to samej alokacji przestrzeni std::vector, dzieki czemu unika sie narzutu na ponowne alokacje na stercie w nastepnych klatkach).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
**Zaleznosci wejsciowe (Inbound):**
- Modul wywolywany jest przede wszystkim przez system sceny (np. `InstanceSceneManager`, menedzery zmodernizowanego C++23 `ECSWorldRegistry` / `SpatialHashGrid` do filtrowania encji oraz `CPythonCharacterManager`), zglaszajacy obiekty `CInstanceBase` do wyrysowania.

**Zaleznosci wyjsciowe (Outbound):**
- **EterLib::Render:** Wykorzystuje `RenderQueue`, `LinearFrameAllocator`, oraz `SortKeyBuilder` jako baze infrastruktury deferred draw command.
- **DirectX 9 API:** D3D9 makra, typy i funkcje takie jak `LPDIRECT3DDEVICE9`, `D3DMATRIX`, a w wykonaniu `SetStreamSourceFreq` i `DrawIndexedPrimitive`. (Ukrywane za `#ifndef TEST_MOCK_D3D9` na potrzeby testow CI).

**Drzewo dyrektyw `#include`:**
- `d3d9.h` / `d3dx9math.h` (dla macierzy D3DMATRIX i renderowania)
- `RenderQueue.h`, `LinearFrameAllocator.h`, `SortKeyBuilder.h` (zaleznosci wewnatrz pakietu renderujacego)
- Biblioteki standardowe: `<unordered_map>`, `<vector>`, `<cstdint>`, `<span>`, `<cstring>`

**Model pamieciowy:**
- Obiekt przechowuje zgrupowane instancje poprzez `std::unordered_map<uint32_t, std::vector<ActorInstanceData>>` - kontenery alokujace dynamicznie, lecz per-model zyja tak dlugo, jak batcher jest dzialajacy, unikajac reinstancjacji.
- Kopiowanie danych do pamieci ramki wykorzystuje surowe wskazniki pamieci przydzielanej przez alokator liniowy (void* / C-style) i standardowe operacje blokowe (`std::memcpy`). To minimalizuje narzut smart pointerow.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur:**
| Nazwa | Rola | Wielkosc | Wlasciciel Watku |
|-------|------|----------|------------------|
| `EterLib::Render::ActorInstanceData` | Definiuje unikalny stan 1 instancji: polozenie (D3DMATRIX - 64 bajty) i kolor bazowy (uint32_t - 4 bajty). | 68 bajtow (+ padding) | Glowny Watek (Game Loop) |
| `EterLib::Render::ActorInstancedBatcher` | Punkt wejscia do zrzucania instancji per-model, zarzadzanie batchowaniem. | Nieistotna (poza danymi z heap) | Glowny Watek (Game Loop) |
| `EterLib::Render::ActorBatchCommand` | Klasa implementujaca komende bezposrednio aplikowana na urzadzeniu D3D9. Kapsulkuje parametry `Execute`. | Zalezy od x64 (ID, PTR, SIZE) | Watek Renderujacy / Glowny |

**Tabela Metod Publicznych:**
| Sygnatura | Wartosc zwracana | Warunki wstepne i Skutki uboczne |
|-----------|------------------|-----------------------------------|
| `void AddActorInstance(uint32_t modelId, const D3DMATRIX& world, uint32_t tint)` | `void` | Skutek: Dodaje element do wewnetrznej wektora w hash mapie. Warunek: Prawidlowy ID modelu. |
| `void Flush(RenderQueue& queue, LinearFrameAllocator& allocator)` | `void` | Skutek: Zrzuca wszystkie pakiety wektorow na alokator liniowy i zglasza zadanie `ActorBatchCommand` do glownej kolejki. |
| `void Clear() noexcept` | `void` | Skutek: Oproznia wektory instancji w hash mapie. |
| `void Execute(LPDIRECT3DDEVICE9 device) const noexcept` (w klasie ActorBatchCommand) | `void` | Warunek: Wskaznik na urzadzenie D3D i wgrany wczesniej globalny bufor indexow/vertexow na strumienie pipeline. Skutek: Przelaczenie `SetStreamSourceFreq` i wykonanie draw call'a. |

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
`ActorInstanceData`:
- Offset 0x00: `D3DMATRIX world` (4x4 floaty).
- Offset 0x40: `uint32_t tint` (kolor / maska dla zmaterializowanych shadera).
Layout ten jest w pelni podpiety pod instancing Direct3D (prawdopodobnie w shaderze pod rejestr wyciagajacy transformacje instancji ze strumienia vertexow #1).

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Pakiety Sieciowe:** Brak bezposredniego polaczenia. Modul ten lezy calkowicie po stronie silnika renderujacego klienta. Odbiera dane o aktualnym polozeniu mobow od systemow fizyki / sieci `SNetworkActorData`, przepuszczonych i sfiltrowanych przez odleglosciowe bound boxy.
- **Metody Pythona (`PyMethodDef`):** Modul jest silnie backendowy, brak bezposredniego mapowania do C-API w Pythonie. Ustawienia globalne dla instancingu moga ewentualnie pochodzic od menedzera `CGraphicImage`, eksponowanego gdzies w module systemu lub skryptach konfiguracyjnych gry (np. `systemSetting`).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Wszystkie wywolania do metody `AddActorInstance` i `Flush` musza odbywac sie z glownego watku podczas przejscia ramki gry (Game Loop), poniewaz `RenderQueue` nie jest wspolbieznie synchronizowany. Sam `ActorBatchCommand::Execute` jest wywolywany po Flush z watku D3D9.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Brak przydzielonej pamieci w `LinearFrameAllocator` podczas `Flush` na bardzo zageszczonych mapach - powoduje, ze batch danego modelu zostanie uciety w srodku dzialania, co objawia sie niepokazaniem czesci mobow, dlatego `allocator.Allocate()` zabezpieczony jest sprawdzeniem na `nullptr`.
  - W klasie `ActorBatchCommand`, brak przypisanego urzadzenia do D3D lub zerowy rozmiar `instancesCount` doprowadzi do segfault jesli nie byloby instrukcji warunkowej wejscia do `Execute`. Zabezpieczono wewnetrznym if-em.
- **Zarzadzanie zasobami (RAII):** Kod unika bezposredniego new/delete. Tymczasowe bloki sa wrzucane do alokatora liniowego (RAM), zdejmujac narzut zwiazany ze sterowaniem zycia elementow. Struktura wirtualna wyczyszcza je przed kolejna klatka.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Jezeli chcesz wstrzyknac nowe wlasciwosci na instancje (np. stan zamrozenia, oswietlenie per-instancja):
  2. Dodaj nowe pole do struktury `ActorInstanceData`. Pytanie, czy nie zaburzy to formatu wejsciowego bufora w `IDirect3DVertexDeclaration9` dla shadera (pamietaj o aktualizacji plikow .vsh/hlsl na GPU i layoutu zrodlowego).
  3. Przekaz ten parametr w sygnaturze metody `AddActorInstance`.
  4. Wyzeruj go badz zupdatuj w punktach wejscia.
- **Jak debugowac i logowac:**
  - By sledzic dlaczego pewne moby moga migotac przy duzej ich ilosci, zainstaluj punkt zatrzymania przed komenda `allocator.Allocate()` w petli `Flush`. Zweryfikuj, czy zadany obszar pamieci `dataSize` nie zapelnia calego dostepnego segmentu alokatora dla aktualnego kadru.
  - Ostrzezenie! Aby zweryfikowac, jakie instrukcje zostana wywolane w logach `EterBase::ModernLogger` (nie w tym module bezposrednio). Mozna by bylo go dodac na przypadek niepowodzenia `Allocate`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  - Wykorzystac makro `TEST_MOCK_D3D9`.
  - Stworzyc falszywe srodowisko testowe dla `LinearFrameAllocator`. Wypelniac `AddActorInstance`, a potem recznie zwolac `Flush` do `RenderQueue` - badajac, czy poprawne polecenia zostaly wyslane z dokladnie zdefiniowanym sort_key dla poszczegolnego ID modelu. Brak wywolan na `d3d9` pozwoli sprawnie uruchomic te testy na maszynie CI opartej na Linux.
