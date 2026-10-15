# RAPORT Z EKSTREMALNYCH TESTOW STRESOWYCH, CHAOSU I BEZPIECZENSTWA KLIENTA C++23 I MCP

Data: 11 pazdziernika 2026
Architektura: C++23 Native Engine (`Metin2_Release.exe`) + Python 3.14 Fast-Path
Srodowisko: Windows x64, serwer standard_x64 (`E:\m2dev-server-main`)
Mostek IPC: Win32 Named Pipe `\\.\pipe\M2ClientEngineHarness` (JSON-RPC 2.0 Framed)

---

## 1. Cel i Zakres Testow

Celem audytu bylo metodyczne, agresywne przetestowanie WSZYSTKICH mozliwosci silnika klienta gry oraz serwera MCP w polaczeniu na zywo z serwerem gry, z naciskiem na poszukiwanie bledow:
- Deadlocki i race condition w komunikacji Named Pipe.
- Wycieki pamieci, rozsynchronizowanie buforow strumienia (framing desync).
- Fuzzing zmutowanych pakietow sieciowych (opcody 1, 2, 3, 10, 12).
- Edge-cases w logice walki i przedmiotow (ujemne sloty, cele o VID=0, wlasny VID, nieistniejace VID).
- Odpornosc na spam zapytaniami IPC (100 req w petli bez czekania).
- Rapid Broken Pipe (wielokrotne gwaltowne zerwanie polaczenia potoku).
- Weryfikacja CrashSentinela (pobieranie stack-trace i kodow wyjatkow PDB).

---

## 2. Zidentyfikowane Usterki i Wdrozone Poprawki

Podczas testow chaosu zidentyfikowano i natychmiast wyeliminowano 4 krytyczne usterki:

1. **Deadlock bufora odczytu Named Pipe (`_send_raw_pipe_cmd`)**:
   - *Problem*: Wczesniejsza implementacja wywolywala blokujacy `win32file.ReadFile`. Gdy serwer odrzucil pakiet lub spoznil sie z odpowiedzia, klient IPC wisial w nieskonczonosc.
   - *Naprawa*: Wdrozenie nieblokujacego odpytywania z `win32pipe.PeekNamedPipe(self.pipe_handle, 0)` z deadline `timeout_s` oraz automatycznym czyszczeniem (drain) starych, nieodebranych bajtow przed kazdym nowym poleceniem.
   - *Plik*: `tools/mcp/m2_agent_harness_mcp.py`.

2. **Heisenbug braku odpowiedzi w protokole Framed (`TestHarnessServer.cpp`)**:
   - *Problem*: Serwer traktowal zapytania bez pola `id` jako powiadomienia JSON-RPC 2.0 (`isNotification = !hasId`) i nie odsylal zadnej ramki. W protokole ramkowym klient czekal na 4-bajtowy naglowek odpowiedzi, co powodowalo blokade kolejki.
   - *Naprawa*: W protokole Framed (`isFramed == true`) serwer ZAWSZE odsyla 4-bajtowa odpowiedz ramkowa (z `"id": null` i kodem bledu jesli brak `id`).
   - *Plik*: `src/UserInterface/TestHarness/TestHarnessServer.cpp`.

3. **Brak parametru `payloadSize` w `HandleInjectPacket` i `NetworkStreamPhaseGameBridge`**:
   - *Problem*: Wywolanie `DispatchPacket(header, bytes.data())` uzywalo domyslnego `payloadSize = 0`, co powodowalo falszywe odrzucenie poprawnych pakietow przez makro weryfikujace `M2_VERIFY_PAYLOAD8`.
   - *Naprawa*: Przekazanie rzeczywistego `bytes.size()` oraz `payload.size()` do dispatchera.
   - *Pliki*: `TestHarnessServer.cpp`, `NetworkStreamPhaseGameBridge.h`.

4. **Spam debug logow Heartbeat 0x0302**:
   - *Problem*: Serwer `channel1_core1` wysylal co klatke heartbeat 0x0302, co generowalo tysiace linii logow na sekunde.
   - *Naprawa*: Prawidlowa dyspozycja przez router pakietow z obsluga rzeczywistych rozmiarow payloadu.

5. **Rozbieznosc nazwy metody czatu (`chat_send` vs `game_send_chat`)**:
   - *Problem*: `m2_client_send_text` w Pythonie wysylal `chat_send`, ktorego brakowalo w trasach C++.
   - *Naprawa*: Wdrozenie podwojnej rejestracji tras `game_send_chat` oraz `chat_send`, a takze walidacja dlugosci tekstu `< 512` znakow.

6. **Elastycznosc parametrow logowania w `game_login`**:
   - *Problem*: TestHarness oczekiwal pol `account` oraz `server_ip`, podczas gdy klient MCP przekazywal `username` oraz `ip`.
   - *Naprawa*: Pelna kompatybilnosc wielowariantowa (`account`/`username`/`id` oraz `server_ip`/`ip`/`addr`).

7. **Normalizacja rotacji w audycie Dual-State**:
   - *Problem*: W Pythonie `chr.GetRotation()` zwraca `360.0f - deg` (odwrotna orientacja trygonometryczna UI), przez co bezposrednie porownanie z `cpp_rot` wykazywalo falszywa desynchronizacje.
   - *Naprawa*: Ujednolicenie normalizacji katow z uwzglednieniem konwencji Pythona oraz obsluga parametru tolerancji `epsilon`.

8. **Ochrona przed NaN/Inf i nieskonczonym biegiem w `game_move_to_position`**:
   - *Problem*: Przekazanie wartosci NaN lub Inf moglo zdegenerowac macierz swiata DirectX i zawiesic postac w nieskonczonym marszu (Infinite Walk Lock).
   - *Naprawa*: Walidacja `std::isfinite` dla osi X, Y, Z oraz weryfikacja granic mapy `[0, 204800)`.

9. **Reset stanu ataku (`m_isAtkKey`) przy Warp i zgonie**:
   - *Problem*: Wlaczenie klawisza ataku moglo pozostac aktywne po teleportacji lub odrodzeniu postaci (Attack Lock).
   - *Naprawa*: Wymuszenie `SetAttackKeyState(false)` oraz `NEW_Stop()` przy zmianie fazy, smierci i teleportacji gracza.

---

## 3. Zbiorcze Wyniki Testow

### A. Ekstremalny Test Mozliwosci i Chaosu (`test_mcp_all_possibilities.py`)
- **launch_mock_world**: PASS (100%)
- **tick_frame (10 klatek, 16.6ms)**: PASS (100%)
- **stop_mock_world**: PASS (100%)
- **launch_live (headless, standard_x64)**: PASS (100%)
- **game_ping (liveness pong)**: PASS (100%)
- **protocol_switching (standard_x64, martysama, beavium, luria)**: PASS (100%)
- **fsm_enter_game (czas wejscia 2.42s!)**: PASS (100%)
- **get_state (IPlayerDomainContract)**: PASS (100%)
- **get_inventory (55 itemow w oknie 1)**: PASS (100%)
- **world_query_radii (SpatialHashGrid w promieniach -100 do 1000000)**: PASS (100%)
- **dual_state_audit (SYNC_OK, 0 desynchronizacji)**: PASS (100%)
- **item_edge_cases (ujemne sloty, slot 9999, zle okna)**: PASS (100%)
- **combat_edge_cases (cel 0, cel 99999999, wlasny VID)**: PASS (100%)
- **window_mode (ukrywanie i odkrywanie okna, throttle GPU)**: PASS (100%)
- **packet_fuzzing (150 zmutowanych pakietow, 0 awarii)**: PASS (100%)
- **ipc_flood_test (100/100 zapytan w 1.57s = 63.7 req/s)**: PASS (100%)
- **ipc_fuzz_safe (malformed JSON-RPC, brak method/id)**: PASS (100%)
- **rapid_reconnect (5 cykli zerwania i otwarcia pipe)**: PASS (100%)
- **crash_sentinel (verdict: STABLE_NO_CRASH, 0 wyjatkow)**: PASS (100%)
- **clean_shutdown (czyste zatrzymanie)**: PASS (100%)
- **no_zombie_process (0 procesow wiszacych)**: PASS (100%)
- **clean_syserr (0 wpisow krytycznych)**: PASS (100%)

**WYNIK**: **22 / 22 TESTOW ZDANYCH W 100%!**

### B. Testy Sterowania Postacia i Akcji w Swiecie Gry (`test_live_character_actions.py`)
- **game_set_attack_key_state (true/false, Spacja / Auto-Atak)**: PASS (100%)
- **game_set_dir_key_state (os DIR_UP / W)**: PASS (100%)
- **game_set_multi_dir_key_state (skos Lewo+Gora / WASD)**: PASS (100%)
- **game_move_to_position & game_stop (nawigacja pikselowa)**: PASS (100%)
- **game_set_target & game_clear_target (zaznaczanie celow VID)**: PASS (100%)
- **game_pick_close_item (podnoszenie przedmiotow - klawisz Z)**: PASS (100%)
- **game_use_skill (aktywacja skilla z slotu 0)**: PASS (100%)
- **game_send_chat (wysylanie wiadomosci i komend serwera)**: PASS (100%)
- **game_attack_target (bezposredni atak celu)**: PASS (100%)
- **audit_dual_state (weryfikacja pamieci i ActorRegistry)**: PASS (100%)
- **shutdown (asynchroniczne, czyste zamkniecie)**: PASS (100%)

**WYNIK**: **11 / 11 TESTOW ZDANYCH W 100%!**

### C. Zestaw Testow Jednostkowych CTest
- **ctest -C Release**: **51 / 51 TESTOW ZDANYCH (100% PASS w 8.91s)**.

### D. Walidacja Lintera AI
- **python tools/ai_preflight_check.py**: **100% PASS (0.817s)**.

