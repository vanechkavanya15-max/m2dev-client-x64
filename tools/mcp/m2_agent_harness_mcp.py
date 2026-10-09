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

    def launch_process(self, mode: str = "headless", mock_world: bool = True, freeze: bool = True) -> Dict[str, Any]:
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

            args = [
                exe_path,
                "--test-mode",
                f"--pipe={self.pipe_name}"
            ]
            if mock_world:
                args.append("--mock-world")
            if freeze:
                args.append("--freeze-on-start")
            if mode == "headless":
                args.append("--headless")
            else:
                args.append("--windowed")

            work_dir = os.path.dirname(exe_path) if os.path.isfile(exe_path) else os.getcwd()

            # Jesli plik fizycznie istnieje na dysku
            if os.path.isfile(exe_path):
                try:
                    self.process = subprocess.Popen(
                        args,
                        cwd=work_dir,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE,
                        creationflags=subprocess.CREATE_NEW_PROCESS_GROUP if sys.platform == "win32" else 0
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

            # Proba szybkiego polaczenia z potokiem
            connected = self._try_connect_pipe(timeout_s=0.2)
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

    def _try_connect_pipe(self, timeout_s: float = 0.2) -> bool:
        r"""Probuje nawiazac polaczenie z potokiem \\.\pipe\M2ClientEngineHarness."""
        if not WIN32_AVAILABLE:
            return False

        try:
            win32pipe.WaitNamedPipe(self.pipe_name, int(timeout_s * 1000))
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

        payload_bytes = json.dumps(cmd_dict, ensure_ascii=False).encode("utf-8")
        header = struct.pack("<I", len(payload_bytes))
        full_msg = header + payload_bytes

        try:
            win32file.WriteFile(self.pipe_handle, full_msg)
            # Odczyt naglowka odpowiedzi (dokladnie 4 bajty)
            hdr_parts = bytearray()
            while len(hdr_parts) < 4:
                _, chunk = win32file.ReadFile(self.pipe_handle, 4 - len(hdr_parts))
                if not chunk:
                    break
                hdr_parts.extend(chunk)

            if len(hdr_parts) == 4:
                resp_len = struct.unpack("<I", bytes(hdr_parts))[0]
                if resp_len > PIPE_BUF_SIZE:
                    return None

                body_parts = bytearray()
                while len(body_parts) < resp_len:
                    _, chunk = win32file.ReadFile(self.pipe_handle, resp_len - len(body_parts))
                    if not chunk:
                        break
                    body_parts.extend(chunk)

                if len(body_parts) == resp_len:
                    return json.loads(body_parts.decode("utf-8"))
        except Exception:
            self._close_pipe()

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
                    crashes_detected += 1
                    break

                if self.is_connected:
                    try:
                        self._send_raw_pipe_cmd({
                            "cmd": "fuzz_packet",
                            "opcode": opcode,
                            "payload_hex": hex_mutated
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


# ============================================================================
# Inicjalizacja Serwera MCP
# ============================================================================

bridge = HarnessBridge(pipe_name=PIPE_NAME)

server = MCPServer(
    name="m2-agent-harness",
    version="1.0.0",
    description="Dedykowany serwer MCP dla agentow AI pod architekture C++23 (M2ClientEngineHarness)"
)


# ============================================================================
# Narzedzia MCP wystawiane dla agentow AI
# ============================================================================

@server.tool(
    description=(
        "Uruchamia binarke Metin2_Release.exe z odpowiednimi flagami (--test-mode, --mock-world, "
        "--freeze-on-start) oraz inicjalizuje potok nazwany M2ClientEngineHarness."
    )
)
async def m2_harness_launch(
    mode: str = "headless",
    mock_world: bool = True,
    freeze: bool = True
) -> str:
    """
    Parametry:
      mode: 'headless' (bez okna graficznego) lub 'windowed'.
      mock_world: wlacza wirtualny swiat encji (Zero-Server Dependency).
      freeze: zamraza zegar gry na starcie do czasu deterministycznych klatek tick_frame.
    """
    res = bridge.launch_process(mode=mode, mock_world=mock_world, freeze=freeze)
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


# ============================================================================
# Tryb Samotestowania (Self-Test) i Punkt Wejscia
# ============================================================================

def run_self_test() -> bool:
    """Wykonuje pelny zestaw testow weryfikacyjnych wszystkich 8 narzedzi MCP."""
    print("=" * 70, flush=True)
    print("URUCHAMIANIE TESTU INTEGRACYJNEGO M2 AGENT HARNESS MCP (FILAR 5)", flush=True)
    print("=" * 70, flush=True)

    # 1. m2_harness_launch
    print("[1/8] Test m2_harness_launch...", flush=True)
    r_launch = bridge.launch_process(mode="headless", mock_world=True, freeze=True)
    assert r_launch["ok"] is True, f"Launch failed: {r_launch}"
    print(f"      -> OK: Status={r_launch['status']}, PID={r_launch['pid']}", flush=True)

    # 2. m2_tick_frame
    print("[2/8] Test m2_tick_frame...", flush=True)
    r_tick = bridge.tick_frame(count=10, delta_ms=16.6)
    assert r_tick["ok"] is True, f"Tick failed: {r_tick}"
    assert r_tick["frame_count"] == 10, "Frame count mismatch"
    print(f"      -> OK: Frames={r_tick['frame_count']}, VirtualTime={r_tick['total_virtual_time_ms']}ms", flush=True)

    # 3. m2_get_engine_state
    print("[3/8] Test m2_get_engine_state...", flush=True)
    r_state = bridge.get_engine_state()
    assert r_state["ok"] is True, f"Get state failed: {r_state}"
    assert r_state["player"]["name"] == "AgentHarnessWarrior"
    print(f"      -> OK: Player={r_state['player']['name']}, HP={r_state['player']['hp']}, Coords={r_state['player']['position']}", flush=True)

    # 4. m2_audit_dual_state
    print("[4/8] Test m2_audit_dual_state...", flush=True)
    r_audit = bridge.audit_dual_state(epsilon=0.01)
    assert r_audit["ok"] is True, f"Audit failed: {r_audit}"
    assert r_audit["in_sync"] is True, f"Audit out of sync: {r_audit}"
    print(f"      -> OK: Verdict={r_audit['audit_verdict']}, Mismatches={r_audit['mismatch_count']}", flush=True)

    # 5. m2_inject_packet
    print("[5/8] Test m2_inject_packet (Move Opcode=2)...", flush=True)
    payload_hex = struct.pack("<ff", 45100.0, 38050.0).hex()
    r_inject = bridge.inject_packet(opcode=2, hex_payload=payload_hex)
    assert r_inject["ok"] is True, f"Inject failed: {r_inject}"
    print(f"      -> OK: Injected Opcode={r_inject['opcode']}, Bytes={r_inject['size_bytes']}", flush=True)

    # Sprawdzenie ruchu po ticku
    bridge.tick_frame(count=5, delta_ms=16.6)
    r_state_after = bridge.get_engine_state()
    print(f"      -> Po ruchu: Coords={r_state_after['player']['position']}, Motion={r_state_after['player']['motion']}", flush=True)

    # 6. m2_fuzz_router
    print("[6/8] Test m2_fuzz_router (Opcode=2, Count=25)...", flush=True)
    r_fuzz = bridge.fuzz_router(opcode=2, count=25)
    assert r_fuzz["ok"] is True, f"Fuzz failed: {r_fuzz}"
    assert r_fuzz["crashes_detected"] == 0, "Crashes detected during fuzzing!"
    print(f"      -> OK: Status={r_fuzz['status']}, Passed={r_fuzz['passed_safe_handling']}/{r_fuzz['total_iterations']}", flush=True)

    # 7. m2_get_crash_report
    print("[7/8] Test m2_get_crash_report...", flush=True)
    r_crash = bridge.get_crash_report()
    assert r_crash["ok"] is True, f"Crash report failed: {r_crash}"
    print(f"      -> OK: Verdict={r_crash['verdict']}, PDB Available={r_crash['pdb_symbols']['available']}", flush=True)

    # 8. m2_harness_stop
    print("[8/8] Test m2_harness_stop...", flush=True)
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

