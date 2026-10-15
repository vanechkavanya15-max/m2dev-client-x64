# WORKBOOK POSTEPU: STRUMIEN B2 - UNIFIKACJA STANU GRY (STATE UNIFICATION & SINGLE SOURCE OF TRUTH)
## Odpowiedzialny: Agent Lead B2 (State & Domain Architecture Specialist)

**Cel Strumienia:** Likwidacja patologii **Split-Brain State** w pamieci klienta. Calkowite odciecie starego `CPythonPlayer::m_playerStatus` i XOR-owania `POINT_MAGIC_NUMBER = 0xe73ac1da`. Ustanowienie `Client::Core::WorldContext` jako jedynego zrodla prawdy (Single Source of Truth) o stanie postaci i swiata.  
**Standard:** C++23, thread-safe access przez `EterBase::ModernMutex`, silne typy domenowe (`StrongTypes.h`).

---

## 1. DOKLADNA MAPA ZAGROZEN I STARYCH STRUKTUR

1. **Stary stan w `src/UserInterface/PythonPlayer.cpp`:**
   ```cpp
   // PATOLOGIA DO USUNIECIA:
   const DWORD POINT_MAGIC_NUMBER = 0xe73ac1da;
   void CPythonPlayer::SPlayerStatus::SetPoint(UINT ePoint, long lPoint) {
       m_alPoint[ePoint] = lPoint ^ POINT_MAGIC_NUMBER;
   }
   ```
2. **Nowy stan w `src/Client/Core/WorldContext.h`:**
   Pelny, jawny rekord pamieci bez magicznych liczb, integrujacy:
   - Statystyki (`currentHp`, `maxHp`, `currentSp`, `maxSp`, `currentGold`, `level`).
   - Przestrzen (`localPlayerCoords`, `localPlayerRotation`, `currentMapIndex`, `currentChannel`).
   - Flagi stanu (`isDead`, `isStunned`, `combatMode`).
   - Kontenery domenowe (`InventoryDomain`, `SkillDomain`, `CombatDomain`).

---

## 2. REJESTR ZADAN ATOMOWYCH (SWARM TASK LIST)

| ID | Status | Nazwa Zadania / Obszar | Plik Zrodlowy | Plik Testu Jednostkowego | Przypisany Agent |
|---|---|---|---|---|---|
| **B2-01** | [x] | Utworzenie pelnej struktury `WorldContext` i metod akcesorow | `Client/Core/WorldContext.h` | `test_c26_game_session.cpp` | Jules Worker #23 |
| **B2-02** | [x] | Wdrozenie `StranglerFacade` do proxy metod walki i ruchu | `Client/Bridge/StranglerFacade.h/.cpp` | `test_c26_combat_domain.cpp` | Jules Worker #24 |
| **B2-03** | [ ] | Przepiecie `CPythonPlayer::GetStatus()` bezposrednio do `WorldContext` | `UserInterface/PythonPlayer.cpp` | `test_c26_player_points_handler.cpp` | Jules Worker #25 |
| **B2-04** | [ ] | Usuniecie `POINT_MAGIC_NUMBER` i struktury `SPlayerStatus` | `UserInterface/PythonPlayer.h/.cpp` | Kompilacja UserInterface | Jules Worker #26 |
| **B2-05** | [x] | Integracja domenowej siatki inwentarza `InventoryGridManager` | `Client/Gameplay/InventoryGridManager.cpp` | `test_c26_inventory_grid_manager.cpp` | Jules Worker #27 |
| **B2-06** | [ ] | Przepiecie slotow ekwipunku z `CPythonPlayer` do `InventoryDomain` | `UserInterface/PythonPlayer.cpp` | `test_c26_inventory_domain.cpp` | Jules Worker #28 |
| **B2-07** | [x] | Implementacja `SpatialHashGrid` do szybkiego wyszukiwania encji (Radar) | `Client/World/SpatialHashGrid.h/.cpp` | `test_c26_spatial_hash_grid.cpp` | Jules Worker #29 |
| **B2-08** | [ ] | Przepiecie pobierania celow z `CPythonCharacterManager` na `SpatialHashGrid` | `UserInterface/PythonCharacterManager.cpp` | `test_c26_combat_target_validator.cpp` | Jules Worker #30 |
| **B2-09** | [x] | Zabezpieczenie wspolbieznosci `WorldContext` przez `ModernMutex` | `Client/Core/WorldContext.h` | `test_c26_modern_mutex.cpp` | Jules Worker #31 |
| **B2-10** | [ ] | Walidacja spojnosci stanu w tescie ciaglym (0 desyncow) | `tests/test_c26_state_snapshot.cpp` | `test_c26_state_snapshot.cpp` | Jules Worker #32 |

---

## 3. KRYTERIA AKCEPTACJI DLA AGENTA B2 (DEFINITION OF DONE)
1. W calej bazie kodu nie ma ani jednego wystapienia stalej `POINT_MAGIC_NUMBER`.
2. Dowolny odczyt punktow postaci z poziomu C++, Pythona oraz serwera MCP zwraca DOKLADNIE te sama wartosc z `WorldContext`.
3. Brak wyciekow pamieci i brak wyscigow watkow (`data races`) przy rownoleglym odczycie telemetrii i zapisie pakietow.
