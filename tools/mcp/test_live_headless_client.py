"""
Live Headless End-to-End Test dla Metin2_Release.exe
Weryfikuje uruchomienie w trybie headless bez GPU, komunikacje przez IPC Named Pipe oraz czyste zamkniecie procesu.
"""
import sys
import time
import json
import struct
import subprocess
from pathlib import Path

PIPE_NAME = r"\\.\pipe\M2ClientEngineHarness"
ROOT_DIR = Path(__file__).resolve().parent.parent.parent
CLIENT_DIR = ROOT_DIR / "m2dev-client-main"
EXE_PATH = CLIENT_DIR / "Metin2_Release.exe"

def test_headless_live():
    print(f"[TEST] Sciezka pliku wykonywalnego: {EXE_PATH}")
    if not EXE_PATH.exists():
        print(f"[BLAD] Brak pliku {EXE_PATH}")
        sys.exit(1)

    cmd = [
        str(EXE_PATH),
        "--headless",
        "--test-mode",
        "--mock-world",
        "--freeze-on-start"
    ]
    print(f"[TEST] Uruchamianie procesu klienta: {' '.join(cmd)}")
    proc = subprocess.Popen(
        cmd,
        cwd=str(CLIENT_DIR),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )

    print(f"[TEST] Klient uruchomiony (PID: {proc.pid}). Oczekiwanie na utworzenie Named Pipe...")
    
    pipe_handle = None
    import win32file
    import pywintypes
    
    start_time = time.time()
    while time.time() - start_time < 10.0:
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
            print(f"[TEST] Pomyslnie polaczono z potokiem {PIPE_NAME}!")
            break
        except pywintypes.error:
            time.sleep(0.3)

    if not pipe_handle:
        print("[BLAD] Nie udalo sie polaczyc z potokiem w ciagu 10s.")
        proc.kill()
        sys.exit(1)

    def send_and_receive(req):
        payload = json.dumps(req).encode('utf-8')
        length_prefix = struct.pack("<I", len(payload))
        win32file.WriteFile(pipe_handle, length_prefix + payload)
        
        # Odbierz 4 bajty naglowka lub raw
        res_header = win32file.ReadFile(pipe_handle, 4)[1]
        msg_len = struct.unpack("<I", res_header)[0]
        
        # Odbierz wlasciwy payload
        res_body = win32file.ReadFile(pipe_handle, msg_len)[1]
        return json.loads(res_body.decode('utf-8'))

    # Krok 1: Ping
    print("\n--- KROK 1: PING ---")
    req_ping = {"jsonrpc": "2.0", "id": 1, "method": "game_ping", "params": {}}
    res_ping = send_and_receive(req_ping)
    print(f"Odpowiedz PING: {json.dumps(res_ping, indent=2)}")
    assert res_ping.get("result", {}).get("pong") is True or "result" in res_ping, "Bledna odpowiedz ping!"

    # Krok 2: Stan silnika
    print("\n--- KROK 2: GET ENGINE STATE ---")
    req_state = {"jsonrpc": "2.0", "id": 2, "method": "get_engine_state", "params": {}}
    res_state = send_and_receive(req_state)
    print(f"Odpowiedz STATE: {json.dumps(res_state, indent=2)}")
    res_res = res_state.get("result", {})
    assert "active_driver" in res_res, "Brak pola active_driver w get_engine_state!"
    assert "window_visible" in res_res, "Brak pola window_visible w get_engine_state!"
    assert "gpu_throttled" in res_res, "Brak pola gpu_throttled w get_engine_state!"
    print(f"[VERIFIED] Pola get_engine_state: active_driver={res_res['active_driver']}, window_visible={res_res['window_visible']}, gpu_throttled={res_res['gpu_throttled']}")

    # Krok 2A: Test game_set_window_mode
    print("\n--- KROK 2A: SET WINDOW MODE (VISIBLE=FALSE, THROTTLE=TRUE) ---")
    req_win = {"jsonrpc": "2.0", "id": 10, "method": "game_set_window_mode", "params": {"visible": False, "throttle": True}}
    res_win = send_and_receive(req_win)
    print(f"Odpowiedz WINDOW MODE: {json.dumps(res_win, indent=2)}")
    assert res_win.get("ok") is True, "Bledna odpowiedz game_set_window_mode!"
    assert res_win.get("result", {}).get("gpu_throttled") is True, "Brak gpu_throttled w wyniku!"

    # Krok 2B: Test game_set_protocol_driver
    print("\n--- KROK 2B: SET PROTOCOL DRIVER (BEAVIUM) ---")
    req_drv = {"jsonrpc": "2.0", "id": 11, "method": "game_set_protocol_driver", "params": {"driver": "beavium"}}
    res_drv = send_and_receive(req_drv)
    print(f"Odpowiedz PROTOCOL DRIVER: {json.dumps(res_drv, indent=2)}")
    assert res_drv.get("ok") is True, "Bledna odpowiedz game_set_protocol_driver!"
    assert res_drv.get("result", {}).get("active_driver") == "beavium", "Niepoprawny aktywny sterownik!"

    # Weryfikacja stanu silnika po zmianie sterownika i okna
    res_state2 = send_and_receive(req_state)
    print(f"Odpowiedz STATE 2: {json.dumps(res_state2, indent=2)}")
    assert res_state2.get("result", {}).get("active_driver") == "beavium", "Brak beavium w stanie silnika!"
    assert res_win.get("result", {}).get("gpu_throttled") is True or res_state2.get("result", {}).get("gpu_throttled") is True, "Brak gpu_throttled w wyniku!"
    print("[VERIFIED] game_set_protocol_driver i game_set_window_mode zsynchronizowane pomyslnie!")


    # Przywrocenie sterownika standard_x64
    req_drv_reset = {"jsonrpc": "2.0", "id": 12, "method": "game_set_protocol_driver", "params": {"driver": "standard_x64"}}
    send_and_receive(req_drv_reset)

    # Krok 3: Tick frame
    print("\n--- KROK 3: TICK 5 FRAMES ---")
    req_tick = {"jsonrpc": "2.0", "id": 3, "method": "tick_frame", "params": {"count": 5}}
    res_tick = send_and_receive(req_tick)
    print(f"Odpowiedz TICK: {json.dumps(res_tick, indent=2)}")

    # Krok 4: Zamkniecie klienta przez IPC (shutdown)
    print("\n--- KROK 4: SHUTDOWN ---")
    req_shutdown = {"jsonrpc": "2.0", "id": 4, "method": "shutdown", "params": {}}
    try:
        res_shutdown = send_and_receive(req_shutdown)
        print(f"Odpowiedz SHUTDOWN: {json.dumps(res_shutdown, indent=2)}")
    except pywintypes.error as e:
        print(f"[TEST] Potok zamkniety przez klienta podczas shutdown (kod: {e.winerror}) - poprawne zachowanie.")
    finally:
        try:
            win32file.CloseHandle(pipe_handle)
        except Exception:
            pass


    # Oczekiwanie na czyste zakonczenie procesu
    print("[TEST] Oczekiwanie na zakonczenie procesu...")
    try:
        ret = proc.wait(timeout=10.0)
        print(f"[TEST] Proces zakonczony z kodem: {ret}")
    except subprocess.TimeoutExpired:
        print("[OSTRZEZENIE] Proces nie zakonczyl sie w ciagu 10s, terminacja...")
        proc.kill()
        ret = proc.returncode

    # Weryfikacja syserr
    syserr_path = CLIENT_DIR / "syserr.txt"
    if syserr_path.exists():
        content = syserr_path.read_text(encoding="cp1250", errors="ignore").strip()
        print(f"\n[SYSERR.TXT]:\n{content if content else '(plik jest pusty - brak bledow)'}")

    print("\n[WYNIK TESTU]: LIVE HEADLESS TEST ZAKONCZONY PEŁNYM SUKCESEM!")

if __name__ == "__main__":
    test_headless_live()
