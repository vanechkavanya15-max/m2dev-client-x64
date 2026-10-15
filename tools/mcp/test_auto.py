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

def run_test():
    print(f"[TEST] Uruchamianie: {EXE_PATH}")
    log_out = open(CLIENT_DIR / "log_client_stdout.txt", "w", encoding="utf-8", errors="ignore")
    log_err = open(CLIENT_DIR / "log_client_stderr.txt", "w", encoding="utf-8", errors="ignore")
    proc = subprocess.Popen(
        [str(EXE_PATH), "--headless", "--server=standard_x64"],
        cwd=str(CLIENT_DIR),
        stdout=log_out,
        stderr=log_err
    )
    print(f"[TEST] PID: {proc.pid}. Oczekiwanie na IPC Pipe...")

    h_pipe = None
    t0 = time.time()
    while time.time() - t0 < 10.0:
        if proc.poll() is not None:
            print(f"[BLAD] Proces padl: {proc.returncode}")
            out, err = proc.communicate()
            print("OUT:", out.decode("cp1250", errors="ignore"))
            print("ERR:", err.decode("cp1250", errors="ignore"))
            return
        try:
            h_pipe = win32file.CreateFile(
                PIPE_NAME,
                win32file.GENERIC_READ | win32file.GENERIC_WRITE,
                0, None,
                win32file.OPEN_EXISTING,
                0, None
            )
            print("[TEST] Polaczono z potokiem!")
            break
        except pywintypes.error:
            time.sleep(0.2)

    if not h_pipe:
        print("[BLAD] Nie udalo sie polaczyc z potokiem!")
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

    # Ping
    ping_res = send_rpc("game_ping")
    print("PING:", ping_res)

    print("\n[TEST] Obserwacja cyklu logowania i fazy gry...")
    reached_game = False
    selected_char = False
    sent_enter = False
    for step in range(10):
        time.sleep(0.5)
        st = send_rpc("get_engine_state")
        res = st.get("result", {})
        net = res.get("network", {})
        phase = net.get("phase", "Unknown")
        plr = res.get("player", {})
        vid = plr.get("vid", 0)
        name = plr.get("name", "")
        hp = plr.get("hp", 0)
        max_hp = plr.get("max_hp", 0)
        pos = plr.get("position", {})

        print(f"[{step+1:02d}] Faza: {phase} | VID: {vid} | Gracz: '{name}' | HP: {hp}/{max_hp} | Pos: ({pos.get('x',0):.1f}, {pos.get('y',0):.1f})")

        if phase == "Select" and not selected_char:
            print("  -> [MCP] Wykryto faze Select! Wywolywanie game_select_character(slot=0)...")
            sel_res = send_rpc("game_select_character", {"slot": 0})
            print(f"  -> [MCP] Wynik game_select_character: {sel_res}")
            selected_char = True

        if phase == "Loading":
            if not sent_enter:
                time.sleep(0.5)
                print("  -> [MCP] Wykryto faze Loading! Wywolywanie game_enter_game()...")
                enter_res = send_rpc("game_enter_game")
                print(f"  -> [MCP] Wynik game_enter_game: {enter_res}")
                if enter_res.get("ok"):
                    sent_enter = True
            elif hp > 0:
                print("  -> [MCP] W fazie Loading z HP > 0, ponawianie game_enter_game()...")
                enter_res = send_rpc("game_enter_game")
                print(f"  -> [MCP] Wynik game_enter_game: {enter_res}")

        if phase == "Game" and vid > 0:
            reached_game = True
            print(f"\n>>> [SUKCES] OSIAGNIETO FAZE GRY! Postac: {name} (VID: {vid}) <<<")
            break

    if reached_game:
        print("\n--- TEST METOD MCP W GRZE ---")
        
        # 1. game_get_state
        state_mcp = send_rpc("game_get_state")
        print("\n[MCP] game_get_state:")
        print(json.dumps(state_mcp, indent=2))

        # 2. game_get_inventory
        inv_mcp = send_rpc("game_get_inventory", {"window": 1})
        print("\n[MCP] game_get_inventory:")
        print(json.dumps(inv_mcp, indent=2)[:800])

        # 3. game_get_nearby_entities
        nearby_mcp = send_rpc("game_get_nearby_entities", {"radius": 5000.0})
        print("\n[MCP] game_get_nearby_entities:")
        print(json.dumps(nearby_mcp, indent=2)[:800])

        # 4. audit_dual_state
        audit_mcp = send_rpc("audit_dual_state")
        print("\n[MCP] audit_dual_state:")
        print(json.dumps(audit_mcp, indent=2))

    send_rpc("shutdown")
    win32file.CloseHandle(h_pipe)
    proc.wait(timeout=5.0)
    log_out.close()
    log_err.close()
    print(f"\n[TEST] Klient zakonczony, exit code: {proc.returncode}")

if __name__ == "__main__":
    run_test()
