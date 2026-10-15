---
task_id: "atlas_c06_01_grp_device_d3d9"
cluster: "RND"
module_name: "CGraphicDevice - Urzadzenie Direct3D 9 i Zarzadzanie Oknem"
target_files:
- src/EterLib/GrpDevice.cpp
- src/EterLib/GrpDevice.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c06_01_grp_device_d3d9.md"
architecture_layer: "Renderowanie, Direct3D 9, Shading i Culling"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport z audytu AI: CGraphicDevice - Urzadzenie Direct3D 9 i Zarzadzanie Oknem

## Cel Biznesowy i Architektura
Modul `CGraphicDevice` pelni kluczowa role w kliencie Metin2, stanowiac glowna warstwe abstrakcji i zarzadzania cyklem zycia urzadzenia renderujacego w oparciu o API Direct3D 9 (`IDirect3DDevice9Ex`). 
Jest to obiekt inicjalizowany na bardzo wczesnym etapie uruchamiania gry, zanim jeszcze zaladuje sie wiekszosc silnika, zaraz po utworzeniu glownego okna Windows.

**Cykl zycia i funkcja:**
1. **Alokacja i Inicjalizacja:** Nastepuje w wywolaniu metody `Create()`. Tam konfigurowane sa parametry prezentacji (`D3DPRESENT_PARAMETERS`), formaty pikseli, antyaliasing (MSAA) oraz odswiezanie. Nastepnie wolana jest wewnetrzna metoda Direct3D `CreateDeviceEx`. W tym momencie inicjowane sa rowniez obiekty zarzadzania stanem (`CStateManager`), macierze, shadery wierzcholkow (VS) dla strumieni, indeksy oraz bufory PDT.
2. **Utrata Urzadzenia i Reset:** Choc sam proces obslugi Lost Device jest wspierany przez silnik, metoda `Reset()` (lub obsluga poprzez inne wywolania zaleznie od stanu zwroconego przez `GetDeviceState()`) pozwala odzyskac dzialanie D3D9 i odtworzyc niezbedne zasoby (np. bufory bez D3DPOOL_MANAGED).
3. **Zarzadzanie Ramka:** Po inicjalizacji, klasa zarzadza parametrami zaleznymi od okna (jak np. `ResizeBackBuffer()`), jednak sama petla rysowania wywoluje funkcje z bazowej klasy `CGraphicBase` oraz innych menedzerow okna (np. `CGraphicScreen`).
4. **Dealokacja:** Nastepuje przez metode `Destroy()`, w ktorej system zwalnia COM obiekty poprzez `safe_release()` lub `Release()`, w tym glownie `IDirect3DDevice9Ex` (`ms_lpd3dDevice`) oraz `IDirect3D9Ex` (`ms_lpd3d`), niszczy shadery, wlasne bufory (PDT i domyslne indeksy) oraz konteksty.

## Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
- System inicjalizacji aplikacji (prawdopodobnie faza odpalenia z `UserInterface` lub z glownej petli gry), odpowiedzialny za wywolanie `CGraphicDevice::Create(...)`.
- Zewnetrzne wywolania zarzadzajace oknem i przegladarka (np. `EnableWebBrowserMode`, `ResizeBackBuffer`).
- Podsystem zarzadzania zasobami graficznymi (Tekstury, Mesh, GUI) ktore wymagaja waznego urzadzenia.

**Zaleznosci wyjsciowe (Outbound):**
- **DirectX 9 API:** API d3d9.h, `IDirect3D9Ex`, `IDirect3DDevice9Ex`, makra D3DX (np. `D3DXCreateMatrixStack`, `D3DXCreateSphere`, macierze `D3DXMatrixIdentity` itp.). Zalezy silnie od SDK DirectX (d3d9 i d3dx9).
- **CStateManager:** Reprezentuje wlasny cache stanow urzadzenia DirectX (zapobiegajacy redundantnym wywolaniom D3D API, np. SetRenderState).
- **EterLib (Lokalne Moduly):** `CGraphicBase` (dziedziczy z niego), funkcje logujace `Tracen`, `Tracenf`.
- **Win32 API:** Obsluga `HWND`, `HDC`, pobieranie `GetDeviceCaps`.

**Drzewo dyrektyw #include:**
- `GrpDevice.h` dolacza `GrpBase.h` (bazowa klasa grupujaca stale statyczne D3D i glowne zmienne) oraz `StateManager.h` (zarzadca stanow D3D).
- `GrpBase.h` (podstawa dla urzadzenia) dolacza `Ray.h` (fizyka promieni i przeciec z widoku).

**Model pamieciowy:**
Kod operuje niemal wylacznie na surowych wskaznikach C-style oraz wskaznikach na interfejsy COM zliczajace referencje (`safe_release`, `Release()`). Nie wystepuje tu zarzadzanie pamiecia przy uzyciu `std::shared_ptr` czy `unique_ptr` w odniesieniu do obiektow DirectX. Do zliczania wskaznikow i buforow stosuje sie prymitywne petle. Sam wskaznik na menadzer stanu (`m_pStateManager`) alokowany i zwalniany jest manualnie za pomoca nowej alokacji (`new`/`delete`). Wystepuja dziedziczone stale/wskazniki statyczne w klasie `CGraphicBase` (np. `ms_lpd3dDevice`).

## Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabele Klas, Struktur i Enumow
**Klasa: `CGraphicDevice` (dziedziczy po `CGraphicBase`)**
- Rola: Zarzadca inicjalizacji i cyklu zycia globalnego urzadzenia graficznego.
- Wielkosc: Rozmiar wynika glownie ze sterty lokalnych struktur i odziedziczonych pol z `CGraphicBase` (ktora trzyma znaczna wiekszosc pol jako zmienne statyczne `ms_`). Sam obiekt zajmuje niewiele miejsca (`m_uBackBufferCount`, mapka, i wskaznik `m_pStateManager`).
- Wlasciciel watku: Glowny watek renderujacy aplikacji.

**Enum: `EDeviceState`**
- `DEVICESTATE_OK`
- `DEVICESTATE_BROKEN`
- `DEVICESTATE_NEEDS_RESET`
- `DEVICESTATE_NULL`
Opis: Sluzy do kontrolowania stanu okna oraz tego czy sterownik wywalil tzw. Device Lost, zmuszajac system do resetowania/zabicia zasobow D3DPOOL_DEFAULT.

**Enum: `ECreateReturnValues`**
- Maski bitowe oznaczajace bledy poczas `Create` (np. `CREATE_NO_DIRECTX`, `CREATE_BAD_DRIVER`, `CREATE_FORMAT`). Sluzy jako kody bledow z powrotem do funkcji inicjujacej gre.

### Tabela Metod Publicznych
- `CGraphicDevice()` - Konstruktor. Inicjuje zmienne do zera. Brak bezposrednich skutkow ubocznych w D3D.
- `virtual ~CGraphicDevice()` - Wywoluje `Destroy()`.
- `void InitBackBufferCount(UINT uBackBufferCount)` - Inicjuje ilosc uzywanych buforow swap (Back buffers).
- `void Destroy()` - Skutki: niszczy wszystkie zasoby urzadzenia wlacznie ze zwalnianiem d3d, macierzy, buforow statycznych. Zeruje stan globalny.
- `int Create(HWND hWnd, int hres, int vres, bool Windowed = true, int bit = 32, int ReflashRate = 0)` - Glowna faza tworzenia. Returnuje wartosci maski Enum `ECreateReturnValues`. Modyfikuje statyczne pola D3D z `CGraphicBase`. Moze retry'owac tworzenie (ErrorCorrection) jesli sprzet odrzuci niektore konfiguracje MSAA/odswiezania.
- `EDeviceState GetDeviceState()` - Zwraca obecny stan (OK lub wymaga resetu).
- `bool Reset()` - Resetuje Device D3D uzywajac `D3DPRESENT_PARAMETERS`. (Kod prawdpoodobnie odwoluje sie do zasobow z wnetrza, sam w pliku zaimplementowany byc moze nie zostal lub uzywa wbudowanej funkcji).
- `bool ResizeBackBuffer(UINT uWidth, UINT uHeight)` - Reaguje na zmiane rozmiaru okna, np. wywolujac zmiane perspektywy/rozmiaru ramki w D3D.

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
Klasa `CGraphicDevice` posiada trzy wlasne pola (nie liczac vtable poniewaz dziedziczy wirtualnie/z metodami wirtualnymi):
- Wirtualny destruktor powoduje istnienie vtable na pierwszej pozycji (offset 0).
- `DWORD m_uBackBufferCount` (offset np. 8 lub 4 w zaleznosci od x86/x64).
- `std::map<UINT, std::string> m_kMap_strWarningMessage` (obiekt mapy po poprzednim polu).
- `CStateManager* m_pStateManager` (wskaznik wystepuje na samym koncu struktury klasy).

Reszta stanu operowana jest na wspoldzielonych dla calej aplikacji zmiennych `static` nalezacych do przestrzeni `CGraphicBase`, przez co hooking powyzszych operacji graficznych czesto opiera sie na znalezieniu globalnego wskaznika do tychze struktur (np. `CGraphicBase::ms_lpd3dDevice`).

## Mostki Sieciowe, Protokol i Python C-API

**Pakiety Sieciowe:** 
Brak bezposredniego powiazania ze scislymi pakietami wymiany w architekturze. Modul dziala czysto lokalnie, przetwarza zadania graficzne. Nie eksportuje funkcji na zewnatrz poprzez siec.

**Metody Pythona (PyMethodDef):**
Ten konkretny plik i klasa bezposrednio nie eskportuja wywolan typu C-API do interpretera Python. Python posiada swoj wrapper interfejsu (prawdopodobnie w module `system` lub `grp`), ktory globalnie odwoluje sie do menadzera silnika i nastepnie wywoluje przeksztalcenia na tym urzadzeniu (np. zmiany rozdzielczosci z menu gry). Nie ma tu zadnych obiektow `PyObject*`.

## Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

**Zasady wielowatkowosci:** 
Zdecydowanie wszystkie wywolania DirectX9 z poziomu tego modulu MUSZA byc wykonywane glownie z watku D3D. Utrzymuje to stabilnosc, m.in dlatego alokacje tekstur czy zarzadzanie fontami musi dziac sie w obrebie zabezpieczonej strefy lub glownym watku gry. Wspolbiezne dotykanie modulu renderujacego (z innych watkow sieciowych/ladowania mapy, chyba ze uzywaja np. D3DCREATE_MULTITHREADED) skutkuje natychmiastowym crashem drivera D3D lub aplikacji. W Metinie standardem jest render w pojedynczym watku.

**Potencjalne punkty awarii (Crash Points & Edge Cases):**
1. **Dangling Pointers i COM Leaks:** Przy blednym obsluzeniu obrotow gry / minimalizacji, jesli urzadzenie ma zostac odnowione (`Reset`), niszczenie (`safe_release`) nalezy uzywac uwaznie - zliczanie referencji COM wplywa na poprawne wywolanie destruktora w DLLce Windowsa. Niedopatrzenia uwalniajace zasob dwa razy skoncza sie calkowita awaria.
2. **Niedobor pamieci (D3DERR_OUTOFVIDEOMEMORY):** Wystapic moze w systemach ze slaba konfiguracja, system loguje problem jednak bezposrednio bez awaryjnego odzysku pamieci i wyrzuca blad wyzej. Zabezpieczenia na to sa pobiezne (`ms_isLowTextureMemory` jest oznaczane dla systemu teksturowego w przypadku VRAM < 64MB).
3. Brak uwzglednienia zewnetrznych utrat okna. Interfejsy COM maja rygorystyczne procedury zwiazane z tzw. `Device Lost`. Jesli jakikolwiek zasob (`D3DPOOL_DEFAULT`) nie zostanie zwolniony przed `Reset()`, metoda ta zrzuci zly HRESULT i silnik gry sie zawiesi badz wpadnie w czarny ekran (infinite loop prob resetowania).

**Zarzadzanie zasobami (RAII):**
Kod archaiczny; nie stosuje pelnoprawnego mechanizmu RAII. Wszystkie menedzery buforow wyposazone sa w metody typu `__DestroyDefaultIndexBufferList`, ktore wymagaja sekwencyjnego, ostroznego wolania w manualnym `Destroy()`. Przyszly agent w razie potrzeby dopisywania jakichkolwiek zarzadcow tablic buforow / shaderow uzywanych statycznie, nie moze zapomniec dopisac do metody niszczacej procedury zwalniajacej COM `Release()`.

## Poradnik dla Przyszlego Agenta AI

**Instrukcja dodawania nowej funkcji:**
1. Zlokalizowac zaleznosc - jesli to nowy bufor wierzcholkow wspoldzielony globalnie, zadeklarowac wskaznik COM np. `LPDIRECT3DVERTEXBUFFER9 ms_mojBufor` w `GrpBase.h`.
2. Dopisac wywolania `CreateBuffer...` w dedykowanej metodzie `__Create...()` dodawanej wewnatrz sekwencji po stworzeniu pomyslnym urzadzenia D3D9 (`ms_lpd3dDevice`) w funkcji `CGraphicDevice::Create()`.
3. Pamietac o dodaniu bezpiecznego niszczenia np. `safe_release(ms_mojBufor)` gdzies na koncu w `Destroy()` badz `__Destroy...()`.
4. Nigdy nie zapominac wziac pod uwage ze obiekt urzadzenia moze byc resetowany - obsluzyc re-alokacje, jesli nowa flaga uzywa `D3DPOOL_DEFAULT`.

**Jak debugowac i logowac:**
Zastosowane sa funkcje z `Tracef`/`Tracenf` rzucajace strumien na wlasciwy plik syslogowy silnika gry (`syserr.txt` lub log systemowy Metin2). Debugowanie okna inicjalizacji sprowadza sie do zakladania breakpointow w blokach sprawdzajacych `FAILED(...)` np. na `CreateDeviceEx`. Utrata skupienia mozna zepsuc proces podczas podgladu w Visual Studio - zalecane logi dyskowe podczas zabawy na zywym organizmie. Zmienna zlozona do nasluchiwania to glownie `ms_d3dPresentParameter` ktora zawiera format ramki/okna.

**Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
Brak natywnego d3d9.h w srodowisku CI pociaga za soba kaskade problemow. Jesli potrzebny jest unit-test, podklada sie wyabstrahowany zasymulowany obiekt implementujacy tylko to, czego oczekuje aplikacja (Dummy Struct). Nie da sie puscic powyzszego `CreateDeviceEx` bez podlozenia wlasnej DLLki D3D9 lub wyodrebnienia logiki ustawiajacej tylko same zaleznosci do innej czystej matematycznie, nie posiadajacej zaleznosci do COM metody (np. zrobienie abstrakcji dla menadzera perspektywy/rozmiaru niezaleznego od `IDirect3DDevice9`). Testowanie jednostkowe COM-owych zaleznosci jest bezcelowe i odradzane - testy naleza do warstwy integracyjnej / end-to-end z render-harness podlaczonym do okna.
