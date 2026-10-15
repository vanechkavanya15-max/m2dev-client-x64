#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
AI PREFLIGHT CHECK (C++ Fast Sanity & Anti-Crash Guard)
Wersja 1.0 dla Metin2 x64 Client2026.

Uruchamiany natychmiast po zmianach w kodzie C++ w ulamku sekundy (0.2s),
aby wychwycic krytyczne usterki przed uruchomieniem powolnej kompilacji MSVC / ctest.

Zgodnosc z zasadami projektu: wylacznie jezyk polski BEZ polskich znakow diakrytycznych.

Weryfikowane reguly:
1. Format String Mismatch (TraceError, printf, sprintf, snprintf)
2. Singleton Crash Detector (CPythonPlayer / CPythonCharacterManager ::Instance() w src/Client/)
3. C-ABI Structure Alignment Check (sizeof(ActorSnapshot) == 32B w BotApi_FFI.h)
4. CTest Registry Check (rejestracja plikow test_*.cpp w tests/CMakeLists.txt)
"""

import os
import sys
import re
import time
import subprocess
from pathlib import Path
from typing import List, Dict, Tuple, Optional, Set


# ============================================================================
# KONFIGURACJA I TYPY
# ============================================================================

TYPE_ALIGNMENT_MAP: Dict[str, Tuple[int, int]] = {
    # typ: (rozmiar_w_bajtach, naturalne_wyrownanie)
    'uint8_t': (1, 1),
    'int8_t': (1, 1),
    'char': (1, 1),
    'unsigned char': (1, 1),
    'BYTE': (1, 1),
    'bool': (1, 1),
    'uint16_t': (2, 2),
    'int16_t': (2, 2),
    'short': (2, 2),
    'unsigned short': (2, 2),
    'WORD': (2, 2),
    'uint32_t': (4, 4),
    'int32_t': (4, 4),
    'int': (4, 4),
    'unsigned int': (4, 4),
    'DWORD': (4, 4),
    'float': (4, 4),
    'uint64_t': (8, 8),
    'int64_t': (8, 8),
    'double': (8, 8),
}

FUNC_FMT_ARG_INDEX: Dict[str, int] = {
    'TraceError': 0,
    'printf': 0,
    'sprintf': 1,
    'wsprintf': 1,
    'wsprintfA': 1,
    'wsprintfW': 1,
    'snprintf': 2,
    '_snprintf': 2,
}


# ============================================================================
# MODUL 1: FORMAT STRING MISMATCH
# ============================================================================

def count_format_specifiers(fmt_str: str) -> int:
    """Zlicza wymagana liczbe argumentow w ciagu formatujacym printf."""
    # Podwojny procent '%%' nie wymaga argumentu
    clean_fmt = fmt_str.replace('%%', '')
    
    # Standardowe specyfikatory konwersji printf z uwzglednieniem flag i modyfikatorow
    specifier_re = re.compile(
        r'%[-+ 0#]*(\*|\d+)?(?:\.(\*|\d+))?(?:hh|h|ll|l|L|z|j|t|I64|I32|I)?([diuoxXfFeEgGaAcspn])'
    )
    
    arg_count = 0
    for m in specifier_re.finditer(clean_fmt):
        arg_count += 1
        width = m.group(1)
        prec = m.group(2)
        if width == '*':
            arg_count += 1
        if prec == '*':
            arg_count += 1
            
    return arg_count


def extract_string_literal(arg_expr: str) -> Optional[str]:
    """Ekstrahuje sklejone literaly lancuchowe, np. 'czesc ' 'swiecie'."""
    matches = re.findall(r'"((?:[^"\\]|\\.)*)"', arg_expr)
    if matches:
        return "".join(matches)
    return None


def scan_file_format_strings(file_path: Path) -> List[str]:
    """Skanuje plik .cpp pod katem bledow liczby argumentow w TraceError/sprintf/printf."""
    errors: List[str] = []
    
    try:
        content = file_path.read_text(encoding='utf-8', errors='ignore')
    except Exception as exc:
        return [f"Nie mozna odczytac pliku {file_path}: {exc}"]

    # Szybki filtr wstepny - jesli brak slow kluczowych, pomijamy caly plik natychmiast
    if not any(k in content for k in ("TraceError", "printf", "sprintf")):
        return []

    call_re = re.compile(r'\b(TraceError|printf|sprintf|snprintf|_snprintf|wsprintf|wsprintfA|wsprintfW)\s*\(')
    content_len = len(content)

    for match in call_re.finditer(content):
        func_name = match.group(1)
        fmt_arg_idx = FUNC_FMT_ARG_INDEX.get(func_name, 0)
        start_idx = match.end() - 1  # indeks znaku '('

        paren_depth = 0
        in_string = False
        in_char = False
        escape = False
        args: List[str] = []
        cur_arg: List[str] = []
        end_idx = -1

        i = start_idx
        while i < content_len:
            ch = content[i]

            if in_string:
                cur_arg.append(ch)
                if escape:
                    escape = False
                elif ch == '\\':
                    escape = True
                elif ch == '"':
                    in_string = False
            elif in_char:
                cur_arg.append(ch)
                if escape:
                    escape = False
                elif ch == '\\':
                    escape = True
                elif ch == "'":
                    in_char = False
            else:
                if ch == '"':
                    in_string = True
                    cur_arg.append(ch)
                elif ch == "'":
                    in_char = True
                    cur_arg.append(ch)
                elif ch == '(':
                    paren_depth += 1
                    if paren_depth > 1:
                        cur_arg.append(ch)
                elif ch == ')':
                    paren_depth -= 1
                    if paren_depth == 0:
                        end_idx = i
                        raw = ''.join(cur_arg).strip()
                        if raw:
                            args.append(raw)
                        break
                    else:
                        cur_arg.append(ch)
                elif ch == ',' and paren_depth == 1:
                    args.append(''.join(cur_arg).strip())
                    cur_arg = []
                elif ch == '/' and i + 1 < content_len and content[i + 1] == '/':
                    while i < content_len and content[i] != '\n':
                        i += 1
                    continue
                elif ch == '/' and i + 1 < content_len and content[i + 1] == '*':
                    i += 2
                    while i + 1 < content_len and not (content[i] == '*' and content[i + 1] == '/'):
                        i += 1
                    i += 1
                    continue
                else:
                    cur_arg.append(ch)
            i += 1

        if end_idx != -1 and len(args) > fmt_arg_idx:
            raw_fmt = args[fmt_arg_idx]
            fmt_str = extract_string_literal(raw_fmt)
            
            # Weryfikujemy wylacznie statyczne literaly lancuchowe
            if fmt_str is not None:
                expected_args = count_format_specifiers(fmt_str)
                provided_args = len(args) - (fmt_arg_idx + 1)
                
                if expected_args > provided_args:
                    line_no = content[:match.start()].count('\n') + 1
                    errors.append(
                        f"{file_path}:{line_no}: Blad format string w {func_name}(): "
                        f"ciag wymaga {expected_args} argumentow, a podano tylko {provided_args}! "
                        f"Format: \"{fmt_str}\""
                    )

    return errors


# ============================================================================
# MODUL 2: SINGLETON CRASH DETECTOR
# ============================================================================

def check_singleton_safety(root_dir: Path) -> List[str]:
    """
    Skanuje katalog src/Client/ w poszukiwaniu bezposrednich wywolan Instance().
    W headless/mock/test srodowisku brak inicializacji singletonu powoduje natychmiastowy crash.
    Wymagane jest uzycie InstancePtr() z weryfikacja nullptr.
    """
    client_dir = root_dir / "src" / "Client"
    if not client_dir.exists():
        return [f"Katalog {client_dir} nie istnieje!"]

    singleton_re = re.compile(r'\b(CPythonPlayer|CPythonCharacterManager)::Instance\(\)')
    errors: List[str] = []

    for file_path in client_dir.rglob('*'):
        if file_path.suffix in ('.cpp', '.h', '.hpp', '.inl'):
            try:
                text = file_path.read_text(encoding='utf-8', errors='ignore')
            except Exception:
                continue

            # Szybki test
            if "::Instance()" not in text:
                continue

            for line_no, line in enumerate(text.splitlines(), 1):
                clean_line = line.strip()
                # Pomin komentarze
                if clean_line.startswith("//") or clean_line.startswith("*"):
                    continue

                m = singleton_re.search(line)
                if m:
                    singleton_name = m.group(1)
                    rel_path = file_path.relative_to(root_dir)
                    errors.append(
                        f"{rel_path}:{line_no}: Blad bezpieczenstwa singletonu! "
                        f"Wykryto bezposrednie wywolanie {singleton_name}::Instance(). "
                        f"Wymagane jest bezpieczne InstancePtr() z ochrona przed nullptr."
                    )

    return errors


# ============================================================================
# MODUL 3: C-ABI STRUCTURE ALIGNMENT CHECK
# ============================================================================

def check_structure_alignment(root_dir: Path) -> List[str]:
    """
    Weryfikuje poprawnosc wyrownania pamieci oraz scisly rozmiar 32 bajtow
    struktury ActorSnapshot w pliku src/Client/Bridge/BotApi_FFI.h.
    """
    header_path = root_dir / "src" / "Client" / "Bridge" / "BotApi_FFI.h"
    if not header_path.exists():
        return [f"Plik {header_path} nie istnieje!"]

    try:
        text = header_path.read_text(encoding='utf-8', errors='ignore')
    except Exception as exc:
        return [f"Nie mozna odczytac {header_path}: {exc}"]

    # Szukamy definicji struktury ActorSnapshot
    match = re.search(r'struct\s+ActorSnapshot\s*\{([^}]+)\}', text)
    if not match:
        return [f"Nie znaleziono definicji struktury ActorSnapshot w {header_path}!"]

    struct_body = match.group(1)
    errors: List[str] = []

    # Sprawdzamy czy struktura jest otoczona #pragma pack(push, 1)
    if "#pragma pack(push, 1)" not in text:
        errors.append("Brak dyrektywy '#pragma pack(push, 1)' przed strukturami C-ABI!")

    offset = 0
    fields: List[Tuple[str, str, int, int]] = []

    for line in struct_body.splitlines():
        # Usun komentarze
        line = re.sub(r'//.*', '', line)
        line = re.sub(r'/\*.*?\*/', '', line).strip()
        if not line or not line.endswith(';'):
            continue
        line = line[:-1].strip()

        # Dopasuj typ z TYPE_ALIGNMENT_MAP (najdluzsze najpierw)
        matched_type: Optional[str] = None
        names_str: Optional[str] = None
        for t in sorted(TYPE_ALIGNMENT_MAP.keys(), key=len, reverse=True):
            if line.startswith(t + " ") or line.startswith(t + "\t"):
                matched_type = t
                names_str = line[len(t):].strip()
                break

        if not matched_type or not names_str:
            errors.append(f"Nieznany typ lub niepoprawna deklaracja pola: '{line}'")
            continue

        base_size, align = TYPE_ALIGNMENT_MAP[matched_type]
        var_names = [v.strip() for v in names_str.split(',') if v.strip()]

        for vname in var_names:
            # Obsluga tablic, np. arr[10]
            arr_m = re.search(r'\[(\d+)\]', vname)
            if arr_m:
                arr_len = int(arr_m.group(1))
                vname = re.sub(r'\[\d+\]', '', vname).strip()
                field_size = base_size * arr_len
            else:
                field_size = base_size

            # Sprawdzenie naturalnego wyrownania w obrebie bufora
            if offset % align != 0:
                errors.append(
                    f"Blad wyrownania C-ABI: pole '{vname}' (typ {matched_type}, wymagane {align}B) "
                    f"rozpoczyna sie na niewyrownanym offsetcie {offset}B (reszta: {offset % align})!"
                )

            fields.append((vname, matched_type, offset, field_size))
            offset += field_size

    total_size = offset
    if total_size != 32:
        errors.append(
            f"Blad rozmiaru C-ABI: sizeof(ActorSnapshot) wynosi {total_size} bajtow "
            f"(wymagane scisle dokladnie 32 bajty dla zero-copy FFI)!"
        )

    if total_size % 4 != 0:
        errors.append(
            f"Blad wyrownania calkowitego: laczny rozmiar {total_size}B nie jest wielokrotnoscia 4B!"
        )

    return errors


# ============================================================================
# MODUL 4: CTEST REGISTRY CHECK
# ============================================================================

def check_ctest_registry(root_dir: Path) -> List[str]:
    """
    Weryfikuje czy pliki testowe w tests/ (test_*.cpp) sa zarejestrowane
    w tests/CMakeLists.txt za pomoca add_executable oraz add_test.
    """
    tests_dir = root_dir / "tests"
    cmakelists_path = tests_dir / "CMakeLists.txt"

    if not cmakelists_path.exists():
        return [f"Plik {cmakelists_path} nie istnieje!"]

    try:
        cmake_text = cmakelists_path.read_text(encoding='utf-8', errors='ignore')
    except Exception as exc:
        return [f"Nie mozna odczytac {cmakelists_path}: {exc}"]

    # 1. Pobierz rejestracje z CMakeLists.txt
    re_exe = re.compile(r'add_executable\s*\(\s*([A-Za-z0-9_]+)\s+([^)]+)\)', re.DOTALL)
    executables: Dict[str, List[str]] = {}
    for m in re_exe.finditer(cmake_text):
        target = m.group(1)
        sources = [s.strip() for s in m.group(2).split() if s.strip().endswith('.cpp')]
        executables[target] = sources

    re_test = re.compile(r'add_test\s*\(\s*NAME\s+([A-Za-z0-9_]+)\s+COMMAND\s+([^)\n]+)\)')
    ctest_commands: Dict[str, str] = {}
    for m in re_test.finditer(cmake_text):
        test_name = m.group(1)
        cmd_target = m.group(2).split()[0]
        ctest_commands[test_name] = cmd_target

    errors: List[str] = []

    # 2. Sprawdz czy kazdy add_executable ma odpowiadajacy add_test
    for target in executables:
        if target not in ctest_commands.values():
            errors.append(
                f"Cel testowy '{target}' jest zdefiniowany w add_executable, "
                f"ale brakuje powiazanego wywolania add_test(NAME ... COMMAND {target})!"
            )

    # 3. Sprawdz czy pliki zrodel z add_executable fizycznie istnieja
    for target, sources in executables.items():
        for src_file in sources:
            if not src_file.startswith("${") and not src_file.startswith(".."):
                phys_path = tests_dir / src_file
                if not phys_path.exists():
                    errors.append(
                        f"Plik zrodlowy '{src_file}' zarejestrowany dla celu '{target}' nie istnieje w tests/!"
                    )

    # 4. Sprawdz zmodyfikowane/nowe pliki w gicie (staged/unstaged/untracked)
    git_test_files: Set[str] = set()
    try:
        git_res = subprocess.run(
            ["git", "status", "--porcelain"],
            cwd=str(root_dir),
            capture_output=True,
            text=True,
            timeout=3
        )
        if git_res.returncode == 0:
            for line in git_res.stdout.splitlines():
                if len(line) > 3:
                    path_str = line[3:].strip()
                    if path_str.startswith("tests/") and path_str.endswith(".cpp"):
                        filename = Path(path_str).name
                        if filename.startswith("test_") and filename != "test_main.cpp":
                            git_test_files.add(filename)
    except Exception:
        pass

    # Kazdy nowy lub zmieniony plik test_*.cpp musi byc w CMakeLists.txt
    for gfile in sorted(git_test_files):
        if gfile not in cmake_text:
            errors.append(
                f"Nowy/zmodyfikowany plik testowy 'tests/{gfile}' NIE jest zarejestrowany "
                f"w tests/CMakeLists.txt (brak add_executable / add_test)!"
            )

    # 5. Sprawdz aktywne pliki testowe od c27 wzwyz
    all_test_files = sorted([f.name for f in tests_dir.glob("test_*.cpp")])
    for tf in all_test_files:
        if tf == "test_main.cpp":
            continue
        # Wszystkie testy od c27 wzwyz bezwzglednie musza byc w CMakeLists.txt
        if not tf.startswith("test_c26_"):
            if tf not in cmake_text:
                errors.append(
                    f"Plik testowy 'tests/{tf}' nie jest zarejestrowany w tests/CMakeLists.txt!"
                )

    return errors


# ============================================================================
# MODUL 5: PYTHON TYPE STUBS AST VALIDATOR
# ============================================================================

def check_python_stubs(root_dir: Path) -> List[str]:
    """Weryfikuje poprawnosc skladniowa AST dla wszystkich plikow .pyi w assets/root/stubs/."""
    errors: List[str] = []
    stubs_dir = root_dir / "m2dev-client-main" / "assets" / "root" / "stubs"
    if not stubs_dir.exists():
        return errors

    import ast
    for pyi in stubs_dir.glob("*.pyi"):
        try:
            with open(pyi, "r", encoding="utf-8") as f:
                code = f.read()
            ast.parse(code)
        except Exception as e:
            errors.append(f"Blad skladni w stubie 'stubs/{pyi.name}': {e}")
    return errors


# ============================================================================
# MODUL 6: PROTOCOL MAP SCHEMA SANITY CHECK
# ============================================================================

def check_protocol_map_schema(root_dir: Path) -> List[str]:
    """Weryfikuje unikalnosc opkodow i poprawnosc strukturalna AI_PROTOCOL_MAP.json."""
    errors: List[str] = []
    json_path = root_dir / "docs" / "AI_PROTOCOL_MAP.json"
    if not json_path.exists():
        return errors

    import json
    try:
        with open(json_path, "r", encoding="utf-8") as f:
            data = json.load(f)
    except Exception as e:
        return [f"Blad parsowania JSON w AI_PROTOCOL_MAP.json: {e}"]

    for prof_name, prof_data in data.get("profiles", {}).items():
        for dir_name in ["opcodes_cg", "opcodes_gc"]:
            opcodes = prof_data.get(dir_name, {})
            seen: Dict[Tuple[str, str], str] = {}
            for name, val in opcodes.items():
                h = (val.get("hex") or f"0x{val.get('dec', 0):02X}").lower()
                phase = val.get("phase", "any")
                key = (h, phase)
                if key in seen:
                    errors.append(
                        f"Kolizja opkodu w '{prof_name}' ({dir_name}): "
                        f"opkod {h} w fazie '{phase}' uzyty przez '{seen[key]}' oraz '{name}'"
                    )
                else:
                    seen[key] = name
    return errors


# ============================================================================
# GLOWNY ORKIESTRATOR PREFLIGHT
# ============================================================================

def run_preflight() -> int:
    """Uruchamia wszystkie 6 preflight checks i mierzy calkowity czas wykonania."""
    t_start = time.perf_counter()

    script_path = Path(__file__).resolve()
    # Zakladamy, ze skrypt lezy w <ROOT>/tools/
    root_dir = script_path.parent.parent

    print("=" * 78)
    print("AI PREFLIGHT CHECK - Szybka Walidacja C++ przed Kompilacja (Client2026)")
    print(f"Katalog glowny projektu: {root_dir}")
    print("=" * 78)

    all_errors: List[Tuple[str, List[str]]] = []

    # ------------------------------------------------------------------------
    # KROK 1: Format String Mismatch
    # ------------------------------------------------------------------------
    t_step = time.perf_counter()
    src_cpp_files = list((root_dir / "src").rglob("*.cpp")) + list((root_dir / "tests").rglob("*.cpp"))
    fmt_errors: List[str] = []
    for cpp in src_cpp_files:
        errs = scan_file_format_strings(cpp)
        if errs:
            fmt_errors.extend(errs)
    t_step_dur = time.perf_counter() - t_step

    if fmt_errors:
        all_errors.append(("1. Format String Mismatch", fmt_errors))
        print(f"[-] [BLAD] Format String Mismatch ({len(fmt_errors)} usterek, {t_step_dur:.3f}s)")
        for e in fmt_errors[:5]:
            print(f"    * {e}")
        if len(fmt_errors) > 5:
            print(f"    * ... oraz {len(fmt_errors) - 5} kolejnych usterek.")
    else:
        print(f"[+] [OK] Format String Mismatch: brak bledow ({len(src_cpp_files)} plikow, {t_step_dur:.3f}s)")

    # ------------------------------------------------------------------------
    # KROK 2: Singleton Crash Detector
    # ------------------------------------------------------------------------
    t_step = time.perf_counter()
    singleton_errors = check_singleton_safety(root_dir)
    t_step_dur = time.perf_counter() - t_step

    if singleton_errors:
        all_errors.append(("2. Singleton Crash Detector", singleton_errors))
        print(f"[-] [BLAD] Singleton Crash Detector ({len(singleton_errors)} usterek, {t_step_dur:.3f}s)")
        for e in singleton_errors[:5]:
            print(f"    * {e}")
    else:
        print(f"[+] [OK] Singleton Crash Detector: pelna ochrona InstancePtr() ({t_step_dur:.3f}s)")

    # ------------------------------------------------------------------------
    # KROK 3: C-ABI Structure Alignment Check
    # ------------------------------------------------------------------------
    t_step = time.perf_counter()
    align_errors = check_structure_alignment(root_dir)
    t_step_dur = time.perf_counter() - t_step

    if align_errors:
        all_errors.append(("3. C-ABI Structure Alignment", align_errors))
        print(f"[-] [BLAD] C-ABI Structure Alignment ({len(align_errors)} usterek, {t_step_dur:.3f}s)")
        for e in align_errors:
            print(f"    * {e}")
    else:
        print(f"[+] [OK] C-ABI Structure Alignment: ActorSnapshot 32B wyrownany ({t_step_dur:.3f}s)")

    # ------------------------------------------------------------------------
    # KROK 4: CTest Registry Check
    # ------------------------------------------------------------------------
    t_step = time.perf_counter()
    ctest_errors = check_ctest_registry(root_dir)
    t_step_dur = time.perf_counter() - t_step

    if ctest_errors:
        all_errors.append(("4. CTest Registry Check", ctest_errors))
        print(f"[-] [BLAD] CTest Registry Check ({len(ctest_errors)} usterek, {t_step_dur:.3f}s)")
        for e in ctest_errors[:5]:
            print(f"    * {e}")
        if len(ctest_errors) > 5:
            print(f"    * ... oraz {len(ctest_errors) - 5} kolejnych usterek.")
    else:
        print(f"[+] [OK] CTest Registry Check: wszystkie aktywne testy zarejestrowane ({t_step_dur:.3f}s)")

    # ------------------------------------------------------------------------
    # KROK 5: Python Type Stubs AST Validator
    # ------------------------------------------------------------------------
    t_step = time.perf_counter()
    stubs_errors = check_python_stubs(root_dir)
    t_step_dur = time.perf_counter() - t_step

    if stubs_errors:
        all_errors.append(("5. Python Type Stubs AST Validator", stubs_errors))
        print(f"[-] [BLAD] Python Type Stubs ({len(stubs_errors)} usterek, {t_step_dur:.3f}s)")
        for e in stubs_errors[:5]:
            print(f"    * {e}")
    else:
        print(f"[+] [OK] Python Type Stubs: 100% poprawne skladniowo AST ({t_step_dur:.3f}s)")

    # ------------------------------------------------------------------------
    # KROK 6: Protocol Map Schema Sanity
    # ------------------------------------------------------------------------
    t_step = time.perf_counter()
    proto_errors = check_protocol_map_schema(root_dir)
    t_step_dur = time.perf_counter() - t_step

    if proto_errors:
        all_errors.append(("6. Protocol Map Schema Sanity", proto_errors))
        print(f"[-] [BLAD] Protocol Map Schema ({len(proto_errors)} usterek, {t_step_dur:.3f}s)")
        for e in proto_errors[:5]:
            print(f"    * {e}")
    else:
        print(f"[+] [OK] Protocol Map Schema: brak kolizji opkodow/faz ({t_step_dur:.3f}s)")

    # ------------------------------------------------------------------------
    # PODSUMOWANIE I STATUS WYJSCIOWY
    # ------------------------------------------------------------------------
    total_time = time.perf_counter() - t_start
    print("-" * 78)
    if not all_errors:
        print(f"[SUKCES] Preflight check ZALICZONY w 100%! Czas: {total_time:.3f}s")
        print("Kod spelnia wszystkie standardy. Mozna bezpiecznie uruchomic kompilacje C++.")
        return 0
    else:
        print(f"[PORAZKA] Wykryto usterki w {len(all_errors)} obszarach! Czas: {total_time:.3f}s")
        print("Popraw powyzsze bledy przed uruchomieniem kompilacji C++.")
        return 1


if __name__ == "__main__":
    sys.exit(run_preflight())
