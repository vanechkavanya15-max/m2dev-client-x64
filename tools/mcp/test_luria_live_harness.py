import sys
import time
import json
import struct
import subprocess
from pathlib import Path
import win32file
import pywintypes

PIPE_NAME = r"\\.\pipe\M2ClientEngineHarness"
CLIENT_DIR = Path(r"E:\m2dev-client-src-mainOryginalx64\m2dev-client-main")
EXE_PATH = CLIENT_DIR / "Metin2_Release.exe"

def test_luria():
    print("[TEST_LURIA] Uruchamianie klienta w trybie headless z profilem Luria...")
    log_out = open(CLIENT_DIR / "log_luria_stdout.txt", "w", encoding="utf-8", errors="ignore")
    log_err = open(CLIENT_DIR / "log_luria_stderr.txt", "w", encoding="utf-8", errors="ignore")
    proc = subprocess.Popen(
        [str(EXE_PATH), "--headless", "--server=luria"],
        cwd=str(CLIENT_DIR),
        stdout=log_out,
        stderr=log_err
    )
    print(f"[TEST_LURIA] PID: {proc.pid}. Oczekiwanie na IPC Pipe...")

    h_pipe = None
    t0 = time.time()
    while time.time() - t0 < 10.0:
        if proc.poll() is not None:
            print(f"[BLAD] Proces padl przed polaczeniem: {proc.returncode}")
            return
        try:
            h_pipe = win32file.CreateFile(
                PIPE_NAME,
                win32file.GENERIC_READ | win32file.GENERIC_WRITE,
                0, None,
                win32file.OPEN_EXISTING,
                0, None
            )
            print("[TEST_LURIA] Polaczono z potokiem IPC!")
            break
        except pywintypes.error:
            time.sleep(0.2)

    if not h_pipe:
        print("[BLAD] Timeout polaczenia z potokiem!")
        proc.kill()
        return

    def send_rpc(method, params={}):
        req = {"jsonrpc": "2.0", "id": 1, "method": method, "params": params}
        payload = json.dumps(req).encode('utf-8')
        win32file.WriteFile(h_pipe, struct.pack("<I", len(payload)) + payload)
        hdr = win32file.ReadFile(h_pipe, 4)[1]
        msg_len = struct.unpack("<I", hdr)[0]
        body = win32file.ReadFile(h_pipe, msg_len)[1]
        return json.loads(body.decode('utf-8'))

    ping_res = send_rpc("game_ping")
    print("[TEST_LURIA] Ping:", ping_res)

    print("[TEST_LURIA] Wywolywanie game_login dla Luria CH4...")
    login_res = send_rpc("game_login", {
        "account": "loginek60",
        "password": "!a123123",
        "pin": "1544",
        "server_ip": "46.105.73.82",
        "auth_port": 11469,
        "game_port": 29301,
        "slot": 0
    })
    print("[TEST_LURIA] Wynik game_login:", login_res)

    print("[TEST_LURIA] Obserwacja cyklu logowania (12 sekund)...")
    for step in range(12):
        time.sleep(1.0)
        st = send_rpc("get_engine_state")
        res = st.get("result", {})
        net = res.get("network", {})
        phase = net.get("phase", "Unknown")
        plr = res.get("player", {})
        vid = plr.get("vid", 0)
        name = plr.get("name", "")
        print(f"  Step {step+1:02d}: Faza={phase} | VID={vid} | Name='{name}' | Raw={json.dumps(res.get('network', {}))}")

    print("[TEST_LURIA] Zamykanie procesu...")
    send_rpc("shutdown", {})
    proc.wait(timeout=5.0)
    print("[TEST_LURIA] Zakonczono pomyslnie.")

if __name__ == "__main__":
    test_luria()
