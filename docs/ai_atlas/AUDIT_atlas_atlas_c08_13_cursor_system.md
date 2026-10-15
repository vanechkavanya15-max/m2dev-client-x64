---
task_id: "atlas_c08_13_cursor_system"
cluster: "UI"
module_name: "System Kursorow Myszy i Zmiany Kontekstowe"
target_files:
- src/UserInterface/PythonApplicationProcedure.cpp
- src/UserInterface/Cursors/CursorManager.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c08_13_cursor_system.md"
architecture_layer: "Interfejs Uzytkownika, Okna i System Tekstu"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul odpowiada za calosciowe zarzadzanie kursorami myszy w kliencie gry Metin2. Obsluguje ladowanie, wyswietlanie oraz zmiany kontekstowe kursorow w zaleznosci od akcji uzytkownika i stanu gry. Dziala na poziomie `CPythonApplication`, zarzadzajac kursorami sprzetowymi Windows (Hardware Cursors) ladowanymi z pliku zasobow (.rc) poprzez wywolanie funkcji API Win32 (`LoadImage`, `SetCursor`, `ShowCursor`).

- **Zarzadzanie Cyklem Zycia (Lifecycle):** Inicjalizacja kursorow nastepuje w `CPythonApplication::CreateCursors()` podzas rozruchu aplikacji, gdzie ladowane sa ikony sprzetowe i przypisywane do mapy `m_CursorHandleMap`. Zwalnianie odbywa sie w `DestroyCursors()` wywolujacym `DestroyCursor()`.
- **Zarzadzanie Stanem (Control Flow):** System aktualizuje aktualny kursor nasluchujac komunikatu `WM_SETCURSOR` z petli okien w `CPythonApplication::WindowProcedure()`. Jezeli aplikacja jest aktywna i wlaczony jest tryb sprzetowy (`CURSOR_MODE_HARDWARE`), kursor jest nadpisywany zgodnie ze stanem `m_hCurrentCursor`.
- **Zmiany Kontekstowe:** Zmiana kursora nastepuje m.in. poprzez funkcje `SetCursorNum()`, ktora moze zostac wywolana ze skryptow Pythona (np. przy najechaniu na wroga - `CURSOR_SHAPE_ATTACK`, lub na NPC - `CURSOR_SHAPE_TALK`).
- **Zapamietywanie Poprzedniego Stanu:** Posiada logike zapamietywania "ciaglego" kursora (`m_iContinuousCursorNum`), ktora pozwala powrocic do prawidlowego glownego kursora po chwilowych zmianach.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):**
  - **Petla komunikatow Windows (Win32):** Komunikaty takie jak `WM_SETCURSOR` przechwytywane przez `CPythonApplication::WindowProcedure`.
  - **Mostek Python (Python C-API):** Metody zadeklarowane w `src/UserInterface/PythonApplicationModule.cpp` (np. `appSetCursor`, `appShowCursor`).
  - **Interakcja UI i Ruch Gracza:** Kursor moze byc ukrywany/wyswietlany w zaleznosci od sterowania kamera w `PythonPlayerInputMouse.cpp` (`CAMERA_ROTATE`).
- **Zaleznosci wyjsciowe (Outbound):**
  - **Win32 API:** Wykorzystanie `SetCursor`, `ShowCursor`, `LoadImage`, `DestroyCursor`, `ReleaseCapture`, `SetCapture`.
  - **Zasoby Windows (resource.h):** Zaleznosc od starych identyfikatorow zasobow kursora (np. `IDC_CURSOR_NORMAL`, `IDC_CURSOR_ATTACK`).
  - **Python C-API:** Delegacja obslugi zdarzen myszy do skryptow Pythona poprzez `PyCallClassMemberFunc(m_poMouseHandler, "ChangeCursor", ...)`.
- **Drzewo dyrektyw `#include`:** `PythonApplicationProcedure.cpp` oraz powiazane kody do kursorow dolaczaja standardowe wewnetrzne systemy EterLib oraz `<winuser.h>`.
- **Model pamieciowy:** Uchwyty Win32 typu `HANDLE` (`HCURSOR`) zarzadzane m.in. w kontenerze `std::map<int, HANDLE> m_CursorHandleMap`. Klasa centralna `CPythonApplication` pelni role pseudo-singletona dla UI. (Wymieniony w zadaniu `CursorManager.h` jest artefaktem logicznym zespolonym w ewaluacji z `PythonApplication`).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
| Nazwa | Rola | Watkowosc |
|-------|------|-----------|
| `CPythonApplication` | Centralna aplikacja i menedzer kursorow w architekturze, obsluguje glowna petle zdarzen UI | Watek glowny (UI/RenderThread) |

**Tabela Metod Publicznych (`CPythonApplication` dot. kursorow):**
| Sygnatura | Opis | Skutki Uboczne i Warunki |
|-----------|------|--------------------------|
| `bool CreateCursors()` | Inicjalizuje uchwyty kursorow sprzetowych. | Wymaga dostepu do zasobow z `ms_hInstance`. Rejestruje logike kursorow dla interfejsu. |
| `void DestroyCursors()` | Zwalnia pamiec po kursorach (Win32 API). | Dealokuje uchwyty. Musi byc wolana przy wyjsciu. |
| `void SetCursorVisible(BOOL bFlag, bool bLiarCursorOn)` | Ustawia widocznosc kursora przez API Win32. | Manipuluje globalnym stanem Windows dla procesu wywolujac `ShowCursor()`. |
| `BOOL SetCursorNum(int iCursorNum)` | Zmienia aktualny identyfikator i ksztalt kursora (np. Normal -> Attack). | Cofa/aktualizuje uchwyt do `m_hCurrentCursor`. Emituje `ChangeCursor` w Pythonie jesli uchwyt jest podpiety. |
| `void SetCursorMode(int iMode)` | Zmienia tryb miedzy H/W (sprzetowy) a S/W (softwareowy - Pythonowy wycinek GUI). | Dla S/W ukrywa standardowy kursor Windowsowy pozwalajac na rendering w DX9. |

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
W `CPythonApplication`:
- `int m_iCursorNum;` - Indeks aktualnego kursora.
- `int m_iContinuousCursorNum;` - Zapasowy identyfikator domyslnego ksztaltu.
- `std::map<int, HANDLE> m_CursorHandleMap;` - Dynamiczna mapa pamietajaca odniesienia do instancji `HCURSOR`.
- `HANDLE m_hCurrentCursor;` - Cached uchwyt aktywnego kursora podawany z powrotem do Windows w `WM_SETCURSOR`.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Brak bezposredniego powiazania ze strumieniem sieciowym, kursor decyduje sie w oparciu o stan lokalny obiektu/ui, lub ewentualnie posrednio, gdy pakiet zmiany strefy ukrywa UI.
- **Metody Pythona (`PyMethodDef` w `app` / `PythonApplicationModule.cpp`):**
  - `app.GetCursorPosition()` -> Zwraca pozycje logiczna w oknie DX.
  - `app.SetCursor(int iCursorNum)` -> Mapuje na `SetCursorNum`.
  - `app.GetCursor()` -> Zwraca `GetCursorNum()`.
  - `app.ShowCursor()` / `app.HideCursor()` / `app.IsShowCursor()` -> Sterowanie widocznoscia.
  - `app.IsLiarCursorOn()` -> Zwraca flage ukrytego "oszukanego" kursora uzywanego np. do dragowania przedmiotow w tle.
  - `app.SetSoftwareCursor()` / `app.SetHardwareCursor()` -> Kontroluje model wyswietlania.
- Eksportowane sa stale `app.NORMAL`, `app.ATTACK`, `app.TARGET`, `app.TALK`, `app.CANT_GO`, `app.PICK`, `app.DOOR`, `app.CHAIR`, `app.MAGIC`, `app.BUY`, `app.SELL`, `app.CAMERA_ROTATE`, `app.HSIZE`, `app.VSIZE`, `app.HVSIZE`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Wszystkie dzialania na kursorze przez Win32 API (`SetCursor`, `ShowCursor`) MUSZA odbywac sie w watku glownym (UI), w ktorym znajduje sie petla obslugi wiadomosci okna (Window Procedure). Wywolywanie ich z innych watkow np. Network lub Loaders zakonczy sie brakiem reakcji lub wyscigami systemowymi.
- **Zarzadzanie zasobami (RAII) & Ograniczenia `ShowCursor`:** W systemie Windows funkcja `ShowCursor(BOOL)` dziala inkrementalnie jako stos (counter). W kliencie obslugiwane jest to poprawnie w `SetCursorVisible`, ktore uzywa petli `do { } while()` az do unormowania licznika, aby ukryc lub pokazac kursor niezaleznie od aktualnego glebokiego stanu licznika ukryc. Jesli zostanie to zmienione na pojedyncze zgloszenie, naruszy to integralnosc.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** Gdy `LoadImage` w `CreateCursors()` zwroci NULL (np. brak poprawnego powiazania zasobu w .rc lub zla wtyczka klienta), system przerwie ladowanie interfejsu (zwraca false, app sie zatrzymuje). Upewnij sie, ze kursorom podpiete sa ID w `resource.h`.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** 
  1. Zdefiniuj nowy stan (np. `CURSOR_SHAPE_FISH`) w `CPythonApplication::ECursorShape`.
  2. Eksportuj stala w `PythonApplicationModule.cpp` uzywajac `PyModule_AddIntConstant()`.
  3. Dodaj referencje do zasobu ID w tablicy `ResourceID` w funkcji `CPythonApplication::CreateCursors()`.
  4. Jezeli ten kursor ma byc powracajacym (non-volatile), pamietaj dodac go do `__IsContinuousChangeTypeCursor`.
- **Jak debugowac i logowac:** W przypadku niepoprawnego ladowania zasobu dodaj `TraceError` lub `EterBase::ModernLogger::Error` zaraz po ewaluacji `NULL == hCursor` w `CreateCursors`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Modul jest silnie zwiazany z Windows API. Podczas mockowania na srodowiskach Linux (headless) uzywaj pustych makr/stubow pod `HANDLE`, `SetCursor` czy `ShowCursor`. W `PythonApplicationCursor.cpp` doloz dyrektywy `#ifndef _WIN32` jesli bedzie zaleznosc wieloplatformowa, a logike stanu `m_iCursorNum` testuj przez `EXPECT_EQ`.
