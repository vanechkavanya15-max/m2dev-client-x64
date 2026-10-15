#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
check_layer_boundaries.py - Straznik Granic Architektonicznych (Filar CI / Faza 0)

Weryfikuje scisle reguly architektoniczne warstw silnika klienta gry:
1. src/Client/ nie moze importowac bezposrednio UserInterface/*.h (dozwolone wylacznie UserInterface/Contracts/* oraz whitelista legacy).
2. src/EterBase/ nie moze zalezec od wyzszych warstw (EterLib, GameLib, UserInterface, Client).
3. src/EterLib/ nie moze zalezec od GameLib, UserInterface, Client.
4. src/GameLib/ nie moze zalezec od UserInterface, Client (np. odwrocona zaleznosc GameLib -> UserInterface/Core/EventBus.h).
5. Liczba naruszen na whiteliscie legacy nie moze rosnac.
"""

import sys
import os
import re
import json
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = REPO_ROOT / "src"
WHITELIST_FILE = REPO_ROOT / "tools" / "layer_boundary_whitelist.json"

def scan_includes(file_path):
    """Zwraca liste krotek (nr_linii, docelowy_include)"""
    results = []
    include_regex = re.compile(r'^\s*#\s*include\s*["<]([^">]+)[">]')
    try:
        with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
            for idx, line in enumerate(f, 1):
                match = include_regex.match(line)
                if match:
                    results.append((idx, match.group(1).replace("\\", "/")))
    except Exception as e:
        print(f"[BLAD] Nie mozna odczytac {file_path}: {e}", file=sys.stderr)
    return results

def collect_all_violations():
    """Przeszukuje repozytorium i zwraca liste wszystkich naruszen."""
    violations = []

    for root, _, files in os.walk(SRC_DIR):
        for fname in files:
            if not fname.endswith((".h", ".hpp", ".cpp", ".c", ".inl")):
                continue

            full_path = Path(root) / fname
            rel_path = full_path.relative_to(REPO_ROOT).as_posix()
            includes = scan_includes(full_path)

            for line_no, inc in includes:
                # Regula 1: src/Client/ nie moze importowac UserInterface poza Contracts
                if rel_path.startswith("src/Client/"):
                    if "UserInterface/" in inc and not ("UserInterface/Contracts/" in inc or inc.startswith("UserInterface/Contracts/")):
                        violations.append({
                            "rule": "Client -> UserInterface (nielegalny import z legacy)",
                            "source_file": rel_path,
                            "line_no": line_no,
                            "target_inc": inc,
                            "key": f"{rel_path}:{inc}",
                            "solution": "Wyekstrahuj interfejs do src/Contracts/ lub uzyj EventBus/Contracts."
                        })

                # Regula 2: src/EterBase/ nie moze zalezec od EterLib, GameLib, UserInterface, Client
                if rel_path.startswith("src/EterBase/"):
                    for forbidden in ["EterLib/", "GameLib/", "UserInterface/", "Client/"]:
                        if forbidden in inc:
                            violations.append({
                                "rule": f"EterBase -> {forbidden} (odwrocona zaleznosc fundamentu)",
                                "source_file": rel_path,
                                "line_no": line_no,
                                "target_inc": inc,
                                "key": f"{rel_path}:{inc}",
                                "solution": "Przenies logike wyzej lub uzyj polimorfizmu szablonowego."
                            })

                # Regula 3: src/EterLib/ nie moze zalezec od GameLib, UserInterface, Client
                if rel_path.startswith("src/EterLib/"):
                    for forbidden in ["GameLib/", "UserInterface/", "Client/"]:
                        if forbidden in inc:
                            violations.append({
                                "rule": f"EterLib -> {forbidden} (naruszenie warstwy grafiki/renderera)",
                                "source_file": rel_path,
                                "line_no": line_no,
                                "target_inc": inc,
                                "key": f"{rel_path}:{inc}",
                                "solution": "Grafika nie moze zalezec od logiki gry ani UI."
                            })

                # Regula 4: src/GameLib/ nie moze zalezec od UserInterface, Client
                if rel_path.startswith("src/GameLib/"):
                    for forbidden in ["UserInterface/", "Client/"]:
                        if forbidden in inc:
                            violations.append({
                                "rule": f"GameLib -> {forbidden} (naruszenie warstwy swiata gry)",
                                "source_file": rel_path,
                                "line_no": line_no,
                                "target_inc": inc,
                                "key": f"{rel_path}:{inc}",
                                "solution": "Logika swiata/map/obiektow nie moze zalezec od klienta gry ani UI."
                            })

    return violations

def generate_whitelist():
    """Generuje baseline plik whitelisty z obecnego stanu kodu."""
    violations = collect_all_violations()
    keys = sorted(list(set(v["key"] for v in violations)))
    data = {
        "description": "Whitelista znanych legacy couplingow (Faza 0 Baseline). Zadaniem kolejnych faz jest redukcja tych wpisow do 0.",
        "baseline_count": len(keys),
        "entries": keys
    }
    with open(WHITELIST_FILE, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    print(f"[OK] Wygenerowano whiteliste z {len(keys)} naruszeniami w {WHITELIST_FILE}")

def check_layer_boundaries():
    print("=" * 70)
    print("URUCHAMIANIE STRAZNIKA GRANIC ARCHITEKTONICZNYCH (check_layer_boundaries)")
    print("=" * 70)

    if not WHITELIST_FILE.exists():
        print(f"[OSTRZEZENIE] Brak pliku whitelisty {WHITELIST_FILE}. Generuje baseline...")
        generate_whitelist()

    with open(WHITELIST_FILE, "r", encoding="utf-8") as f:
        whitelist_data = json.load(f)
    
    whitelist_entries = set(whitelist_data.get("entries", []))
    baseline_limit = whitelist_data.get("baseline_count", len(whitelist_entries))

    violations = collect_all_violations()
    
    active_keys = set(v["key"] for v in violations)
    new_violations = [v for v in violations if v["key"] not in whitelist_entries]
    resolved_count = len(whitelist_entries - active_keys)

    print(f"[INFO] Wszystkich naruszen architektonicznych: {len(violations)}")
    print(f"[INFO] Whitelistowane naruszenia legacy: {len(whitelist_entries)} (limit: {baseline_limit})")
    print(f"[INFO] Rozwiazane/naprawione naruszenia: {resolved_count}")

    # Czy pojawily sie nowe naruszenia?
    if new_violations:
        print("\n" + "!" * 70)
        print(f"[BLAD KRYTYCZNY] Wykryto {len(new_violations)} NOWYCH naruszen granic architektonicznych:")
        print("!" * 70)
        for v in new_violations:
            print(f"  * {v['rule']}")
            print(f"    Lokalizacja: {v['source_file']}:{v['line_no']}")
            print(f"    Import:      #include \"{v['target_inc']}\"")
            print(f"    Zalecenie:   {v['solution']}\n")
        print("Build zatrzymany. Nowe zaleznosci miedzywarstwowe sa ZABRONIONE.")
        return 1

    # Czy laczna liczba nie przekroczyla limitu?
    if len(violations) > baseline_limit:
        print(f"\n[BLAD KRYTYCZNY] Liczba naruszen ({len(violations)}) przekroczyla limit baseline ({baseline_limit})!")
        return 1

    print("\n[SUKCES] Brak nowych naruszen architektonicznych! Granice warstw sa nienaruszone.")
    return 0

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--generate-whitelist":
        generate_whitelist()
        sys.exit(0)
    sys.exit(check_layer_boundaries())
