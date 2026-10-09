---
task_id: "atlas_c06_03_state_manager"
cluster: "RND"
module_name: "CStateManager - Cache Stanow Urzadzenia Graficznego"
target_files:
- src/EterLib/StateManager.cpp
- src/EterLib/StateManager.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c06_03_state_manager.md"
architecture_layer: "Renderowanie, Direct3D 9, Shading i Culling"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja w architekturze**: `CStateManager` dziala jako posrednik (proxy) i pamiec podreczna (cache) miedzy silnikiem gry a sterownikiem DirectX 9. Zapobiega nadmiarowym wywolaniom kosztownych funkcji API D3D9 (takich jak `SetRenderState`, `SetTexture`, `SetTransform`), zapisujac w pamieci uzytkownika obecny stan urzadzenia. Ponadto umozliwia zapisywanie i przywracanie stanow na stosach, dzialajac na zasadzie Push/Pop dla szybkiego nakladania lokalnych parametrow renderowania bez trwalej modyfikacji calego srodowiska.
- **Moment wywolania**: Modul ten jest najbardziej obciazony w fazie `OnRender` w kazdej klatce. Obsluguje cale kolejkowanie rysowania 3D, terenu, instancji aktorow, cieni oraz interfejsu UI w 2D (wywolywany miedzy innymi przez `UIPassDispatcher`).
- **Przeplyw danych**: Kiedy element gry decyduje sie zmienic stan (np. wylaczyc Z-Buffer w UI), uzywa wywolania np. `STATEMANAGER->SetRenderState()`. `CStateManager` sprawdza lokalna kopie danego rejestru w strukturze `CStateManagerState m_CurrentState`. Jesli wartosc rozni sie od docelowej, przesyla instrukcje prosto do `LPDIRECT3DDEVICE9EX`, i uaktualnia swoja zmienna.
- **Cykl zycia**: Instancjonowany z reguly jako klasa typu wzorzec Singleton (`CSingleton<CStateManager>`), uruchamiany przez klase urzadzenia graficznego (`CGraphicDevice`) we wczesnym etapie inicjalizacji okna aplikacji. Zostaje odpiety z pamieci i zwalnia urzadzenie COM po wylaczeniu glownego okna.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound)**: Wykrzystywany przez praktycznie wszystkie obiekty rysujace w Metin2:
  - System renderingu EterLib/EterGrnLib (UIPassDispatcher, RenderStateSnapshot, WorldMatrixScope, BlendStateScope).
  - Graficzne systemy logiki Pythona i C++ (efekty czasteczkowe, modele postaci, ladowanie terenu).
- **Zaleznosci wyjsciowe (Outbound)**: 
  - DirectX 9 API: `LPDIRECT3DDEVICE9EX`, macierze `D3DXMATRIX`, struktury konfiguracyjne `D3DMATERIAL9`, oswietlenie `D3DLIGHT9`.
  - Infrastruktura wewnetrzna: `CSingleton` z `EterBase`, `GrpLightManager`.
- **Drzewo dyrektyw `#include`**: 
  - `StateManager.h`: `<d3d9.h>`, `<d3dx9.h>`, `<vector>`, `<stack>`, `"EterBase/Singleton.h"`
  - `StateManager.cpp`: `"StdAfx.h"`, `"StateManager.h"`, `"GrpLightManager.h"`
  - Istnieje tu duze zaleznosci od Windows API i specyficznych interfejsow Microsoft D3D.
- **Model pamieciowy**: Stan D3D w cache reprezentowany jest na masowo kopiowanych tablicach pre-alokowanych typow prostych. Zarzadzanie wskaznikami polega wylacznie na referencjach COM `AddRef`/`Release` przy obsludze interfejsu urzadzenia. Zmienne globalne zasobow (Textury, Buffery) sa trzymane jako wolne wskazniki (`LPDIRECT3DVERTEXBUFFER9`), wiec odpiete gdzies indziej moga zostac jako "dangling pointers" w stosach Managera.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur**:
  - `CStateManager`: Rozbudowany Singleton zarzadzajacy zlecaniami do karty graficznej i buforem w klatce. Calkowicie ograniczony do glownego watku renderujacego.
  - `CStateManagerState`: Ogromna struktura (alokacja setek KB) majaca na celu przechowac baze dla `m_CurrentState`. Zawiera macierze wartosci np. `m_TextureStates[8][128]`, `m_Matrices[300]`.
  - `CStreamData`: Mala (8-16 bajtow) struktura laczaca VertexBuffer D3D z jego 'Stride' parametrem do szybkiego porownania czy strumien wymaga podmiany na karcie graficznej.
  - `CIndexData`: Odpowiednik dla bufora indeksow `IDirect3DIndexBuffer9`.
- **Tabela Metod Publicznych (CStateManager)**:
  - `SetRenderState(D3DRENDERSTATETYPE, DWORD)`: Ustawia wartosc maszyny stanow wewnetrznych dla DX9. Wymaga by urzadzenie d3d bylo aktywne. Moze wplynac na dzialanie bufora Z czy culling (Skutek uboczny: graficzna zmiana na ekranie).
  - `SaveRenderState(Type, Value)` i `RestoreRenderState(Type)`: Para polecen zrzucajaca na `std::vector` i sciagajaca element w starym RAII. Zwraca: void.
  - `BeginScene()`, `EndScene()`: Przygotowuje srodowisko pod batching lub odswieza bufory klatki ekranu.
  - `SetVertexShader`, `SetPixelShader`, `SetTexture`: Zmienia shadery bindowane do rysowanego prymitywu, rowniez z modelem sprawdzajacym (if param == cache -> return).
- **Pamieciowy Layout Struktur (Memory Layout & Offsets)**:
  - Pola w `CStateManagerState` ulozone sa po kolei. Dla zaawansowanych botow/hookingow, pierwsze pole to `m_RenderStates[256]` (1024 bajty). Caly obiekt `CStateManager` instancjonowany we wczesnym stadium, moze miec statyczne podpiecie poprzez Hook vtable urzadzenia.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe**: Brak bezposredniego powiazania - warstwa wybitnie renderujaca bez komunikacji pakietowej z hostem.
- **Metody Pythona**: Brak udostepnionych komend (PyMethodDef) w samej klasie. Do CStateManagera Python dobiera sie droga okrezna - wywolujac kod UI, renderowanie efektow czy `PythonGraphicModernBridge.h`, ktory deleguje to bezkolizyjnie i chroni CStateManager.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci**: CStateManager i sterowniki DirectX 9 w tym projekcie pracuja z zasady z modelem JEDNOWATKOWYM (Single Threaded Device). Kazda asynchroniczna zmiana stanu, tekstury lub wywolanie `SaveTexture` z ThreadWorkerow zawiesi maszyne lub wywali Driver karty graficznej na platformie Windows.
- **Potencjalne punkty awarii (Crash Points & Edge Cases)**:
  - Rozsynchronizowanie stosu: Klasyczne uzycie `SaveX()` i powrot z funkcji z pominieciem `RestoreX()` (np. poprzez wczesny return albo uzycie wyjatku C++ bez opakowania try-catch/RAII) calkowicie dewastuje stos, skutkujac rysowaniem mrugajacych tekstur i zablokowaniem buforow Z-Buffera na kolejne dni w tej samej instancji. Nalezy korzystac z nowoczesnych scope'ow RAII (np. zadeklarowanych w RenderStateSnapshot.h/WorldMatrixScope).
  - Kontekst przerywany (Device Lost): Gdy okno traci srodowisko wylacznosci (Alt+Tab na Fullscreenie), wszystkie bufory na karcie umieraja i CStateManager nie ma swiadomosci - dlatego musi zostac wykonana pelna reinstalacja zmiennych przy DeviceReset w glownej logice urzadzenia EterLib.
- **Zarzadzanie zasobami (RAII)**: Samo trzymanie wektorow `std::vector` z rezerwacja chroni czesciowo, jednak upewnij sie ze referencje wejsciowe np. D3DVERTEXBUFFER przezywaja czas buforowania na stosie stanu z Managera.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide)**:
  1. Jesli DirectX 9.0c pozwala na nowy rodzaj rejestru (nie uzywanego tu), dodaj miejsce na macierz do `CStateManagerState`.
  2. Zainicjuj te macierz bezpieczna wartoscia neutralna dla grafiki (np. w `CStateManagerState::ResetState()`).
  3. Dodaj publiczne wywolania (Getter, Setter, Save, Restore) do definicji `CStateManager`.
  4. Dodaj strukture stosu jako prywatny czlonek `std::vector` (np. `m_NewStateStack`).
  5. Zweryfikuj dodane pola kompilacja Mock testow.
- **Jak debugowac i logowac**: Modul loguje bledne sparowania wektorow (wylapuje pusty stos gdy wykonuje sie pop() przy uzyciu makra `StateManager_Assert`). Do debugowania "SetCall" nadmiarow, wystarczy dolozyc tymczasowe zapytania `Tracef` do pliku przy przelaczaniu stanow, lub monitorowac zmienna `m_iDrawCallCount` do debugowania batchingu poligonow.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness)**: Skompilowanie narzedzi wylapujacych logike wymaga podlozenia sztucznego interfejsu. W pliku kompilacji doctest ustaw flagi `-DTEST_MOCK_D3D9`, by interfejs IDirect3DDevice9 i COM w `GetDevice` zachowywaly sie jak Linuxowy zestaw stub'ow, i mockuj tablice transformacji uzywajac `-DD3DMATRIX=D3DXMATRIX`. Wszelkie zasady Testowania Z-Buffera daja sie tak sprawdzic w izolacji narzedziami Linux-compatible (bez CMake C++23 calosci klienta).
