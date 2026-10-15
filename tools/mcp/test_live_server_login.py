r"""
Test polaczenia na zywo klienta z serwerem standard_x64 (E:\m2dev-server-main).
Loguje sie na konto admin / admin, wchodzi do gry postacia [SA]Admin i testuje metody MCP.
"""

import sys
import time
import json
import struct
import subprocess
from pathlib import Path

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(line_buffering=True)

PIPE_NAME = r"\\.\pipe\M2ClientEngineHarness"
CLIENT_DIR = Path(r"E:\m2dev-client-src-mainOryginalx64\m2dev-client-main")
EXE_PATH = CLIENT_DIR / "Metin2_Release.exe"

def test_live_login():
    print(f"[TEST LIVE] Uruchamianie klienta: {EXE_PATH}")
    if not EXE_PATH.exists():
        print(f"[BLAD] Brak pliku {EXE_PATH}")
        sys.exit(1)

    # Uruchamiamy w trybie headless z flaga serwera standard_x64
    cmd = [
        str(EXE_PATH),
        "--headless",
        "--server=standard_x64"
    ]
    print(f"[TEST LIVE] Komenda: {' '.join(cmd)}")
    log_out = open(CLIENT_DIR / "log_live_stdout.txt", "w", encoding="utf-8", errors="ignore")
    log_err = open(CLIENT_DIR / "log_live_stderr.txt", "w", encoding="utf-8", errors="ignore")
    proc = subprocess.Popen(
        cmd,
        cwd=str(CLIENT_DIR),
        stdout=log_out,
        stderr=log_err
    )

    print(f"[TEST LIVE] Klient uruchomiony (PID: {proc.pid}). Oczekiwanie na Named Pipe...")

    import win32file
    import pywintypes

    pipe_handle = None
    start_time = time.time()
    while time.time() - start_time < 12.0:
        if proc.poll() is not None:
            print(f"[BLAD] Proces zakonczyl sie przedwczesnie z kodem: {proc.returncode}")
            stdout, stderr = proc.communicate()
            print(f"STDOUT: {stdout.decode('cp1250', errors='ignore')}")
            print(f"STDERR: {stderr.decode('cp1250', errors='ignore')}")
            sys.exit(1)
        try:
            pipe_handle = win32file.CreateFile(
                PIPE_NAME,
                win32file.GENERIC_READ | win32file.GENERIC_WRITE,
                0,
                None,
                win32file.OPEN_EXISTING,
                0,
                None
            )
            print(f"[TEST LIVE] Polaczono z potokiem {PIPE_NAME}!")
            break
        except pywintypes.error:
            time.sleep(0.3)

    if not pipe_handle:
        print("[BLAD] Nie udalo sie polaczyc z potokiem IPC klienta.")
        proc.kill()
        sys.exit(1)

    def send_rpc(req):
        payload = json.dumps(req).encode('utf-8')
        length_prefix = struct.pack("<I", len(payload))
        win32file.WriteFile(pipe_handle, length_prefix + payload)
        res_header = win32file.ReadFile(pipe_handle, 4)[1]
        msg_len = struct.unpack("<I", res_header)[0]
        res_body = win32file.ReadFile(pipe_handle, msg_len)[1]
        return json.loads(res_body.decode('utf-8'))

    # Krok 1: Ping
    print("\n--- TEST 1: PING ---")
    res = send_rpc({"jsonrpc": "2.0", "id": 1, "method": "game_ping", "params": {}})
    print(f"PING: {res}")

    # Krok 2: Monitorowanie fazy sieciowej i wejscie do gry
    print("\n--- TEST 2: MONITOROWANIE FAZY LOGOWANIA I WEJSCIA DO GRY ---")
    reached_game = False
    last_phase = ""
    selected_character = False
    login_sent = False

    for attempt in range(80):
        time.sleep(0.5)
        state_res = send_rpc({"jsonrpc": "2.0", "id": 100 + attempt, "method": "get_engine_state", "params": {}})
        res_data = state_res.get("result", {})
        phase = res_data.get("network", {}).get("phase", "Unknown")
        player_vid = res_data.get("player", {}).get("vid", 0)
        player_name = res_data.get("player", {}).get("name", "")

        if phase != last_phase or attempt % 4 == 0:
            print(f"  [proba {attempt+1:02d}] Faza: '{phase}' | Gracz VID: {player_vid} ('{player_name}')")
            last_phase = phase

        # Jesli po 3 sekundach klient nadal jest OffLine, wyzwalamy game_login
        if attempt >= 6 and phase == "OffLine" and not login_sent:
            print("  -> Klient w stanie OffLine. Wywolywanie explicit game_login...")
            login_payload = {
                "jsonrpc": "2.0",
                "id": 2,
                "method": "game_login",
                "params": {
                    "account": "admin",
                    "password": "admin",
                    "slot": 0,
                    "server_ip": "127.0.0.1",
                    "auth_port": 11000,
                    "game_port": 11011
                }
            }
            login_res = send_rpc(login_payload)
            print(f"  -> GAME_LOGIN RES: {login_res}")
            login_sent = True

        # Jesli faza to Select i jeszcze nie wybrano postaci
        if phase == "Select" and not selected_character:
            print("  -> Osiagnieto faze Select! Wywolywanie game_select_character(slot=0)...")
            sel_res = send_rpc({"jsonrpc": "2.0", "id": 50, "method": "game_select_character", "params": {"slot": 0}})
            print(f"  -> GAME_SELECT_CHARACTER RES: {sel_res}")
            selected_character = True

        # Gdy jestesmy w grze i postac jest zaladowana
        if phase == "Game":
            reached_game = True
            print(f"\n[SUKCES] Osiagnieto pelna faze gry ({phase}) z zaladowana postacia VID={player_vid} ('{player_name}') w probie {attempt+1}!")
            time.sleep(1.0)
            break

    # Krok 3: Pobranie stanu gracza i swiata
    print("\n--- TEST 3: STAN GRACZA W SWIECIE GRY ---")
    state_final = send_rpc({"jsonrpc": "2.0", "id": 200, "method": "get_engine_state", "params": {}})
    player_data = state_final.get("result", {}).get("player", {})
    print(f"  Gracz: VID={player_data.get('vid')}, Nazwa='{player_data.get('name')}', Pozycja={player_data.get('position')}")
    print(f"  Status: HP={player_data.get('hp')}/{player_data.get('max_hp')}, SP={player_data.get('sp')}/{player_data.get('max_sp')}, Poziom={player_data.get('level')}")

    # Krok 4: Test pobrania encji w poblizu
    print("\n--- TEST 4: POBRANIE ENTITIES W POBLIZU ---")
    nearby_res = send_rpc({"jsonrpc": "2.0", "id": 300, "method": "game_get_nearby_entities", "params": {"radius": 5000.0}})
    print(f"  Wynik pobliskich encji: {json.dumps(nearby_res, indent=2)[:500]}")

    # Krok 5: Test pobrania ekwipunku
    print("\n--- TEST 5: POBRANIE EKWIPUNKU ---")
    inv_res = send_rpc({"jsonrpc": "2.0", "id": 400, "method": "game_get_inventory", "params": {}})
    print(f"  Wynik ekwipunku: {json.dumps(inv_res, indent=2)[:500]}")

    # Krok 6: Test audytu stanu
    print("\n--- TEST 6: AUDYT DUAL-STATE C++ / PYTHON ---")
    audit_res = send_rpc({"jsonrpc": "2.0", "id": 500, "method": "audit_dual_state", "params": {}})
    print(f"  Wynik audytu stanu: {json.dumps(audit_res, indent=2)}")

    # Krok 7: Zamkniecie klienta
    print("\n--- TEST 7: CZYSTE ZAMKNIECIE KLIENTA ---")
    send_rpc({"jsonrpc": "2.0", "id": 999, "method": "shutdown", "params": {}})
    win32file.CloseHandle(pipe_handle)
    proc.wait(timeout=5.0)
    log_out.close()
    log_err.close()
    print(f"[TEST LIVE] Klient zamkniety pomyslnie z kodem: {proc.returncode}")

if __name__ == "__main__":
    test_live_login()
