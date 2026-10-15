---
task_id: "atlas_c01_06_phase_game_dispatch"
cluster: "NET"
module_name: "Glowny Dyspozytor Pakietow Fazy Gry (PhaseGame Core)"
target_files:
- src/UserInterface/PythonNetworkStreamPhaseGame.cpp
- src/Client/Network/ModernPacketDispatcher.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c01_06_phase_game_dispatch.md"
architecture_layer: "Silnik Sieciowy, Maszyna Faz i Protokol"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Ten modul stanowi glowny wezel komunikacyjny (dyspozytor) dla fazy gry (Game Phase) klienta Metin2. Odpowiada za odbieranie pakietow sieciowych z serwera, interpretowanie ich naglowkow (opcode) oraz kierowanie ich do wyspecjalizowanych funkcji obslugi. 
Glowna petla dyspozytora (`CPythonNetworkStream::GamePhase`) wywolywana jest w kazdej klatce klienta w ramach cyklu zycia sieci (OnUpdate/Network Tick).
Przeplyw danych:
1. Dane binarne sa pobierane z gniazda sieciowego (TCP) do bufora odbiorczego.
2. `DispatchPacket` wyluskuje opcode i wykorzystuje zdefiniowane tabele zaleznosci (np. `m_gameHandlers`) by przekazac strukture pakietu (np. `TPacketGCAffectAdd`, `TPacketGCLandList`) do odpowiedniej metody (np. `RecvAffectAddPacket`, `RecvLandPacket`).
3. Nowoczesny system sieciowy (`Client::Network::ModernPacketDispatcher`) wspiera to dzialanie za pomoca tablicy wirtualnych interfejsow `IPacketHandler`, zapewniajacych zunifikowany interfejs obslugi, walidacji dlugosci oraz bezpiecznej alokacji dla warstw nowszych.
4. Nastepnie dane aktualizuja globalny stan klienta (np. instancje postaci, minimape) i synchronizuja UI poprzez delegacje do Pythona w warstwie okienkowej `m_apoPhaseWnd[PHASE_WINDOW_GAME]`.

Cykl zycia:
- **Inicjalizacja**: `__InitializeGamePhase()` resetuje flagi interfejsu uzytkownika i stan instancji. `SetGamePhase()` ustawia odpowiednie fazy dla mostkow (np. `PhaseGameSyncBridge`).
- **Aktualizacja**: W petli `GamePhase()` analizowane jest maksymalnie `MAX_RECV_COUNT` pakietow na klatke. Nastepnie obslugiwane sa zadania asynchronicznego odswiezania GUI, z uzyciem tzw. Throttle'ingu (`s_nextRefreshTime`).
- **Zakonczenie**: `__LeaveGamePhase()` czysci stan zsynchronizowany, przygotowujac do zmiany fazy (np. wylogowania lub ekranu wczytywania).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound)**: 
  - Glowna petla `CPythonApplication` uruchamia aktualizacje sieci dla `CPythonNetworkStream`.
  - Pakiety przychodzace (Server -> Client).
  - Skrypty Pythona powiazane ze zmiana stanow UI (np. wywolywanie `__RefreshGuildWindowGradePage()`).

- **Zaleznosci wyjsciowe (Outbound)**: 
  - **Mostki sieciowe (Bridges/StranglerFacade)**: Przekierowywanie wykonania logiki gry m.in. z `CPythonNetworkStreamPhaseGame.cpp` do nowszych domen C++23 (np. `PhaseGameWorldBridge`, `PhaseGameTargetBridge`, `PhaseGameRefineBridge`, `Client::Bridge::StranglerFacade`).
  - **System Instancji / Postaci**: `CPythonCharacterManager`, `IAbstractCharacterManager`, `CInstanceBase` - manipulacja postaciami w swiecie 3D.
  - **Ecosystem UI (EterPythonLib)**: Rejestracja callbackow Pythona, okien gry (`m_apoPhaseWnd`).
  - **Zewnetrzne biblioteki**: (Opcjonalnie) `Discord RPC` jesli wlaczone (integracja w `Discord_Update()`).
  - Nowoczesny system `Client::Network::ModernPacketDispatcher`, wykorzystujacy `EterBase::PacketResult` (alias dla `std::expected`).

- **Drzewo dyrektyw `#include`**: 
  - Naglowki Pythona: `PythonGuild.h`, `PythonCharacterManager.h`, `PythonPlayer.h`, `PythonMiniMap.h`, itd.
  - Interfejsy narzedziowe: `ProcessCRC.h`, `AbstractApplication.h`, `InstanceBase.h`.
  - Mostki sieciowe: `PythonNetworkStreamPhaseGameGuild.h`, `PythonNetworkStreamPhaseGameParty.h`, itd.
  - Nowoczesne biblioteki C++23: `<span>`, `<array>`, `Result.h`.

- **Model pamieciowy**:
  - `ModernPacketDispatcher` wykorzystuje z gory zaalokowana tablice zwyklych wskaznikow `IPacketHandler*` (`m_handlers[256]`). Dispatcher nie zarzadza cyklem zycia wskaznikow (korzysta ze wstrzykiwania zaleznosci).
  - Tradycyjny kod uzywa glownie klas statycznych i singletonow (`Instance()`, `GetSingleton()`) oraz wskaznikow C dla instancji modeli (`CInstanceBase*`). Referencje sa czesto wykorzystywane przy rzutowaniu singletonow (`IAbstractCharacterManager& rkChrMgr = IAbstractCharacterManager::GetSingleton()`).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur:**
- `CPythonNetworkStream`: (Ogromna klasa, Singleton-like). Modul wezlowy sieci. Dziala w watku glownym.
- `Client::Network::IPacketHandler`: Interfejs dla nowych handlerow. Wymaga implementacji `Handle()`, `GetExpectedSize()` i `IsDynamicSize()`. Brak alokacji.
- `Client::Network::ModernPacketDispatcher`: Zarzadca nowej generacji `IPacketHandler` pod C++23. Okolo 256 * sizeof(void*) bajtow pamieci. Brak wielowatkowosci per dispatcher.
- `PERF_PacketInfo` / `PERF_PacketTimeAnalyzer`: Narzedzia diagnostyczne do sledzenia wydajnosci przetwarzania opcodow (dostepne jesli `__PERFORMANCE_CHECK__` jest aktywne).

**Tabela Metod Publicznych (`Client::Network::ModernPacketDispatcher` i `CPythonNetworkStream` z wycinka):**
- `RegisterHandler(uint8_t opcode, IPacketHandler* handler)`: void. Rejestruje logike w nowym interfejsie.
- `UnregisterHandler(uint8_t opcode)`: void. 
- `Dispatch(uint8_t opcode, std::span<const uint8_t> payload)`: Zwraca `EterBase::PacketResult<void>`.
- `GamePhase()`: void. Zdjecie max 32 pakietow z kolejki per klatka, a nastepnie wykonanie throttle'owanych odswiezen UI.
- `SetGamePhase()`: void. Przygotowuje srodowisko na wejscie w gre, w tym mostki.
- `__Refresh...()` (np. `__RefreshInventoryWindow`): Metody podpinajace flagi `m_isRefresh... = true` na potrzeby pozniejszego odswiezenia przez `GamePhase()`.
- Metody `Recv...()`: Zwracaja `bool`. Typowo czytaja konkretny rozmiar i parsowaly dane (np. `RecvLandPacket`, `RecvObserverAddPacket`).

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
- `ModernPacketDispatcher` ma prosta strukture: `std::array<IPacketHandler*, 256> m_handlers` od offsetu 0x0. Latwe wstrzykniecie poprzez nadpisanie pol tablicy przez FFI (np. Arthion Bot).

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

- **Pakiety Sieciowe (Network Packets):**
  - Odbierane w tym pliku (wycinek): `TPacketGCAffectAdd`, `TPacketGCChannel`, `TPacketGCViewEquip`, `TPacketGCLandList`, `TPacketGCTargetCreate`, `TPacketGCTargetCreateNew`, `TPacketGCLoverInfo`, `TPacketGCDigMotion`.
  - Wysylane stad (wycinek): `TPacketCGDragonSoulRefine`, `TPacketCGHack`, `TPacketCGMessenger`, `TPacketCGMove`.

- **Metody Pythona (`PyMethodDef` / `PyCallClassMemberFunc`):**
  Modul deleguje rysowanie bezposrednio do interpretera Python poprzez instancje zdefiniowana w `m_apoPhaseWnd[PHASE_WINDOW_GAME]`.
  Najczesciej wolane: 
  - `"BINARY_NEW_AddAffect"`: Czas trwania buffow.
  - `"OpenEquipmentDialog"`, `"SetEquipmentDialogItem"`: Okno podgladu EQ.
  - `"RefreshCharacter"`, `"RefreshInventory"`, itd.: Masowe odswiezanie calych stron w kliencie.
  - `"RefreshTargetBoard"`, `"BINARY_UpdateLovePoint"`.
  Wiele starych komend (np. C++ `RecvTargetCreatePacketNew`) wolaja bezposrednio `"BINARY_OpenAtlasWindow"`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Caly subsystem `PhaseGame` musi byc uruchamiany **TYLKO I WYLACZNIE** w glownym watku, z uwagi na powiazania z singletonami Direct3D i Python GUI (`PyCallClassMemberFunc`). Wywolywanie funkcji GUI w innych watkach wywola natychmiastowy crash klienta (brak GIL/wlasnych mutexow w starym kodzie).
- **Zarzadzanie Pamiecia i Zaleznosci (RAII):** Nalezy uwazac przy obsludze starych funkcji `Recv(...)`. Jezeli wyczyta sie zle dane, polaczenie ulegnie desynchronizacji i natychmiastowo klient ulegnie zawieszeniu z powodu nieprawidlowych struktur naglowkow w buforze odbiorczym.
- **Potencjalne punkty awarii (Crash Points):** `CInstanceBase* pkInstMain = rkChrMgr.GetMainActorPtr()`. Funkcje (np. powiazane z duel, ruchem lub `RecvDigMotionPacket`) zakladaja istnienie postaci gracza, gdy zwroci `NULL`, moze przerwac obsluge (dobra praktyka: zawsze sprawdzaj wczesnie if(!pkInstMain) return true/false;).
- **Limitowanie przepustowosci (Throttle):** `MAX_RECV_COUNT = 32`. Klienci przetwarzaja zaledwie 32 opcody na jedno wywolanie `GamePhase()` w ramach jednej klatki. Jezeli serwer wysyla spam, `GamePhase()` odetnie reszte, bufor urosnie, az zostanie przetworzony w kolejnej klatce. Limit bezpieczenstwa to bufor powyzej `8192` bajtow (`SAFE_RECV_BUFSIZE`), kiedy klient przyspieszy przetwarzanie.
- **Strangler Pattern (Modernizacja):** Wszystkie nowe pakiety/obslugi nalezy kierowac przez mostki, tak jak `PhaseGameSyncBridge` lub `Client::Bridge::StranglerFacade::Instance().ExecuteMove(...)`, zapobiegajac rozrostowi monolitu `PythonNetworkStreamPhaseGame.cpp`.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zidentyfikuj opcode pakietu. Dla C++23, preferuj implementacje interfejsu `Client::Network::IPacketHandler`.
  2. Zarejestruj ten handler w instancji `ModernPacketDispatcher` zamiast w klasycznej jump-table (`m_gameHandlers`).
  3. Cialo pakietu musi miec pragme `#pragma pack(push, 1)` w naglowku pakietu.
  4. Przy delegacji do logiki domenowej (np. Walka) uzyj Mostka `PhaseGame[Nazwa]Bridge` lub `StranglerFacade`. Wywolaj tam konkretna funkcje C++ uzywajaca `std::expected`. Nie wklejaj calej logiki do funkcji pakietu sieciowego.
  5. Jesli wymagasz interakcji z UI, uzyj funkcji z przedrostkiem `__Refresh...()` i ustaw odpowiednia flage, by `GamePhase()` wywolalo `PyCallClassMemberFunc` w bezpiecznym przedziale co 300 ms.

- **Jak debugowac i logowac:**
  Uzywaj makra `Tracef("opis %d\n", wartosc)` do zrzucania informacji do `syserr.txt` (dostepne lokalnie) w przypadku awarii logiki (np. gdy serwer przyslal zly packet length). Wlaczenie `__PERFORMANCE_CHECK__` zapisze opoznienia petli dispatchera w pliku `perf_dispatch_packet_result.txt`.

- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  Do testowania nowej logiki (C++23) za pomoca narzedzia doctest w srodowisku Linux:
  1. Stworz plik testowy testujacy zachowanie samej klasy `ModernPacketDispatcher`.
  2. Zamockuj implementacje `IPacketHandler` - wyzwol testowy pakiet za pomoca `std::span<const uint8_t>`.
  3. Kompilujac przez `g++ -std=c++23 tests/test.cpp tests/test_main.cpp -I tests/mock_includes`, upewnisz sie, ze C++ radzi sobie z wywolaniami bez pelnego zestawienia srodowiska Windows/DirectX. Trzymaj logike sieci oddzielnie od makr UI (ktore psuja doctesta).
