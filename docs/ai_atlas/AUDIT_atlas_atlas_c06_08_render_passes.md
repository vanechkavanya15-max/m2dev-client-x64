---
task_id: "atlas_c06_08_render_passes"
cluster: "RND"
module_name: "Dyspozytory Faz Renderowania (Opaque, AlphaTest, Blend, Additive)"
target_files:
- src/EterLib/Render/AdditivePassDispatcher.cpp
- src/EterLib/Render/AlphaBlendPassDispatcher.cpp
- src/EterLib/Render/AlphaTestPassDispatcher.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c06_08_render_passes.md"
architecture_layer: "Renderowanie, Direct3D 9, Shading i Culling"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Analizowane moduly z przestrzeni `EterLib::Render` sluza jako dyspozytory faz renderowania w rurociagu (Pipeline) klienta Metin2. Przejmuja odpowiedzialnosc za precyzyjne ustawianie i przywracanie stanow Direct3D 9 w zaleznosci od typu renderowanych materialow. Zastepuja one archaiczne i obciazone bledami zaglebienia `SetRenderState` i manualne sledzenie stanow, integrujac sie glownie przez wzorzec fasady w `STATEMANAGER`.

- **AlphaTestPassDispatcher:** Aktywuje D3DRS_ALPHATESTENABLE, D3DRS_ZWRITEENABLE i D3DRS_ALPHAFUNC. Sluzi do renderowania obiektow z materialami 1-bit alpha (przezroczystosc, np. liscie na drzewach, trawa). Wlacza Z-Buffer dla poprawnych rzutow cieni.
- **AlphaBlendPassDispatcher:** Aktywuje klasyczny Alpha Blending (`SRCALPHA`, `INVSRCALPHA`), WYLACZA Z-Write (`D3DRS_ZWRITEENABLE = FALSE`), pozostawiajac Z-Test. Jest kluczowy dla sortowania obiektow polprzezroczystych metoda Back-to-Front. Klasa ta buforuje takze komendy renderowania (funktory `std::function`) sortujac je po glebokosci (`Z-depth`).
- **AdditivePassDispatcher:** Podobnie jak blend, wylacza zapis do bufora glebokosci (`Z-Write = FALSE`). Typ blendingu modyfikuje na `ONE`, `ONE` w D3D9, co wykorzystywane jest przy renderingu efektow czasteczkowych, magii i aur postaci, wymuszajac wysoka jasnosc i "przepalanie".

**Flow kontrolny i wykonania:**
Moduly te wchodza w sklad warstwy `FramePipelineCoordinator`, wywolywane glownie w trakcie cyklu `OnRender` (faza Dispatch). Kontrola i alokacja zaczynaja sie we wczesnych fazach (np. Collect), gdzie `AlphaBlendPassDispatcher::Dispatch` zapamietuje komendy. Faza wywolujaca `Flush()` sortuje je po glebii i aplikuje zmiany D3D9 bezposrednio przed rzutowaniem pikseli.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound):** 
  - Faza `PhaseDispatch` z klasy `FramePipelineCoordinator` wola `BeginPass` oraz `EndPass` i uzywa `Activate` w `PassDispatcher`.
- **Zaleznosci wyjsciowe (Outbound):** 
  - Singleton `CStateManager` (`STATEMANAGER`), w tym metody: `SaveRenderState`, `RestoreRenderState`.
  - Stos wywolan zalezy tez wprost od makr deklrujacych SDK DirectX 9 (np. enumeracje D3DBLEND, D3DRS_). 
  - Standardowa biblioteka `<vector>`, `<functional>`, `<algorithm>`.
- **Drzewo dyrektyw `#include`:** 
  - `<d3d9.h>`, `../StdAfx.h`, `../StateManager.h` (to glowne serce zarzadzajace render state'em Direct3D).
  - W `AlphaBlendPassDispatcher` uzyte moduly algorytmiki `<algorithm>` i `<utility>`.
- **Model pamieciowy:** 
  - Klasy dispatchera stanowiska to obiekty przydzielane statycznie (w skladzie `FramePipelineCoordinator`), bez `unique_ptr`.
  - Pule zarzadzajace wewnetrzne `m_items` w `AlphaBlendPassDispatcher` wykorzystuja sterty przez wektory dynamizujace standardu C++ `std::vector<AlphaBlendItem>`.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur**
| Nazwa | Rola | Wielkosc w bajtach | Wlasciciel watku |
|---|---|---|---|
| `AdditivePassDispatcher` | Obsluga stanow Additive (ONE, ONE). Wylacza Z-Write. | minimalna (~1 bajt dummy w czystym C++) | Main (D3D9) |
| `AlphaTestPassDispatcher`| Obsluga stanow AlphaTest. Ustawia Cut-Off. | minimalna | Main (D3D9) |
| `AlphaBlendPassDispatcher`| Buforuje, sortuje polecenia i zarzadza Blend state. | Zalezne od wektora, dynamiczne | Main (D3D9) |
| `AlphaBlendItem` | Element przechowujacy glebie Z oraz callback `std::function` | Zalezy od implementacji std::function | N/A |

**Tabela Metod Publicznych**
| Sygnatura | Argumenty | Zwrot | Cel i uwagi |
|---|---|---|---|
| `BeginPass(LPDIRECT3DDEVICE9 dev) noexcept` | Wskaznik na urzadzenie D3D9 | `void` | Zapisuje do STATEMANAGER stare parametry i naklada nowe dla Pass-a. |
| `EndPass(LPDIRECT3DDEVICE9 dev) noexcept` | Wskaznik na urzadzenie D3D9 | `void` | Sciaga narzuty z pipeline i odtwarza LIFO zapisane stany. |
| `Dispatch(float, std::function<void()>)` | `float depth, std::function<void()> cmd` | `void` | Tylko `AlphaBlendPassDispatcher`. Zapisuje drawcall. |
| `Flush()` | Brak | `void` | Tylko `AlphaBlendPassDispatcher`. Wykonuje std::sort (malejaco Z) i wola cmd(). |

**Pamieciowy Layout Struktur (Memory Layout & Offsets)**
Kluczem architektonicznym przy hookowaniu modulu bedzie pominiecie `CStateManager`, ktory juz implementuje stos `STATEMANAGER_MAX_RENDERSTATES`. `AlphaBlendPassDispatcher` posiada `std::vector<AlphaBlendItem> m_items` zawierajacy lambdy renderowania. Offest `m_items` nie jest Gwarantowany ze wzgledu na brak pol wyrownawczych (w obiekcie `AlphaBlendPassDispatcher` zachowane sa 4 pola typu DWORD (`m_savedAlphaBlendEnable`, itp), ktore mimo ich braku w definicjach naglowkowych Alpha/Additive, moga pojawiac sie w wersjach deweloperskich).

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
Te moduly operuja po stronie Renderingu i nie maja bezposredniego styku z `Python C-API` (brak `PyMethodDef`). Parametry przekazywane sa w logice klienta do podsystemow efektow (np. Fly/Particle). Te moduly nie operuja na pakietach sieciowych.
  
### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Wszystkie wywolania funkcji Direct3D musza byc wykonywane glownie w watku D3D, a `std::vector::push_back` nie jest tu ubezpieczony muteksami. Obowiazuje gwarancja, ze dispatcher jest wylaczny na klatke (`BeginPass`->`EndPass`).
- **Gotcha LIFO StateManager:** W systemie `STATEMANAGER`, wszelkie pary operacji musza byc calkowicie zbalansowane (Zapis-Odczyt w kolejnosci odwrotnej, LIFO), poniewaz niektore elementy sterty narzutu opieraja sie na mechanice `std::vector::pop_back`. Niezbalansowanie doprowadza do Memory Leak lub usterki renderu nastepnej klatki.
- **Gotcha Z-Depth Sort:** W `AlphaBlendPassDispatcher::Flush()`, funkcja `std::sort` ustawia element z wartoscia wyzsza (a.depth > b.depth) na poczatek strumienia, poniewaz Back-to-Front w standardowym D3D wymaga byc rysowane z daleka (daleki clipping) przed blizszymi ekranowi pozycjami.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Jak debugowac i logowac:** Nalezy sledzic wektory stanu w `CStateManager::m_RenderStateStack` poniewaz zapisuja one modyfikacje narzucone tu. Zrzutowanie wartosci zwracanej (jesli dodamy je przez EterBase ModernLogger) w fazie BeginPass i EndPass pozwala sledzic zgubienia stanu (State Leak).
- **Dodawanie Nowej Fazy Renderowania:** Aby np. dodac SpecularPass:
  1. Utworz klase `SpecularPassDispatcher` posiadajaca `BeginPass` / `EndPass`.
  2. Skorzystaj z modulu `STATEMANAGER.SaveRenderState(...)`.
  3. Pamietaj o wywolaniu ODWROTNYCH stanow `STATEMANAGER.RestoreRenderState()` w `EndPass`.
  4. Dodaj klucz i PassType w `RenderQueue.h` uzywanym w `RenderPipelineExecutor`.
- **Testowanie Headless:** Do analizy (bez D3D) nalezy zmockowac interfejsy `IDirect3DDevice9` oraz makro `STATEMANAGER` (#define) a nastepnie uruchomic gtest/catch w piaskownicy weryfikujacy odwracalny stos dzialania LIFO w zdefiniowanych Pass-ach.
