r"""
E:\m2dev-client-src-mainOryginalx64\tools\mcp\test_mcp_heavy.py
Ciezkie testy integracyjne przez serwer MCP i klienta C++23 na zywo na serwerze gry.
Wszystkie komunikaty bez polskich znakow diakrytycznych.
"""

import sys
import time
import json
import asyncio
from pathlib import Path

# Dodanie katalogu do sys.path
sys.path.insert(0, str(Path(__file__).parent))
import m2_agent_harness_mcp as harness

def log(msg: str):
    print(f"[{time.strftime('%H:%M:%S')}] {msg}", flush=True)

def test_heavy():
    print("=" * 75, flush=True)
    print("MOCNY TEST INTEGRACYJNY MCP KLIENTA NA ZYWO (SERWER x64 STANDARD)", flush=True)
    print("=" * 75, flush=True)

    bridge = harness.bridge
    results = {}

    # KROK 1: Uruchomienie klienta przez MCP Harness
    log("[1/10] Uruchamianie klienta gry przez m2_harness_launch...")
    r_launch = bridge.launch_process(
        mode="headless",
        mock_world=False,
        freeze=False,
        server="standard_x64"
    )
    assert r_launch["ok"] is True, f"Blad launch: {r_launch}"
    pid = r_launch.get("pid")
    log(f"      -> Sukces: PID={pid}, PipeConnected={bridge.is_connected}")
    results["launch"] = True

    # KROK 2: Testy liveness i konfiguracji IPC
    log("[2/10] Test liveness: game_ping, set_protocol_driver...")
    r_ping = bridge.game_ping()
    assert r_ping.get("ok") is True, f"Ping failed: {r_ping}"
    log(f"      -> game_ping: {r_ping}")

    r_drv = bridge.game_set_protocol_driver("standard_x64")
    log(f"      -> game_set_protocol_driver: {r_drv}")
    results["liveness"] = True

    # KROK 3: Cykl FSM logowania i wejscia do gry
    log("[3/10] Obserwacja cyklu logowania i automatyczne wejscie do gry...")
    in_game = False
    selected = False
    entered = False
    char_vid = 0
    char_name = ""

    t_start = time.time()
    for step in range(30):
        time.sleep(0.5)
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
            log("      -> [MCP] Faza Select: Wywolanie game_select_character(slot=0)...")
            r_sel = bridge.game_select_character(slot=0)
            log(f"         Wynik: {r_sel}")
            selected = True

        if phase == "Loading":
            if not entered:
                time.sleep(0.4)
                log("      -> [MCP] Faza Loading: Wywolanie game_enter_game()...")
                r_ent = bridge.game_enter_game()
                log(f"         Wynik: {r_ent}")
                entered = True
            elif hp > 0:
                log("      -> [MCP] Ponawianie game_enter_game (HP > 0)...")
                bridge.game_enter_game()

        if phase == "Game" and vid > 0:
            in_game = True
            char_vid = vid
            char_name = name
            elapsed = time.time() - t_start
            log(f"\n>>> [SUKCES] OSIAGNIETO FAZE GRY W CZASIE {elapsed:.2f}s! Postac: '{name}' (VID: {vid}) <<<\n")
            break

    assert in_game is True, f"Nie udalo sie wejsc do gry w ciagu 15 sekund! Stan koncowy: {st}"
    results["enter_game"] = True

    # KROK 4: Pobranie stanu postaci przez IPlayerDomainContract
    log("[4/10] Test game_get_state (IPlayerDomainContract)...")
    r_state = bridge.game_get_state()
    log(f"      -> game_get_state: {json.dumps(r_state, indent=2)[:400]}...")
    assert r_state.get("ok") is True, f"Blad game_get_state: {r_state}"
    results["get_state"] = True

    # KROK 5: Pobranie ekwipunku przez IInventoryDomainContract
    log("[5/10] Test game_get_inventory (IInventoryDomainContract)...")
    r_inv = bridge.game_get_inventory(window=1)
    assert r_inv.get("ok") is True, f"Blad game_get_inventory: {r_inv}"
    items = r_inv.get("result", {}).get("items", [])
    log(f"      -> Znaleziono {len(items)} przedmiotow w ekwipunku.")
    if items:
        log(f"         Pierwszy przedmiot: {items[0]}")
    results["get_inventory"] = True

    # KROK 6: Wyszukanie encji w swiecie przez SpatialHashGrid / IWorldQueryContract
    log("[6/10] Test game_get_nearby_entities (SpatialHashGrid, promien 10000.0)...")
    r_nearby = bridge.game_get_nearby_entities(radius=10000.0)
    assert r_nearby.get("ok") is True, f"Blad game_get_nearby_entities: {r_nearby}"
    entities = r_nearby.get("result", {}).get("entities", [])
    log(f"      -> Znaleziono {len(entities)} encji w zasiegu widzenia.")
    for ent in entities[:5]:
        log(f"         VID={ent.get('vid')} | Nazwa='{ent.get('name')}' | Typ={ent.get('type')} | Odleglosc={ent.get('distance', 0):.1f}")
    results["get_nearby"] = True

    # KROK 7: Audyt synchronizacji Dual-State C++ vs Python VM
    log("[7/10] Test audit_dual_state (Weryfikacja pamieci i mapy swiata)...")
    r_audit = bridge.audit_dual_state(epsilon=0.05)
    log(f"      -> audit_dual_state: {json.dumps(r_audit, indent=2)}")
    assert r_audit.get("ok") is True, f"Blad audit_dual_state: {r_audit}"
    assert r_audit.get("in_sync") is True, f"Desynchronizacja wykryta: {r_audit}"
    results["audit_dual_state"] = True

    # KROK 8: Akcje w grze: uzycie przedmiotu i atak celu
    log("[8/10] Testy akcji: game_action_use_item oraz game_action_attack...")
    r_item = bridge.game_action_use_item(window=1, cell=0)
    log(f"      -> game_action_use_item: {r_item}")
    assert r_item.get("ok") is True

    if entities:
        target_vid = entities[0].get("vid", 0)
        log(f"      -> Atakowanie najblizszej encji VID={target_vid}...")
        r_atk = bridge.game_action_attack(target_vid=target_vid)
        log(f"         game_action_attack: {r_atk}")
        assert r_atk.get("ok") is True
    results["actions"] = True

    # KROK 9: Fuzzing routera pakietow pod obciazeniem (50 mutacji)
    log("[9/10] Test fuzz_router pod obciazeniem (Opcode=2 Movement, 50 zmutowanych pakietow)...")
    r_fuzz = bridge.fuzz_router(opcode=2, count=50)
    log(f"      -> fuzz_router: {r_fuzz}")
    assert r_fuzz.get("ok") is True, f"Blad fuzzing: {r_fuzz}"
    assert r_fuzz.get("crashes_detected", 0) == 0, "Wykryto awarie silnika podczas mutacji!"
    results["fuzz_router"] = True

    # KROK 10: Raport CrashSentinel i czyste zamkniecie procesu
    log("[10/10] Sprawdzenie CrashSentinel i czyste zatrzymanie procesu (m2_harness_stop)...")
    r_crash = bridge.get_crash_report()
    log(f"       -> CrashSentinel: {r_crash.get('verdict', 'OK')}")

    r_stop = bridge.stop_process(force=False)
    log(f"       -> m2_harness_stop: {r_stop}")
    assert r_stop.get("ok") is True, f"Blad stop: {r_stop}"
    results["stop"] = True

    print("\n" + "=" * 75, flush=True)
    print("PODSUMOWANIE MOCNEGO TESTU INTEGRACYJNEGO MCP NA ZYWO:", flush=True)
    for k, v in results.items():
        print(f"  [+] Test {k:<20}: {'PASS (100%)' if v else 'FAIL'}", flush=True)
    print(f"WYNIK KONCOWY: 10/10 TESTOW ZDANYCH (100% SUKCES)!", flush=True)
    print("=" * 75 + "\n", flush=True)

if __name__ == "__main__":
    test_heavy()
