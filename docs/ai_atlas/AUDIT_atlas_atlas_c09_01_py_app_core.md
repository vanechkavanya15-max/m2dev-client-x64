---
task_id: "atlas_c09_01_py_app_core"
cluster: "PY"
module_name: "CPythonApplication - Glowna Petla Aplikacji i Klatek Gry"
target_files:
- src/UserInterface/PythonApplication.cpp
- src/UserInterface/PythonApplication.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c09_01_py_app_core.md"
architecture_layer: "Mostek Pythona, Moduly C-API i Skrypty Gry"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport AI: CPythonApplication - Glowna Petla Aplikacji i Klatek Gry

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Modul `CPythonApplication` jest absolutnym sercem i punktem centralnym (Singletonem) klienta gry Metin2. Stanowi pomost miedzy systemem operacyjnym Win32, warstwa renderowania DirectX 9, srodowiskiem skryptowym Pythona oraz systemami silnika EterLib/GameLib.
- **Funkcja w architekturze:** CPythonApplication inicjalizuje gre, koordynuje wszystkie glowne podsystemy (jak CPythonPlayer, CPythonNetworkStream, CPythonBackground, menedzery obiektow, interfejs uzytkownika UI) oraz utrzymuje ich jednoczesne dzialanie.
- **Miejsce wywolywania:** Kod dziala poprzez nieustannie aktywna petle glowna gry: `CPythonApplication::Loop()`. 
- **Przeplyw danych (Data & Control Flow):** System Win32 informuje o oknach i zdarzeniach wejscia -> Pady, Klawiatura, Mysz odswiezane sa asynchronicznie (UpdateKeyboard, UpdateMouse) i dystrybuowane na eventy (OnMouseUpdate, OnUIUpdate).
- **Cykl zycia (Lifecycle):** Obiekt inicjalizowany jest raz (wywoluje `Create()` gdzie wstaje caly silnik i Win32, m.in. Direct3D Device). Petla `Loop()` wywoluje `MessageProcess()` oraz glowne serce gry: `Process()`. Na sam koniec wywolywane jest gigantyczne czyszczenie podsystemow w `Destroy()`.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound):** Glowny plik uruchomieniowy procesu klienta tworzy i podtrzymuje instancje tej klasy. Rozne hooki i callbacki systemowe oraz modulowy Python wykorzystuja istnienie tej aplikacji, aby wymuszac operacje na silniku, np. wychodzenie (`Exit()`, `Abort()`). Menedzery systemowe odwoluja sie do instancji za pomoca `CPythonApplication::Instance()`.
- **Zaleznosci wyjsciowe (Outbound):** Potezna siec powiazan do silnika. Miedzy innymi:
  - DirectX 9 / EterLib (CGraphicDevice `m_grpDevice`, CNetworkDevice, CTimer, FrameTimer).
  - GameLib / EffectLib (CEffectManager, CRaceManager, CItemManager, CFlyingManager).
  - Skrypty i interfejs GUI (UI::CWindowManager, CPythonPlayer, CPythonNetworkStream, CPythonIME, CPythonTextTail).
  - Granny 3D (CGrannyMaterial, CGrannyLODController).
  - Obsluga systemu dzwieku (`m_SoundEngine`).
- **Drzewo dyrektyw `#include`:** Dolacza caly wachlarz klas z katalogu glownego oraz systemowych bibliotek, w szczegolnosci `PythonPlayer.h`, `PythonNetworkStream.h`, `PythonCharacterManager.h`, `EterLib/FrameTimer.h`, `eterLib/GrpDevice.h`. Silna ryzykowna zaleznosc cykliczna przy odwolaniach do Singletonow innych modulow zostala zminimalizowana za pomoca wzorcow menedzera z przodu (forward declarations badz interfejsy).
- **Model pamieciowy:** W obiekcie przechowywane sa z reguly silnie wyizolowane menedzery wartosciowo (tzw. kompozycja), a nie przez pamiec dynamiczna wskaznikow, co poprawia lokalnosc pamieci, e.g. `CPythonPlayer m_pyPlayer;` badz `CPythonBackground m_pyBackground;`. Wystepuja klasyczne zaleznosci oparte na czystych wskaznikach w przypadku elementow DirectShow (`m_pGraphBuilder`, `m_pVideoWnd`). Nie uzywa sie tu std::shared_ptr/unique_ptr, wymuszone RAII przez parowanie funkcji `Create`/`Destroy`.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur:**
- `CPythonApplication` - Klasa dziedziczaca z `CMSApplication`, `CInputKeyboard` oraz `IAbstractApplication`. Glowny kontroler gry operujacy na watku glownym renderujacym. Posiada dziesiatki kompozycyjnych modulow i Singletonow.
- `SCameraSpeed` - Struktura trzymajaca aktualna predkosc obrotu oraz przyblizenia kamery, wazne przy plynnej interpolacji kamery myszka. Wypelnia dane na stosie (float).

**Tabela Enums:**
- `EDeviceState` (DEVICE_STATE_FALSE, DEVICE_STATE_SKIP, DEVICE_STATE_OK) - Stany zarzadzania silnikiem (LostDevice) z D3D.
- `ECursorMode` (CURSOR_MODE_HARDWARE, CURSOR_MODE_SOFTWARE) - Determinuje, czy kursor jest zalezny od D3D czy renderowany softwarowo przez OS.
- `ECursorShape` - Zawiera typowe operacje na GUI kursora: TARGET, ATTACK, TALK, PICK, MAGIC.

**Tabela Metod Publicznych i Sygnatur (Wazne interfejsy dla mostka FFI i AI):**
- `bool Process()` -> Zwraca true, chyba ze aplikacja wylacza sie. Serce klatki renderujacej.
- `void UpdateGame()` -> Aktualizuje transformacje aktorow, logike, podsystemy kolizji, obiekty na ziemi (Item) oraz pozycje srodka (`m_pyPlayer.NEW_GetMainActorPosition`).
- `void RenderGame()` -> Odsyla rysowanie grafiki do odpowiednich modulow (Sky, Cloud, Terrain, Water, Snow, UI, Items).
- `bool Create(PyObject* poSelf, const char* c_szName, int width, int height, int Windowed)` -> Pelna alokacja podsystemow, okna MSWindow, zasobow DirectX, podpiecie Pythona i ladowanie wstepne. Inicjalizuje rowniez `CGameThreadPool`.
- `void Loop()` -> Glowne wywolanie petli.
- `void SetServerTime(time_t tTime)` -> Koordynacja czasu miedzy klientem a serwerem na podstawie pingow synchronizujacych.
- `int CheckDeviceState()` -> Weryfikacja polaczenia ekranu i czy okno renderowania jest zepsute (`m_pyGraphic.IsLostDevice()`).

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- Metody nie udostepniaja sie w C++ natywnie (nie mamy tu typowej definicji z `PyMethodDef`), jednak interfejs jest gleboko osadzony poprzez pole `m_poMouseHandler`, wywolujace bezposrednie delegacje ze zdarzen myszy do skryptow `game.py`.
- Obsluga sieci sprowadza sie do regularnego wywolywania `m_pyNetworkStream.Process()`, `m_kGuildMarkDownloader.Process()`, `m_kGuildMarkUploader.Process()`, `m_kAccountConnector.Process()` na poziomie 60 klatek na sekunde (`FrameTimer::Instance().Tick`). Obiekty te dbaja o weryfikacje pakietow GC oraz wysylanie pakietow CG.
- `CPythonApplication::NotifyHack(const char* c_szFormat, ...)` uzywa streamu by podkablowac anomalie gry prosto do weryfikatora serwerowego.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Wielowatkowosc:** Kod z zalozenia korzysta z jednowatkowej semantyki Direct3D. Zmiana widoku w pamieci albo ladowanie shaderow/stanow renderowania `m_pyGraphic` wymaga uzywania ekskluzywnego dostepu do glownego watku gry, unikaj operacji D3D gdzies w theadowej puli `CGameThreadPool`, ze wzgledu na mozliwe `D3DERR_INVALIDCALL`.
- **Pulapki Crashy z Utrata Urzadzenia D3D:** Poteznym zagrozeniem sa zminimalizowane okna i Alt+Tabowanie. Wewnatrz funkcji `Process()`, system aktywnie dlawi wywolania rysowania grafiki kiedy okno jest ukryte (oddaje zasoby uzywajac `Sleep(10)` dla OS'u). Proba wymuszenia instrukcji D3D w stanie Lost Device zakonczy sie katastrofa. 
- **Pamiec (Memory Leak):** Obiekty instancjonowane sa bez RAII, wiec metoda `CPythonApplication::Destroy()` MUSI zniszczyc dokladnie wszystkie utworzone managery, obiekty graficzne i urzadzenia z zachowaniem scislej odwrotnej kolejnosci do inicjalizacji. Utrata nawet jednego zwalniania spowoduje powolne psucie pamieci przy restartach okien / soft-restartach serwera w kliencie.
- **Taktowanie Klatek (Framerate Check):** W `Process()` znajduje sie asynchroniczne odswiezanie logiki wzgledem wideo: Logika chodzi w stalych ramkach wymuszonych przez FrameTimer(60Hz lambda), podczas gdy render uzywa interpolowanych transformacji na kazdej klatce ekranu (wsparcie 144Hz+).
- **Zasady Bezpieczenstwa UI:** Unikaj polaczenia watku gry z uzyciem stalych dlawikow w UI, by nie zerwac synchronizacji sieci.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Krok-po-kroku (Dodawanie nowego podsystemu/Managera):**
  1. Dodaj obiekt jako polowe czlonkowskie na koncu bloku w pliku `PythonApplication.h` (uzyj deklaracji `class` by nie psuc kompilacji zaleznosci, np. `class CNowyManager;`).
  2. W sekcji `Destroy()` pliku `PythonApplication.cpp` wywolaj niszczenie obiektu (`m_Nowy.Destroy()` albo usun ze wskaznika). Nalezy trzymac sie hierarchii likwidowania najpierw narzedzi wyzszego rzedu, a pozniej klas bazowych (D3D na samym koncu).
  3. Wywolaj zintegrowane ticki (`Update()` lub `Render()`) w odpowiadajacych im funkcjach `CPythonApplication::UpdateGame()` oraz `CPythonApplication::RenderGame()`.
- **Debugowanie i Profilowanie:** W funkcji `Process()` zbierany jest log co 1000 ms by ustalic wartosci FPS, ms dla updatow, updateCount oraz polycount rzedow trojkatow. Sprawdzac uzywajac wbudowanego monitora statystyk (m.in. `GetFaceCount()`, `GetUpdateFPS()`, `GetRenderFPS()`). Uzywaj macro `TraceError("Tekst Bledu");` by poslac blad prosto do pliku `syserr.txt` (czesta praktyka).
- **Testowanie (Headless Harness):** Brak wsparcia ze strony Win32 D3D do dzialania zupelnie bez okna (headless device). Mockowanie podzespolu wymaga przekazywania wyjatku D3D na urzadzenie `NULL` (referencyjne badz mock_d3d). Czysty C++ gtest bezposrednio nie wywola `Process()` bez stworzenia Dummy Window Handlera i przekazania parametrow. Zamiast tego do testowania pojedynczych obiektow odpinaj interfejs poszczegolnych systemow (np. samo testowanie funkcji matematycznych dla kamery nie wymaga `Process`).

---
Raport poprawnie zweryfikowany z uzyciem metadanych AI i zasad ZERO-CONFLICT klasteryzacji. Gotowe pod implementacje nastepnych instrukcji roju.
