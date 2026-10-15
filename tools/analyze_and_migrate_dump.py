#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ANALYZE AND MIGRATE DUMP - Kompleksowe narzedzie inspekcji i migracji zrzutu klienta gry.
Skanuje wskazany katalog zrzutu (np. E:\\Alune-AkademiaDUMP), kategoryzuje zasoby,
analizuje pokrycie konwersji glTF 2.0 i opcjonalnie uruchamia wielowatkowa konwersje.

Zasady: Czysty ASCII, brak znakow diakrytycznych.
"""

import os
import sys
import argparse
import subprocess
import time
from collections import defaultdict
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed

CATEGORIES = {
    "models_gr2": [".gr2"],
    "models_glb": [".glb", ".gltf"],
    "textures": [".dds", ".tga", ".png", ".jpg", ".bmp", ".sub"],
    "motion_scripts": [".msm", ".msa", ".mse", ".mde"],
    "proto_databases": ["item_proto", "mob_proto", "item_names.txt", "mob_names.txt", "item_proto.txt", "mob_proto.txt"],
    "python_scripts": [".py"],
    "maps_and_terrain": [".raw", ".atr", ".prb", ".prd", ".prt"],
    "audio": [".wav", ".mp3", ".ogg"],
}

def find_converter_exe(custom_path=None):
    if custom_path and os.path.isfile(custom_path):
        return custom_path

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

def convert_single_gr2(converter_exe, input_file, output_file, overwrite=False):
    if not overwrite and os.path.exists(output_file):
        return (True, input_file, "Pominieto")

    cmd = [converter_exe, "--input", str(input_file), "--output", str(output_file)]
    try:
        res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=60)
        if res.returncode == 0:
            return (True, input_file, "OK")
        else:
            return (False, input_file, f"Blad ({res.returncode}): {res.stderr.strip() or res.stdout.strip()}")
    except Exception as e:
        return (False, input_file, f"Wyjatek: {str(e)}")

def scan_dump_directory(dump_path: Path):
    print(f"[*] Rozpoczynam skanowanie katalogu zrzutu: {dump_path}...")
    start_time = time.time()

    stats = {
        "total_files": 0,
        "categories": defaultdict(int),
        "dir_breakdown": defaultdict(lambda: defaultdict(int)),
        "unconverted_gr2": [],
        "converted_gr2": 0,
    }

    for root, dirs, files in os.walk(dump_path):
        rel_root = Path(root).relative_to(dump_path)
        top_dir = rel_root.parts[0] if rel_root.parts else "root"

        for f in files:
            stats["total_files"] += 1
            f_lower = f.lower()
            p = Path(root) / f
            suffix = p.suffix.lower()

            categorized = False
            # Check proto database files
            for proto_name in CATEGORIES["proto_databases"]:
                if proto_name in f_lower:
                    stats["categories"]["proto_databases"] += 1
                    stats["dir_breakdown"][top_dir]["proto_databases"] += 1
                    categorized = True
                    break

            if categorized:
                continue

            for cat, exts in CATEGORIES.items():
                if suffix in exts:
                    stats["categories"][cat] += 1
                    stats["dir_breakdown"][top_dir][cat] += 1
                    categorized = True
                    break

            if not categorized:
                stats["categories"]["other"] += 1
                stats["dir_breakdown"][top_dir]["other"] += 1

            if suffix == ".gr2":
                glb_counterpart = p.with_suffix(".glb")
                if glb_counterpart.exists():
                    stats["converted_gr2"] += 1
                else:
                    stats["unconverted_gr2"].append(p)

    elapsed = time.time() - start_time
    print(f"[*] Skanowanie zakonczone w {elapsed:.2f} s. Przeanalizowano {stats['total_files']} plikow.")
    return stats

def print_summary(stats, dump_path):
    print("\n" + "=" * 70)
    print("=== RAPORT ANALIZY ZRZUTU KLIENTA GRY ===")
    print("=" * 70)
    print(f"Katalog zrodlowy:       {dump_path}")
    print(f"Laczna liczba plikow:   {stats['total_files']}\n")

    print("Kategorie zasobow:")
    for cat, count in sorted(stats["categories"].items(), key=lambda x: x[1], reverse=True):
        print(f"  - {cat:<22}: {count:>6} plikow")

    total_gr2 = stats["categories"].get("models_gr2", 0)
    converted = stats["converted_gr2"]
    unconverted = len(stats["unconverted_gr2"])
    pct = (converted / total_gr2 * 100) if total_gr2 > 0 else 0.0

    print("\nStan migracji modeli 3D (.gr2 -> .glb):")
    print(f"  - Lacznie modeli .gr2:  {total_gr2}")
    print(f"  - Skonwertowane (.glb): {converted} ({pct:.1f}%)")
    print(f"  - Do konwersji:         {unconverted}")

    print("\nPodzial wedlug glownych podkatalogow (Top Folders):")
    for top_dir, cats in sorted(stats["dir_breakdown"].items(), key=lambda x: sum(x[1].values()), reverse=True)[:15]:
        total_in_dir = sum(cats.values())
        gr2_in_dir = cats.get("models_gr2", 0)
        glb_in_dir = cats.get("models_glb", 0)
        print(f"  [{top_dir:<22}] Lacznie: {total_in_dir:>5} | .gr2: {gr2_in_dir:>4} | .glb: {glb_in_dir:>4}")
    print("=" * 70 + "\n")

def export_markdown_report(stats, dump_path, output_md):
    with open(output_md, "w", encoding="utf-8") as f:
        f.write("# AUDYT ZRZUTU KLIENTA GRY I RAPORT GOTOWOSCI MIGRACJI\n\n")
        f.write(f"- **Katalog zrzutu**: `{dump_path}`\n")
        f.write(f"- **Data audytu**: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write(f"- **Laczna liczba plikow**: {stats['total_files']}\n\n")

        f.write("## 1. Pokrycie Zasobow Wedlug Kategorii\n\n")
        f.write("| Kategoria Zasobow | Liczba Plikow | Opis |\n")
        f.write("|---|---|---|\n")
        f.write(f"| `models_gr2` | {stats['categories'].get('models_gr2', 0)} | Modele 3D i animacje Granny (.gr2) |\n")
        f.write(f"| `models_glb` | {stats['categories'].get('models_glb', 0)} | Nowe modele glTF 2.0 (.glb) |\n")
        f.write(f"| `textures` | {stats['categories'].get('textures', 0)} | Tekstury (.dds, .tga, .png, .sub) |\n")
        f.write(f"| `motion_scripts` | {stats['categories'].get('motion_scripts', 0)} | Skrypty ruchu i animacji (.msm, .msa, .mse) |\n")
        f.write(f"| `proto_databases` | {stats['categories'].get('proto_databases', 0)} | Bazy prototypow item/mob proto |\n")
        f.write(f"| `python_scripts` | {stats['categories'].get('python_scripts', 0)} | Skrypty Pythona (UI i mechanika gry) |\n")
        f.write(f"| `maps_and_terrain` | {stats['categories'].get('maps_and_terrain', 0)} | Teren, wysokosci i kolizje (.raw, .atr) |\n")
        f.write(f"| `audio` | {stats['categories'].get('audio', 0)} | Sciezki dzwiekowe i efekty (.wav, .mp3) |\n\n")

        total_gr2 = stats["categories"].get("models_gr2", 0)
        converted = stats["converted_gr2"]
        unconverted = len(stats["unconverted_gr2"])
        pct = (converted / total_gr2 * 100) if total_gr2 > 0 else 0.0

        f.write("## 2. Status Migracji glTF 2.0\n\n")
        f.write(f"- Modele Granny (.gr2): **{total_gr2}**\n")
        f.write(f"- Przekonwertowane (.glb): **{converted}** ({pct:.1f}%)\n")
        f.write(f"- Oczekujace na konwersje: **{unconverted}**\n\n")

        f.write("## 3. Top Podkatalogi\n\n")
        f.write("| Katalog | Lacznie Plikow | Pliki .gr2 | Pliki .glb |\n")
        f.write("|---|---|---|---|\n")
        for top_dir, cats in sorted(stats["dir_breakdown"].items(), key=lambda x: sum(x[1].values()), reverse=True)[:25]:
            f.write(f"| `{top_dir}` | {sum(cats.values())} | {cats.get('models_gr2', 0)} | {cats.get('models_glb', 0)} |\n")

    print(f"[*] Raport Markdown zapisany: {output_md}")

def run_migration(stats, converter_exe, threads=8, overwrite=False):
    tasks = []
    for gr2_path in stats["unconverted_gr2"]:
        glb_path = gr2_path.with_suffix(".glb")
        tasks.append((gr2_path, glb_path))

    if not tasks:
        print("[*] Brak plikow do konwersji (wszystkie modele posiadaja juz odpowiedniki .glb).")
        return

    print(f"\n[*] Rozpoczynam masowa konwersje {len(tasks)} modeli na {threads} watkach...")
    start_time = time.time()
    success = 0
    fail = 0

    with ThreadPoolExecutor(max_workers=threads) as executor:
        futures = {
            executor.submit(convert_single_gr2, converter_exe, inp, outp, overwrite): inp
            for inp, outp in tasks
        }
        for future in as_completed(futures):
            ok, inp_path, msg = future.result()
            if ok:
                success += 1
            else:
                fail += 1
                print(f"[BLAD] {inp_path}: {msg}")

    elapsed = time.time() - start_time
    print(f"[*] Konwersja zakonczona w {elapsed:.2f} s. Sukces: {success}, Bledy: {fail}.\n")

def main():
    parser = argparse.ArgumentParser(description="Audyt i migracja zrzutu klienta Metin2")
    parser.add_argument("--dir", required=True, help="Sciezka do katalogu zrzutu klienta (np. E:\\Alune-AkademiaDUMP)")
    parser.add_argument("--report", default=None, help="Sciezka do eksportu raportu Markdown")
    parser.add_argument("--migrate-models", action="store_true", help="Uruchom konwersje brakujacych modeli .gr2 do .glb")
    parser.add_argument("--threads", type=int, default=os.cpu_count() or 8, help="Liczba watkow")
    parser.add_argument("--converter", default=None, help="Sciezka do konwertera gr2_to_glb.exe")
    parser.add_argument("--overwrite", action="store_true", help="Wymus nadpisywanie istniejacych plikow .glb")
    args = parser.parse_args()

    dump_path = Path(args.dir).resolve()
    if not dump_path.is_dir():
        print(f"BLAD: Katalog nie istnieje: {dump_path}")
        sys.exit(1)

    stats = scan_dump_directory(dump_path)
    print_summary(stats, dump_path)

    if args.report:
        export_markdown_report(stats, dump_path, Path(args.report).resolve())

    if args.migrate_models:
        converter = find_converter_exe(args.converter)
        if not converter:
            print("BLAD: Nie znaleziono gr2_to_glb.exe!")
            sys.exit(1)
        run_migration(stats, converter, threads=args.threads, overwrite=args.overwrite)

if __name__ == "__main__":
    main()
