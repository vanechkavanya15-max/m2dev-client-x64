#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Wsadowy konwerter Granny (.gr2) do glTF 2.0 (.glb) dla klienta Metin2 x64.
Automatycznie przeszukuje wskazany katalog i konwertuje modele uzywajac gr2_to_glb.exe.
"""

import os
import sys
import argparse
import subprocess
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

def find_converter_exe(custom_path=None):
    if custom_path and os.path.isfile(custom_path):
        return custom_path

    # Standardowe sciezki w projekcie
    script_dir = Path(__file__).resolve().parent
    repo_root = script_dir.parent
    candidates = [
        repo_root / "build" / "tools" / "gr2_to_glb" / "Release" / "gr2_to_glb.exe",
        repo_root / "build" / "tools" / "gr2_to_glb" / "gr2_to_glb.exe",
        repo_root / "bin" / "gr2_to_glb.exe",
        script_dir / "gr2_to_glb.exe",
    ]
    for c in candidates:
        if c.is_file():
            return str(c)
    return None

def convert_single_file(converter_exe, input_file, output_file, overwrite=False):
    if not overwrite and os.path.exists(output_file):
        return (True, input_file, "Pominieto (plik istnieje)")

    cmd = [converter_exe, "--input", str(input_file), "--output", str(output_file)]
    try:
        res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=60)
        if res.returncode == 0:
            return (True, input_file, "OK")
        else:
            return (False, input_file, f"Blad ({res.returncode}): {res.stderr.strip() or res.stdout.strip()}")
    except Exception as e:
        return (False, input_file, f"Wyjatek: {str(e)}")

def main():
    parser = argparse.ArgumentParser(description="Wsadowy konwerter Metin2 GR2 -> GLB")
    parser.add_argument("--dir", required=True, help="Katalog wejsciowy z plikami .gr2")
    parser.add_argument("--outdir", default=None, help="Opcjonalny katalog wyjsciowy (domyslnie obok pliku .gr2)")
    parser.add_argument("--converter", default=None, help="Sciezka do pliku gr2_to_glb.exe")
    parser.add_argument("--threads", type=int, default=os.cpu_count() or 4, help="Liczba watkow")
    parser.add_argument("--overwrite", action="store_true", help="Nadpisuj istniejace pliki .glb")
    args = parser.parse_args()

    converter = find_converter_exe(args.converter)
    if not converter:
        print("BLAD: Nie znaleziono gr2_to_glb.exe. Zbuduj projekt w CMake!")
        sys.exit(1)

    print(f"Uzyty konwerter: {converter}")
    input_root = Path(args.dir).resolve()
    if not input_root.is_dir():
        print(f"BLAD: Katalog wejsciowy nie istnieje: {input_root}")
        sys.exit(1)

    print(f"Skanowanie katalogu: {input_root}...")
    gr2_files = list(input_root.rglob("*.gr2")) + list(input_root.rglob("*.GR2"))
    # Usun duplikaty jesli system jest case-insensitive
    unique_gr2 = list({f.resolve(): f for f in gr2_files}.values())
    total_files = len(unique_gr2)
    print(f"Znaleziono {total_files} plikow .gr2.")

    if total_files == 0:
        print("Brak plikow do konwersji.")
        return

    out_root = Path(args.outdir).resolve() if args.outdir else None

    tasks = []
    for f in unique_gr2:
        if out_root:
            rel = f.relative_to(input_root)
            out_file = (out_root / rel).with_suffix(".glb")
            out_file.parent.mkdir(parents=True, exist_ok=True)
        else:
            out_file = f.with_suffix(".glb")
        tasks.append((f, out_file))

    start_time = time.time()
    success_count = 0
    fail_count = 0
    skip_count = 0

    print(f"Uruchamianie konwersji na {args.threads} watkach...")
    with ThreadPoolExecutor(max_workers=args.threads) as executor:
        futures = {
            executor.submit(convert_single_file, converter, inp, outp, args.overwrite): inp
            for inp, outp in tasks
        }
        for future in as_completed(futures):
            ok, inp_path, msg = future.result()
            if ok:
                if "Pominieto" in msg:
                    skip_count += 1
                else:
                    success_count += 1
            else:
                fail_count += 1
                print(f"[BLAD] {inp_path}: {msg}")

    elapsed = time.time() - start_time
    print("-" * 60)
    print(f"Podsumowanie konwersji:")
    print(f"  Lacznie plikow: {total_files}")
    print(f"  Sukces:         {success_count}")
    print(f"  Pominieto:      {skip_count}")
    print(f"  Bledy:          {fail_count}")
    print(f"  Czas trwania:   {elapsed:.2f} s")
    print("-" * 60)

if __name__ == "__main__":
    main()
