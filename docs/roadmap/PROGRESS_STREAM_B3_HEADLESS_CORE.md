# WORKBOOK POSTEPU: STRUMIEN B3 - HEADLESS CORE & PETLA GLOWNA (HEADLESS AUTONOMY)
## Odpowiedzialny: Agent Lead B3 (Runtime & Engine Lifecycle Specialist)

**Cel Strumienia:** Calkowite uniezaleznienie glownej petli klienta od pulpitu Windows, okna Win32 (`HWND`) i karty graficznej. Wdrozenie natywnego trybu `--headless`, umozliwiajacego uruchamianie klienta w chmurze Google, kontenerach Linux (przez Wine/Proton) oraz wewnetrznych procesach testowych bez renderowania ekranu.  
**Zasada:** W trybie graficznym renderer D3D9 dziala normalnie (nienaruszony!). W trybie `--headless` logika taktowana jest wirtualnym zegarem `FrameTimer` w `HeadlessGameLoopController`.

---

## 1. DOKLADNA MAPA ZALEZNOSCI WIN32 DO USUNIECIA / ZMOCKOWANIA

1. **`UserInterface/UserInterface.cpp`:**
   - Obecnie: `WinMain` sztywno tworzy okno i od razu probuje inicjalizowac Direct3D9.
   - Docelowo: Jesli podano flage `--headless`, proces pomija `CreateDevice()`, nie tworzy widzialnego okna i odpala `HeadlessGameLoopController::Run()`.
2. **`UserInterface/PythonApplication.cpp`:**
   - Obecnie: `Process()` czyta fizyczna mysz: `GetCursorPos(&Point); ScreenToClient(m_hWnd, &Point);`.
   - Docelowo: Wirtualna kolejka wejscia `VirtualInputQueue` przyjmujaca syntetyczne komendy ruchu i klikniec z szyny domenowej.
3. **`UserInterface/Core/HeadlessGameLoopController.h`:**
   - Gotowa klasa z autorskim cyklem aktualizacji: `Tick(deltaTime) -> NetworkStream::Process() -> GameSession::Tick()`.

---

## 2. REJESTR ZADAN ATOMOWYCH (SWARM TASK LIST)

| ID | Status | Nazwa Zadania / Obszar | Plik Zrodlowy | Plik Testu Jednostkowego | Przypisany Agent |
|---|---|---|---|---|---|
| **B3-01** | [x] | Szkielet sterownika petli bezokienkowej `HeadlessGameLoopController` | `UserInterface/Core/HeadlessGameLoopController.h` | `test_c26_frame_timer.cpp` | Jules Worker #33 |
| **B3-02** | [ ] | Parser argumentow wiersza polecen (`--headless`, `--server`, `--mcp-pipe`) | `UserInterface/UserInterface.cpp` | `test_c26_server_profile.cpp` | Jules Worker #34 |
| **B3-03** | [ ] | Rozgalezienie w `WinMain`: start petli standardowej LUB headless | `UserInterface/UserInterface.cpp` | Test startu binarki z `--headless` | Jules Worker #35 |
| **B3-04** | [ ] | Wdrozenie wirtualnego wejscia `VirtualInputQueue` zastepujacego `GetCursorPos` | `Client/Platform/VirtualInputQueue.h/.cpp` | `test_c26_movement_command_handler.cpp` | Jules Worker #36 |
| **B3-05** | [ ] | Ciche ladowanie VFS dla Headless (tylko protos, bez wgrywania tekstur na GPU) | `Client/Platform/VFSManager.cpp` | `test_c26_vfs_pack_manager.cpp` | Jules Worker #37 |
| **B3-06** | [ ] | Bezpieczne zamkniecie procesu (Clean Shutdown) na komende z zewnatrz | `UserInterface/Core/HeadlessGameLoopController.h` | Test automatycznego zakonczenia sesji | Jules Worker #38 |
| **B3-07** | [ ] | Weryfikacja zuzycia pamieci i CPU w trybie headless (<100MB RAM, <2% CPU) | Profiler / Skrypt testowy | Test benchmarkowy CI | Jules Worker #39 |
| **B3-08** | [ ] | Pelna symulacja wejscia i wyjscia z gry w 50 ms bez monitora | `tests/test_c26_headless_smoke.cpp` | `test_c26_headless_smoke.cpp` | Jules Worker #40 |

---

## 3. KRYTERIA AKCEPTACJI DLA AGENTA B3 (DEFINITION OF DONE)
1. Polecenie `Metin2.exe --headless` uruchamia sie bez tworzenia okna Windows i bez crasha na maszynie bez dedykowanej karty graficznej.
2. Logika gry, pakiety sieciowe i komendy sterujace dzialaja ze 100% poprawnoscia w petli headless.
3. Standardowy tryb z oknem i grafika D3D9 dziala bez zmian (zero regresji wizualnej).
