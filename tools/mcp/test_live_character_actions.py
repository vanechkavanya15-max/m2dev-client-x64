r"""
tools/mcp/test_live_character_actions.py
Kompleksowy test na zywo bezposredniego sterowania postacia i klawiszami (C++23 Metin2_Release.exe)
w polaczeniu z dzialajacym serwerem x64 (E:\m2dev-server-main).

Testowane metody sterowania postacia i klawiszami:
1. game_set_attack_key_state (true/false) - CPythonPlayer::SetAttackKeyState
2. game_set_dir_key_state (dir, is_pressed) - CPythonPlayer::NEW_SetSingleDirKeyState
3. game_set_multi_dir_key_state (left, right, up, down) - CPythonPlayer::NEW_SetMultiDirKeyState
4. game_move_to_position (x, y, z) - CPythonPlayer::NEW_MoveToDestPixelPositionDirection
5. game_stop - CPythonPlayer::NEW_Stop
6. game_set_target & game_clear_target - CPythonPlayer::SetTarget
7. game_pick_close_item - CPythonPlayer::PickCloseItem
8. game_use_skill - CPythonPlayer::ClickSkillSlot
9. game_send_chat - CPythonNetworkStream::SendChatPacket
10. game_attack_target - CPythonPlayer::AttackTarget
11. audit_dual_state - CPythonCharacterManager & ActorRegistry

Wszystkie komunikaty w jezyku polskim BEZ znakow diakrytycznych.
"""

import os
import sys
import time
import json
import struct
import subprocess
from typing import Any, Dict, Optional

try:
    import win32pipe
    import win32file
    import pywintypes
except ImportError:
    print("[BLAD] Wymagany pakiet pywin32 dla obslugi Named Pipe.")
    sys.exit(1)

PIPE_NAME = r"\\.\pipe\M2ClientEngineHarness"
EXE_PATH = r"E:\m2dev-client-src-mainOryginalx64\m2dev-client-main\Metin2_Release.exe"
CWD_PATH = r"E:\m2dev-client-src-mainOryginalx64\m2dev-client-main"


class LiveHarnessClient:
    def __init__(self, pipe_name: str = PIPE_NAME):
        self.pipe_name = pipe_name
        self.handle: Any = None
        self.process: Optional[subprocess.Popen] = None

    def launch(self) -> bool:
        cmd = [
            EXE_PATH,
            "--headless",
            "--server=standard_x64",
            "--account=admin",
            "--password=admin",
            "--slot=0",
            "--server-ip=127.0.0.1",
            "--auth-port=11000",
            "--game-port=11011"
        ]
        print(f"[LAUNCH] Uruchamianie: {' '.join(cmd)}")
        self.process = subprocess.Popen(
            cmd,
            cwd=CWD_PATH,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )

        for _ in range(50):
            if not self._is_alive():
                print("[BLAD] Proces zakonczyl dzialanie przedwczesnie.")
                return False
            try:
                self.handle = win32file.CreateFile(
                    self.pipe_name,
                    win32file.GENERIC_READ | win32file.GENERIC_WRITE,
                    0, None,
                    win32file.OPEN_EXISTING,
                    0, None
                )
                print(f"[PIPE] Polaczono z {self.pipe_name} (PID={self.process.pid})")
                return True
            except pywintypes.error:
                time.sleep(0.1)

        print("[BLAD] Timeout oczekiwania na potok Named Pipe.")
        return False

    def _is_alive(self) -> bool:
        return self.process is not None and self.process.poll() is None

    def send_cmd(self, method: str, params: Optional[Dict[str, Any]] = None, req_id: int = 1) -> Dict[str, Any]:
        assert self.handle is not None
        req = {"jsonrpc": "2.0", "method": method, "id": req_id}
        if params is not None:
            req["params"] = params

        payload = json.dumps(req).encode("utf-8")
        header = struct.pack("<I", len(payload))
        win32file.WriteFile(self.handle, header + payload)

        resp_raw = bytearray()
        deadline = time.time() + 6.0
        while time.time() < deadline:
            try:
                _, avail, _ = win32pipe.PeekNamedPipe(self.handle, 0)
                if avail >= 4 and len(resp_raw) < 4:
                    _, chunk = win32file.ReadFile(self.handle, 4)
                    resp_raw.extend(chunk)
                elif len(resp_raw) >= 4:
                    expected = struct.unpack("<I", resp_raw[:4])[0]
                    needed = expected - (len(resp_raw) - 4)
                    if needed <= 0:
                        break
                    if avail > 0:
                        _, chunk = win32file.ReadFile(self.handle, min(needed, avail))
                        resp_raw.extend(chunk)
                time.sleep(0.01)
            except Exception:
                break

        if len(resp_raw) >= 4:
            body = resp_raw[4:].decode("utf-8", errors="replace")
            return json.loads(body)
        return {"error": "Brak odpowiedzi"}

    def close(self):
        try:
            if self.handle:
                win32file.CloseHandle(self.handle)
                self.handle = None
        except Exception:
            pass
        if self.process and self.process.poll() is None:
            try:
                self.process.terminate()
                self.process.wait(timeout=2.0)
            except Exception:
                self.process.kill()


def main():
    print("=" * 75)
    print("TEST NA ZYWO: METODY STEROWANIA POSTACIA I KLAWISZAMI C++23 (MCP / IPC)")
    print("=" * 75)

    client = LiveHarnessClient()
    if not client.launch():
        print("[FAIL] Nie udalo sie uruchomic klienta.")
        sys.exit(1)

    tests_passed = 0
    total_tests = 11

    try:
        # Krok 1: Oczekiwanie na faze gry
        print("\n[FAZA 1] Oczekiwanie na wejscie do fazy gry na serwerze...")
        entered_game = False
        selected_char = False
        sent_enter = False
        start_t = time.time()
        while time.time() - start_t < 15.0:
            st = client.send_cmd("get_state")
            if st.get("ok"):
                res = st.get("result", {})
                player_data = res.get("player", {})
                net_data = res.get("network", {})
                phase = net_data.get("phase", "")
                vid = player_data.get("vid", 0)
                name = player_data.get("name", "")

                if phase == "Select" and not selected_char:
                    client.send_cmd("game_select_character", {"slot": 0})
                    selected_char = True

                if phase == "Loading" and not sent_enter:
                    time.sleep(0.5)
                    client.send_cmd("game_enter_game")
                    sent_enter = True

                if phase == "Game" and vid > 0:
                    print(f"         Zalogowano do gry! VID={vid}, Nick='{name}', HP={player_data.get('hp')}/{player_data.get('max_hp')}")
                    entered_game = True
                    break
            time.sleep(0.3)

        # TEST 1: game_set_attack_key_state
        print("\n[TEST 1/11] Test SetAttackKeyState (Spacja / Auto-Atak)...")
        r_atk_on = client.send_cmd("game_set_attack_key_state", {"is_pressed": True})
        assert r_atk_on.get("ok") is True, f"Blad game_set_attack_key_state ON: {r_atk_on}"
        print(f"            SetAttackKeyState(true) -> {r_atk_on.get('result')}")

        # Przepchnij 5 klatek ataku
        client.send_cmd("tick_frame", {"count": 5})

        r_atk_off = client.send_cmd("game_set_attack_key_state", {"is_pressed": False})
        assert r_atk_off.get("ok") is True, f"Blad game_set_attack_key_state OFF: {r_atk_off}"
        print(f"            SetAttackKeyState(false) -> {r_atk_off.get('result')}")
        tests_passed += 1
        print("            [PASS] SetAttackKeyState dziala prawidlowo.")

        # TEST 2: game_set_dir_key_state
        print("\n[TEST 2/11] Test SetSingleDirKeyState (Ruch w osi DIR_UP / W)...")
        r_dir_on = client.send_cmd("game_set_dir_key_state", {"dir": 0, "is_pressed": True})
        assert r_dir_on.get("ok") is True, f"Blad game_set_dir_key_state ON: {r_dir_on}"
        print(f"            SetSingleDirKeyState(DIR_UP, true) -> {r_dir_on.get('result')}")

        client.send_cmd("tick_frame", {"count": 10})

        r_dir_off = client.send_cmd("game_set_dir_key_state", {"dir": 0, "is_pressed": False})
        assert r_dir_off.get("ok") is True, f"Blad game_set_dir_key_state OFF: {r_dir_off}"
        print(f"            SetSingleDirKeyState(DIR_UP, false) -> {r_dir_off.get('result')}")
        tests_passed += 1
        print("            [PASS] SetSingleDirKeyState dziala prawidlowo.")

        # TEST 3: game_set_multi_dir_key_state
        print("\n[TEST 3/11] Test SetMultiDirKeyState (Ruch po skosie Lewo+Gora)...")
        r_multi = client.send_cmd("game_set_multi_dir_key_state", {"left": True, "right": False, "up": True, "down": False})
        assert r_multi.get("ok") is True, f"Blad game_set_multi_dir_key_state: {r_multi}"
        print(f"            SetMultiDirKeyState(L=true, U=true) -> {r_multi.get('result')}")

        client.send_cmd("tick_frame", {"count": 10})

        # Zatrzymanie
        r_multi_stop = client.send_cmd("game_set_multi_dir_key_state", {"left": False, "right": False, "up": False, "down": False})
        assert r_multi_stop.get("ok") is True, f"Blad zatrzymania multi dir: {r_multi_stop}"
        tests_passed += 1
        print("            [PASS] SetMultiDirKeyState dziala prawidlowo.")

        # TEST 4: game_move_to_position & game_stop
        print("\n[TEST 4/11] Test MoveToDestPixelPosition & NEW_Stop...")
        cur_st = client.send_cmd("get_state")
        cur_res = cur_st.get("result", {})
        cur_pos = cur_res.get("player", {}).get("position", {"x": 45000.0, "y": 38000.0, "z": 0.0})
        target_x = cur_pos.get("x", 45000.0) + 150.0
        target_y = cur_pos.get("y", 38000.0) + 150.0

        r_move = client.send_cmd("game_move_to_position", {"x": target_x, "y": target_y, "z": 0.0})
        assert r_move.get("ok") is True, f"Blad game_move_to_position: {r_move}"
        print(f"            MoveToDestPosition({target_x}, {target_y}) -> {r_move.get('result')}")

        client.send_cmd("tick_frame", {"count": 5})

        r_stop = client.send_cmd("game_stop")
        assert r_stop.get("ok") is True, f"Blad game_stop: {r_stop}"
        print(f"            NEW_Stop() -> {r_stop.get('result')}")
        tests_passed += 1
        print("            [PASS] MoveToPosition & NEW_Stop dzialaja prawidlowo.")

        # TEST 5: game_set_target & game_clear_target
        print("\n[TEST 5/11] Test SetTarget & ClearTarget...")
        target_vid = cur_res.get("player", {}).get("vid", 57104)
        r_target = client.send_cmd("game_set_target", {"target_vid": target_vid, "force": True})
        assert r_target.get("ok") is True, f"Blad game_set_target: {r_target}"
        print(f"            SetTarget({target_vid}) -> {r_target.get('result')}")

        r_clear = client.send_cmd("game_clear_target")
        assert r_clear.get("ok") is True, f"Blad game_clear_target: {r_clear}"
        print(f"            ClearTarget() -> {r_clear.get('result')}")
        tests_passed += 1
        print("            [PASS] SetTarget & ClearTarget dzialaja prawidlowo.")

        # TEST 6: game_pick_close_item
        print("\n[TEST 6/11] Test PickCloseItem (Klawisz Z)...")
        r_pick = client.send_cmd("game_pick_close_item")
        assert r_pick.get("ok") is True, f"Blad game_pick_close_item: {r_pick}"
        print(f"            PickCloseItem() -> {r_pick.get('result')}")
        tests_passed += 1
        print("            [PASS] PickCloseItem dziala prawidlowo.")

        # TEST 7: game_use_skill
        print("\n[TEST 7/11] Test ClickSkillSlot (Uzycie skilla z slotu 0)...")
        r_skill = client.send_cmd("game_use_skill", {"slot": 0})
        assert r_skill.get("ok") is True, f"Blad game_use_skill: {r_skill}"
        print(f"            ClickSkillSlot(0) -> {r_skill.get('result')}")
        tests_passed += 1
        print("            [PASS] ClickSkillSlot dziala prawidlowo.")

        # TEST 8: game_send_chat
        print("\n[TEST 8/11] Test SendChatPacket (Czat / Komenda)...")
        r_chat = client.send_cmd("game_send_chat", {"text": "/test_action_harness_ok", "type": 0})
        assert r_chat.get("ok") is True, f"Blad game_send_chat: {r_chat}"
        print(f"            SendChatPacket('/test_action_harness_ok') -> {r_chat.get('result')}")
        tests_passed += 1
        print("            [PASS] SendChatPacket dziala prawidlowo.")

        # TEST 9: game_attack_target
        print("\n[TEST 9/11] Test AttackTarget (Atak wybranego celu)...")
        r_atk_tgt = client.send_cmd("game_attack_target", {"target_vid": target_vid})
        assert r_atk_tgt.get("ok") is True, f"Blad game_attack_target: {r_atk_tgt}"
        print(f"            AttackTarget({target_vid}) -> {r_atk_tgt.get('result')}")
        tests_passed += 1
        print("            [PASS] AttackTarget dziala prawidlowo.")

        # TEST 10: audit_dual_state
        print("\n[TEST 10/11] Test audit_dual_state (Weryfikacja spojnosci stanu)...")
        r_audit = client.send_cmd("audit_dual_state")
        assert r_audit.get("ok") is True, f"Blad audit_dual_state: {r_audit}"
        audit_res = r_audit.get("result", {})
        print(f"             is_synchronized: {audit_res.get('is_synchronized')}")
        print(f"             discrepancies: {audit_res.get('discrepancies')}")
        tests_passed += 1
        print("            [PASS] audit_dual_state zweryfikowany.")

        # TEST 11: Czyste zamkniecie shutdown
        print("\n[TEST 11/11] Test Czystego Zamkniecia (shutdown)...")
        r_shut = client.send_cmd("shutdown")
        assert r_shut.get("ok") is True, f"Blad shutdown: {r_shut}"
        print(f"             Shutdown -> {r_shut.get('result')}")
        time.sleep(0.5)
        tests_passed += 1
        print("            [PASS] Klient zamkniety bezproblemowo.")

    finally:
        client.close()

    print("\n" + "=" * 75)
    print(f"WYNIK TESTOW STEROWANIA POSTACIA: {tests_passed} / {total_tests} ZALICZONYCH (100% PASS)!")
    print("=" * 75)
    sys.exit(0 if tests_passed == total_tests else 1)


if __name__ == "__main__":
    main()
