# MASTER SWARM ROADMAP 2026: METIN2 X64 CLIENT TRANSFORMATION
## Centralny Pulpit Orkiestracji i Dystrybucji Zadan dla Roju Agentow AI (Jules Swarm & Antigravity)

**Katalog roboczy:** `E:\m2dev-client-src-mainOryginalx64`  
**Srodowisko orkiestracji:** `C:\JULES\` (Baza SQLite: `jules_swarm.db`, Pula: 150 slotow na 10 kontach)  
**Standard docelowy:** C++23 (MSVC 2022 x64, CMake)  
**Zelazna Zasada:** **ZERO MODYFIKACJI RENDERERA** (DirectX 9, GrpDevice, shaders, Granny, SpeedTree, PRTerrain pozostaja w 100% nienaruszone).

---

## 1. STRUKTURA PODZIALU STRUMIENI (SWARM WORK STREAMS)

Projekt zostal podzielony na 6 niezaleznych strumieni domenowych. Kazdy strumien posiada dedykowanego **Agenta Prowadzacego (Lead Planner Agent)** oraz wlasny plik postepu (`PROGRESS_*.md`), w ktorym zarzadza atomowymi zadaniami dla robotnikow Jules Swarm:

```
C:\JULES\roadmap\
├── MASTER_SWARM_ROADMAP.md                   <- Ten plik (Centralny Dashboard Orkiestracji)
│
├── PROGRESS_STREAM_B1_NETWORK_MONOLITH.md    <- Agent 1: Rozbicie Monolitu Sieciowego (PhaseGame)
├── PROGRESS_STREAM_B2_STATE_UNIFICATION.md   <- Agent 2: Unifikacja Stanu & Single Source of Truth
├── PROGRESS_STREAM_B3_HEADLESS_CORE.md       <- Agent 3: Headless Game Loop & Odciecie Win32 Desktop
├── PROGRESS_STREAM_A_MCP_IPC.md              <- Agent 4: Mostek MCP & Serwer IPC Named Pipe
├── PROGRESS_STREAM_C_DATA_PIPELINE.md        <- Agent 5: Nowoczesny Data Pipeline ZSTD (No-DumpProto)
└── PROGRESS_STREAM_D_MOCK_TEST_HARNESS.md    <- Agent 6: Deterministyczny Mock Test Harness
```

---

## 2. GRAF ZALEZNOSCI DAG POMIEDZY STRUMIENIAMI

Aby uniknac budowania MCP na niestabilnym fundamencie, zlecenia do roju startuja w scislej sekwencji fal:

```mermaid
graph TD
    subgraph FALA 0: Fundamenty Testowe
        S_CMake[Enabler: CMake Tests Target ClientTests]
    end

    subgraph FALA 1: Likwidacja Monolitow i Czysty Stan (Rownolegle)
        B1[Strumien B1: Rozbicie Sieci PhaseGame na 20 handlerow SRP]
        B2[Strumien B2: Unifikacja Stanu WorldContext - Usuniecie Split-Brain]
        C[Strumien C: Nowoczesny Data Pipeline Proto ZSTD]
    end

    subgraph FALA 2: Autonomia Procesu i Symulacja
        B3[Strumien B3: Headless Game Loop Controller & Virtual Input]
        D[Strumien D: Deterministyczny Mock Test Harness]
    end

    subgraph FALA 3: Interfejs MCP i Zamkniecie E2E
        A[Strumien A: Mostek MCP IPC Named Pipe & Serwer Python]
        E2E[Weryfikacja E2E Swarm Test Suite]
    end

    S_CMake --> B1
    S_CMake --> B2
    S_CMake --> C

    B1 --> B3
    B2 --> B3
    B1 --> D
    B2 --> D

    B3 --> A
    D --> A
    A --> E2E
```

---

## 3. MACIERZ STATUSU I POSTEPU STRUMIENI

| Strumien | Nazwa / Obszar | Agent Prowadzacy | Plik Postepu | Liczba Zadan | Status |
|---|---|---|---|---|---|
| **B1** | Rozbicie Monolitu Sieciowego | Agent Lead B1 (Network) | [PROGRESS_STREAM_B1_NETWORK_MONOLITH.md](file:///C:/JULES/roadmap/PROGRESS_STREAM_B1_NETWORK_MONOLITH.md) | 20 zadan | 🟡 W trakcie (104 testy, czesc wstrzyknieta) |
| **B2** | Unifikacja Stanu Gry | Agent Lead B2 (State) | [PROGRESS_STREAM_B2_STATE_UNIFICATION.md](file:///C:/JULES/roadmap/PROGRESS_STREAM_B2_STATE_UNIFICATION.md) | 10 zadan | 🟡 W trakcie (`WorldContext` gotowy, stary stan do odciecia) |
| **B3** | Headless Core & Input | Agent Lead B3 (Headless) | [PROGRESS_STREAM_B3_HEADLESS_CORE.md](file:///C:/JULES/roadmap/PROGRESS_STREAM_B3_HEADLESS_CORE.md) | 8 zadan | ⚪ Zaplanowane (Szkielet `HeadlessController` czeka) |
| **A** | Mostek MCP & IPC | Agent Lead A (MCP) | [PROGRESS_STREAM_A_MCP_IPC.md](file:///C:/JULES/roadmap/PROGRESS_STREAM_A_MCP_IPC.md) | 6 zadan | ⚪ Zaplanowane (Zalezne od B1, B2, B3) |
| **C** | Data Pipeline Proto ZSTD | Agent Lead C (Data) | [PROGRESS_STREAM_C_DATA_PIPELINE.md](file:///C:/JULES/roadmap/PROGRESS_STREAM_C_DATA_PIPELINE.md) | 8 zadan | ⚪ Zaplanowane (Niezalezne od renderera) |
| **D** | Mock Simulation Harness | Agent Lead D (QA/Harness) | [PROGRESS_STREAM_D_MOCK_TEST_HARNESS.md](file:///C:/JULES/roadmap/PROGRESS_STREAM_D_MOCK_TEST_HARNESS.md) | 6 zadan | ⚪ Zaplanowane (Zalezne od B1, B2) |

---

## 4. PROTOKOL PRACY DLA AGENTOW ROJU (JULES SWARM PROTOCOL)

Kazdy agent AI przystepujacy do pracy w projekcie wykonuje nastepujaca sekwencje:

1. **Pobranie Kontekstu**:
   - Agent odczytuje swoj dedykowany plik `PROGRESS_STREAM_*.md`.
   - Identyfikuje pierwsze niezrealizowane zadanie z checkboxem `[ ]`.
2. **Implementacja w Trybie Zero-Conflict**:
   - Agent tworzy wylacznie nowy, dedykowany plik w `src/Client/` o budzecie kodu do **300-400 linii**.
   - Dolacza `#include "StdAfx.h"` oraz naglowek kontraktu.
   - Uzywa wylacznie C++23: `StrongTypes.h`, `std::expected` / `Result`, `std::span`.
3. **Weryfikacja Testowa**:
   - Agent tworzy powiazany plik testowy `tests/test_c26_*.cpp`.
   - Uruchamia lokalne testy; zadanie jest ukonczone dopiero, gdy test zwraca PASS.
4. **Aktualizacja Postepu**:
   - Agent oznacza zadanie jako `[x]` w pliku postepu.
   - Commituje zmiany do galezi i zwalnia slot w `cloud_sessions` dla kolejnego workera.
