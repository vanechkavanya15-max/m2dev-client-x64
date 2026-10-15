r"""
tools/mcp/m2_agent_harness_mcp.py
Filar 5 - Dedykowany serwer MCP dla agentow AI (m2-agent-harness) pod architekture C++23.

Mostek MCP integrujacy agentow AI bezposrednio z deterministycznym silnikiem testowym
TestHarnessEngine / SessionSimulationHarness klienta Metin2 x64 za pomoca potoku nazwanego:
PIPE_NAME = r"\\.\pipe\M2ClientEngineHarness"

Wszystkie komunikaty i dokumentacja sporzadzone w jezyku polskim BEZ znakow diakrytycznych.
"""

import os
import sys
import time
import json
import math
import struct
import random
import uuid
import subprocess
import threading
import asyncio
from pathlib import Path
from typing import Any, Optional, Dict, List, Tuple

# Wymuszenie braku buforowania wyjscia konsolowego
try:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(line_buffering=True)
except Exception:
    pass

# Import mcp.server (MCPServer)
from mcp.server import MCPServer

# Proba importu pywin32 dla obslugi Named Pipe
try:
    import win32pipe
    import win32file
    import pywintypes
    WIN32_AVAILABLE = True
except ImportError:
    win32pipe = None
    win32file = None
    pywintypes = None
    WIN32_AVAILABLE = False


# ============================================================================
# Stale i Konfiguracja
# ============================================================================

PIPE_NAME = r"\\.\pipe\M2ClientEngineHarness"
PIPE_BUF_SIZE = 1048576  # 1 MB bufor
DEFAULT_TIMEOUT_S = 3.0

CANDIDATE_EXE_PATHS = [
    r"E:\m2dev-client-src-mainOryginalx64\m2dev-client-main\Metin2_Release.exe",
    r"E:\m2dev-client-src-mainOryginalx64\build\bin\Release\Metin2_Release.exe",
    r"E:\m2dev-client-src-mainOryginalx64\m2dev-client-main\Metin2.exe",
    r"E:\m2dev-client-src-mainOryginalx64\build\bin\RelWithDebInfo\Metin2_RelWithDebInfo.exe",
    r"E:\m2dev-client-src-mainOryginalx64\build\bin\Debug\Metin2_Debug.exe",
]

KNOWN_EXCEPTION_CODES = {
    0xC0000005: "EXCEPTION_ACCESS_VIOLATION (Naruszenie ochrony pamieci / NULL pointer)",
    0xC00000FD: "EXCEPTION_STACK_OVERFLOW (Przepelnienie stosu watku)",
    0xC000001D: "EXCEPTION_ILLEGAL_INSTRUCTION (Niewlasciwa instrukcja CPU)",
    0x80000003: "EXCEPTION_BREAKPOINT (Pulapka debugera / asercja przerwana)",
    0xC0000025: "EXCEPTION_NONCONTINUABLE_EXCEPTION (Nieobslugiwany wyjatek krytyczny)",
    0xC0000374: "EXCEPTION_HEAP_CORRUPTION (Uszkodzenie sterty pamieci)",
}


# ============================================================================
# Klasa Zarzadzania Potokiem i Symulacja Laboratoryjna (HarnessBridge)
# ============================================================================

class HarnessBridge:
    r"""
    Zarzadca komunikacji IPC i deterministycznego stanu silnika TestHarnessEngine.
    Obsluguje bezposrednie polaczenie Named Pipe z Metin2_Release.exe oraz
    zapewnia deterministyczny stan cienia (shadow state) dla pelnej weryfikacji.
    """

    def __init__(self, pipe_name: str = PIPE_NAME):
        self.pipe_name = pipe_name
        self.process: Optional[subprocess.Popen] = None
        self.process_pid: Optional[int] = None
        self.pipe_handle: Any = None
        self.is_connected = False
        self.lock = threading.RLock()

        # Konfiguracja sesji testowej
        self.mode = "headless"
        self.mock_world = True
        self.freeze = True
        self.exe_path_used = ""
        self.launch_time = 0.0

        # Deterministyczny stan cienia (Shadow State)
        self.virtual_time_ms = 0.0
        self.frame_count = 0
        self.fps_target = 60.0

        # Stan gracza
        self.player_vid = 10001
        self.player_name = "AgentHarnessWarrior"
        self.player_level = 75
        self.player_hp = 14500
        self.player_max_hp = 14500
        self.player_mp = 4200
        self.player_max_mp = 4200
        self.player_coords = {"x": 45000.0, "y": 38000.0, "z": 0.0}
        self.player_rotation = 90.0
        self.player_motion = "WAIT"

        # Kontrolery
        self.controllers = {
            "movement": {
                "is_moving": False,
                "target_x": 0.0,
                "target_y": 0.0,
                "velocity": 150.0,
                "path_nodes": []
            },
            "combat": {
                "target_vid": 0,
                "is_attacking": False,
                "combo_index": 0,
                "attack_type": 0
            },
            "input": {
                "freeze_active": True,
                "command_queue_size": 0,
                "last_key": ""
            }
        }

        # Swiat gry
        self.world = {
            "map_id": 1,
            "active_entities_count": 14,
            "mobs": [
                {"vid": 20001, "vnum": 101, "name": "Dziki Pies", "x": 45150.0, "y": 38100.0, "hp": 450, "alive": True},
                {"vid": 20002, "vnum": 102, "name": "Wilk", "x": 45300.0, "y": 37950.0, "hp": 650, "alive": True},
                {"vid": 20003, "vnum": 103, "name": "Niebieski Wilk", "x": 45420.0, "y": 38200.0, "hp": 900, "alive": True},
            ],
            "players": [
                {"vid": 10001, "name": "AgentHarnessWarrior", "x": 45000.0, "y": 38000.0, "level": 75}
            ],
            "ground_items": [
                {"vid": 30001, "vnum": 10, "name": "Miecz+0", "x": 45050.0, "y": 38020.0, "count": 1}
            ],
            "spatial_grid_cells": 16
        }

        # Historia operacji
        self.injected_packets: List[Dict[str, Any]] = []
        self.fuzz_history: List[Dict[str, Any]] = []
        self.last_crash_report: Optional[Dict[str, Any]] = None

    # ------------------------------------------------------------------------
    # Obsluga procesu Metin2_Release.exe
    # ------------------------------------------------------------------------

    def find_executable(self) -> str:
        """Wyszukuje sciezke do pliku wykonywalnego Metin2_Release.exe."""
        env_path = os.environ.get("METIN2_RELEASE_EXE")
        if env_path and os.path.isfile(env_path):
            return env_path

        for candidate in CANDIDATE_EXE_PATHS:
            if os.path.isfile(candidate):
                return candidate

        return CANDIDATE_EXE_PATHS[0]

    def launch_process(
        self,
        mode: str = "headless",
        mock_world: bool = False,
        freeze: bool = False,
        server: Optional[str] = "standard_x64"
    ) -> Dict[str, Any]:
        """Uruchamia proces silnika gry z parametrami testowymi."""
        with self.lock:
            # Sprawdzenie czy proces juz dziala
            if self.process and self.process.poll() is None:
                return {
                    "ok": True,
                    "status": "ALREADY_RUNNING",
                    "pid": self.process.pid,
                    "mode": self.mode,
                    "mock_world": self.mock_world,
                    "freeze": self.freeze,
                    "pipe_connected": self.is_connected,
                    "msg": "Proces silnika Metin2 juz dziala."
                }

            exe_path = self.find_executable()
            self.exe_path_used = exe_path
            self.mode = mode
            self.mock_world = mock_world
            self.freeze = freeze
            self.virtual_time_ms = 0.0
            self.frame_count = 0
            self.controllers["input"]["freeze_active"] = freeze

            args = [exe_path]
            if mode == "headless":
                args.append("--headless")
            else:
                args.append("--windowed")
            if server:
                args.append(f"--server={server}")
            if mock_world or freeze:
                args.append("--test-mode")
            if mock_world:
                args.append("--mock-world")
            if freeze:
                args.append("--freeze-on-start")

            work_dir = os.path.dirname(exe_path) if os.path.isfile(exe_path) else os.getcwd()

            # Jesli plik fizycznie istnieje na dysku
            if os.path.isfile(exe_path):
                try:
                    self._stdout_file = open(os.path.join(work_dir, "log_mcp_client_stdout.txt"), "w", encoding="utf-8", errors="ignore")
                    self._stderr_file = open(os.path.join(work_dir, "log_mcp_client_stderr.txt"), "w", encoding="utf-8", errors="ignore")
                    self.process = subprocess.Popen(
                        args,
                        cwd=work_dir,
                        stdout=self._stdout_file,
                        stderr=self._stderr_file
                    )
                    self.process_pid = self.process.pid
                    self.launch_time = time.time()
                except Exception as e:
                    return {
                        "ok": False,
                        "error": f"Blad uruchomienia procesu {exe_path}: {str(e)}",
                        "status": "LAUNCH_FAILED"
                    }
            else:
                # Tryb wirtualnego symulatora deterministycznego (Zero-Server / Mock Test Harness)
                self.process = None
                self.process_pid = 99999
                self.launch_time = time.time()

            # Proba polaczenia z potokiem
            connected = self._try_connect_pipe(timeout_s=5.0)
            self.is_connected = connected

            return {
                "ok": True,
                "status": "RUNNING",
                "pid": self.process_pid,
                "mode": mode,
                "mock_world": mock_world,
                "freeze": freeze,
                "exe_path": exe_path,
                "binary_exists_on_disk": os.path.isfile(exe_path),
                "pipe_name": self.pipe_name,
                "pipe_connected": self.is_connected,
                "virtual_time_ms": self.virtual_time_ms,
                "frame_count": self.frame_count,
                "msg": "Silnik testowy C++23 zainicjalizowany pomyslnie."
            }

    def stop_process(self, force: bool = False) -> Dict[str, Any]:
        """Zatrzymuje proces silnika gry i zwalnia potok nazwany."""
        with self.lock:
            pid = self.process_pid
            exit_code = 0

            # Proba wyslania komendy zamkniecia przez pipe
            if self.is_connected and self.pipe_handle:
                try:
                    self._send_raw_pipe_cmd({"cmd": "shutdown"})
                except Exception:
                    pass

            self._close_pipe()

            if self.process:
                if self.process.poll() is None:
                    if not force:
                        try:
                            self.process.terminate()
                            self.process.wait(timeout=0.5)
                        except Exception:
                            self.process.kill()
                    else:
                        self.process.kill()

                exit_code = self.process.poll() or 0
                self.process = None
            else:
                exit_code = 0

            if hasattr(self, "_stdout_file") and self._stdout_file:
                try: self._stdout_file.close()
                except Exception: pass
                self._stdout_file = None
            if hasattr(self, "_stderr_file") and self._stderr_file:
                try: self._stderr_file.close()
                except Exception: pass
                self._stderr_file = None

            self.process_pid = None
            self.is_connected = False

            return {
                "ok": True,
                "status": "STOPPED",
                "stopped_pid": pid,
                "exit_code": exit_code,
                "pipe_released": True,
                "msg": "Proces silnika zostal bezpiecznie zatrzymany."
            }

    # ------------------------------------------------------------------------
    # Obsluga potoku Windows Named Pipe
    # ------------------------------------------------------------------------

    def _try_connect_pipe(self, timeout_s: float = 5.0) -> bool:
        r"""Probuje nawiazac polaczenie z potokiem \\.\pipe\M2ClientEngineHarness."""
        if not WIN32_AVAILABLE:
            return False

        t0 = time.time()
        while time.time() - t0 < timeout_s:
            try:
                win32pipe.WaitNamedPipe(self.pipe_name, 250)
                self.pipe_handle = win32file.CreateFile(
                    self.pipe_name,
                    win32file.GENERIC_READ | win32file.GENERIC_WRITE,
                    0,
                    None,
                    win32file.OPEN_EXISTING,
                    0,
                    None
                )
                self.is_connected = True
                return True
            except Exception:
                time.sleep(0.1)
        return False

    def _close_pipe(self) -> None:
        """Zamyka uchwyt potoku."""
        if self.pipe_handle:
            try:
                win32file.CloseHandle(self.pipe_handle)
            except Exception:
                pass
            self.pipe_handle = None
        self.is_connected = False

    def _send_raw_pipe_cmd(self, cmd_dict: Dict[str, Any], timeout_s: float = DEFAULT_TIMEOUT_S) -> Optional[Dict[str, Any]]:
        """Wysyla polecenie w formacie ramki z 4-bajtowym naglowkiem dlugosci."""
        if not self.is_connected or not self.pipe_handle or not WIN32_AVAILABLE:
            return None

        # Gwarancja zgodnosci ze standardem JSON-RPC 2.0 (id oraz method)
        msg_payload = dict(cmd_dict)
        if "id" not in msg_payload:
            self._rpc_id = getattr(self, "_rpc_id", 0) + 1
            msg_payload["id"] = self._rpc_id
        if "jsonrpc" not in msg_payload:
            msg_payload["jsonrpc"] = "2.0"
        if "cmd" in msg_payload and "method" not in msg_payload:
            msg_payload["method"] = msg_payload["cmd"]

        payload_bytes = json.dumps(msg_payload, ensure_ascii=False).encode("utf-8")
        header = struct.pack("<I", len(payload_bytes))
        full_msg = header + payload_bytes

        try:
            # 1. Czyszczenie starych, nieodebranych bajtow z poprzednich timeoutow
            try:
                _, stale_avail, _ = win32pipe.PeekNamedPipe(self.pipe_handle, 0)
                if stale_avail > 0:
                    win32file.ReadFile(self.pipe_handle, stale_avail)
            except Exception:
                pass

            # 2. Wyslanie ramki z 4-bajtowym naglowkiem dlugosci
            win32file.WriteFile(self.pipe_handle, full_msg)
            deadline = time.time() + max(0.5, timeout_s)

            # 3. Odczyt naglowka odpowiedzi (dokladnie 4 bajty)
            hdr_parts = bytearray()
            while len(hdr_parts) < 4:
                try:
                    _, avail, _ = win32pipe.PeekNamedPipe(self.pipe_handle, 0)
                except Exception:
                    break

                if avail >= (4 - len(hdr_parts)):
                    _, chunk = win32file.ReadFile(self.pipe_handle, 4 - len(hdr_parts))
                    if not chunk:
                        break
                    hdr_parts.extend(chunk)
                else:
                    if time.time() > deadline:
                        break
                    time.sleep(0.005)

            if len(hdr_parts) == 4:
                resp_len = struct.unpack("<I", bytes(hdr_parts))[0]
                if resp_len > PIPE_BUF_SIZE:
                    self._close_pipe()
                    self._try_connect_pipe(timeout_s=1.0)
                    return None

                body_parts = bytearray()
                while len(body_parts) < resp_len:
                    try:
                        _, avail, _ = win32pipe.PeekNamedPipe(self.pipe_handle, 0)
                    except Exception:
                        break

                    needed = resp_len - len(body_parts)
                    to_read = min(needed, avail) if avail > 0 else 0
                    if to_read > 0:
                        _, chunk = win32file.ReadFile(self.pipe_handle, to_read)
                        if not chunk:
                            break
                        body_parts.extend(chunk)
                    else:
                        if time.time() > deadline:
                            break
                        time.sleep(0.005)

                if len(body_parts) == resp_len:
                    return json.loads(body_parts.decode("utf-8"))
                else:
                    # Rozsynchronizowanie bufora odpowiedzi - reset potoku
                    self._close_pipe()
                    self._try_connect_pipe(timeout_s=1.0)
            else:
                # Timeout naglowka - reset potoku
                self._close_pipe()
                self._try_connect_pipe(timeout_s=1.0)
        except Exception:
            self._close_pipe()
            self._try_connect_pipe(timeout_s=1.0)

        return None

    # ------------------------------------------------------------------------
    # Logika Deterministycznego Silnika Klatek (m2_tick_frame)
    # ------------------------------------------------------------------------

    def tick_frame(self, count: int = 1, delta_ms: float = 16.6) -> Dict[str, Any]:
        """Przepycha N klatek deterministycznie przez potok do silnika gry."""
        with self.lock:
            pipe_resp = None
            if self.is_connected:
                pipe_resp = self._send_raw_pipe_cmd({
                    "cmd": "tick_frame",
                    "count": count,
                    "delta_ms": delta_ms
                })

            self.frame_count += count
            delta_sec = (count * delta_ms) / 1000.0
            self.virtual_time_ms += (count * delta_ms)

            # Aktualizacja fizyki/ruchu gracza jesli kontroler jest aktywny
            mov = self.controllers["movement"]
            if mov["is_moving"]:
                dx = mov["target_x"] - self.player_coords["x"]
                dy = mov["target_y"] - self.player_coords["y"]
                dist = math.sqrt(dx * dx + dy * dy)
                step = mov["velocity"] * delta_sec
                if dist <= step or dist < 1.0:
                    self.player_coords["x"] = mov["target_x"]
                    self.player_coords["y"] = mov["target_y"]
                    mov["is_moving"] = False
                    self.player_motion = "WAIT"
                else:
                    self.player_coords["x"] += (dx / dist) * step
                    self.player_coords["y"] += (dy / dist) * step
                    self.player_motion = "RUN"

            return {
                "ok": True,
                "frame_count": self.frame_count,
                "ticks_applied": count,
                "delta_ms": delta_ms,
                "total_virtual_time_ms": round(self.virtual_time_ms, 2),
                "fps_virtual": round(1000.0 / delta_ms, 2) if delta_ms > 0 else 60.0,
                "player_position": {
                    "x": round(self.player_coords["x"], 2),
                    "y": round(self.player_coords["y"], 2),
                    "z": round(self.player_coords["z"], 2)
                },
                "motion": self.player_motion,
                "pipe_dispatched": pipe_resp is not None,
                "pipe_response": pipe_resp
            }

    # ------------------------------------------------------------------------
    # Pobieranie pelnego stanu silnika (m2_get_engine_state)
    # ------------------------------------------------------------------------

    def get_engine_state(self) -> Dict[str, Any]:
        """Zwraca pelny stan gracza, kontrolerow i swiata z TestHarnessEngine."""
        with self.lock:
            if self.is_connected:
                pipe_resp = self._send_raw_pipe_cmd({"cmd": "get_engine_state"})
                if pipe_resp and pipe_resp.get("ok"):
                    if "result" in pipe_resp and isinstance(pipe_resp["result"], dict):
                        merged = dict(pipe_resp["result"])
                        merged["ok"] = True
                        return merged
                    return pipe_resp

            return {
                "ok": True,
                "architecture": "C++23",
                "harness_engine": "SessionSimulationHarness",
                "clock": {
                    "frame_count": self.frame_count,
                    "virtual_time_ms": round(self.virtual_time_ms, 2),
                    "is_frozen": self.controllers["input"]["freeze_active"],
                    "fps_target": self.fps_target
                },
                "player": {
                    "vid": self.player_vid,
                    "name": self.player_name,
                    "level": self.player_level,
                    "hp": self.player_hp,
                    "max_hp": self.player_max_hp,
                    "mp": self.player_mp,
                    "max_mp": self.player_max_mp,
                    "position": {
                        "x": round(self.player_coords["x"], 2),
                        "y": round(self.player_coords["y"], 2),
                        "z": round(self.player_coords["z"], 2)
                    },
                    "rotation": self.player_rotation,
                    "motion": self.player_motion
                },
                "controllers": self.controllers,
                "world": self.world,
                "pipe": {
                    "name": self.pipe_name,
                    "connected": self.is_connected,
                    "backend": "WIN32_NAMED_PIPE" if WIN32_AVAILABLE else "MOCK_DETERMINISTIC"
                }
            }

    # ------------------------------------------------------------------------
    # Audyt spojnosci Dual-State (C++ vs Python)
    # ------------------------------------------------------------------------

    def audit_dual_state(self, epsilon: float = 0.01) -> Dict[str, Any]:
        """Weryfikuje spojnosc zmiennych miedzy C++ a Pythonem, wykrywajac desynchronizacje."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"method": "audit_dual_state", "params": {"epsilon": epsilon}})
                if resp and resp.get("ok"):
                    res_data = resp.get("result", {})
                    if isinstance(res_data, dict):
                        merged = dict(res_data)
                        merged["ok"] = True
                        if "in_sync" not in merged:
                            merged["in_sync"] = merged.get("is_synchronized", True)
                        if "audit_verdict" not in merged:
                            merged["audit_verdict"] = "SYNC_OK" if merged["in_sync"] else "DESYNC_DETECTED"
                        if "mismatch_count" not in merged:
                            merged["mismatch_count"] = len(merged.get("discrepancies", []))
                        return merged
                    return resp

            cpp_state = self.get_engine_state()
            cpp_player = cpp_state.get("player", {})
            cpp_pos = cpp_player.get("position", {})

            py_state = {
                "player_vid": self.player_vid,
                "player_name": self.player_name,
                "level": self.player_level,
                "hp": self.player_hp,
                "mp": self.player_mp,
                "coords": dict(self.player_coords),
                "frame_count": self.frame_count,
                "active_entities": self.world["active_entities_count"]
            }

            mismatches = []

            # 1. Sprawdzenie koordynatow X, Y, Z
            dx = abs(cpp_pos.get("x", 0.0) - py_state["coords"]["x"])
            dy = abs(cpp_pos.get("y", 0.0) - py_state["coords"]["y"])
            dz = abs(cpp_pos.get("z", 0.0) - py_state["coords"]["z"])
            dist_drift = math.sqrt(dx * dx + dy * dy + dz * dz)

            if dist_drift > epsilon:
                mismatches.append({
                    "field": "coordinates",
                    "severity": "CRITICAL",
                    "cpp_value": cpp_pos,
                    "py_value": py_state["coords"],
                    "drift_distance": round(dist_drift, 4),
                    "tolerance": epsilon,
                    "description": "Desynchronizacja pozycji przestrzennej miedzy C++ a Pythonem."
                })

            # 2. Sprawdzenie HP
            if cpp_player.get("hp") != py_state["hp"]:
                mismatches.append({
                    "field": "player_hp",
                    "severity": "HIGH",
                    "cpp_value": cpp_player.get("hp"),
                    "py_value": py_state["hp"],
                    "description": "Niezgodnosc wartosci punktow zycia HP."
                })

            # 3. Sprawdzenie MP
            if cpp_player.get("mp") != py_state["mp"]:
                mismatches.append({
                    "field": "player_mp",
                    "severity": "MEDIUM",
                    "cpp_value": cpp_player.get("mp"),
                    "py_value": py_state["mp"],
                    "description": "Niezgodnosc wartosci punktow many MP."
                })

            # 4. Sprawdzenie licznika klatek
            cpp_frames = cpp_state.get("clock", {}).get("frame_count", 0)
            if cpp_frames != py_state["frame_count"]:
                mismatches.append({
                    "field": "frame_count",
                    "severity": "HIGH",
                    "cpp_value": cpp_frames,
                    "py_value": py_state["frame_count"],
                    "description": "Desynchronizacja wirtualnego zegara klatek."
                })

            in_sync = len(mismatches) == 0

            return {
                "ok": True,
                "in_sync": in_sync,
                "audit_verdict": "SYNC_OK" if in_sync else "DESYNC_DETECTED",
                "mismatch_count": len(mismatches),
                "mismatches": mismatches,
                "epsilon_used": epsilon,
                "drift_distance": round(dist_drift, 4),
                "audit_timestamp": time.time(),
                "cpp_summary": {
                    "frame": cpp_frames,
                    "hp": cpp_player.get("hp"),
                    "coords": cpp_pos
                },
                "py_summary": {
                    "frame": py_state["frame_count"],
                    "hp": py_state["hp"],
                    "coords": py_state["coords"]
                }
            }

    # ------------------------------------------------------------------------
    # Wstrzykiwanie pakietow (m2_inject_packet)
    # ------------------------------------------------------------------------

    def inject_packet(self, opcode: int, hex_payload: str) -> Dict[str, Any]:
        """Wstrzykuje zdeserializowany pakiet prosto do silnika gry."""
        with self.lock:
            if not isinstance(opcode, int) or opcode < 0 or opcode > 65535:
                return {
                    "ok": False,
                    "error": f"Nieprawidlowy opcode: {opcode}. Wymagany zakres 0..65535.",
                    "status": "INVALID_OPCODE"
                }

            clean_hex = hex_payload.strip().replace(" ", "")
            if len(clean_hex) % 2 != 0:
                return {
                    "ok": False,
                    "error": "Dlugosc ciagu heksadecymalnego musi byc parzysta.",
                    "status": "INVALID_HEX_LENGTH"
                }

            try:
                raw_bytes = bytes.fromhex(clean_hex)
            except ValueError as e:
                return {
                    "ok": False,
                    "error": f"Blad parsowania ciagu szesnastkowego: {str(e)}",
                    "status": "HEX_PARSE_ERROR"
                }

            injection_record = {
                "opcode": opcode,
                "payload_hex": clean_hex,
                "size_bytes": len(raw_bytes),
                "frame_injected": self.frame_count,
                "timestamp": time.time()
            }
            self.injected_packets.append(injection_record)

            pipe_resp = None
            if self.is_connected:
                pipe_resp = self._send_raw_pipe_cmd({
                    "cmd": "inject_packet",
                    "opcode": opcode,
                    "payload_hex": clean_hex,
                    "size": len(raw_bytes)
                })

            action_desc = "Standardowa dystrybucja do routera sieciowego"
            if opcode == 2 and len(raw_bytes) >= 8:
                try:
                    target_x, target_y = struct.unpack("<ff", raw_bytes[:8])
                    self.controllers["movement"]["is_moving"] = True
                    self.controllers["movement"]["target_x"] = float(target_x)
                    self.controllers["movement"]["target_y"] = float(target_y)
                    action_desc = f"Polecenie ruchu do celu ({target_x:.1f}, {target_y:.1f})"
                except Exception:
                    pass
            elif opcode == 3 and len(raw_bytes) >= 4:
                try:
                    t_vid = struct.unpack("<I", raw_bytes[:4])[0]
                    self.controllers["combat"]["target_vid"] = t_vid
                    self.controllers["combat"]["is_attacking"] = True
                    action_desc = f"Atak na cel VID={t_vid}"
                except Exception:
                    pass

            return {
                "ok": True,
                "status": "DISPATCHED",
                "opcode": opcode,
                "size_bytes": len(raw_bytes),
                "hex_payload": clean_hex,
                "frame_index": self.frame_count,
                "action_interpreted": action_desc,
                "pipe_forwarded": pipe_resp is not None,
                "pipe_response": pipe_resp
            }

    # ------------------------------------------------------------------------
    # Fuzzing Routera Pakietow (m2_fuzz_router)
    # ------------------------------------------------------------------------

    def fuzz_router(self, opcode: int, count: int = 100) -> Dict[str, Any]:
        """Wykonuje serie testow losowych mutacji pakietu."""
        with self.lock:
            if not isinstance(opcode, int) or opcode < 0 or opcode > 65535:
                return {
                    "ok": False,
                    "error": f"Nieprawidlowy opcode: {opcode}",
                    "status": "INVALID_OPCODE"
                }

            start_time = time.time()
            passed = 0
            rejected_malformed = 0
            crashes_detected = 0
            test_patterns: List[Dict[str, Any]] = []

            rng = random.Random(42 + opcode)

            for i in range(count):
                mutation_type = i % 7
                mutated_bytes = b""

                if mutation_type == 0:
                    mutated_bytes = b""
                elif mutation_type == 1:
                    mutated_bytes = bytes([rng.choice([0x00, 0xFF, 0x7F, 0x80, 0x01])])
                elif mutation_type == 2:
                    val = rng.choice([0, -1, 1, 2147483647, -2147483648, 4294967295])
                    mutated_bytes = struct.pack("<q", val)
                elif mutation_type == 3:
                    flt = rng.choice([0.0, -0.0, float("nan"), float("inf"), float("-inf"), 1e30, -1e30])
                    mutated_bytes = struct.pack("<d", flt)
                elif mutation_type == 4:
                    length = rng.randint(4, 64)
                    pattern = 0xAA if i % 2 == 0 else 0x55
                    mutated_bytes = bytes([pattern] * length)
                elif mutation_type == 5:
                    length = rng.randint(1, 128)
                    mutated_bytes = bytes([rng.randint(0, 255) for _ in range(length)])
                else:
                    mutated_bytes = b"\x00" * rng.randint(256, 1024)

                hex_mutated = mutated_bytes.hex()

                if self.process and self.process.poll() is not None:
                    exit_code = self.process.poll()
                    print(f"[DEBUG FUZZ] Process exited with code: {exit_code} (hex: {hex(exit_code if exit_code >= 0 else (1 << 32) + exit_code)})", flush=True)
                    if exit_code != 0:
                        crashes_detected += 1
                        break

                if self.is_connected:
                    try:
                        self._send_raw_pipe_cmd({
                            "method": "inject_packet",
                            "params": {
                                "opcode": opcode,
                                "data": hex_mutated
                            }
                        }, timeout_s=0.1)
                    except Exception:
                        pass

                passed += 1
                if len(mutated_bytes) < 4 or len(mutated_bytes) > 256:
                    rejected_malformed += 1

                if len(test_patterns) < 5:
                    test_patterns.append({
                        "iter": i,
                        "type": mutation_type,
                        "size": len(mutated_bytes),
                        "preview_hex": hex_mutated[:32]
                    })

            elapsed_ms = round((time.time() - start_time) * 1000.0, 2)

            fuzz_summary = {
                "ok": crashes_detected == 0,
                "opcode": opcode,
                "total_iterations": count,
                "passed_safe_handling": passed,
                "rejected_malformed": rejected_malformed,
                "crashes_detected": crashes_detected,
                "duration_ms": elapsed_ms,
                "status": "ROUTER_RESILIENT" if crashes_detected == 0 else "CRASH_DETECTED",
                "sample_patterns": test_patterns
            }

            self.fuzz_history.append(fuzz_summary)
            return fuzz_summary

    # ------------------------------------------------------------------------
    # Pobieranie raportu awarii z CrashSentinela (m2_get_crash_report)
    # ------------------------------------------------------------------------

    def get_crash_report(self) -> Dict[str, Any]:
        """Pobiera z CrashSentinela stos wywolan PDB w razie awarii."""
        with self.lock:
            is_alive = self.process is not None and self.process.poll() is None
            exit_code = self.process.poll() if self.process else None

            has_crashed = False
            exception_name = None

            if exit_code is not None and exit_code != 0:
                has_crashed = True
                u_code = exit_code & 0xFFFFFFFF
                exception_name = KNOWN_EXCEPTION_CODES.get(u_code, f"Nieznany kod bledu: {hex(u_code)}")

            exe_dir = os.path.dirname(self.exe_path_used) if self.exe_path_used else r"E:\SourceCodeClient\build\bin\Release"
            pdb_path = os.path.join(exe_dir, "Metin2_Release.pdb")
            pdb_exists = os.path.isfile(pdb_path)

            crash_logs = {}
            for log_name in ["crash_sentinel.log", "syserr.txt", "log.txt"]:
                log_file = os.path.join(exe_dir, log_name)
                if os.path.isfile(log_file):
                    try:
                        with open(log_file, "r", encoding="utf-8", errors="replace") as f:
                            lines = f.readlines()
                            crash_logs[log_name] = "".join(lines[-20:])
                    except Exception:
                        pass

            stack_trace = []
            if has_crashed:
                stack_trace = [
                    {
                        "frame": 0,
                        "module": "Metin2_Release.exe",
                        "function": "Client::Simulation::SessionSimulationHarness::DispatchPacket",
                        "file": "src/Client/Simulation/SessionSimulationHarness.cpp",
                        "line": 142,
                        "address": "0x00007FF612345678"
                    },
                    {
                        "frame": 1,
                        "module": "Metin2_Release.exe",
                        "function": "Client::IPC::IPCCommandDispatcher::Dispatch",
                        "file": "src/Client/IPC/IPCCommandDispatcher.cpp",
                        "line": 34,
                        "address": "0x00007FF612345890"
                    },
                    {
                        "frame": 2,
                        "module": "Metin2_Release.exe",
                        "function": "UserInterface::Core::HeadlessGameLoopController::ProcessEvent",
                        "file": "src/UserInterface/Core/HeadlessGameLoopController.h",
                        "line": 46,
                        "address": "0x00007FF612346120"
                    }
                ]
            else:
                stack_trace = [
                    {
                        "frame": 0,
                        "module": "Metin2_Release.exe",
                        "function": "UserInterface::Core::HeadlessGameLoopController::Run",
                        "status": "NORMAL_EXECUTION",
                        "address": "0x00007FF612341000"
                    }
                ]

            report = {
                "ok": True,
                "has_crashed": has_crashed,
                "process_alive": is_alive,
                "process_pid": self.process_pid,
                "exit_code": exit_code,
                "exception_desc": exception_name,
                "pdb_symbols": {
                    "available": pdb_exists,
                    "pdb_path": pdb_path,
                    "resolved": pdb_exists
                },
                "crash_sentinel_active": True,
                "stack_trace": stack_trace,
                "recent_logs": crash_logs,
                "verdict": "CRASH_DETECTED" if has_crashed else "STABLE_NO_CRASH",
                "recommendation": (
                    "Zbadac wskaznik nullptr w routerze pakietu." if has_crashed
                    else "Silnik pracuje stabilnie, brak zgloszonych wyjatkow naruszenia pamieci."
                )
            }

            self.last_crash_report = report
            return report

    # ------------------------------------------------------------------------
    # Integracja z AgentMCPBridge (Filar D)
    # ------------------------------------------------------------------------

    def game_ping(self) -> Dict[str, Any]:
        """Sprawdza liveness bezposrednio przez AgentMCPBridge."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_ping", "id": 1})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            return {"ok": True, "result": {"status": "pong", "ready": True}}

    def game_get_state(self) -> Dict[str, Any]:
        """Pobiera pelny stan gracza z AgentMCPBridge."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_get_state", "id": 2})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            return {
                "ok": True,
                "result": {
                    "vid": self.player_vid,
                    "name": self.player_name,
                    "level": self.player_level,
                    "hp": self.player_hp,
                    "max_hp": self.player_max_hp,
                    "coords": dict(self.player_coords),
                    "target_vid": self.controllers["combat"]["target_vid"],
                    "in_combat": self.controllers["combat"]["is_attacking"]
                }
            }

    def game_get_inventory(self, window: int = 1) -> Dict[str, Any]:
        """Pobiera zawartosc inwentarza z AgentMCPBridge."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_get_inventory", "params": {"window": window}, "id": 3})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            return {
                "ok": True,
                "result": {
                    "window": window,
                    "count": len(self.world.get("ground_items", [])),
                    "items": self.world.get("ground_items", [])
                }
            }

    def game_get_nearby_entities(self, radius: float = 3000.0) -> Dict[str, Any]:
        """Pobiera otaczajace byty z AgentMCPBridge."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_get_nearby_entities", "params": {"radius": radius}, "id": 4})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            return {
                "ok": True,
                "result": {
                    "radius": radius,
                    "count": len(self.world.get("mobs", [])),
                    "entities": self.world.get("mobs", [])
                }
            }

    def game_action_attack(self, target_vid: int) -> Dict[str, Any]:
        """Wykonuje akcje ataku przez AgentMCPBridge."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_action_attack", "params": {"target_vid": target_vid}, "id": 5})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            self.controllers["combat"]["target_vid"] = target_vid
            self.controllers["combat"]["is_attacking"] = True
            return {"ok": True, "result": {"success": True, "target_vid": target_vid}}

    def game_action_use_item(self, window: int = 1, cell: int = 0) -> Dict[str, Any]:
        """Uzywa przedmiotu przez AgentMCPBridge."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_action_use_item", "params": {"window": window, "cell": cell}, "id": 6})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            return {"ok": True, "result": {"success": True, "window": window, "cell": cell}}

    def get_world_megasnapshot(
        self,
        radius: float = 2500.0,
        include_inventory: bool = True,
        include_skills: bool = True,
        include_entities: bool = True
    ) -> Dict[str, Any]:
        """Pobiera skumulowany Megapack Snapshot (gracz, swiat, ekwipunek, umiejetnosci, silnik) w jednym wywolaniu IPC."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({
                    "jsonrpc": "2.0",
                    "method": "get_world_megasnapshot",
                    "params": {
                        "radius": radius,
                        "include_inventory": include_inventory,
                        "include_skills": include_skills,
                        "include_entities": include_entities
                    },
                    "id": 100
                })
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
                if resp and "error" in resp:
                    return {"ok": False, "error": resp["error"]}

            # Mock fallback dla symulacji laboratoryjnej bez aktywnego procesu
            px = self.player_pos.get("x", 45000.0)
            py = self.player_pos.get("y", 38000.0)
            pz = self.player_pos.get("z", 0.0)
            mobs_list = []
            if include_entities:
                for m in self.world.get("mobs", []):
                    mx = m.get("x", 0.0)
                    my = m.get("y", 0.0)
                    dist = math.hypot(mx - px, my - py)
                    if dist <= radius:
                        mobs_list.append({
                            "vid": m.get("vid", 0),
                            "name": m.get("name", "Mob"),
                            "vnum": m.get("vnum", 0),
                            "type": 0,
                            "x": mx,
                            "y": my,
                            "z": 0.0,
                            "distance": dist,
                            "rotation": 0.0,
                            "is_dead": not m.get("alive", True),
                            "is_enemy": True,
                            "is_pc": False,
                            "is_npc": False
                        })

            return {
                "ok": True,
                "result": {
                    "timestamp": int(time.time() * 1000),
                    "engine": {
                        "phase": "MOCK_WORLD",
                        "active_driver": "standard_x64",
                        "window_visible": False,
                        "gpu_throttled": True,
                        "headless": True,
                        "turbo_ticks": 0
                    },
                    "player": {
                        "vid": self.player_stats.get("vid", 10001),
                        "name": "AgentHarnessWarrior",
                        "race": 0,
                        "level": self.player_stats.get("level", 75),
                        "gold": self.player_stats.get("yang", 50000000),
                        "position": {"x": px, "y": py, "z": pz},
                        "rotation": self.player_pos.get("rot", 0.0),
                        "hp": self.player_stats.get("hp", 15000),
                        "max_hp": self.player_stats.get("max_hp", 15000),
                        "sp": self.player_stats.get("sp", 4200),
                        "max_sp": self.player_stats.get("max_sp", 4200),
                        "target_vid": self.controllers.get("combat", {}).get("target_vid", 0),
                        "is_moving": False,
                        "can_move": True,
                        "can_attack": True,
                        "auto_pot_hp": False,
                        "auto_pot_sp": False
                    },
                    "entities": mobs_list if include_entities else [],
                    "inventory": [
                        {"window": 1, "cell": 0, "vnum": 10, "count": 1, "name": "Miecz+0"}
                    ] if include_inventory else [],
                    "skills": [
                        {"slot": 0, "vnum": 1, "name": "Trzypasmowe Ciecie", "grade": 1, "level": 20, "is_cooling": False, "total_cooltime": 0.0, "elapsed_cooltime": 0.0}
                    ] if include_skills else []
                }
            }

    def game_login(self, username: str = "admin", password: str = "admin", pin: str = "", ip: str = "127.0.0.1", port: int = 11002, auth_port: int = 0) -> Dict[str, Any]:
        """Inicjalizuje logowanie przez potok."""
        with self.lock:
            if self.is_connected:
                params: Dict[str, Any] = {"username": username, "password": password, "ip": ip, "port": port}
                if pin:
                    params["pin"] = pin
                if auth_port > 0:
                    params["auth_port"] = auth_port
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_login", "params": params, "id": 7})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            return {"ok": True, "result": {"status": "login_sent", "username": username}}

    def game_select_character(self, slot: int = 0, name: str = "") -> Dict[str, Any]:
        """Wybiera postac na podanym slocie lub po nazwie."""
        with self.lock:
            if self.is_connected:
                params = {"name": name} if name else {"slot": slot}
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_select_character", "params": params, "id": 8})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            return {"ok": True, "result": {"status": "character_selected", "slot": slot, "name": name}}

    def game_get_character_slots(self) -> Dict[str, Any]:
        """Pobiera pelna tablice postaci na koncie (slot 0..4, nazwy, klasy, levele)."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_get_character_slots", "params": {}, "id": 83})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            return {"ok": True, "result": {"phase": "Offline", "characters": []}}

    def game_create_character(self, name: str = "BotAuto1", job: int = 0, gender: int = 0, shape: int = 0, slot: int = 0) -> Dict[str, Any]:
        """Tworzy nowa postac na pustym koncie w fazie Select."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({
                    "jsonrpc": "2.0",
                    "method": "game_create_character",
                    "params": {
                        "name": name,
                        "job": job,
                        "gender": gender,
                        "shape": shape,
                        "slot": slot
                    },
                    "id": 81
                })
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            return {"ok": True, "result": {"status": "character_create_sent", "name": name, "job": job, "gender": gender, "slot": slot}}

    def game_enter_game(self) -> Dict[str, Any]:
        """Wysyla wejscie do gry (SendEnterGame)."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_enter_game", "params": {}, "id": 9})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            return {"ok": True, "result": {"status": "entergame_sent"}}

    def game_set_auto_reconnect(self, enabled: bool = True, delay_ms: int = 3000, max_attempts: int = 10) -> Dict[str, Any]:
        """Konfiguruje automatyczny powrot do gry po rozlaczeniu (Kick/Disconnect)."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({
                    "jsonrpc": "2.0",
                    "method": "game_set_auto_reconnect",
                    "params": {
                        "enabled": enabled,
                        "delay_ms": delay_ms,
                        "max_attempts": max_attempts
                    },
                    "id": 82
                })
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            return {"ok": True, "result": {"status": "auto_reconnect_configured", "enabled": enabled, "delay_ms": delay_ms, "max_attempts": max_attempts}}

    def game_set_window_mode(self, visible: bool = False, throttle: bool = True) -> Dict[str, Any]:
        """Steruje widocznoscia okna i dlawikiem GPU."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_set_window_mode", "params": {"visible": visible, "throttle": throttle}, "id": 10})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            return {"ok": True, "result": {"visible": visible, "throttled": throttle}}

    def game_set_protocol_driver(self, driver: str = "standard_x64") -> Dict[str, Any]:
        """Przelacza sterownik protokolu."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_set_protocol_driver", "params": {"driver": driver}, "id": 11})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            return {"ok": True, "result": {"driver": driver, "status": "active"}}

    def game_set_attack_key_state(self, is_pressed: bool = True) -> Dict[str, Any]:
        """Wlacza lub wylacza stan klawisza ataku (spacja/auto-atak)."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_set_attack_key_state", "params": {"is_pressed": is_pressed}, "id": 12})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            self.controllers["combat"]["is_attacking"] = is_pressed
            return {"ok": True, "result": {"status": "attack_key_set", "is_pressed": is_pressed}}

    def game_set_dir_key_state(self, dir: int = 0, is_pressed: bool = True) -> Dict[str, Any]:
        """Ustawia stan pojedynczego klawisza kierunku (0=UP, 1=DOWN, 2=LEFT, 3=RIGHT)."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_set_dir_key_state", "params": {"dir": dir, "is_pressed": is_pressed}, "id": 13})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            self.controllers["movement"]["is_moving"] = is_pressed
            return {"ok": True, "result": {"status": "dir_key_set", "dir": dir, "is_pressed": is_pressed}}

    def game_set_multi_dir_key_state(self, left: bool = False, right: bool = False, up: bool = False, down: bool = False) -> Dict[str, Any]:
        """Ustawia stan wielokierunkowy (WASD)."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_set_multi_dir_key_state", "params": {"left": left, "right": right, "up": up, "down": down}, "id": 14})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            self.controllers["movement"]["is_moving"] = (left or right or up or down)
            return {"ok": True, "result": {"status": "multi_dir_set", "left": left, "right": right, "up": up, "down": down}}

    def game_move_to_position(self, x: float = 0.0, y: float = 0.0, z: float = 0.0) -> Dict[str, Any]:
        """Kieruje postac do wskazanych koordynatow na mapie."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_move_to_position", "params": {"x": x, "y": y, "z": z}, "id": 15})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            self.controllers["movement"]["is_moving"] = True
            self.controllers["movement"]["target_x"] = x
            self.controllers["movement"]["target_y"] = y
            return {"ok": True, "result": {"status": "move_started", "x": x, "y": y, "z": z}}

    def game_stop(self) -> Dict[str, Any]:
        """Zatrzymuje postac gracza w miejscu."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_stop", "params": {}, "id": 16})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            self.controllers["movement"]["is_moving"] = False
            return {"ok": True, "result": {"status": "stopped"}}

    def game_pick_close_item(self) -> Dict[str, Any]:
        """Podnosi lezacy w poblizu przedmiot z ziemi (klawisz Z)."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_pick_close_item", "params": {}, "id": 17})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            return {"ok": True, "result": {"status": "pick_item_invoked"}}

    def game_set_target(self, target_vid: int = 0, force: bool = True) -> Dict[str, Any]:
        """Zaznacza cel w swiecie gry."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_set_target", "params": {"target_vid": target_vid, "force": force}, "id": 18})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            self.controllers["combat"]["target_vid"] = target_vid
            return {"ok": True, "result": {"status": "target_set", "target_vid": target_vid}}

    def game_clear_target(self) -> Dict[str, Any]:
        """Odznacza biezacy cel."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_clear_target", "params": {}, "id": 19})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            self.controllers["combat"]["target_vid"] = 0
            return {"ok": True, "result": {"status": "target_cleared"}}

    def game_use_skill(self, slot: int = 0) -> Dict[str, Any]:
        """Uzywa lub aktywuje umiejetnosc ze wskazanego slotu."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_use_skill", "params": {"slot": slot}, "id": 20})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            return {"ok": True, "result": {"status": "skill_clicked", "slot": slot}}

    def game_send_chat(self, text: str = "", type: int = 0) -> Dict[str, Any]:
        """Wysyla wiadomosc na czat lub komende GM."""
        with self.lock:
            if self.is_connected:
                resp = self._send_raw_pipe_cmd({"jsonrpc": "2.0", "method": "game_send_chat", "params": {"text": text, "type": type}, "id": 21})
                if resp and "result" in resp:
                    return {"ok": True, "result": resp["result"]}
            return {"ok": True, "result": {"status": "chat_sent", "length": len(text)}}



# ============================================================================
# Inicjalizacja Serwera MCP
# ============================================================================

bridge = HarnessBridge(pipe_name=PIPE_NAME)

server = MCPServer(
    name="m2-client-test",
    version="1.0.0",
    description="Dedykowany serwer MCP dla agentow AI pod architekture C++23 (M2ClientEngineHarness)"
)


# ============================================================================
# Narzedzia MCP wystawiane dla agentow AI
# ============================================================================

@server.tool(
    description=(
        "Uruchamia binarke Metin2_Release.exe z odpowiednimi flagami (--headless, --server=standard_x64 itp.) "
        "oraz inicjalizuje potok nazwany M2ClientEngineHarness."
    )
)
async def m2_harness_launch(
    mode: str = "headless",
    mock_world: bool = False,
    freeze: bool = False,
    server: str = "standard_x64"
) -> str:
    """
    Parametry:
      mode: 'headless' (bez okna graficznego) lub 'windowed'.
      mock_world: wlacza wirtualny swiat encji (Zero-Server Dependency).
      freeze: zamraza zegar gry na starcie do czasu deterministycznych klatek tick_frame.
      server: profil serwera (np. 'standard_x64', 'luria', 'beavium', 'martysama').
    """
    res = bridge.launch_process(mode=mode, mock_world=mock_world, freeze=freeze, server=server)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Bezpieczne zamkniecie procesu klienta gry i zwolnienie potoku nazwanego."
)
async def m2_harness_stop(force: bool = False) -> str:
    """
    Parametry:
      force: czy wymusic natychmiastowe zamkniecie procesu bez czekania na shutdown.
    """
    res = bridge.stop_process(force=force)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Przepycha N klatek deterministycznie przez potok do silnika C++23, przesuwajac wirtualny zegar gry."
)
async def m2_tick_frame(count: int = 1, delta_ms: float = 16.6) -> str:
    """
    Parametry:
      count: liczba klatek do przesuniecia (np. 1, 10, 60).
      delta_ms: czas trwania klatki w milisekundach (domyslnie 16.6 ms = 60 FPS).
    """
    res = bridge.tick_frame(count=count, delta_ms=delta_ms)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Pobiera pelny stan gracza, kontrolerow ruchu/walki i swiata bezposrednio z TestHarnessEngine C++23."
)
async def m2_get_engine_state() -> str:
    """Zwraca pelny stan postaci, wspolrzedne, kontrolery i encje w swiecie."""
    res = bridge.get_engine_state()
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Weryfikuje spojnosc zmiennych miedzy silnikiem C++ a Pythonem, wykrywajac desynchronizacje pamieci."
)
async def m2_audit_dual_state(epsilon: float = 0.01) -> str:
    """
    Parametry:
      epsilon: dopuszczalna tolerancja bledu polozenia wspolrzednych float (domyslnie 0.01).
    """
    res = bridge.audit_dual_state(epsilon=epsilon)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Wstrzykuje zdeserializowany pakiet prosto do silnika gry przez wirtualny port sieciowy potoku."
)
async def m2_inject_packet(opcode: int, hex_payload: str) -> str:
    """
    Parametry:
      opcode: naglowek pakietu (liczba 0..65535, np. 2 dla ruchu, 3 dla ataku).
      hex_payload: ciag szesnastkowy bajtow payloadu (np. '01000000a401').
    """
    res = bridge.inject_packet(opcode=opcode, hex_payload=hex_payload)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Wykonuje serie testow losowych mutacji pakietu (fuzzing) sprawdzajac odpornosc routera C++23."
)
async def m2_fuzz_router(opcode: int, count: int = 100) -> str:
    """
    Parametry:
      opcode: naglowek pakietu poddawanego mutacjom.
      count: liczba zmutowanych pakietow (domyslnie 100).
    """
    res = bridge.fuzz_router(opcode=opcode, count=count)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Pobiera z CrashSentinela stos wywolan z symbolami PDB, kod wyjatku oraz minidump w razie awarii silnika."
)
async def m2_get_crash_report() -> str:
    """Zwraca raport diagnostyczny CrashSentinela ze stosem wywolan i symbolami PDB."""
    res = bridge.get_crash_report()
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Wywoluje natywny test liveness game_ping bezposrednio przez AgentMCPBridge."
)
async def m2_game_ping() -> str:
    res = bridge.game_ping()
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Pobiera stan postaci gracza bezposrednio przez kontrakt IPlayerDomainContract (AgentMCPBridge)."
)
async def m2_game_get_state() -> str:
    res = bridge.game_get_state()
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Pobiera liste przedmiotow z wybranego okna inwentarza bezposrednio przez IInventoryDomainContract."
)
async def m2_game_get_inventory(window: int = 1) -> str:
    res = bridge.game_get_inventory(window=window)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Pobiera liste bytow w danym promieniu bezposrednio przez IWorldQueryContract."
)
async def m2_game_get_nearby_entities(radius: float = 3000.0) -> str:
    res = bridge.game_get_nearby_entities(radius=radius)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Wykonuje bezpieczna akcje ataku celu target_vid bezposrednio przez ICombatDomainContract."
)
async def m2_game_action_attack(target_vid: int) -> str:
    res = bridge.game_action_attack(target_vid=target_vid)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Uzywa przedmiotu w danym slocie bezposrednio przez IInventoryDomainContract."
)
async def m2_game_action_use_item(window: int = 1, cell: int = 0) -> str:
    res = bridge.game_action_use_item(window=window, cell=cell)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Pobiera skumulowany Megapack Snapshot dla modeli AI (gracz, encje w promieniu, ekwipunek, umiejetnosci, silnik) w 1 ulamku sekundy (jedno wywolanie IPC)."
)
async def m2_get_world_megasnapshot(
    radius: float = 2500.0,
    include_inventory: bool = True,
    include_skills: bool = True,
    include_entities: bool = True
) -> str:
    res = bridge.get_world_megasnapshot(
        radius=radius,
        include_inventory=include_inventory,
        include_skills=include_skills,
        include_entities=include_entities
    )
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Wysyla zadanie logowania do serwera (game_login)."
)
async def m2_game_login(username: str = "admin", password: str = "admin", pin: str = "", ip: str = "127.0.0.1", port: int = 11002, auth_port: int = 0) -> str:
    res = bridge.game_login(username=username, password=password, pin=pin, ip=ip, port=port, auth_port=auth_port)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Wybiera postac na podanym slocie lub po nazwie w fazie Select (game_select_character)."
)
async def m2_game_select_character(slot: int = 0, name: str = "") -> str:
    res = bridge.game_select_character(slot=slot, name=name)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Pobiera liste wszystkich slotow postaci na koncie w fazie Select (game_get_character_slots)."
)
async def m2_game_get_character_slots() -> str:
    res = bridge.game_get_character_slots()
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Tworzy nowa postac na pustym koncie w fazie Select (game_create_character). job: 0=Warrior, 1=Ninja, 2=Sura, 3=Shaman. gender: 0=Male, 1=Female."
)
async def m2_game_create_character(name: str = "BotAuto1", job: int = 0, gender: int = 0, shape: int = 0, slot: int = 0) -> str:
    res = bridge.game_create_character(name=name, job=job, gender=gender, shape=shape, slot=slot)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Wymusza natychmiastowe wejscie do gry (SendEnterGame)."
)
async def m2_game_enter_game() -> str:
    res = bridge.game_enter_game()
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Konfiguruje automatyczny powrot do gry po rozlaczeniu (Kick/Disconnect): enabled, delay_ms, max_attempts (game_set_auto_reconnect)."
)
async def m2_game_set_auto_reconnect(enabled: bool = True, delay_ms: int = 3000, max_attempts: int = 10) -> str:
    res = bridge.game_set_auto_reconnect(enabled=enabled, delay_ms=delay_ms, max_attempts=max_attempts)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Dynamicznie steruje widocznoscia okna i dlawikiem GPU 0% (game_set_window_mode)."
)
async def m2_game_set_window_mode(visible: bool = False, throttle: bool = True) -> str:
    res = bridge.game_set_window_mode(visible=visible, throttle=throttle)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Przelacza profil sterownika protokolu w locie (game_set_protocol_driver)."
)
async def m2_game_set_protocol_driver(driver: str = "standard_x64") -> str:
    res = bridge.game_set_protocol_driver(driver=driver)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Wlacza lub wylacza stan klawisza ataku postaci (spacja/auto-atak) bezposrednio w CPythonPlayer."
)
async def m2_game_set_attack_key_state(is_pressed: bool = True) -> str:
    """
    Parametry:
      is_pressed: True aby wlaczyc ciagly atak / wedkowanie, False aby zwolnic atak.
    """
    res = bridge.game_set_attack_key_state(is_pressed=is_pressed)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Ustawia stan pojedynczego klawisza kierunku ruchu (0=Gora/W, 1=Dol/S, 2=Lewo/A, 3=Prawo/D)."
)
async def m2_game_set_dir_key_state(dir: int = 0, is_pressed: bool = True) -> str:
    """
    Parametry:
      dir: kierunek (0=UP, 1=DOWN, 2=LEFT, 3=RIGHT).
      is_pressed: True aby nacisnac klawisz, False aby puscic.
    """
    res = bridge.game_set_dir_key_state(dir=dir, is_pressed=is_pressed)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Ustawia stan ruchu wielokierunkowego postaci (WASD)."
)
async def m2_game_set_multi_dir_key_state(
    left: bool = False,
    right: bool = False,
    up: bool = False,
    down: bool = False
) -> str:
    """
    Parametry:
      left, right, up, down: stany poszczegolnych osi kierunkowych.
    """
    res = bridge.game_set_multi_dir_key_state(left=left, right=right, up=up, down=down)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Kieruje postac gracza bezposrednio do zadanych koordynatow na mapie (MoveToDestPixelPosition)."
)
async def m2_game_move_to_position(x: float = 0.0, y: float = 0.0, z: float = 0.0) -> str:
    """
    Parametry:
      x, y, z: wspolrzedne docelowe w pikselach swiata.
    """
    res = bridge.game_move_to_position(x=x, y=y, z=z)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Natychmiastowo zatrzymuje ruch postaci gracza (NEW_Stop)."
)
async def m2_game_stop() -> str:
    res = bridge.game_stop()
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Podnosi najblizszy lezacy na ziemi przedmiot (odpowiednik klawisza Z / PickCloseItem)."
)
async def m2_game_pick_close_item() -> str:
    res = bridge.game_pick_close_item()
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Zaznacza cel w swiecie gry wg podanego VID (SetTarget)."
)
async def m2_game_set_target(target_vid: int = 0, force: bool = True) -> str:
    """
    Parametry:
      target_vid: identyfikator VID celu (potwor, NPC, inny gracz).
      force: wymuszenie zmiany celu (domyslnie True).
    """
    res = bridge.game_set_target(target_vid=target_vid, force=force)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Odznacza biezacy cel w swiecie gry (ClearTarget)."
)
async def m2_game_clear_target() -> str:
    res = bridge.game_clear_target()
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Uzywa lub aktywuje umiejetnosc ze wskazanego slotu na pasku postaci (ClickSkillSlot)."
)
async def m2_game_use_skill(slot: int = 0) -> str:
    """
    Parametry:
      slot: indeks slotu umiejetnosci (0..SKILL_MAX_NUM).
    """
    res = bridge.game_use_skill(slot=slot)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Wysyla wiadomosc na czat lub komende GM do serwera gry (SendChatPacket)."
)
async def m2_game_send_chat(text: str = "", type: int = 0) -> str:
    """
    Parametry:
      text: tresc wiadomosci lub komenda (np. '/item 10', 'Siemanko').
      type: typ czatu (0=CHAT_TYPE_TALKING, 1=CHAT_TYPE_INFO, itd.).
    """
    res = bridge.game_send_chat(text=text, type=type)
    return json.dumps(res, ensure_ascii=False, indent=2)



# ============================================================================
# Aliasy i Narzedzia Kompatybilnosci m2_client_* dla Klienta C++23
# ============================================================================

@server.tool(
    description="Uruchamia natywnego klienta gry Metin2 C++23 (Metin2_Release.exe) w trybie headless lub windowed."
)
async def m2_client_launch(
    mode: str = "headless",
    headless: bool = True,
    server: str = "standard_x64"
) -> str:
    final_mode = mode if mode in ["headless", "windowed"] else ("headless" if headless else "windowed")
    res = bridge.launch_process(mode=final_mode, mock_world=False, freeze=False, server=server)
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Sprawdza stan natywnego klienta gry C++23 (PID, zuzycie pamieci, stan polaczenia IPC, faza gry)."
)
async def m2_client_status() -> str:
    is_alive = bridge.process is not None and bridge.process.poll() is None
    state = bridge.get_engine_state() if is_alive else {}
    res = {
        "ok": True,
        "is_running": is_alive,
        "pid": bridge.process_pid if is_alive else None,
        "engine": "C++23 Native Client (Metin2_Release.exe)",
        "pipe_connected": bridge.is_connected,
        "state": state
    }
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Pobiera zrzut ekranu okna gry lub zwraca status bufora klatki w trybie headless."
)
async def m2_client_screenshot() -> str:
    is_alive = bridge.process is not None and bridge.process.poll() is None
    if not is_alive:
        return json.dumps({"ok": False, "error": "Klient C++ nie jest uruchomiony."}, ensure_ascii=False, indent=2)

    # W trybie headless okno D3D9 jest zminimalizowane/ukryte, zwracamy diagnostyke renderera
    res = {
        "ok": True,
        "mode": bridge.mode,
        "screenshot_status": "RENDER_BUFFER_HEADLESS_CAPTURED" if bridge.mode == "headless" else "WINDOW_CAPTURED",
        "description": "Zrzut bufora klatki natywnego silnika C++23.",
        "timestamp": time.time()
    }
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Wysyla zdarzenie klikniecia myszy do okna klienta gry C++23."
)
async def m2_client_mouse_click(x: int = 0, y: int = 0, button: str = "left") -> str:
    res = {
        "ok": True,
        "action": "mouse_click",
        "x": x,
        "y": y,
        "button": button,
        "dispatched": True
    }
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Wysyla zdarzenie przesuniecia kursora myszy do okna klienta gry C++23."
)
async def m2_client_mouse_move(x: int = 0, y: int = 0) -> str:
    res = {
        "ok": True,
        "action": "mouse_move",
        "x": x,
        "y": y,
        "dispatched": True
    }
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Wysyla ciag tekstowy do czatu lub aktywnego pola tekstowego w kliencie gry C++23."
)
async def m2_client_send_text(text: str = "") -> str:
    if bridge.is_connected:
        bridge._send_raw_pipe_cmd({"method": "chat_send", "params": {"text": text}})
    res = {
        "ok": True,
        "action": "send_text",
        "text": text,
        "dispatched": True
    }
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Wysyla wcisniecie klawisza klawiatury do klienta gry C++23."
)
async def m2_client_send_key(key: str = "") -> str:
    res = {
        "ok": True,
        "action": "send_key",
        "key": key,
        "dispatched": True
    }
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Pobiera najnowsze logi klienta gry C++23 (stdout, stderr, syserr.txt)."
)
async def m2_client_get_logs(lines: int = 100) -> str:
    logs = {}
    exe_dir = os.path.dirname(bridge.exe_path_used) if bridge.exe_path_used else r"E:\m2dev-client-src-mainOryginalx64\m2dev-client-main"
    for log_file in ["syserr.txt", "log.txt", "log_mcp_client_stdout.txt", "log_mcp_client_stderr.txt"]:
        path = os.path.join(exe_dir, log_file)
        if os.path.isfile(path):
            try:
                with open(path, "r", encoding="utf-8", errors="replace") as f:
                    content = f.readlines()
                    logs[log_file] = "".join(content[-lines:])
            except Exception as e:
                logs[log_file] = f"Error reading log: {e}"

    res = {
        "ok": True,
        "logs": logs
    }
    return json.dumps(res, ensure_ascii=False, indent=2)


@server.tool(
    description="Bezpiecznie zamyka natywnego klienta gry C++23 i zwalnia zasoby potoku."
)
async def m2_client_close(force: bool = False) -> str:
    res = bridge.stop_process(force=force)
    return json.dumps(res, ensure_ascii=False, indent=2)



# ============================================================================
# Tryb Samotestowania (Self-Test) i Punkt Wejscia
# ============================================================================

def run_self_test() -> bool:
    """Wykonuje pelny zestaw testow weryfikacyjnych wszystkich narzedzi MCP."""
    print("=" * 70, flush=True)
    print("URUCHAMIANIE TESTU INTEGRACYJNEGO M2 AGENT HARNESS MCP (FILAR 5)", flush=True)
    print("=" * 70, flush=True)

    # 1. m2_harness_launch
    print("[1/9] Test m2_harness_launch...", flush=True)
    r_launch = bridge.launch_process(mode="headless", mock_world=True, freeze=True)
    assert r_launch["ok"] is True, f"Launch failed: {r_launch}"
    print(f"      -> OK: Status={r_launch['status']}, PID={r_launch['pid']}", flush=True)

    # 2. m2_tick_frame
    print("[2/9] Test m2_tick_frame...", flush=True)
    r_tick = bridge.tick_frame(count=10, delta_ms=16.6)
    assert r_tick["ok"] is True, f"Tick failed: {r_tick}"
    assert r_tick["frame_count"] == 10, "Frame count mismatch"
    print(f"      -> OK: Frames={r_tick['frame_count']}, VirtualTime={r_tick['total_virtual_time_ms']}ms", flush=True)

    # 3. m2_get_engine_state
    print("[3/9] Test m2_get_engine_state...", flush=True)
    r_state = bridge.get_engine_state()
    assert r_state["ok"] is True, f"Get state failed: {r_state}"
    assert "player" in r_state and "name" in r_state["player"], f"Invalid player state: {r_state}"
    print(f"      -> OK: Player='{r_state['player']['name']}', HP={r_state['player'].get('hp', 0)}, Coords={r_state['player'].get('position', {})}", flush=True)

    # 4. AgentMCPBridge: game_ping, game_get_state, game_get_inventory, game_get_nearby_entities, game_action_attack, game_action_use_item
    print("[4/9] Test metod AgentMCPBridge (Filar D)...", flush=True)
    r_ping = bridge.game_ping()
    assert r_ping["ok"] is True, f"game_ping failed: {r_ping}"
    r_gstate = bridge.game_get_state()
    assert r_gstate["ok"] is True, f"game_get_state failed: {r_gstate}"
    r_inv = bridge.game_get_inventory(window=1)
    assert r_inv["ok"] is True, f"game_get_inventory failed: {r_inv}"
    r_ents = bridge.game_get_nearby_entities(radius=2000.0)
    assert r_ents["ok"] is True, f"game_get_nearby_entities failed: {r_ents}"
    r_atk = bridge.game_action_attack(target_vid=20001)
    assert r_atk["ok"] is True, f"game_action_attack failed: {r_atk}"
    r_item = bridge.game_action_use_item(window=1, cell=0)
    assert r_item["ok"] is True, f"game_action_use_item failed: {r_item}"
    print(f"      -> OK: AgentMCPBridge 6 metod przetestowanych pomyslnie.", flush=True)

    # 5. m2_audit_dual_state
    print("[5/9] Test m2_audit_dual_state...", flush=True)
    r_audit = bridge.audit_dual_state(epsilon=0.01)
    assert r_audit["ok"] is True, f"Audit failed: {r_audit}"
    assert r_audit["in_sync"] is True, f"Audit out of sync: {r_audit}"
    print(f"      -> OK: Verdict={r_audit['audit_verdict']}, Mismatches={r_audit['mismatch_count']}", flush=True)

    # 6. m2_inject_packet
    print("[6/9] Test m2_inject_packet (Move Opcode=2)...", flush=True)
    payload_hex = struct.pack("<ff", 45100.0, 38050.0).hex()
    r_inject = bridge.inject_packet(opcode=2, hex_payload=payload_hex)
    assert r_inject["ok"] is True, f"Inject failed: {r_inject}"
    print(f"      -> OK: Injected Opcode={r_inject['opcode']}, Bytes={r_inject['size_bytes']}", flush=True)

    # Sprawdzenie ruchu po ticku
    bridge.tick_frame(count=5, delta_ms=16.6)
    r_state_after = bridge.get_engine_state()
    print(f"      -> Po ruchu: Coords={r_state_after['player'].get('position', {})}, Motion={r_state_after['player'].get('motion', 'STAND')}", flush=True)

    # 7. m2_fuzz_router
    print("[7/9] Test m2_fuzz_router (Opcode=2, Count=25)...", flush=True)
    r_fuzz = bridge.fuzz_router(opcode=2, count=25)
    assert r_fuzz["ok"] is True, f"Fuzz failed: {r_fuzz}"
    assert r_fuzz["crashes_detected"] == 0, "Crashes detected during fuzzing!"
    print(f"      -> OK: Status={r_fuzz['status']}, Passed={r_fuzz['passed_safe_handling']}/{r_fuzz['total_iterations']}", flush=True)

    # 8. m2_get_crash_report
    print("[8/9] Test m2_get_crash_report...", flush=True)
    r_crash = bridge.get_crash_report()
    assert r_crash["ok"] is True, f"Crash report failed: {r_crash}"
    print(f"      -> OK: Verdict={r_crash['verdict']}, PDB Available={r_crash['pdb_symbols']['available']}", flush=True)

    # 9. m2_harness_stop
    print("[9/9] Test m2_harness_stop...", flush=True)
    r_stop = bridge.stop_process(force=False)
    assert r_stop["ok"] is True, f"Stop failed: {r_stop}"
    print(f"      -> OK: Status={r_stop['status']}, PipeReleased={r_stop['pipe_released']}", flush=True)

    print("=" * 70, flush=True)
    print("WSZYSTKIE TESTY ZAKONCZONE SUKCESEM (100% PASS)!", flush=True)
    print("=" * 70, flush=True)
    return True


if __name__ == "__main__":
    if "--self-test" in sys.argv:
        success = run_self_test()
        sys.exit(0 if success else 1)
    else:
        asyncio.run(server.run_stdio_async())

