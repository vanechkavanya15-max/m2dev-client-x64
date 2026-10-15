---
task_id: "atlas_c09_02_py_app_module"
cluster: "PY"
module_name: "Modul Pythona 'app' - Interfejs C-API Aplikacji"
target_files:
- src/UserInterface/PythonApplicationModule.cpp
- src/UserInterface/UserInterface.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c09_02_py_app_module.md"
architecture_layer: "Mostek Pythona, Moduly C-API i Skrypty Gry"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
- **Funkcja modulu**: Modul `app` sluzy jako glowny most (bridge) pomiedzy logika skryptowa Pythona a rdzeniem silnika napisanym w C++ (`CPythonApplication`). Udostepnia skryptom mozliwosc interakcji ze srodowiskiem aplikacji, zarzadzania oknem, kamera, trybem pracy kursora (SetCursorMode) myszy oraz czasem wewnetrznym gry.
- **Punkt w petli gry**: Modul jest rejestrowany podczas inicjalizacji aplikacji (`initapp()` w `UserInterface.cpp`). Wywolania funkcji nastepuja synchronicznie na zadanie skryptow Pythona, najczesciej w fazie budowy UI (OnCreate) oraz w petlach odswiezania klatek skryptowych (np. OnUpdate() interfejsu).
- **Przeplyw danych (Control Flow & Data Flow)**: 
  1. Skrypt wywoluje metode z przestrzeni `app` (np. `app.SetCamera(...)`).
  2. Interpreter wykonuje wrapper w C++ z `PythonApplicationModule.cpp` (np. `appSetCamera()`).
  3. Parametry wejsciowe (`PyObject* poArgs`) sa parsowane m.in. za pomoca `PyTuple_GetFloat()`.
  4. Nastepuje weryfikacja i przekazanie wywolania do odpowiedniej metody obiektu `CPythonApplication::Instance()`.
  5. Stan jest modyfikowany (np. polozenie kamery w `m_pyGraphic` lub tryb pracy kursora poprzez `SetCursorMode`).
- **Cykl zycia**: Instancja glowna `CPythonApplication` jest alokowana w `Main()` przed wlaczeniem srodowiska Pythona. Rejestracja C-API w `initapp()` ma miejsce w `RunMainScript()`. Obiekt `app` zyje az do zakonczenia glownej petli komunikacyjnej, po czym jest niszczony na koncu `Main()`.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound)**: 
  - Wywolania bezposrednio ze skryptow systemowych gry (np. `system.py`, `ui.py`, `introLogin.py`).
  - Zdarzenia sieciowe i interfejs uzytkownika generowane przez system operacyjny.
- **Zaleznosci wyjsciowe (Outbound)**: 
  - Rdzenne singletoiny silnika: `CPythonApplication`, `CCameraManager`, `CCamera`.
  - Biblioteki Python C-API (Python budowanie krotek, wyjatki, metody wbudowane).
  - Frameworki platformy Windows: Direct3D (kursor sprzetowy, ustawienia graficzne).
- **Drzewo dyrektyw `#include`**: 
  - Glowny naglowek modulu: `PythonApplication.h`
  - Render i zasoby: `EterLib/Camera.h`, `PackLib/PackManager.h`
  - Operacje zewnetrzne: `EterBase/tea.h`, `<stb_image.h>`, `<utf8.h>`
- **Model pamieciowy**:
  - Brak RAII (jak `std::shared_ptr`) na poziomie warstwy C-API, uzycie surowych wskaznikow i referencji Pythona.
  - Wykorzystanie statycznych tabel C-API (`PyMethodDef s_methods[]`) do wiazania wskaznikow funkcyjnych.
  - Cykl zycia glownego menedzera `CPythonApplication` jest zarzadzany globalnie.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabela Klas i Struktur
| Nazwa | Rola | Wlasciciel Watku |
|---|---|---|
| `CPythonApplication` | Glowny obiekt zarzadzajacy klientem, oknem i petla logiki. | Watek Glowny (Main Thread) |
| `CCamera` / `CCameraManager` | Kontrola srodowiska wizualnego, ustawienia matrycy i pola widzenia 3D. | Watek Glowny (Main Thread) |

### Tabela Metod Publicznych (C-API Wrappers)
| Sygnatura C++ | Warunki Wstepne | Skutki Uboczne |
|---|---|---|
| `PyObject* appSetCamera(PyObject* poSelf, PyObject* poArgs)` | Skrypt musi podac krotke (Distance, Pitch, Rotation, DestinationHeight). | Zmiana parametrow docelowych w CCamera. |
| `PyObject* appGetTime(PyObject* poSelf, PyObject* poArgs)` | Brak | Zwraca wartosc typu `float` odzwierciedlajaca `m_fGlobalTime`. |
| `PyObject* appGetGlobalTimeStamp(PyObject* poSelf, PyObject* poArgs)`| Brak | Zwraca czas z serwera lub offset lokalny wzgledem czasu Unixowego. |
| `PyObject* appAbort(PyObject* poSelf, PyObject* poArgs)` | Brak | Wywoluje natychmiastowe ukrocenie dzialania gry `Abort()`. |
| `PyObject* appExit(PyObject* poSelf, PyObject* poArgs)` | Brak | Ustawia flage wylaczenia, petla zamyka sie wdziecznie `Exit()`. |
| `PyObject* appSetSoftwareCursor(...)` / `appSetHardwareCursor(...)` | Zainicjowany kontekst renderowania okna. | Przelacza rysowanie systemowe kursora poprzez `SetCursorMode` (GDI/D3D vs Soft). |

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
- `CPythonApplication`: Dziedziczy wielokrotnie (m.in. po `CMSApplication`, `CInputKeyboard`). Struktura masywna z systemem zarzadzania czasem: `m_fGlobalTime`, `m_tServerTime`. Metody takie jak w `SetCamera` polegaja na modyfikacjach bezposrednio w `m_pyGraphic` co implikuje modyfikacje stanow DirectX.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Metody Pythona (`PyMethodDef`)**:
  - `app.SetCamera(distance, pitch, rotation, destinationHeight)` mapuje na `appSetCamera`.
  - `app.GetTime()` -> `appGetTime` -> `float` w sekundach.
  - `app.GetGlobalTimeStamp()` -> `appGetGlobalTimeStamp` -> unix timestamp (`int`).
  - `app.SetSoftwareCursor()` / `app.SetHardwareCursor()` -> mapuje na C-API modyfikujace w C++ `SetCursorMode`.
  - `app.Abort()` / `app.Exit()` -> sterowanie przeplywem zamkniecia.
- **Pakiety Sieciowe**:
  - Ten konkretny modul nie zarzadza odbiorem ani wysylaniem pakietow bezposrednio. Odbior czasu (`app.GetGlobalTime()`) zalezy posrednio od wczesniejszego zsynchronizowania ping/pong z warstwy network stream.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci**:
  - Wszystkie operacje (szczegolnie manipulacja kamera `SetCamera` i trybem kursora myszy) musza byc realizowane scisle z *Watku Glownego* (Direct3D). Wywolanie tych funkcji z asynchronicznego skryptu badz watku podrzednego zniszczy maszyne stanow DirectX prowadzac do natychmiastowego bledu przy rysowaniu na klatce.
- **Potencjalne punkty awarii (Crash Points & Edge Cases)**:
  - Przekazanie nieodpowiednich typow z Pythona powoduje zwrocenie `Py_BuildException()`. Aplikacja zignoruje wywolanie, co jest bezpieczne (nie wystapi twardy C++ memory crash), jednak zasmieci log syserr i spowolni UI.
  - Wywolanie `SetCamera` na zablokowanej kamerze (np. `IsLockCurrentCamera() == true`) zwroci wyjscie we wraperze C++, ignorujac nakaz ze skryptu.
- **Zarzadzanie zasobami (RAII)**:
  - Kod w C-API nie zwalnia ani nie alokuje pamieci operacyjnej bezposrednio, krotki pobierane z Pythona korzystaja z jego GC. Nalezy pamietac, by zawsze zwracac `Py_BuildNone()` na sukces (nie zwykle `NULL`!), aby uniknac bledow obslugi wyjatkow w wirtualnej maszynie Pythona.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide)**:
  1. Otworz `src/UserInterface/PythonApplicationModule.cpp`.
  2. Zdefiniuj statyczna funkcje: `PyObject* appMyFunction(PyObject* poSelf, PyObject* poArgs)` i pobierz argumenty z uzyciem rodziny `PyTuple_Get*`.
  3. Wywolaj wewnetrzna logike przez `CPythonApplication::Instance().MojaFunkcja()`.
  4. Dodaj wpis w tabeli `s_methods` wewnatrz `initapp()`: `{ "MojaFunkcja", appMyFunction, METH_VARARGS }`.
- **Jak debugowac i logowac**:
  - Uzywaj `TraceError("Informacja")` do generowania logow, ktore zostana zarejestrowane w konsoli deweloperskiej i zrzucone na dysk w pliku `syserr.txt`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness)**:
  - Modul app mocno wiaze sie ze statycznymi singletonami (`CPythonApplication`). W trybie headless najprosciej jest napisac test jednostkowy operujacy na samej tabeli PyMethodDef z odseparowanymi i zamockowanymi instancjami silnika EterLib, omijajac petle logiki rysowania ekranu zalezna od sprzetu.

