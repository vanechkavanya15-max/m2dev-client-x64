# WORKBOOK POSTEPU: STRUMIEN D - DETERMINISTYCZNY MOCK TEST HARNESS (TEST HARNESS & SIMULATION)
## Odpowiedzialny: Agent Lead D (QA, Simulation & Verification Specialist)

**Cel Strumienia:** Budowa w pelni deterministycznego laboratoryjnego srodowiska symulacji klienta (Mock Test Harness) w standardzie C++23. Umozliwienie uruchamiania testow integracyjnych i behawioralnych (ruch, walka, handel, questy) w ulamki sekund bez koniecznosci uruchamiania bazy danych ani serwera gry.  
**Standard:** C++23, wirtualny zegar krokowy (`VirtualClock`), symulowany port sieciowy (`MockNetworkPortAdvanced`).

---

## 1. DOKLADNA MAPA KOMPONENTOW DO ZBUDOWANIA

```
[ Test Behawioralny / Scenariusz CI ]
                   │
                   ▼
     [ SessionSimulationHarness ]
     (Wirtualny Zegar, Kontroler Kroków)
          │                      │
          ▼                      ▼
  [ GameSession ]      [ MockNetworkPortAdvanced ]
  (Domeny, Logika)     (Dwukierunkowa kolejka pakietow)
          │                      ▲
          ▼                      │
  [ WorldContext ]     [ VirtualPacketGenerator ]
  (Stan encji w RAM)   (Syntetyczne pakiety GC serwera)
```

1. **`src/Client/Simulation/MockNetworkPortAdvanced.h/.cpp`:**
   - Dwukierunkowa kolejka pakietow w RAM: rejestruje wychodzace pakiety CG i pozwala wstrzykiwac syntetyczne pakiety GC.
   - Posiada wbudowane asercje: `ExpectPacket(opcode)`, `AssertPacketSent(opcode, payloadMatcher)`.
2. **`src/Client/Simulation/SessionSimulationHarness.h/.cpp`:**
   - Pozwala posuwac czas gry krok po kroku: `AdvanceTime(deltaSeconds)`, `StepFrames(count)`.
   - Wstrzykuje wirtualne encje do `WorldContext` i `SpatialHashGrid`.
3. **`src/Client/Simulation/VirtualPacketGenerator.h/.cpp`:**
   - Fabryka poprawnych pakietow GC (np. `SpawnMonster(vid, vnum, x, y)`, `InflictDamage(targetVid, amount)`).

---

## 2. REJESTR ZADAN ATOMOWYCH (SWARM TASK LIST)

| ID | Status | Nazwa Zadania / Obszar | Plik Zrodlowy | Plik Testu Jednostkowego | Przypisany Agent |
|---|---|---|---|---|---|
| **D-01** | [x] | Architektura srodowiska symulacji laboratoryjnej | `C:\JULES\plan_obszar_D_mock_harness.md` | Dok. specyfikacji | Agent Lead D |
| **D-02** | [ ] | Implementacja `MockNetworkPortAdvanced` z kolejka pakietow w RAM | `Client/Simulation/MockNetworkPortAdvanced.h/.cpp` | `test_c26_mock_network_port.cpp` | Jules Worker #53 |
| **D-03** | [ ] | Implementacja `VirtualPacketGenerator` (fabryka pakietow GC) | `Client/Simulation/VirtualPacketGenerator.h/.cpp` | `test_c26_virtual_packet_gen.cpp` | Jules Worker #54 |
| **D-04** | [ ] | Implementacja `SessionSimulationHarness` ze sterowaniem krokowym | `Client/Simulation/SessionSimulationHarness.h/.cpp` | `test_c26_simulation_harness.cpp` | Jules Worker #55 |
| **D-05** | [ ] | Scenariusz symulacji walki: Spawn moba -> Atak -> Obrazenia -> Zgon | `tests/scenarios/test_scenario_combat.cpp` | Test integracyjny walki | Jules Worker #56 |
| **D-06** | [ ] | Scenariusz symulacji handlu: Spawn NPC -> Otwarcie sklepu -> Zakup | `tests/scenarios/test_scenario_shop.cpp` | Test integracyjny sklepu | Jules Worker #57 |

---

## 3. KRYTERIA AKCEPTACJI DLA AGENTA D (DEFINITION OF DONE)
1. Pelna symulacja sekwencji walki (10 ciosow combo, reakcja na obrazenia, usuniecie moba) wykonuje sie w pamieci w czasie **ponizej 5 milisekund**.
2. Zadna symulacja nie otwiera socketu sieciowego ani nie wymaga polaczenia z internetem / serwerem gry.
3. Testy w 100% deterministycznie sprawdzaja poprawnosc logiki i kontraktow.
