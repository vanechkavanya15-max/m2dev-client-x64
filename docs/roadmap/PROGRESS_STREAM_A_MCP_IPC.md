# WORKBOOK POSTEPU: STRUMIEN A - MOSTEK MCP & SERWER IPC (MCP BRIDGE & IPC SERVER)
## Odpowiedzialny: Agent Lead A (MCP & Autonomous AI Protocols Specialist)

**Cel Strumienia:** Budowa ultra-lekkiego, asynchronicznego serwera IPC w C++23 wewnatrz procesu klienta oraz dostarczenie nowoczesnego serwera MCP w Pythonie (`m2_client_mcp_server.py`). Umozliwienie agentom AI (Antigravity, Jules) pelnej inspekcji stanu oraz sterowania postacia bez uzywania symulacji myszy i zrzutow ekranu.  
**Standard:** Windows Named Pipes (`\\.\pipe\M2ClientMcpBridge`) z protokolem JSON-RPC 2.0 i opcjonalnym 16-bajtowym naglowkiem binarnym dla akcji wysokiej czestotliwosci.

---

## 1. DOKLADNA MAPA KOMPONENTOW DO ZBUDOWANIA

1. **`src/Client/Bridge/Mcp/McpPipeServer.h/.cpp`:**
   - Watek roboczy ze standardowym Win32 Overlapped I/O lub niemultipleksowanym asynchronicznym Named Pipe.
   - Odczytuje ramki z potoku, parsuje JSON-RPC i wysyla do routera.
2. **`src/Client/Bridge/Mcp/McpCommandRouter.h/.cpp`:**
   - Mapuje komendy JSON-RPC:
     - Zapytania o stan -> bezposredni, natychmiastowy odczyt z `Client::Core::WorldContext` (czas reakcji < 0.1 ms).
     - Zlecenia akcji -> delegowanie do `Client::Core::GameSession::Execute(...)`.
     - Subskrypcje zdarzen -> nasluchiwanie na `Core::EventBus`.
3. **`C:\JULES\m2_client_mcp_server.py`:**
   - Narzedzia Fast MCP: `m2_inspect_state`, `m2_inspect_inventory`, `m2_inspect_radar`, `m2_action_move`, `m2_action_attack`, `m2_harness_assert`.

---

## 2. REJESTR ZADAN ATOMOWYCH (SWARM TASK LIST)

| ID | Status | Nazwa Zadania / Obszar | Plik Zrodlowy | Plik Testu Jednostkowego | Przypisany Agent |
|---|---|---|---|---|---|
| **A-01** | [x] | Architektura protokolu ramki binarnej i opcodow IPC | `C:\JULES\plan_obszar_A_mcp_ipc.md` | Dok. specyfikacji | Agent Lead A |
| **A-02** | [ ] | Implementacja `McpPipeServer` (asynchroniczny serwer potokow nazwanych) | `Client/Bridge/Mcp/McpPipeServer.h/.cpp` | `test_c26_mcp_pipe_server.cpp` | Jules Worker #41 |
| **A-03** | [ ] | Implementacja `McpCommandRouter` dla telemetrii (`state`, `inventory`, `radar`) | `Client/Bridge/Mcp/McpCommandRouter.h/.cpp` | `test_c26_mcp_telemetry.cpp` | Jules Worker #42 |
| **A-04** | [ ] | Implementacja akcji sterujacych (`move`, `attack`, `use_item`, `cast_skill`) | `Client/Bridge/Mcp/McpActionDispatcher.h/.cpp` | `test_c26_mcp_actions.cpp` | Jules Worker #43 |
| **A-05** | [ ] | Aktualizacja serwera MCP `C:\JULES\m2_client_mcp_server.py` pod Named Pipe | `C:\JULES\m2_client_mcp_server.py` | Test e2e narzedzi MCP | Jules Worker #44 |
| **A-06** | [ ] | Weryfikacja opoznien: czas odpowiedzi na inspekcje stanu < 1 milisekunda | Skrypt testowy `test_mcp_latency.py` | Benchmark latencji | Jules Worker #45 |

---

## 3. KRYTERIA AKCEPTACJI DLA AGENTA A (DEFINITION OF DONE)
1. Agent wywolujacy narzedzie `m2_inspect_state` otrzymuje kompletny JSON ze statystykami gracza w czasie ponizej 1 ms.
2. Akcja `m2_action_move(x, y)` natychmiast powoduje wyliczenie pozycji i wyslanie pakietu bez dotykania kursora myszy systemu operacyjnego.
3. Serwer potoku zamyka sie bez zawieszania procesu i bez wyciekow deskryptorow przy zamknieciu klienta.
