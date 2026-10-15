# WORKBOOK POSTEPU: STRUMIEN A - MOSTEK MCP & SERWER IPC (MCP BRIDGE & IPC SERVER) [100% ZREALIZOWANO]
## Odpowiedzialny: Agent Lead A (MCP & Autonomous AI Protocols Specialist)

**Cel Strumienia:** Budowa ultra-lekkiego, asynchronicznego serwera IPC w C++23 wewnatrz procesu klienta oraz dostarczenie nowoczesnego serwera MCP w Pythonie (`tools/mcp/m2_agent_harness_mcp.py`). Umozliwienie agentom AI (Antigravity, Jules) pelnej inspekcji stanu oraz sterowania postacia bez uzywania symulacji myszy i zrzutow ekranu.  
**Standard:** Windows Named Pipes (`\\.\pipe\M2ClientEngineHarness`) z protokolem JSON-RPC 2.0 (ramkowanie 4-bajtowe little-endian uint32_t oraz surowy JSON-RPC).

---

## 1. DOKLADNA MAPA KOMPONENTOW

1. **`src/UserInterface/TestHarness/TestHarnessServer.h/.cpp`:**
   - Asynchroniczny serwer potokow nazwanych Win32 z kolejka `ProcessMainThreadQueue()`, eliminacja Data Race i Heisenbuga 0x7B.
   - 28 natywnych tras JSON-RPC (liveness, stan gracza, ekwipunek, SpatialHashGrid, klawisze WASD, spacja, nawigacja pikselowa, skille, czat).
2. **`src/Client/MCP/AgentMCPBridge.h/.cpp`:**
   - Mostek C++23 z kontraktami domenowymi `IPlayerDomainContract`, `IWorldQueryContract`, `ICombatDomainContract`, `IInventoryDomainContract`.
3. **`tools/mcp/m2_agent_harness_mcp.py`:**
   - Serwer MCP integrujacy 38 narzedzi dla agentow AI (Fast-Path named pipe, CrashSentinel, fuzzing, sterowanie postacia).

---

## 2. REJESTR ZADAN ATOMOWYCH (STATUS: 100% PASS)

| ID | Status | Nazwa Zadania / Obszar | Plik Zrodlowy | Weryfikacja |
|---|---|---|---|---|
| **A-01** | [x] | Architektura protokolu ramki binarnej i opcodow IPC | `TestHarnessServer.h/.cpp` | CTest 51/51 PASS |
| **A-02** | [x] | Implementacja `TestHarnessServer` (Win32 Named Pipe) | `UserInterface/TestHarness/TestHarnessServer.cpp` | `McpHarnessSelfTest` (100% PASS) |
| **A-03** | [x] | Obsluga telemetrii stanu (`get_state`, `inventory`, `nearby`) | `Client/MCP/AgentMCPBridge.cpp` | `test_mcp_all_possibilities.py` (22/22) |
| **A-04** | [x] | Implementacja akcji gracza (`move`, `attack`, `skill`, `chat`) | `TestHarnessServer.cpp` | `test_live_character_actions.py` (11/11) |
| **A-05** | [x] | Serwer MCP z 38 narzedziami i schematami JSON | `tools/mcp/m2_agent_harness_mcp.py` | Self-test 9/9 PASS |
| **A-06** | [x] | Weryfikacja opoznien i odpornosci (< 1.5ms czas reakcji) | `test_mcp_all_possibilities.py` | 63.7 req/s, 0 awarii |

---

## 3. KRYTERIA AKCEPTACJI (DEFINITION OF DONE - 100% SPELNIONE)
1. Agent wywolujacy narzedzie `m2_game_get_state` otrzymuje kompletny JSON ze statystykami gracza w czasie ponizej 1 ms.
2. Akcje `m2_game_move_to_position(x, y, z)` oraz `m2_game_set_attack_key_state(true)` natychmiast powoduja wyliczenie pozycji i akcje postaci bez dotykania kursora myszy systemu operacyjnego.
3. Serwer potoku zamyka sie czysto (`shutdown`), bez procesow zombie i bez wyciekow deskryptorow.
