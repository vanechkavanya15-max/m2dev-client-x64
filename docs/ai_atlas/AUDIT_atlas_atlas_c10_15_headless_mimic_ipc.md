---
task_id: "atlas_c10_15_headless_mimic_ipc"
cluster: "SYS"
module_name: "Mostki IPC i Tryb Klienta Beztrybowego (Headless Mimic)"
target_files:
- src/Client/IPC/IPCBridge.h
- src/Client/Mimic/ClientMimic.h
- src/Client/Simulation/GameSimulator.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c10_15_headless_mimic_ipc.md"
architecture_layer: "VFS, Szyfrowanie, Audio, Proto i Platforma"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul "Mostki IPC i Tryb Klienta Beztrybowego (Headless Mimic)" odpowiada za umozliwienie zewnetrznego sterowania klientem Metin2, co jest kluczowe dla automatyzacji, botow AI (np. Arthion Bot) oraz symulacji i testow jednostkowych w srodowisku bez interfejsu graficznego (headless).
Wymienione docelowe pliki `IPCBridge.h`, `ClientMimic.h` oraz `GameSimulator.h` nie istnieja w aktualnej galazce repozytorium. Zamiast nich system jest realizowany przez zbiory klas rozproszonych w katalogach `Client/IPC`, `Client/Mimic` oraz `Client/Simulation`, takich jak `IPCCommandDispatcher`, `IPCTransportPipe`, `SessionSimulationHarness`, `PingPongEngine` czy `ActionRateLimiter`.
Modul ten wykorzystuje potoki nazwane (Named Pipes w systemie Windows) do dwukierunkowej komunikacji Miedzyprocesowej (IPC), gdzie wysyla JSON z danymi gracza i otoczenia (przez `IPCQueryHandler`) oraz odbiera komendy w specjalnym formacie `M2IP` (`IPCProtocolCodec`).
W trybie beztrybowym (Simulation), petla glowna jest napedzana przez `SessionSimulationHarness::AdvanceTime`, omijajac tradycyjne wywolania `OnUpdate` / `OnRender`. Obiekty srodowiskowe (monstra, przedmioty) moga byc symulowane i wstrzykiwane przez `GameSession`. Modul zarzadza ruchem za pomoca `SplineMotionInterpolator`, utrzymuje sesje `PingPongEngine` oraz limituje ilosc akcji przy pomocy `ActionRateLimiter`. Cykl zycia obiektow opiera sie czesto na `std::unique_ptr` oraz bezposrednim sterowaniu z poziomu kodu sterujacego, z pominieciem cyklu zycia UI.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):**
  - Narzedzia zewnetrzne, skrypty testujace i boty komunikujace sie przez IPC potoki nazwane (np. `\\.\pipe\m2_client_ipc_PID`).
  - Scenariusze symulacyjne (np. `CombatSimulationScenario`, `StressAndLootSimulationScenario`) sterujace silnikiem beztrybowym bezposrednio z C++.
- **Zaleznosci wyjsciowe (Outbound):**
  - EterLib / EterBase: `EterBase::RingBuffer`, Loggery, Obliczanie CRC32.
  - EventBus: Wysylanie komend sterujacych z `IPCCommandDispatcher`.
  - GameSession / WorldContext: Modyfikacja i pobieranie informacji o jednostkach w swiecie gry i ekwipunku (`InventoryDomain`, `CombatDomain`).
  - Winsock / Windows API: Win32 Named Pipes, `OVERLAPPED` I/O.
- **Drzewo dyrektyw `#include`:**
  - Wymaga systemowych zaleznosci Windows do dzialania IPC (`<windows.h>`).
  - Standardowe biblioteki C++20 (`<span>`, `<expected>`, `<variant>`, `<format>`, `<chrono>`).
  - Uzycie mockowanego `<windows.h>` jest rekomendowane na Linuksie dla zaleznosci.
- **Model pamieciowy:**
  - Masywne korzystanie ze standardowych kontenerow (`std::vector`, `std::array`).
  - Przekazywanie parametrow i danych z IPC wykorzystuje bezpieczne `std::span`.
  - Inteligentne wskazniki (`std::unique_ptr`) zarzadzaja pamiecia I/O (np. `OVERLAPPED`).
  - `SessionSimulationHarness` wykorzystuje `std::shared_ptr` dla interfejsow sieciowych (np. `MockNetworkPortAdvanced`).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur:**
  - `Client::IPC::IPCTransportPipe`: Serwer Named Pipe, wielkosc zalezna od buforow (64KB), wlasciciel watku I/O.
  - `Client::IPC::IPCCommandDispatcher`: Dispatcher przeksztalcajacy DecodedCommand na zdarzenia dla GameSession.
  - `Client::IPC::IPCQueryHandler`: Generuje odpowiedzi JSON z kontekstu swiata (`WorldContext`).
  - `Client::IPC::IPCProtocolCodec`: Koder/Dekoder pakietow `M2IP` z suma kontrolna CRC32.
  - `Client::Simulation::SessionSimulationHarness`: Scaffolding dla srodowiska testowego headless. Posiada wlasny `GameSession` i wstrzykuje mocki sieciowe.
  - `Client::Simulation::CombatSimulationScenario`: Wykonuje deterministyczne scenariusze walki testujac stabilnosc i limity czasowe.
  - `Client::Mimic::PingPongEngine`: Odpowiada na heartbeat serwera (opcody CG_PONG).
  - `Client::Mimic::ActionRateLimiter`: Utrzymuje dozwolona liczbe akcji (Token Bucket) i zapobiega blokadom anty-spam.
- **Tabela Metod Publicznych:**
  - `IPCTransportPipe::PollEvents()`: Brak zwracanej wartosci. Asynchronicznie odczytuje przychodzace dane z pipe i wywoluje powiazane callbacki.
  - `IPCProtocolCodec::DecodeFrame(std::span<const uint8_t> buffer)`: Zwraca `std::expected<DecodedIpcFrame, IpcCodecError>`. Wymaga poprawnego naglowka 'M2IP' oraz sumy kontrolnej.
  - `IPCCommandDispatcher::Dispatch(const DecodedCommand& cmd)`: Zwraca `std::expected<void, IpcDispatchError>`. Emituje event do wewnetrznego EventBusa w zaleznosci od OpCode (Goto, AttackTarget itp.).
  - `SessionSimulationHarness::AdvanceTime(float deltaSeconds)`: Brak zwracanej wartosci. Przesuwa czas symulacji do przodu.
- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  - `IpcFrameHeader`: `pragma pack(1)`. Struktura bez paddingu. Zawiera w kolejnosci: `uint32_t magic`, `uint16_t version`, `uint16_t opcode`, `uint32_t sequenceId`, `uint32_t payloadLength`, `uint32_t payloadCrc32`. Lacznie 18 bajtow. Idealne na szybkie wstrzykiwania binarne z FFI / Rusta.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:**
  - Protokol IPC M2IP (wewnetrzny): Opcody uzywane przez mostek to m.in. `Ping` (0), `AuthRequest` (1), `AuthResponse` (2). Zewnetrzne boty pakuja tam struktury komend.
  - Serwer Gry CG/GC (Game): W systemie symulacyjnym i Mimic generowane sa opcody serwerowe np. `CG_PONG` (zmienne w zaleznosci od `ServerProfile` - 1 lub wiecej bajtow dla naglowkow, wspiera tryb `M2Dev4B`).
- **Metody Pythona (`PyMethodDef`):**
  - Powyzszy zestaw klas funkcjonuje w warstwie C++ ponizej Pythona. IPC Bridge i komponenty Mimic zostaly zaprojektowane glownie pod zewnetrzny dostep bota / system headless, wiec dedykowane C-API do przestrzeni UI Pythona nie odgrywa tu bezposredniej roli.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Funkcje `IPCTransportPipe` wspieraja model `OVERLAPPED` Windowsa, pozwalajac na asynchroniczne I/O. Nalezy jednak ostroznie odpytywac logike domeny (`GameSession`) - operacje tam sa jednowatkowe, dlatego `PollEvents` lub callbacki powinny byc wrzucane w glowna petle zdarzen, a nie wykonywane w odrebnych watkach.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Ogromne rozmiary Payloadu: Zabezpieczone w `ProcessReadData` limitem 64MB dla rozmiaru klatki, chroniac przed DoS.
  - Zle wyliczony CRC32: Wykorzystanie `GetCRC32` zwracajace `0xffffffff` dla pustych payloadow - niezgodnosc sumy odrzuca pakiety jako uszkodzone.
  - Brak profilu serwera w `PingPongEngine`: Spowoduje zwrocenie bledu, pusta odpowiedz na Ping.
- **Zarzadzanie zasobami (RAII):** Kod mocno opiera sie na nowym standardzie C++, `std::unique_ptr` w klasach (dla `OVERLAPPED` struktur Windowsa czy klas w `StressAndLootSimulationScenario`) skutecznie zapobiega wyciekom bez recznego wywolywania destrukcji.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Dodaj nowy element do enum class `IpcOpcode` w pliku naglowkowym dyspozytora.
  2. Utworz `struct IpcNazwaPayload` z polami komendy.
  3. Dodaj typ do `std::variant` w strukturze `DecodedCommand`.
  4. Dopisz nowa prywatna metode do `IPCCommandDispatcher` oraz logike switch wewnatrz implementacji `Dispatch()`.
- **Jak debugowac i logowac:** Do przesledzenia ruchu wystarczy sledzic metody asynchroniczne pipe w `IPCTransportPipe::PollEvents` logowane narzedziem `EterBase::ModernLogger` - nalezy sprawdzic pliki wyjsciowe logow dla zdarzen m.in. braku ciaglosci Pipe.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Wymagane jest uzycie narzedzia `SessionSimulationHarness`. Aby przetestowac zachowania logiki (np. walke), nalezy dodac metode do `CombatSimulationScenario` symulujaca `GameSession::Tick()` wywolujac bezposrednio `AdvanceTime(0.3f)`. Pozwala to sprawdzic skutki uboczne bota calkowicie w pamieci, przy minimalnym narzucie. Uzywac kompilacji narzedzi na wzor Linuksa, mockujac braki dyrektyw specyficznych dla Windows.
