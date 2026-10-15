---
task_id: "atlas_c01_05_phase_loading"
cluster: "NET"
module_name: "Faza Wczytywania Swiata (Loading Phase)"
target_files:
- src/UserInterface/PythonNetworkStreamPhaseLoading.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c01_05_phase_loading.md"
architecture_layer: "Silnik Sieciowy, Maszyna Faz i Protokol"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul `PythonNetworkStreamPhaseLoading.cpp` zarzadza faza ladowania (Loading Phase) po wyborze postaci na ekranie Select (lub po zleceniu warpu w grze).
Podczas tej fazy klient:
- Zmienia aktywna faze na "Loading" (metoda `SetLoadingPhase()`).
- Inicjalizuje struktury menedzera aktorow sieciowych.
- Czeka na pakiety potwierdzajace wejscie gracza do gry (np. `TPacketGCMainCharacter`, `TPacketGCPoints`).
- Dokonuje preladowania muzyki i wysyla parametry ekranu ladowania (np. koordynaty).
- W momencie wywolania `SetLoadingPhase()` czysci wczesniejsze instancje (np. CPythonPlayer::Clear, CFlyingManager, CEffectManager).
Flow (przeplyw):
Wywolywane jest przejscie do `SetLoadingPhase`. Klasa sieci wchodzi w stan, w ktorym jej funkcja processingu ustawiona jest na `LoadingPhase`. Metoda ta jest petla (pusta z cialem `while (DispatchPacket(m_loadingHandlers));`), ktora przetwarza wylacznie pakiety sieciowe specyficzne dla ladowania.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** 
  - Maszyna stanow sieci (zmiany faz `PHASE_LOADING` itp. w `OnProcess` / `SetLoadingPhase`).
  - System faz (PythonNetworkStream).
  - Pakiety od serwera (GC): `TPacketGCMainCharacter`, `TPacketGCPoints`.
- **Zaleznosci wyjsciowe (Outbound):** 
  - `CPythonPlayer` (czyszczenie stanu, ustawianie statusow).
  - `CFlyingManager`, `CEffectManager` (dealokacja instancji).
  - `NetworkActorManager` (`SetMainActorVID`).
  - PyCallClassMemberFunc (do okien UI w Pythonie: `PHASE_WINDOW_LOAD`, `PHASE_WINDOW_GAME`).
  - `CPackManager`, `CMemoryTextFileLoader` (wczytywanie plikow konfiguracji np. filtru wulgaryzmow).
- **Drzewo dyrektyw `#include`:**
  - `"StdAfx.h"`, `"PythonNetworkStream.h"`, `"Packet.h"`, `"PythonApplication.h"`, `"NetworkActorManager.h"`, `"AbstractPlayer.h"`, `"PackLib/PackManager.h"`
- **Model pamieciowy:** Brak skomplikowanego zarzadzania dynamicznego w tym pliku. Opiera sie o Singletony (`Instance()`) np. `CPythonPlayer::Instance()`. Polega na obiektach klas. `m_rokNetActorMgr` to prawdopodobnie wskaznik.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

- **Tabela Klas i Struktur:**
  - `CPythonNetworkStream`: (klasa glowna, w tym pliku zaimplementowano jej metody fazy Loading) Rola: Zarzadza polaczeniem i pakietami. Wlasciciel watku glowny, z asynchronicznym I/O lub blokujacym I/O zaleznie od reszty architektury.
  - Pakiety (np. `TPacketGCMainCharacter`, `TPacketGCPoints`): struktury C mapujace bajty z sieci.

- **Tabela Metod Publicznych:**
  - `void CPythonNetworkStream::EnableChatInsultFilter(bool isEnable)`: Konfiguracja filtrow.
  - `bool CPythonNetworkStream::IsChatInsultIn(const char* c_szMsg)`: Zwraca true jesli jest wulgaryzm (oraz czy wylaczony filtr).
  - `bool CPythonNetworkStream::IsInsultIn(const char* c_szMsg)`: Zwraca true wylacznie z checkera.
  - `bool CPythonNetworkStream::LoadInsultList(const char* c_szInsultListFileName)`: Wczytuje tablice wulgaryzmow z pliku Pack. Zwraca boolean. Efekt: zmiana w kInsultChecker.
  - `bool CPythonNetworkStream::LoadConvertTable(DWORD dwEmpireID, const char* c_szFileName)`: Laduje tabele konwersji teksu. Warunki: `dwEmpireID >= 1 && dwEmpireID < 4`.
  - `void CPythonNetworkStream::LoadingPhase()`: Funkcja stanu ladowania, wola `DispatchPacket(m_loadingHandlers)`.
  - `void CPythonNetworkStream::SetLoadingPhase()`: Inicjuje wejscie do fazy ladowania. Czysci dane (`CPythonPlayer::Clear`, `CEffectManager`, itd.).
  - `bool CPythonNetworkStream::RecvMainCharacter()`: Odbiera pakiet postaci, ustawia UID glownego gracza, rase, imperium, odpala wczytywanie BGM, powiadamia UI Pythona ("LoadData"). Zwraca true przy sukcesie.
  - `bool CPythonNetworkStream::__RecvPlayerPoints()`: Odbiera statystyki postaci (HP, level, itp.), przekazuje do CPythonPlayer, odswieza UI Pythona ("RefreshStatus").
  - `void CPythonNetworkStream::StartGame()`: Ustawia `m_isStartGame = TRUE`.
  - `bool CPythonNetworkStream::SendEnterGame()`: Wysyla pakiet `CG::ENTERGAME` i flushuje bufor pakietow do serwera.
  - Wspierajace (np. muzyka): `__SetFieldMusicFileName`, `__SetFieldMusicFileInfo`, `GetFieldMusicFileName`, `GetFieldMusicVolume`.

- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  - Pola w pakietach: `TPacketGCMainCharacter` posiada `dwVID` (Unique ID), `wRaceNum` (rasa), `byEmpire` (imperium), `szName` (string name), `szBGMName` (nazwa mapy/bgm), `lX`, `lY`.
  - Obiekt z klasa uzywa `m_strPhase`, `m_dwMainActorVID` (kluczowe ID gracza wzgledem VID wokol), `m_loadingHandlers` mapowanie uchwytow.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:**
  - Odbierane w fazie ladowania (Game->Client):
    - Pakiet `TPacketGCMainCharacter`
    - Pakiet `TPacketGCPoints`
  - Wysylane w fazie ladowania (Client->Game):
    - Pakiet `TPacketCGEnterFrontGame` z opcodem `CG::ENTERGAME` - wyjscie z fazy loading i powiadomienie serwera ze mozna slac spawny.
- **Metody Pythona (`PyMethodDef`):**
  - Wywolywane zdarzenia w Python UI: `PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOAD], "LoadData", Py_BuildValue("(ii)", pack.lX, pack.lY))`
  - `PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshStatus", Py_BuildValue("()"))`
  - Ten kod powiadamia okna Pythona z C++. Funkcje wywolywane w srodowisku skryptowym to `LoadData` (z X i Y) i `RefreshStatus`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Wszystko w tym kodzie ma dzialac w jednym, glownym watku, poniewaz modyfikuje bufory sieci i stan grafiki bez lockow.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Przepelnienia przy czytaniu z pliku. Funkcja `LoadConvertTable` weryfikuje wielkosc pliku: `if (file.size()<dwFileSize) return false;`. Jest to prawidlowe.
  - Wywolywanie zlych indeksow dla imperium: `LoadConvertTable` ma guard `if (dwEmpireID<1 || dwEmpireID>=4) return false;` a potem odnosi sie po `m_aTextConvTable[dwEmpireID-1]`.
  - Przepelnienia na tablicy argumentow wulgarnej, choc `strlen` uzywany w `IsInsultIn` zaklada, ze string z C++ lub C ma znak konca stringu `\0`.
  - W `RecvMainCharacter`, brak prawidlowej inicjalizacji po failu `Recv(sizeof(pack), &pack)` nie powoduje korupcji, ale konczy polaczenie `return false;`.
- **Zarzadzanie zasobami (RAII):**
  - Usuwanie poprzednich zasobow (alokacji) w `SetLoadingPhase()` to krytyczny inwariant (kasowanie `CFlyingManager`, `CEffectManager`), zabezpieczajace przed leakami.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zdefiniuj nowy opcode i strukture w `Packet.h`.
  2. Zarejestruj callback dla ladowania w konstruktorze lub setupie sieci.
  3. Dodaj metode np. `bool RecvNewInfo()` do tego pliku `.cpp`. Wewnatrz zawsze uzywaj `Recv(...)` aby potwierdzic otrzymanie pamieci z strumienia. W razie wpadki na strukturze zwracaj `false`.
  4. Dodaj deklaracje w headerze `PythonNetworkStream.h`.
- **Jak debugowac i logowac:**
  - Wykorzystaj globalne makra / funkcje `Tracef` i `Tracen` (juz widoczne jako loggery). Podgladaj zwlaszcza plik `syserr.txt` lub log systemowy. Obserwuj wartosc `m_strPhase`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  - Stworz mock serwera, ktory wpycha bytestream symulujacy `GC::MAIN_CHARACTER` w bufor sieci. Zrob `CPythonNetworkStream` i wywoluj `LoadingPhase()` iterujac dispatchera. Upewnij sie, ze nie dzwonisz wtedy funkcji ui (zmockuj `PyCallClassMemberFunc`).
