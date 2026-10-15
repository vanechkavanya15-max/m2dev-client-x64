#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
check_singleton_count.py - Straznik Liczby Singletonow (Filar CI / Faza 0)

Weryfikuje liczbe klas dziedziczacych po CSingleton/Singleton w kodzie zrodlowym silnika.
Baseline: 51 klas singletonowych.
Cel architektury AI-Native 2026: ZAKAZ tworzenia nowych singletonow; stopniowa migracja na ServiceRegistry/DI.
"""

import sys
import os
import re
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = REPO_ROOT / "src"
BASELINE_MAX_SINGLETONS = 51

SINGLETON_REGEX = re.compile(r'class\s+([A-Za-z0-9_]+)\s*:[^;{]*\b(?:public|protected|private)?\s*(?:C?Singleton)\s*<([A-Za-z0-9_]+)>')

def collect_singletons():
    singletons = []

    for root, _, files in os.walk(SRC_DIR):
        for fname in files:
            if not fname.endswith((".h", ".hpp", ".hxx")):
                continue

            full_path = Path(root) / fname
            rel_path = full_path.relative_to(REPO_ROOT).as_posix()

            try:
                with open(full_path, "r", encoding="utf-8", errors="ignore") as f:
                    content = f.read()
                    matches = SINGLETON_REGEX.findall(content)
                    for cls_name, t_param in matches:
                        singletons.append({
                            "class": cls_name,
                            "type": t_param,
                            "file": rel_path
                        })
            except Exception as e:
                print(f"[BLAD] Nie mozna odczytac {rel_path}: {e}", file=sys.stderr)

    return singletons

def check_singleton_count():
    print("=" * 70)
    print("URUCHAMIANIE STRAZNIKA LICZBY SINGLETONOW (check_singleton_count)")
    print("=" * 70)

    singletons = collect_singletons()
    unique_classes = sorted(list(set(s["class"] for s in singletons)))
    count = len(unique_classes)

    print(f"[INFO] Wykryto unikalnych klas singletonowych: {count} (Limit baseline: {BASELINE_MAX_SINGLETONS})")
    
    # Podzial na moduly
    module_counts = {}
    for s in singletons:
        top_mod = s["file"].split("/")[1] if "/" in s["file"] else "Root"
        module_counts[top_mod] = module_counts.get(top_mod, 0) + 1

    print("\n[INFO] Rozklad singletonow wedlug modulow:")
    for mod, c in sorted(module_counts.items(), key=lambda x: -x[1]):
        print(f"  - src/{mod:<15}: {c} singletonow")

    if count > BASELINE_MAX_SINGLETONS:
        print("\n" + "!" * 70)
        print(f"[BLAD KRYTYCZNY] Liczba singletonow ({count}) PRZEKROCZYLA limit baseline ({BASELINE_MAX_SINGLETONS})!")
        print("!" * 70)
        print("Nowe singletony sa bezwzglednie ZAKAZANE w architekturze klienta 2026.")
        print("Zamiast CSingleton zastosuj wstrzykiwanie zaleznosci (DI) lub ServiceRegistry.")
        return 1

    reduced = BASELINE_MAX_SINGLETONS - count
    if reduced > 0:
        print(f"\n[SUKCES] Zredukowano {reduced} singletonow w stosunku do pierwotnego stanu!")
    else:
        print(f"\n[SUKCES] Liczba singletonow w normie ({count}/{BASELINE_MAX_SINGLETONS}). Brak nowych singletonow.")

    return 0

if __name__ == "__main__":
    sys.exit(check_singleton_count())
