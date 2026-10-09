---
task_id: "atlas_c06_06_d3d9_rhi_pipeline"
cluster: "RND"
module_name: "D3D9 RHI - Nowoczesna Warstwa Abstrakcji Sprzetowej"
target_files:
- src/EterLib/Render/D3D9RenderHardwareInterface.h
- src/EterLib/Render/DrawIndexedCommand.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c06_06_d3d9_rhi_pipeline.md"
architecture_layer: "Renderowanie, Direct3D 9, Shading i Culling"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport Audytu AI: D3D9 RHI - Nowoczesna Warstwa Abstrakcji Sprzetowej

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Modul `D3D9 RHI` jest odpowiedzialny za abstrakcje najnizszej warstwy graficznej DirectX 9, oddzielajac bezposrednie wywolania API od wyzszych warstw logiki klienta Metin2. 

- **Funkcja modulu:** `D3D9RenderHardwareInterface` to implementacja interfejsu `IRenderHardwareInterface` dla starszego API D3D9. Jej zadaniem jest udostepnienie ustandaryzowanego interfejsu do obslugi cyklu zycia klatki renderowania (rozpoczynanie sceny, konczenie sceny, czyszczenie ekranu, ustawianie rzutni/viewportu). Z kolei `DrawIndexedCommand` ma za zadanie zhermetyzowac komende rysujaca geometrie oparta o indeksy (`DrawIndexedPrimitive`) w pojedynczej paczce danych, wraz ze wskazanymi buforami wierzcholkow i indeksow. Ulatwia to kolejkowanie operacji rysowania (Command Buffer/Render Queue) w celu grupowania zasobow czy optymalizacji state-changes w architekturze C++23.
- **Moment w petli gry:** Kod modulu wykonuje sie w scislym kontekscie fazy renderowania klatki `OnRender`. `BeginFrame` na poczatku cyklu rysowania danej klatki graficznej, `EndFrame` na jej zakonczeniu, natomiast instancje `DrawIndexedCommand` wywolywane sa podczas odpytywania kolejki renderingu w sercu petli rysujacej.
- **Control Flow & Data Flow:** 
  1. Engine wywoluje `RHI->BeginFrame()`.
  2. Subsystemy zbieraja geometrie (np. UI, cienie, modele, czastki) i przygotowuja instancje `DrawIndexedCommand` (okreslajac jakiego uzyc buffera, jakie jest jego stride, offset indeksow itp.).
  3. Kolejka renderowania sortuje komendy i iteruje po nich, wywolujac na kazdej komendzie `Execute(m_device)`.
  4. W `Execute`, w sposob wiazacy, wywolywane jest przypisanie buforow `SetStreamSource` i `SetIndices` na obiekcie DX9, a nastepnie komenda API `DrawIndexedPrimitive`.
  5. Engine konczy prace z klatka przez wywolanie `RHI->EndFrame()`.
- **Cykl zycia obiektow:** `D3D9RenderHardwareInterface` to obiekt o cyklu zycia powiazanym z istnieniem instancji podsystemu D3D9. Inicjowany z gotowym urzadzeniem (pointerem do Device). Interfejs sam NIE JEST wlascicielem tego urzadzenia. Z kolei `DrawIndexedCommand` sa bytem ulotnym, czesto alokowanym na stosie, wektorze, badz liniowym alokatorze dla jednej klatki obrazu (np. `LinearFrameAllocator`) i moga byc odrzucone po namalowaniu bez koniecznosci recznej dealokacji (struktura "Plain Old Data").

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

- **Zaleznosci wejsciowe (Inbound):** Kod wywolywany jest przede wszystkim przez rdzenne mechanizmy modulu renderujacego. Obejmuje to system zarzadzania render passami, rendering modeli 3D (`CInstanceBase`, `CRaceData`), UI (np. rysowanie kwadratow obrazkowych, okienek, tekstow - ewentualnie spakowanych do batchy). Interakcje przez wyzszy interfejs bazowy `IRenderHardwareInterface`.
- **Zaleznosci wyjsciowe (Outbound):** Scisla i nieunikniona zaleznosc od natywnego SDK DirectX 9 (`IDirect3DDevice9`, interfejsy buforow `IDirect3DVertexBuffer9`, `IDirect3DIndexBuffer9`, metody `SetStreamSource`, `DrawIndexedPrimitive`).
- **Drzewo dyrektyw `#include`:** 
  - `D3D9RenderHardwareInterface.h` wlacza: `../StdAfx.h` (zabezpieczone przez `#ifndef _WIN32` co jest kluczowe w srodowiskach headles / testach linuksowych), oraz `IRenderHardwareInterface.h`.
  - `DrawIndexedCommand.h` wlacza: `<d3d9.h>`.
  - Ryzyka cyklicznych zaleznosci nie wystepuja, modul uzywa forward declarations (`struct IDirect3DDevice9;`) by minimalizowac "header bloat".
- **Model pamieciowy:** Dominuja tu "raw pointers" (czyste wskazniki w stylu jezyka C). Struktury D3D zarzadzane sa jako COM objekty (np. zliczanie referencji pod spodem - `AddRef() / Release()`), ale na poziomie opisywanego RHI zaklada sie, ze inny menedzer stanow graficznych pilnuje cyklu zycia buforow i urzadzenia (brak RAII, brak `std::unique_ptr` na tych wskaznikach tutaj).

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur:**
- `EterLib::Render::D3D9RenderHardwareInterface` | Rola: Interfejs zarzadzajacy cyklem rysowania klatki oraz czyszczeniem urzadzenia. | Rozmiar: 8 bajtow na systemie 64-bitowym (zajmuje tyle co wskaznik na vtable + 1 wskaznik wlasny na `IDirect3DDevice9*`). | Wlasciciel watku: Main Thread.
- `EterLib::Render::DrawIndexedCommand` | Rola: Enkapsulacja operacji "batch rysowania". | Rozmiar: ~40 bajtow. | Wlasciciel watku: Main Thread.

**Tabela Metod Publicznych:**
- `D3D9RenderHardwareInterface::D3D9RenderHardwareInterface(IDirect3DDevice9* device)`
  - Typ zwracany: Konstruktor.
  - Pre-conditions: Przekazany parametr device powinien byc zainicjowanym obiektem urzadzenia DirectX 9 (ale moze byc NULL, metody zawieraja `if (m_device)`).
- `void D3D9RenderHardwareInterface::BeginFrame()`
  - Pre-conditions: Prawidlowe m_device. Odpowiednik `m_device->BeginScene()`.
- `void D3D9RenderHardwareInterface::EndFrame()`
  - Pre-conditions: Poprzednio zasygnalizowane BeginScene().
- `void D3D9RenderHardwareInterface::SetViewport(int x, int y, int w, int h)`
  - Skutki uboczne: Zmienia macierz transformacji viewportu w API D3D9; sluzy m.in. do renderowania do mniejszych render targetow.
- `void D3D9RenderHardwareInterface::Clear(uint32_t flags, uint32_t color, float depth)`
  - Argumenty: D3DCLEAR_TARGET/D3DCLEAR_ZBUFFER w flags, kolor tla w ARGB.
- `void DrawIndexedCommand::Execute(LPDIRECT3DDEVICE9 device) const noexcept`
  - Argumenty: `LPDIRECT3DDEVICE9 device` z API D3D9.
  - Skutki uboczne: Zmienia aktualny vertex buffer (strumien nr 0) i index buffer urzadzenia graficznego. Wykonuje polecenie renderowania do karty graficznej (pipeline call). Posiada deklaracje `noexcept`.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
`DrawIndexedCommand` - w kolejnosci definicji:
1. `D3DPRIMITIVETYPE primitiveType;` (enum/int)
2. `INT baseVertexIndex;`
3. `UINT minVertexIndex;`
4. `UINT numVertices;`
5. `UINT startIndex;`
6. `UINT primitiveCount;`
7. `LPDIRECT3DVERTEXBUFFER9 vertexBuffer;`
8. `LPDIRECT3DINDEXBUFFER9 indexBuffer;`
9. `UINT stride;`
Struktura pod wzgledem FFI / Hooking (Arthion) jest standardowym blokiem pamieci (typ POD). Wszystkie pola daja prosty offset do dynamicznego debugowania polecen wysylanych do GPU.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

Modul stanowi fundament niskopoziomowej obslugi karty graficznej, przez co JEST CZYSTY POD KATEM DOMEN ZEWNETRZNYCH.
- **Pakiety Sieciowe:** Modul calkowicie nie wchodzi w interakcje z protokolem (brak opcodow, brak Game->Client).
- **Metody Pythona:** Nie mapuje sie bezposrednio do warstwy C-API w Pythonie. Skrypty Python steruja renderowaniem posrednio, alokujac UI (`CWindowManager`, `grpText`), ktore dopiero finalnie translatory generuja odpowiednie komendy graficzne w buforze polecen D3D9.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Zasady wielowatkowosci:** API interfejsu D3D9 nie jest thread-safe (zazwyczaj tworzone jako Single-Threaded by zmaksymalizowac wydajnosc). Oznacza to, ze powyzsze pliki MOGA byc wykonywane jedynie na GLOWNYM WATKU RENDERUJACYM. Jakakolwiek proba dodania watku asynchronicznego wewnatrz `Execute()` doprowadzi do Device Hang badz Crash to Desktop.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Brak obslugi utraty urzadzenia D3D ("Device Lost" D3DERR_DEVICELOST). `BeginFrame` wywoluje bezposrednio `BeginScene`, wiec to zewnetrzny CStateManager lub wlasciciel interfejsu (aplikacja) musi zarzadzac tym stanowiskiem, zanim zawola interfejs RHI.
  - Niektore bufory (np. w `Execute`) jesli sa uszkodzone albo NULLowe beda ignorowane (brak `SetStreamSource`), jednak DirectX moze wtedy uzyc bufora przetrzymywanego ze stanu wczesniejszego, co wygeneruje anomalie geometryczne ("vertex spaghetii").
- **Zarzadzanie zasobami (RAII):** Te dwie klasy nie sa wlascicielami wskazywanych zasobow (urzadzenia i buforow). Nalezy miec pewnosc, ze inne czesci `EterLib::Render` obsluguja `Release()` i `AddRef()` z pamieci COM, by uniknac wyciekow Video RAM.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Przy modyfikacji ogolnych wlasciwosci sprzetu, najpierw zadeklaruj to w warstwie abstrakcji: `IRenderHardwareInterface.h` (np. `virtual void SetBlendState(...) = 0;`).
  2. Nastepnie dokonaj zdefiniowania overridu w `D3D9RenderHardwareInterface.h`.
  3. Kiedy potrzebujesz nowej operacji graficznej o konkretnej specyfice (np. Draw instanced), dodaj nowy plik struct (np. `DrawInstancedCommand.h`) i dodaj implementacje `Execute()` z poprawnym offsetem w DirectX API.
- **Jak debugowac i logowac:**
  Jezeli gra ma problemy wizualne na nowym pipeline - najlepszym sposobem jest zamieszczenie zewnetrznej warstwy D3D9 (np. DXVK) badz RenderDoc (po wrzuceniu warstwy tlumaczenia d3d9 na d3d11) w celu zrzucenia DrawCall'i. W samym kodzie zrodlowym warto monitorowac stan zmiennych wewnatrz `Execute` (z uzyciem breakpointow na glownym watku).
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  - Moduly dziedzicza po `IRenderHardwareInterface`, z czego implementacja D3D9 wchodzi w interakcje wylacznie z wskaznikami z przestrzeni DX9.
  - Gdy chcemy pisac Unit Testy (jak w Linux Sandboxie za pomoca `doctest`), dodajemy tzw. mock-header `tests/mock_includes/d3d9.h`, definiujemy podstawowe interfejsy `IDirect3DDevice9` jako sztuczne klasy z pustymi funkcjami. Nastepnie konstruujemy recznie strukture `DrawIndexedCommand`, wypelniamy fake'owymi offsetami i wolamy `Execute()` sprawdzajac ilosc wywolan np. przez Google Mock lub reczne flagi boolean, zachowujac srodowisko odizolowane.
