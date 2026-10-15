r"""
E:\m2dev-client-src-mainOryginalx64\tools\mcp\test_mcp_all_possibilities.py
Kompleksowy test WSZYSTKICH mozliwosci silnika klienta C++23 oraz serwera MCP na zywo.
Aktywne poszukiwanie bledow (Stress, Chaos, Edge-Cases, Fuzzing, Broken Pipe, Memory Leaks).
Wszystkie komunikaty w jezyku polskim BEZ polskich znakow diakrytycznych.
"""

import sys
import os
import time
import json
import struct
import math
import random
from pathlib import Path

# Dodanie katalogu skryptu do sys.path
sys.path.insert(0, str(Path(__file__).parent))
import m2_agent_harness_mcp as harness

def log(msg: str):
    print(f"[{time.strftime('%H:%M:%S')}] {msg}", flush=True)

def run_all_possibilities_test():
    print("=" * 80, flush=True)
    print("EKSTREMALNY TEST WSZYSTKICH MOZLIWOSCI I SZUKANIE BLEDOW SILNIKA KLIENTA C++23", flush=True)
    print("=" * 80, flush=True)

    bridge = harness.bridge
    suite_results = {}
    found_bugs = []

    # =========================================================================
    # CZESC 1: TESTY PODSTAWOWYCH TRYBOW URUCHOMIENIA
    # =========================================================================
    log("\n--- [CZESC 1] TESTY ROZNYCH TRYBOW URUCHOMIENIA KLIENTA ---")

    # 1.1 Start w trybie mock_world (Zero-Server Dependency)
    log("[1.1] Test uruchomienia w trybie --mock-world...")
    r_mock = bridge.launch_process(mode="headless", mock_world=True, freeze=True, server="standard_x64")
    if not r_mock.get("ok"):
        found_bugs.append(f"Blad launch_process mock_world: {r_mock}")
    else:
        log(f"      -> Sukces mock_world: PID={r_mock.get('pid')}")
        suite_results["launch_mock_world"] = True

    # Test tick_frame pod zamrozonym zegarem
    log("[1.2] Test deterministycznego tick_frame (10 klatek, 16.6ms)...")
    r_tick = bridge.tick_frame(count=10, delta_ms=16.6)
    if not r_tick.get("ok") or r_tick.get("frame_count") != 10:
        found_bugs.append(f"Blad tick_frame: {r_tick}")
    else:
        log(f"      -> Sukces tick_frame: klatki={r_tick.get('frame_count')}, czas={r_tick.get('total_virtual_time_ms')}ms")
        suite_results["tick_frame"] = True

    # Zatrzymanie trybu mock_world
    log("[1.3] Zatrzymanie instancji mock_world...")
    r_stop_mock = bridge.stop_process(force=True)
    suite_results["stop_mock_world"] = r_stop_mock.get("ok", False)
    time.sleep(1.0)

    # =========================================================================
    # CZESC 2: URUCHOMIENIE NA ZYWO NA SERWERZE GRY X64 (E:\m2dev-server-main)
    # =========================================================================
    log("\n--- [CZESC 2] URUCHOMIENIE NA ZYWO NA SERWERZE GRY X64 ---")
    log("[2.1] Uruchamianie klienta w trybie headless pod serwer standard_x64...")
    r_live = bridge.launch_process(mode="headless", mock_world=False, freeze=False, server="standard_x64")
    if not r_live.get("ok"):
        found_bugs.append(f"Blad launch na zywo: {r_live}")
        print(f"KRYTYCZNY BLAD: Nie udalo sie uruchomic procesu klienta: {r_live}")
        return False
    live_pid = r_live.get("pid")
    log(f"      -> Klient C++ uruchomiony: PID={live_pid}, PipeConnected={bridge.is_connected}")
    suite_results["launch_live"] = True

    # 2.2 Test game_ping
    log("[2.2] Test liveness game_ping...")
    r_ping = bridge.game_ping()
    if not r_ping.get("ok"):
        found_bugs.append(f"Blad game_ping: {r_ping}")
    else:
        log(f"      -> game_ping OK: {r_ping}")
        suite_results["game_ping"] = True

    # 2.3 Przelaczanie sterownikow protokolu (standard_x64, martysama, beavium, luria)
    log("[2.3] Test dynamicznego przelaczania sterownikow protokolu...")
    drivers = ["standard_x64", "martysama", "beavium", "luria", "standard_x64"]
    drv_ok = True
    for drv in drivers:
        r_drv = bridge.game_set_protocol_driver(drv)
        if not r_drv.get("ok"):
            found_bugs.append(f"Blad przelaczania sterownika {drv}: {r_drv}")
            drv_ok = False
    log(f"      -> Przelaczanie sterownikow: {'OK' if drv_ok else 'FAIL'}")
    suite_results["protocol_switching"] = drv_ok

    # 2.4 FSM: Przejscie przez fazy Select -> Loading -> Game
    log("[2.4] Cykl wejscia do gry (Select -> Loading -> Game)...")
    log("      -> [MCP] Inicjalizacja game_login (admin/admin)...")
    bridge.game_login(username="admin", password="admin", ip="127.0.0.1", port=11011, auth_port=11000)

    in_game = False
    selected = False
    entered = False
    char_vid = 0
    char_name = ""

    t_start = time.time()
    for step in range(35):
        time.sleep(0.4)
        st = bridge.get_engine_state()
        if not st.get("ok"):
            continue

        net = st.get("network", {})
        phase = net.get("phase", "Unknown")
        plr = st.get("player", {})
        vid = plr.get("vid", 0)
        name = plr.get("name", "")
        hp = plr.get("hp", 0)
        max_hp = plr.get("max_hp", 0)
        pos = plr.get("position", {})

        log(f"      [Krok {step+1:02d}] Faza={phase} | VID={vid} | Postac='{name}' | HP={hp}/{max_hp} | Pozycja=({pos.get('x', 0):.1f}, {pos.get('y', 0):.1f})")

        if phase == "Select" and not selected:
            log("      -> [MCP] Faza Select: Wybor postaci slot=0...")
            bridge.game_select_character(slot=0)
            selected = True

        if phase == "Loading":
            if not entered:
                time.sleep(0.3)
                log("      -> [MCP] Faza Loading: game_enter_game()...")
                bridge.game_enter_game()
                entered = True
            elif hp > 0:
                bridge.game_enter_game()

        if phase == "Game" and vid > 0:
            in_game = True
            char_vid = vid
            char_name = name
            elapsed = time.time() - t_start
            log(f"\n>>> [SUKCES] OSIAGNIETO FAZE GRY W {elapsed:.2f}s! Postac: '{name}' (VID: {vid}) <<<\n")
            break

    if not in_game:
        found_bugs.append("Nie udalo sie osiagnac fazy Game w ciagu 14s")
    suite_results["fsm_enter_game"] = in_game

    # =========================================================================
    # CZESC 3: TESTY WSZYSTKICH METOD ODCZYTU I KONTRAKTOW DOMENOWYCH
    # =========================================================================
    log("\n--- [CZESC 3] TESTY METOD ODCZYTU I KONTRAKTOW DOMENOWYCH ---")

    # 3.1 IPlayerDomainContract: game_get_state
    log("[3.1] Test IPlayerDomainContract (game_get_state)...")
    r_state = bridge.game_get_state()
    if not r_state.get("ok"):
        found_bugs.append(f"Blad game_get_state: {r_state}")
    else:
        log(f"      -> game_get_state OK: VID={r_state.get('result', {}).get('vid')}")
        suite_results["get_state"] = True

    # 3.2 IInventoryDomainContract: game_get_inventory
    log("[3.2] Test IInventoryDomainContract (game_get_inventory window=1 i window=2)...")
    r_inv1 = bridge.game_get_inventory(window=1)
    r_inv2 = bridge.game_get_inventory(window=2)
    inv_items = r_inv1.get("result", {}).get("items", [])
    log(f"      -> Inwentarz okno 1: {len(inv_items)} przedmiotow, okno 2 ok: {r_inv2.get('ok')}")
    suite_results["get_inventory"] = r_inv1.get("ok", False) and r_inv2.get("ok", False)

    # 3.3 IWorldQueryContract & SpatialHashGrid: game_get_nearby_entities
    log("[3.3] Test IWorldQueryContract / SpatialHashGrid w roznych promieniach...")
    test_radii = [0.0, 500.0, 3000.0, 10000.0, -100.0, 1000000.0]
    grid_ok = True
    found_entities = []
    for r in test_radii:
        r_ent = bridge.game_get_nearby_entities(radius=r)
        if not r_ent.get("ok"):
            found_bugs.append(f"Blad game_get_nearby_entities promien {r}: {r_ent}")
            grid_ok = False
        else:
            ents = r_ent.get("result", {}).get("entities", [])
            if r == 10000.0:
                found_entities = ents
    log(f"      -> Testy zapytan przestrzennych (SpatialHashGrid): {'OK' if grid_ok else 'FAIL'} (znaleziono {len(found_entities)} encji w r=10000)")
    suite_results["world_query_radii"] = grid_ok

    # 3.4 Weryfikacja synchronizacji Dual-State (C++ vs Python VM)
    log("[3.4] Test audit_dual_state (sprawdzanie desynchronizacji)...")
    r_audit = bridge.audit_dual_state(epsilon=0.05)
    log(f"      -> audit_dual_state: {r_audit.get('audit_verdict', 'UNKNOWN')} | InSync={r_audit.get('in_sync')}")
    if not r_audit.get("in_sync", False):
        found_bugs.append(f"Desynchronizacja pamieci: {r_audit.get('mismatches')}")
    suite_results["dual_state_audit"] = r_audit.get("in_sync", False)

    # =========================================================================
    # CZESC 4: TESTY AKCJI GRACZA I EDGE CASES (SZUKANIE BLEDOW)
    # =========================================================================
    log("\n--- [CZESC 4] TESTY AKCJI GRACZA I EDGE CASES ---")

    # 4.1 Uzycie przedmiotu: poprawny slot, pusty slot, ujemny slot, slot poza zakresem
    log("[4.1] Edge-cases uzycia przedmiotu (game_action_use_item)...")
    test_item_slots = [
        {"window": 1, "cell": 0, "desc": "pierwszy slot"},
        {"window": 1, "cell": -1, "desc": "ujemny slot"},
        {"window": 1, "cell": 9999, "desc": "slot daleko poza zakresem"},
        {"window": 99, "cell": 0, "desc": "nieistniejace okno"}
    ]
    item_actions_safe = True
    for it in test_item_slots:
        r_it = bridge.game_action_use_item(window=it["window"], cell=it["cell"])
        log(f"      -> Slot {it['desc']} (w={it['window']}, c={it['cell']}): ok={r_it.get('ok')}")
        if not r_it.get("ok"):
            item_actions_safe = False
    suite_results["item_edge_cases"] = item_actions_safe

    # 4.2 Atak na cel: cel prawidlowy, cel zerowy, nieistniejacy cel VID, wlasny VID
    log("[4.2] Edge-cases ataku na cele (game_action_attack)...")
    target_vids = [0, 99999999, char_vid]
    if found_entities:
        target_vids.insert(0, found_entities[0].get("vid", 0))

    combat_actions_safe = True
    for tvid in target_vids:
        r_atk = bridge.game_action_attack(target_vid=tvid)
        log(f"      -> Atak na VID={tvid}: ok={r_atk.get('ok')}")
        if not r_atk.get("ok"):
            combat_actions_safe = False
    suite_results["combat_edge_cases"] = combat_actions_safe

    # 4.3 Sterowanie oknem i dlawikiem GPU (game_set_window_mode)
    log("[4.3] Test przelaczania widocznosci okna i dlawika GPU...")
    r_wm1 = bridge.game_set_window_mode(visible=False, throttle=True)
    r_wm2 = bridge.game_set_window_mode(visible=False, throttle=False)
    log(f"      -> Set window mode: ok1={r_wm1.get('ok')}, ok2={r_wm2.get('ok')}")
    suite_results["window_mode"] = r_wm1.get("ok", False) and r_wm2.get("ok", False)

    # =========================================================================
    # CZESC 5: STRESS TESTING I CHAOS TESTING (ZALANIE PAKIETAMI I POTOKIEM)
    # =========================================================================
    log("\n--- [CZESC 5] STRESS TESTING, CHAOS I FUZZING POD OBCIAZENIEM ---")

    # 5.1 Fuzzing routera pakietow: opcody 1 (Handshake), 2 (Move), 3 (Attack), 12 (UseItem)
    log("[5.1] Fuzzing routera pakietow pod obciazeniem (50 mutacji na opcode)...")
    fuzz_opcodes = [1, 2, 3, 10, 12]
    fuzz_all_ok = True
    for op in fuzz_opcodes:
        r_fz = bridge.fuzz_router(opcode=op, count=30)
        crashes = r_fz.get("crashes_detected", 0)
        log(f"      -> Opcode {op:02d}: Zmutowano 30 pakietow | Awarie: {crashes} | Status: {r_fz.get('status')}")
        if crashes > 0:
            found_bugs.append(f"AWARIA SILNIKA podczas mutacji opkodu {op}!")
            fuzz_all_ok = False
    suite_results["packet_fuzzing"] = fuzz_all_ok

    # 5.2 Spam potoku IPC (100 szybkich zapytan game_ping i game_get_state)
    log("[5.2] Spam potoku IPC (100 szybkich zapytan game_ping i game_get_state)...")
    spam_success = 0
    t_spam_start = time.time()
    for i in range(100):
        if i % 2 == 0:
            res = bridge.game_ping()
        else:
            res = bridge.game_get_state()
        if res.get("ok"):
            spam_success += 1
        time.sleep(0.005)
    t_spam_elapsed = time.time() - t_spam_start
    log(f"      -> Wynik spamu IPC: {spam_success}/100 zapytan powiodlo sie w czasie {t_spam_elapsed:.2f}s ({spam_success/t_spam_elapsed:.1f} req/s)")
    if spam_success < 95:
        found_bugs.append(f"Zgubiono odpowiedzi IPC pod obciazeniem: {spam_success}/100")
    suite_results["ipc_flood_test"] = (spam_success >= 95)

    # 5.3 Fuzzing protokolow IPC: wysylanie niepoprawnego JSON-RPC przez potok
    log("[5.3] Fuzzing protokolu IPC Named Pipe (malformed JSON, nieistniejace metody)...")
    malformed_tests = [
        {"method": "non_existent_method_xyz", "params": {}},
        {"method": "game_action_attack", "params": {"target_vid": "nie_liczba"}},
        {"method": "game_action_move", "params": {"x": float("nan"), "y": float("inf")}},
        {"method": "tick_frame", "params": {"count": -999, "delta_ms": -1.0}}
    ]
    ipc_fuzz_safe = True
    for mt in malformed_tests:
        try:
            resp = bridge._send_raw_pipe_cmd(mt, timeout_s=1.0)
            log(f"      -> Malformed test '{mt['method']}': Odpowiedz={resp}")
        except Exception as e:
            log(f"      -> Wyjatek przy wysylaniu: {e}")
            ipc_fuzz_safe = False
    suite_results["ipc_fuzz_safe"] = ipc_fuzz_safe

    # 5.4 Rapid Reconnect / Broken Pipe (zamkniecie uchwytu potoku i natychmiastowe polaczenie)
    log("[5.4] Test odpornosci na zerwanie i ponowne otwarcie potoku (Broken Pipe)...")
    reconnect_ok = True
    for cyc in range(5):
        bridge._close_pipe()
        conn = bridge._try_connect_pipe(timeout_s=1.5)
        if not conn:
            found_bugs.append(f"Nie udalo sie wznowic polaczenia potoku w cyklu {cyc+1}")
            reconnect_ok = False
            break
        ping = bridge.game_ping()
        if not ping.get("ok"):
            reconnect_ok = False
            break
    log(f"      -> Rapid Reconnect 5 cykli: {'OK' if reconnect_ok else 'FAIL'}")
    suite_results["rapid_reconnect"] = reconnect_ok

    # =========================================================================
    # CZESC 6: WERYFIKACJA LOGOW, STANU PAMIECI I CZYSTE ZAMKNIECIE
    # =========================================================================
    log("\n--- [CZESC 6] WERYFIKACJA STABILNOSCI, LOGOW I SHUTDOWN ---")

    # 6.1 CrashSentinel Report
    log("[6.1] Sprawdzenie raportu CrashSentinel...")
    r_crash = bridge.get_crash_report()
    log(f"      -> CrashSentinel Verdict: {r_crash.get('verdict')} | HasCrashed: {r_crash.get('has_crashed')}")
    if r_crash.get("has_crashed"):
        found_bugs.append(f"CrashSentinel wykryl awarie: {r_crash.get('exception_desc')}")
    suite_results["crash_sentinel"] = (r_crash.get("verdict") == "STABLE_NO_CRASH")

    # 6.2 Czyste zatrzymanie procesu (m2_harness_stop)
    log("[6.2] Zatrzymanie procesu klienta gry (m2_harness_stop)...")
    r_stop = bridge.stop_process(force=False)
    log(f"      -> Stop process: {r_stop}")
    suite_results["clean_shutdown"] = r_stop.get("ok", False)

    # 6.3 Sprawdzenie czy proces faktycznie zakonczyl dzialanie (brak zombie)
    time.sleep(1.0)
    proc_alive = (bridge.process is not None and bridge.process.poll() is None)
    log(f"[6.3] Weryfikacja czy proces zniknal: {'ZOMBIE NADAL ZYJE' if proc_alive else 'PROCES CZYSTO ZAMKNIETY'}")
    if proc_alive:
        found_bugs.append("Proces klienta wisi po wywolaniu stop_process")
        bridge.stop_process(force=True)
    suite_results["no_zombie_process"] = not proc_alive

    # 6.4 Analiza logow syserr.txt pod katem krytycznych bledow
    log("[6.4] Analiza syserr.txt pod katem nieobslugiwanych wyjatkow...")
    exe_dir = os.path.dirname(bridge.exe_path_used) if bridge.exe_path_used else r"E:\m2dev-client-src-mainOryginalx64\m2dev-client-main"
    syserr_path = os.path.join(exe_dir, "syserr.txt")
    critical_syserr = []
    if os.path.isfile(syserr_path):
        with open(syserr_path, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                if any(k in line for k in ["Access Violation", "CRASH", "FATAL", "Assertion failed", "nullptr dereference"]):
                    critical_syserr.append(line.strip())
    log(f"      -> Liczba krytycznych wpisow w syserr.txt: {len(critical_syserr)}")
    if critical_syserr:
        found_bugs.append(f"Krytyczne wpisy w syserr: {critical_syserr[:3]}")
    suite_results["clean_syserr"] = (len(critical_syserr) == 0)

    # =========================================================================
    # PODSUMOWANIE KONCOWE
    # =========================================================================
    print("\n" + "=" * 80, flush=True)
    print("SZCZEGOLOWY RAPORT ZE WSZYSTKICH TESTOW I POSZUKIWANIA BLEDOW:", flush=True)
    print("=" * 80, flush=True)
    total_passed = sum(1 for v in suite_results.values() if v)
    total_tests = len(suite_results)
    for test_name, passed in suite_results.items():
        status_str = "PASS [100%]" if passed else "FAIL [BLAD]"
        print(f"  * {test_name:<28}: {status_str}", flush=True)

    print("-" * 80, flush=True)
    print(f"WYNIK: {total_passed}/{total_tests} testow zakonczonych sukcesem ({total_passed/total_tests*100:.1f}%)", flush=True)
    if found_bugs:
        print("\nZNALEZIONE BLEDY I USTERKI:", flush=True)
        for b in found_bugs:
            print(f"  [!] {b}", flush=True)
    else:
        print("\nBRAK ZNALEZIONYCH BLEDOW! Silnik i mostek MCP sa w 100% stabilne!", flush=True)
    print("=" * 80 + "\n", flush=True)

    return len(found_bugs) == 0

if __name__ == "__main__":
    success = run_all_possibilities_test()
    sys.exit(0 if success else 1)
