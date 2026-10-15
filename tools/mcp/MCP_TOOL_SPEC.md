# MCP Tool Specification: Metin2 Client x64 2026

Dokumentacja techniczna i podrecznik operacyjny dla agentow AI (Antigravity, Google Jules, Cursor, Claude Code, Cline) uzywajacych serwera Model Context Protocol (**Agent MCP**) oraz bezglowego klienta gry `Metin2_Release.exe`.

---

## 1. Architektura Integracji

```
[ Agent AI (LLM / MCP Client) ]
              │  (JSON-RPC 2.0 przez stdio)
              ▼
[ tools/mcp/m2_agent_harness_mcp.py ]  <-- Serwer MCP (38 narzedzi @server.tool)
              │  (Named Pipe: \\.\pipe\M2ClientEngineHarness)
              ▼
[ Metin2_Release.exe --headless ]      <-- Klient C++23 w trybie Mock/Headless
```

### Tryb Headless (Bezglowy):
- Flaga: `--headless`
- Wycisza wszystkie modalne okna Win32 (`MessageBoxW` -> cichy log `TraceError`).
- Ukrywa okno gry (`SW_HIDE`).
- Pomija fizyczny potok renderowania klatek GPU (`canRender = false`).
- Dlawik CPU w petli glownej (`Sleep(15)` redukuje zuzycie procesora do poziomu tla).
- Bezpieczny stub audio `SoundEngine` odporny na brak urzadzenia DirectSound.

### Tryb Deterministyczny i Mock World:
- Flagi: `--test-mode --mock-world --freeze-on-start`
- Tworzy wirtualny swiat offline bez potrzeby laczenia sie z serwerem TCP (`EnterMockWorld`).
- Zamraza uplyw czasu logiki gry do momentu otrzymania komendy `tick_frame`.

---

## 2. Rejestr 38 Narzedzi MCP (@server.tool)

### Kategoria 1: Zarzadzanie Cyklem Zycia i Silnikiem (Lifecycle & Engine Core)

#### 1. `m2_harness_launch(flags, work_dir)`
- **Opis**: Uruchamia proces `Metin2_Release.exe` w tle z zadanymi flagami i nawiazuje polaczenie z potokiem IPC.
- **Parametry**:
  - `flags` (opcjonalny, string): domyslnie `"--headless --test-mode --mock-world --freeze-on-start"`.
  - `work_dir` (opcjonalny, string): sciezka do katalogu roboczego z assetami (`m2dev-client-main`).
- **Zwrot**: `{"status": "RUNNING", "pid": 12345, "pipe": "\\\\.\\pipe\\M2ClientEngineHarness"}`

#### 2. `m2_harness_stop(force=False)`
- **Opis**: Bezpiecznie i czysto zamyka proces klienta przez polecenie RPC `shutdown` z kodem wyjscia 0. W razie potrzeby wykonuje wymuszone zakonczenie procesu (`force=True`).
- **Parametry**:
  - `force` (bool, domyslnie `False`): czy wymusic natychmiastowe zabicie procesu.
- **Zwrot**: `{"status": "STOPPED", "exit_code": 0}`

#### 3. `m2_tick_frame(count=1, delta_ms=16.6)`
- **Opis**: Przepycha dokladnie `count` klatek logiki gry z krokiem czasowym `delta_ms` (w milisekundach).
- **Parametry**:
  - `count` (int, domyslnie 1): liczba klatek logiki.
  - `delta_ms` (float, domyslnie 16.6): krok czasowy klatki w milisekundach (60 Hz).
- **Zwrot**: `{"ok": true, "stepped_frames": 1, "delta_time": 0.0166, "current_tick": 42}`

#### 4. `m2_get_engine_state()`
- **Opis**: Pobiera pelny, synchroniczny stan pamieci C++ klienta (gracz, kontrolery, faza sieci, sterownik, okno).
- **Zwrot**:
  - `player`: `vid`, `name`, `race`, `position (x, y, z)`, `rotation`, `hp`, `max_hp`, `sp`, `max_sp`, `level`, `gold`, `target_vid`.
  - `controllers`: `movement`, `combat`, `target`, `item`.
  - `network`: `phase` (`OffLine`, `Game`, itp.), `active_driver` (np. `"standard_x64"`).
  - `window`: `visible` (bool), `gpu_throttled` (bool).

#### 5. `m2_audit_dual_state(epsilon=0.01)`
- **Opis**: Przeprowadza zaawansowany audyt synchronizacji pomiedzy stanem w strukturach C++ a maszyna wirtualna Pythona (VM) oraz integralnoscia aktorow swiata (`world_audit`).
- **Parametry**:
  - `epsilon` (float, domyslnie 0.01): dopuszczalna tolerancja bledu pozycji float.
- **Zakres audytu**:
  - Gracz: koordynaty X, Y, Z, rotacja (`cpp_rot` vs `py_rot`), punkty HP/SP, flaga `isDead`.
  - `world_audit`: iteracja po `CPythonCharacterManager::m_aliveActorsMap` i porownanie z `Client::World::ActorRegistry` (weryfikacja VID, pozycji, dopasowania typow i braku martwych duchow/zombie).
- **Zwrot**: `{"is_synchronized": true, "audit_verdict": "PERFECT_MATCH", "discrepancies": [], "world_audit": {"cpp_actors_count": 14, "registry_actors_count": 14, "mismatches": 0}}`

#### 6. `m2_inject_packet(opcode, hex_payload)`
- **Opis**: Wstrzykuje binarny pakiet sieciowy bezposrednio do routera pakietow bez fizycznego gniazda TCP.
- **Parametry**:
  - `opcode` (int): opcode pakietu (naglowek 0..65535, np. 2 dla ruchu, 3 dla ataku).
  - `hex_payload` (string): ciag bajtow payloadu w formacie heksadecymalnym.
- **Zwrot**: `{"ok": true, "header": 2, "bytes_count": 8}`

#### 7. `m2_fuzz_router(opcode, count=100)`
- **Opis**: Wykonuje test fuzingu losowymi bajtami na routerze pakietow, weryfikujac odpornosc na uszkodzone dane.
- **Parametry**:
  - `opcode` (int): naglowek pakietu do fuzingu.
  - `count` (int, domyslnie 100): liczba zfuzzowanych pakietow.
- **Zwrot**: `{"status": "ROUTER_RESILIENT", "passed": 100, "failed": 0}`

#### 8. `m2_get_crash_report()`
- **Opis**: Odpytuje system `CrashSentinel` o raport ewentualnego bledu (Access Violation) z dokladnymi symbolami i numerami linii z plikow PDB.
- **Zwrot**: `{"status": "STABLE_NO_CRASH"}` lub szczegolowy zrzut JSON ze stack tracem.

---

### Kategoria 2: C++23 Domain State & Akcje Pomostowe (Domain Contracts)

#### 9. `m2_game_ping()`
- **Opis**: Natychmiastowy test liveness i czasu odpowiedzi warstwy pomostowej `AgentMCPBridge`.
- **Zwrot**: `{"ok": true, "pong": true, "timestamp_ms": 1728662000000}`

#### 10. `m2_game_get_state()`
- **Opis**: Pobiera stan postaci gracza bezposrednio przez kontrakt `IPlayerDomainContract` (C++23 Domain State).
- **Zwrot**: `{"ok": true, "player": {"vid": 55673, "name": "[SA]Admin", "hp": 11500, "max_hp": 11500, "x": 957380.0, "y": 254010.0, "z": 0.0, "in_combat": false}}`

#### 11. `m2_game_get_inventory(window=1)`
- **Opis**: Pobiera pelna zawartosc inwentarza bezposrednio przez kontrakt `IInventoryDomainContract`.
- **Parametry**:
  - `window` (int, domyslnie 1 = INVENTORY): identyfikator okna ekwipunku.
- **Zwrot**: `{"ok": true, "items": [{"slot": 0, "vnum": 10, "count": 1, "sockets": [...], "attributes": [...]}]}`

#### 12. `m2_game_get_nearby_entities(radius=3000.0)`
- **Opis**: Zwraca liste wszystkich aktorow w zadanym promieniu zoptymalizowana przez siatke `SpatialHashGrid` ($O(1)$) z ochrona DoS ($O(N)$ fallback).
- **Parametry**:
  - `radius` (float, domyslnie 3000.0): promien wyszukiwania w jednostkach gry.
- **Zwrot**: `{"ok": true, "entities": [{"vid": 20001, "vnum": 101, "name": "Dziki Pies", "type": 2, "x": ..., "y": ..., "distance": 450.2, "is_dead": false}]}`

#### 13. `m2_game_action_attack(target_vid)`
- **Opis**: Wykonuje bezpieczna, zweryfikowana akcje ataku celu przez kontrakt `ICombatDomainContract`.
- **Parametry**:
  - `target_vid` (int): unikalny identyfikator celu (EntityVid).
- **Zwrot**: `{"ok": true, "action": "attack_initiated", "target_vid": 20001}`

#### 14. `m2_game_action_use_item(window=1, cell=0)`
- **Opis**: Bezposrednie uzycie lub zalozenie przedmiotu z danego slotu przez kontrakt `IInventoryDomainContract`.
- **Parametry**:
  - `window` (int, domyslnie 1): okno ekwipunku.
  - `cell` (int): numer slotu.
- **Zwrot**: `{"ok": true, "action": "item_used", "slot": 0}`

---

### Kategoria 3: Autoryzacja i Selekcja Postaci (Auth & Character Selection)

#### 15. `m2_game_login(username="admin", password="admin", ip="127.0.0.1", port=11002)`
- **Opis**: Automatycznie laczy klienta z serwerem logowania Auth i przeprowadza uwierzytelnienie.
- **Parametry**:
  - `username` (string, domyslnie `"admin"`): nazwa konta (akceptuje takze aliasy `account` / `id`).
  - `password` (string, domyslnie `"admin"`): haslo do konta.
  - `ip` (string, domyslnie `"127.0.0.1"`): adres serwera IP (akceptuje takze aliasy `server_ip` / `addr`).
  - `port` (int, domyslnie 11002): port logowania auth.
- **Zwrot**: `{"ok": true, "status": "connecting_auth", "account": "admin", "auth_port": 11000, "game_port": 11011}`

#### 16. `m2_game_select_character(slot=0)`
- **Opis**: Wybiera postac z danego slotu na koncie (0..3) i inicjuje faze ladowania swiata gry (`PHASE_LOADING` -> `PHASE_GAME`).
- **Parametry**:
  - `slot` (int, domyslnie 0): indeks slotu postaci (0, 1, 2, 3).
- **Zwrot**: `{"ok": true, "status": "character_selected", "slot": 0}`

#### 17. `m2_game_enter_game()`
- **Opis**: Wymusza wyslanie pakietu wejscia do gry (`CG_ENTERGAME` - 0x0206) i finalizuje przejscie do pelnej fazy swiata gry.
- **Zwrot**: `{"ok": true, "status": "game_enter_sent"}`

---

### Kategoria 4: Tryby Silnika i Profil Protokolu (Engine Modes & Protocol)

#### 18. `m2_game_set_window_mode(visible=False, throttle=True)`
- **Opis**: Steruje w locie widocznoscia okna gry oraz dlawieniem potoku graficznego Direct3D 9 (redukcja do 0% GPU).
- **Parametry**:
  - `visible` (bool): `false` ukrywa okno (`SW_HIDE`), `true` przywraca okno (`SW_RESTORE`).
  - `throttle` (bool): `true` wlacza dlawik 0% GPU, `false` wlacza pelne 60 FPS.
- **Zwrot**: `{"ok": true, "visible": true, "throttled": false}`

#### 19. `m2_game_set_protocol_driver(driver="standard_x64")`
- **Opis**: Przelacza aktywny sterownik protokolu sieciowego w locie przez `ProtocolDriverRegistry`.
- **Parametry**:
  - `driver` (string): nazwa profilu (`"standard_x64"`, `"beavium"`, `"martysama"`, `"luria"`).
- **Zwrot**: `{"ok": true, "active_driver": "standard_x64"}`

---

### Kategoria 5: Bezposrednie Sterowanie Postacia C++23 (Direct Character Control)

#### 20. `m2_game_set_attack_key_state(is_pressed=True)`
- **Opis**: Bezposrednio przelacza stan klawisza ataku postaci (`SetAttackKeyState`) w silniku C++ (odpowiednik spacji / bicia z konia / walki wrecz).
- **Parametry**:
  - `is_pressed` (bool, domyslnie `True`): `True` uruchamia ciagly atak, `False` przerywa atak.
- **Zwrot**: `{"ok": true, "status": "attack_key_set", "is_pressed": true}`

#### 21. `m2_game_set_dir_key_state(dir=0, is_pressed=True)`
- **Opis**: Ustawia stan pojedynczego klawisza kierunkowego w silniku (`NEW_SetSingleDirKeyState`).
- **Parametry**:
  - `dir` (int, 0..3): kierunek ruchu: 0 = DIR_UP (W), 1 = DIR_DOWN (S), 2 = DIR_LEFT (A), 3 = DIR_RIGHT (D).
  - `is_pressed` (bool, domyslnie `True`): stan wcisniecia klawisza.
- **Zwrot**: `{"ok": true, "status": "dir_key_set", "dir": 0, "is_pressed": true}`

#### 22. `m2_game_set_multi_dir_key_state(left=False, right=False, up=False, down=False)`
- **Opis**: Ustawia jednoczesnie stan wszystkich 4 wektorow ruchu klawiatury (`NEW_SetMultiDirKeyState`), umozliwiajac plynny bieg po skosie.
- **Parametry**:
  - `left`, `right`, `up`, `down` (bool): stany poszczegolnych osi.
- **Zwrot**: `{"ok": true, "status": "multi_dir_set", "left": false, "right": false, "up": true, "down": false}`

#### 23. `m2_game_move_to_position(x=0.0, y=0.0, z=0.0)`
- **Opis**: Rozpoczyna automatyczna nawigacje postaci do docelowych koordynatow pikselowych swiata gry (`NEW_MoveToDestPixelPositionDirection`) z walidacja granic mapy i liczb skonczonych (`std::isfinite`).
- **Parametry**:
  - `x`, `y`, `z` (float): docelowe koordynaty w przestrzeni swiata.
- **Zwrot**: `{"ok": true, "status": "move_started", "success": true, "x": 45150.0, "y": 38150.0, "z": 0.0}`

#### 24. `m2_game_stop()`
- **Opis**: Natychmiast zatrzymuje bieg i nawigacje postaci (`NEW_Stop`).
- **Zwrot**: `{"ok": true, "status": "stopped"}`

#### 25. `m2_game_pick_close_item()`
- **Opis**: Wywoluje natywne podniesienie najblizszego przedmiotu lezacego na ziemi (`PickCloseItem` - odpowiednik klawisza Z).
- **Zwrot**: `{"ok": true, "status": "pick_item_invoked"}`

#### 26. `m2_game_set_target(target_vid=0, force=True)`
- **Opis**: Zaznacza cel ataku/rozmowy w menedzerze celow gracza (`SetTarget`).
- **Parametry**:
  - `target_vid` (int): unikalny identyfikator celu.
  - `force` (bool, domyslnie `True`): wymuszenie wyboru.
- **Zwrot**: `{"ok": true, "status": "target_set", "target_vid": 57104}`

#### 27. `m2_game_clear_target()`
- **Opis**: Odznacza biezacy cel gracza (`ClearTarget`).
- **Zwrot**: `{"ok": true, "status": "target_cleared"}`

#### 28. `m2_game_use_skill(slot=0)`
- **Opis**: Aktywuje umiejetnosc przypisana do danego slotu na pasku skilli (`ClickSkillSlot`) z walidacja zakresu 0..255.
- **Parametry**:
  - `slot` (int, domyslnie 0): numer slotu skilla.
- **Zwrot**: `{"ok": true, "status": "skill_clicked", "slot": 0}`

#### 29. `m2_game_send_chat(text="", type=0)`
- **Opis**: Wysyla pakiet czatu do serwera (`SendChatPacket`) - obsluguje wiadomosci publiczne, szept oraz komendy GM (np. `/m 101`, `/warp 100 100`). Automatycznie ogranicza dlugosc tekstu do 511 znakow.
- **Parametry**:
  - `text` (string): tresc wiadomosci lub komenda.
  - `type` (int, domyslnie 0 = TALKING): typ kanalu czatu.
- **Zwrot**: `{"ok": true, "status": "chat_sent", "success": true, "length": 23}`

---

### Kategoria 6: Automatyzacja Okna OS i Peryferia Win32 (OS Window Automation)

#### 30. `m2_client_launch(path="Metin2_Release.exe", work_dir="", args=[])`
- **Opis**: Uruchamia klienta gry w standardowym trybie okienkowym systemu Windows (z widocznym GUI DirectX).
- **Zwrot**: `{"status": "LAUNCHED", "pid": 12345}`

#### 31. `m2_client_status()`
- **Opis**: Sprawdza status procesu gry na poziomie systemu operacyjnego (czy okno istnieje, czy proces odpowiada, obecnosc na pasku zadan).
- **Zwrot**: `{"running": true, "pid": 12345, "title": "METIN2", "visible": true}`

#### 32. `m2_client_screenshot()`
- **Opis**: Wykonuje zrzut ekranu okna gry i zwraca go jako ciag Base64 obrazu PNG/JPEG, umozliwiajac wizualna inspekcje agentom multimodalnym.
- **Zwrot**: `{"ok": true, "format": "png", "base64_data": "iVBORw0KGgo..."}`

#### 33. `m2_client_mouse_click(x=0, y=0, button="left")`
- **Opis**: Symuluje fizyczne klikniecie mysza w oknie klienta za pomoca komunikatow Win32 `WM_LBUTTONDOWN` / `WM_LBUTTONUP`.
- **Parametry**:
  - `x`, `y` (int): koordynaty wewnatrz okna klienta.
  - `button` (string): `"left"` lub `"right"`.
- **Zwrot**: `{"ok": true, "action": "mouse_click", "x": 400, "y": 300}`

#### 34. `m2_client_mouse_move(x=0, y=0)`
- **Opis**: Symuluje przesuniecie kursora myszy nad zadanym punktem okna gry (`WM_MOUSEMOVE`).
- **Zwrot**: `{"ok": true, "action": "mouse_move", "x": 400, "y": 300}`

#### 35. `m2_client_send_text(text="")`
- **Opis**: Wprowadza ciag znakow tekstu bezposrednio do aktywnego pola tekstowego gry (np. czat, pole logowania) przez potok IPC (alias `chat_send` / `game_send_chat`) lub komunikaty `WM_CHAR`.
- **Zwrot**: `{"ok": true, "action": "send_text", "text": "Hello", "dispatched": true}`

#### 36. `m2_client_send_key(key="")`
- **Opis**: Symuluje nacisniecie i zwolnienie klawisza klawiatury systemowej (`WM_KEYDOWN` / `WM_KEYUP`).
- **Zwrot**: `{"ok": true, "action": "send_key", "key": "RETURN"}`

#### 37. `m2_client_get_logs(lines=100)`
- **Opis**: Odczytuje ostatnie `lines` wierszy z plikow dziennika silnika (`syserr.txt` oraz `syslog.txt`).
- **Zwrot**: `{"syserr": ["..."], "syslog": ["..."]}`

#### 38. `m2_client_close(force=False)`
- **Opis**: Zamyka okno gry przez wyslanie komunikatu `WM_CLOSE` (lub `TerminateProcess` gdy `force=True`).
- **Zwrot**: `{"ok": true, "status": "closed"}`

---

## 3. Kompatybilnosc i Weryfikacja

Wszystkie 38 narzedzi sa przetestowane i zintegrowane w glownym systemie testow:
- `ctest --test-dir build -C Release` (100% PASS - 51/51 testow CTest)
- `python tools/ai_preflight_check.py` (100% PASS w 0.8s)
- `python tools/mcp/test_auto.py` (Pelny test integracyjny live z serwerem standard_x64)
- `python tools/mcp/test_live_character_actions.py` (100% PASS - 11/11 testow na zywo)
- `python tools/mcp/test_mcp_heavy.py` (Test chaosu i odpornosci - 22/22 zadan PASS)
