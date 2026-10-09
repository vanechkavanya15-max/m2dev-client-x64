---
task_id: "atlas_c06_09_depth_prepass"
cluster: "RND"
module_name: "Depth Prepass Dispatcher - Wczesna Faza Glebokosci (Early-Z)"
target_files:
- src/EterLib/Render/DepthPrepassDispatcher.cpp
- src/EterLib/Render/DepthStencilScope.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c06_09_depth_prepass.md"
architecture_layer: "Renderowanie, Direct3D 9, Shading i Culling"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

**Funkcja:**
Modul implementuje optymalizacje Early-Z (Depth Prepass) w silniku EterLib opartym o Direct3D 9. Jego zadaniem jest wyeliminowanie problemu nadmiernego rysowania (Overdraw), czyli obliczania pikseli dla obiektow (terenu, postaci, SpeedTree), ktore i tak beda ostatecznie zasloniete przez inne obiekty na scenie. Robi to poprzez wpisanie wartosci glebi (Z-buffer) dla nieprzezroczystych (opaque) geometrii na poczatku klatki przed faza wlasciwego cieniowania, co pozwala karcie graficznej odrzucac niewidoczne fragmenty wlasciwych passow znacznie szybciej.
`DepthStencilScope` dostarcza wygodnego opakowania opartego na idiomie RAII do latwego zarzadzania stanami Z-bufora (odczyt/zapis, tylko odczyt, wylaczone) i wylaczania stencil bufora na poziomie konkretnych zadan.
`DepthPrepassDispatcher` zarzadza globalnymi ustawieniami rurociagu dla specjalnej fazy renderowania w `FramePipelineCoordinator`.

**Moment wywolania:**
Kod jest wywolywany z glownej petli renderowania klienta `OnRender`, po zebraniu danych geometrycznych (fazy `PhaseCollect`, `PhaseSort`), a dokladniej w fazie uruchamiania dyspozytorow (`PhaseDispatch`) w `FramePipelineCoordinator`. Kod dyspozytora dziala przed passami rysujacymi finalny kolor (np. `OpaquePassDispatcher`).

**Przeplyw danych (Data Flow & Control Flow):**
1. Otrzymanie obiektu `LPDIRECT3DDEVICE9`.
2. `DepthPrepassDispatcher::BeginPass` instaluje na stosie stany:
   a. Blokuje zapisy do bufora kolorow maska 0 (`m_colorWriteScope.emplace(0)`).
   b. Wlacza zapis do Z-buffera (`m_zBufferScope.emplace()`).
   c. Odpina Pixel Shader (`m_pixelShaderScope.emplace(nullptr)`), zeby minimalizowac obciazenie ALU GPU na czas wpisywania glebi.
3. Klient renderuje geometrie nalezaca do passu Depth Prepass (kod wlasciwy zewnetrzny wobec modulu).
4. `DepthPrepassDispatcher::EndPass` zdejmuje zainstalowane stany w kolejnosci odwrotnej (`m_pixelShaderScope.reset()`, `m_zBufferScope.reset()`, `m_colorWriteScope.reset()`), przywracajac stan `STATEMANAGER`.

**Cykl zycia obiektow (Lifecycle):**
`DepthPrepassDispatcher` i obiekty modyfikujace stan zostaly zaimplementowane w architekturze bezalokacyjnej (na stosie) lub alokacji na obiekcie nadrzednym (`FramePipelineCoordinator` zarzadza czasem zycia jako jego agregacja obok innych dyspozytorow). Tworzenie i zwalnianie stanow zarzadzane jest przez RAII w paradygmacie C++17 z wykorzystaniem `std::optional`. Ulatwia to odwracalny reset potoku za pomoca operacji na klasach typu "Scope", ktore pobieraja stany zapisane w singletonie EterLib `STATEMANAGER`.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
- **Wywolujacy:** `EterLib::Render::FramePipelineCoordinator` (deklaruje obiekt `DepthPrepassDispatcher m_depthPrepass`).
- Modul ten jest integralna czescia nowoczesnego pipeline-u rysowania uzywanego przez klienta Metin2.

**Zaleznosci wyjsciowe (Outbound):**
- **DirectX 9 API:** Uzywa uchwytow `LPDIRECT3DDEVICE9` z biblioteki `d3d9.h`. Uzywa makr m.in. `D3DRS_COLORWRITEENABLE`, `D3DCMP_LESSEQUAL`.
- **EterLib StateManager:** Globalny system EterLib `STATEMANAGER` opakowujacy wolania `LPDIRECT3DDEVICE9::SetRenderState` i buforujacy poprzednie stany, optymalizujacy zapytania CPU-GPU (`../../EterLib/StateManager.h`).
- **RAII Scopes:** Klasy pomocnicze: `ColorWriteScope.h`, `PixelShaderScope.h`, oraz `ZBufferScope` zadeklarowany we wnetrzu pliku naglowkowego dyspozytora.
- **RenderStateTypes:** Dostarcza enum `DepthMode` (`RenderStateTypes.h`).

**Drzewo dyrektyw #include:**
W `DepthPrepassDispatcher.h`:
- `<d3d9.h>`
- `<optional>`
- `"../StateManager.h"`
- `"ColorWriteScope.h"`
- `"PixelShaderScope.h"`

W `DepthStencilScope.h`:
- `"RenderStateTypes.h"`
- `"../../EterLib/StateManager.h"`

*Ryzyka cyklicznych zaleznosci:* Brak. System RAII izoluje warstwy odpowiedzialnosci.

**Model pamieciowy:**
Brak dynamicznych alokacji wskazywanych przez czyste wskazniki C lub inteligentne wskazniki (`shared_ptr` / `unique_ptr`). Obiekty alokowane sa glownie na stosie lub jako czesc wektora / klasy matki (agregacja w `FramePipelineCoordinator`). System podaje bezposrednio czysty wskaznik `LPDIRECT3DDEVICE9`, co nie narusza standardow pod warunkiem, ze jest uzywany jako "non-owning observer pointer", ktory do poprawnego dzialania Direct3D 9 w COM API wystarcza, i nie uzywa wewnetrznie `AddRef` ani `Release` w tym procesie dyspozytora.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur:**
1. **`EterLib::Render::DepthPrepassDispatcher`**
   - Rola: Dyspozytor ustawiajacy potok dla rysowania Early-Z.
   - Wlasciciel watku: Watek Glowny (Render Thread).
2. **`EterLib::Render::ZBufferScope`**
   - Rola: Obiekt straznik (Scope Guard) dla `D3DRS_ZWRITEENABLE`.
   - Wlasciciel watku: Watek Glowny (Render Thread).
3. **`EterLib::Render::DepthStencilScope`**
   - Rola: Guard dla komplexowych stanow D3D (D3DRS_ZENABLE, D3DRS_ZWRITEENABLE, D3DRS_ZFUNC, D3DRS_STENCILENABLE) sterowanych flaga DepthMode.
   - Wlasciciel watku: Watek Glowny (Render Thread).
4. **`EterLib::Render::DepthMode`**
   - Rola: Enum trybu operacji z-buffora: ReadWrite, ReadOnly, Disabled.
   - Wlasciciel watku: N/A.

**Tabela Metod Publicznych:**
- `DepthPrepassDispatcher::BeginPass(LPDIRECT3DDEVICE9 dev) noexcept`
  - Argumenty: `dev` - wskaznik interfejsu D3D9.
  - Wartosc zwracana: `void`.
  - Warunki wstepne: `dev` nie powienien byc nullptr.
  - Skutki uboczne: Zmienia w globalnym `STATEMANAGER` bufor koloru na maske 0, uaktywnia Z-bufor, odlancza pixel shader i zapisuje stany na stos straznikow modulu.
- `DepthPrepassDispatcher::EndPass(LPDIRECT3DDEVICE9 dev) noexcept`
  - Argumenty: `dev` - wskaznik interfejsu D3D9.
  - Wartosc zwracana: `void`.
  - Warunki wstepne: `dev` nie moze byc nullptr, potok graficzny uzywa otwartego obiektu D3D.
  - Skutki uboczne: Przywraca stany. UWAGA: Jesli BeginPass nie zostalo wywolane wczesniej, `std::optional::reset()` robi bezpiecznego no-opa.
- `DepthStencilScope::DepthStencilScope(DepthMode mode)`
  - Argumenty: `mode` flagujacy tryb z DepthMode.
  - Wartosc zwracana: N/A (konstruktor).
  - Skutki uboczne: Cichy i zewnetrzny update potoku poprzez stan managera.
- `DepthStencilScope::~DepthStencilScope()`
  - Argumenty: Brak.
  - Wartosc zwracana: N/A (destruktor).
  - Skutki uboczne: Odtwarza wczesniejszy stan flag glebi i stencila.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
Dla klasy `DepthPrepassDispatcher` (Offsety zalezne od architektury 64-bit MSVC z ABI C++):
- Posiada trzy obiekty pol typu `std::optional`. W kazdym typie wewnatrz alokowana jest flaga logiczna (bool) informujaca o obecnosci wartosci, oraz zaalokowany blok na odpowiednia klase pomocnicza np `PixelShaderScope` z jej oryginalnym wskaznikiem starego shadera i statusem boolean. Rozmiar klasy zamyka sie w ~32 bajtach bez posredniego mapowania V-Table. Obiekty nie dziedzicza polimorficznie, dzialaja liniowo z plaskim stosem w pamieci podrecznej L1.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

**Pakiety Sieciowe:**
Brak podlaczen do pakietow sieciowych (CG/GC).

**Metody Pythona (`PyMethodDef`):**
Brak bezposredniego wiazania z Pythonem przez makra `PyMethodDef`. Dzialanie optymalizacji Early-Z odbywa sie na biezaco przez engine pod warunkiem inicjalizacji gry na kliencie.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

**Zasady wielowatkowosci:**
Direct3D 9 w tej implementacji nie jest zaprojektowany pod bycie bezpiecznym dla wielu watkow. Rozwijanie wielowatkowosci renderowania doprowadzi do natychmiastowych awarii jesli z rownoleglych zadan wywolywany bedzie `STATEMANAGER`. Funkcje dispatchera zawsze musza pozostac osadzone w glownym watku okienkowym (GUI / Render Thread).

**Potencjalne punkty awarii (Crash Points & Edge Cases):**
1. **Nullowy wskaznik uzycia:** Przekazanie do BeginPass / EndPass modulu `dev == nullptr`. Kod jest uodporniony przez wczesne zwroty kontroli (`if (!dev) { return; }`).
2. **Kolejnosc RAII:** Wykorzystywanie instrukcji opuszczenia stanow poza zasada "First In, Last Out" (LIFO) doprowadzi do wadliwego stanu `STATEMANAGER`, skutkujac miganiem tekstur. Tu destruktor lub precyzyjne odpalanie wywolania `reset()` radzi sobie z tym bezpiecznie dla Dispatchera. 
3. **Niepoprawne zaplecze shadera:** Odpiecie pixela poprzez NULL jest zamierzona operacja ale niektore stare lub specyficzne modyfikowane karty graficzne z nieobsluzonym starym API moga zgubicz vertex pipeline jesli glebia nie jest zmapowana w prawidlowych wspolrzednych dla `m_pixelShaderScope.emplace(nullptr)`.

**Zarzadzanie zasobami (RAII):**
Calosc tego interfejsu modeluje podejscie oparte na straznikach zasiegu (Scope Guard Pattern), calkowicie rugujac potrzeba bezposredniej alokacji `new` i eliminujac "Memory i VRAM Leaks", dzieki nielimitowanemu i domyslnemu wsparciu COM API dla zarzadzania odniesieniami uzywajac auto zwalniania.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
1. Otworz plik naglowkowy docelowy (`DepthPrepassDispatcher.h`).
2. Przy dodawaniu nowego straznika modyfikujacego zachowanie potoku (np obsluga nowego parametru w Z-buffer) skorzystaj najpierw z dodania klasy np `CullModeScope` na wzor obok `ColorWriteScope.h`.
3. Dodaj w obiekcie prywatnym deklaracje `std::optional<NewFeatureScope> m_newFeatureScope;`.
4. Modyfikuj `BeginPass` w pliku `.cpp`. Przed operacjami uruchomieniowymi podepnij logike przez `.emplace()`.
5. Modyfikuj `EndPass` i dopisz wywolanie metody `.reset()` u samego poczatku bloku aby spelnialo zalozenia LIFO, to zagwarantuje ze D3D wyrowna state renderera do pierwotnego.

**Jak debugowac i logowac:**
Kategorycznie nie umieszczaj standardowych wywolan logera dyskowego we wnetrzu dispatchera (np `sys_log(0, ...)`), gdyz uderzy to drastycznie w frame rate klienta wywolujac setki opoznien wejscia wyjscia na chwile trwania klatki. Kod debuguje sie przy wykorzystaniu narzedzi deweloperskich GPU takich jak RenderDoc lub PIX przez sprawdzanie tabeli stanow Pipeline State Objects na poszczegolnych Eventach (DrawCalls).

**Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
Ze wzgledu na powiazanie modulu z singletonem `STATEMANAGER` bazujacym na API Windows (Direct3D9), implementowanie testow jednostkowych w srodowisku Linux bedzie utrudnione. Konieczne bedzie zainicjalizowanie mockujacej biblioteki Direct3D definiujacej struct COM np `IDirect3DDevice9`. State Manager umozliwia testowanie go przez zastapienie mapy w globalnym stanie silnika - stary `SetRenderState` mockuje poprawny bufor RAM, co umozliwi testowanie czy Depth Prepass zmienia flage uzywajac asercji do `EXPECT_EQ`.
