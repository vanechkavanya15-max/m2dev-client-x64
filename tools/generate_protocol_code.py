#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generator Kodu Struktur Protokolow Sieciowych Metin2 (tools/generate_protocol_code.py)
Single Source of Truth: docs/AI_PROTOCOL_MAP.json

Generuje:
1. C++23 (--target cpp):
   - src/Client/Network/Protocol/Generated/LuriaProtocol_gen.h + luria_gc_table.inl
   - src/Client/Network/Protocol/Generated/StandardX64Protocol_gen.h + standard_x64_gc_table.inl
   - src/Client/Network/Protocol/Generated/BeaviumProtocol_gen.h + beavium_gc_table.inl
   - src/Client/Network/Protocol/Generated/MartySamaProtocol_gen.h + martysama_gc_table.inl
   Zawiera #pragma pack(push, 1), scisle typy, static_assert(sizeof(...) == X), tablice descriptorow.

2. C# (--target csharp):
   - E:/Metin2Clientless/Packets/Generated/LuriaPackets.cs (oraz BeaviumPackets.cs, MartySamaPackets.cs)
   Zawiera [StructLayout(LayoutKind.Sequential, Pack = 1)], stale opkodow i tablice deskryptorow.

3. Python (--target python):
   - m2dev-client-main/assets/root/stubs/protocol_packets.py
   Zawiera stale opkodow, klasy z prekompilowanymi struct.Struct, tabele deskryptorow GC.

Zgodnosc z zasadami projektu: wylacznie jezyk polski BEZ polskich znakow diakrytycznych.
"""

import os
import sys
import json
import struct
import argparse
from pathlib import Path
from typing import Dict, Any, List, Tuple, Optional


# ============================================================================
# BAZA DEDYKOWANYCH DEFINICJI POL DLA KLUCZOWYCH STRUKTUR
# ============================================================================

KNOWN_STRUCT_FIELDS: Dict[str, List[Tuple[str, str, str, str]]] = {
    # Format krotki: (typ_cpp, nazwa_pola_cpp, nazwa_pola_cs, format_struct_python)
    "TPacketCGAttackLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "bType", "Type", "B"),
        ("uint32_t", "dwVictimVID", "VictimVid", "I"),
        ("uint8_t", "bCRCMagicCubeProcPiece", "CrcProcPiece", "B"),
        ("uint8_t", "bCRCMagicCubeFilePiece", "CrcFilePiece", "B"),
    ],
    "TPacketCGMoveLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "bFunc", "Func", "B"),
        ("uint8_t", "bArg", "Arg", "B"),
        ("float", "fRot", "Rot", "f"),
        ("int32_t", "lX", "X", "i"),
        ("int32_t", "lY", "Y", "i"),
        ("uint32_t", "dwTime", "Time", "I"),
    ],
    "TPacketCGCreateCharacterLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "index", "Index", "B"),
        ("char", "name[25]", "Name", "25s"),
        ("uint16_t", "job", "Job", "H"),
        ("uint8_t", "shape", "Shape", "B"),
        ("uint8_t", "CON", "Con", "B"),
        ("uint8_t", "INT", "Int", "B"),
        ("uint8_t", "STR", "Str", "B"),
        ("uint8_t", "DEX", "Dex", "B"),
    ],
    "TPacketCGDestroyCharacterLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "index", "Index", "B"),
        ("char", "szPrivateCode[8]", "PrivateCode", "8s"),
    ],
    "TPacketCGSelectCharacterLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "index", "Index", "B"),
    ],
    "TPacketCGItemUseLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "window_type", "WindowType", "B"),
        ("uint16_t", "cell", "Cell", "H"),
    ],
    "TPacketCGItemDropLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "window_type", "WindowType", "B"),
        ("uint16_t", "cell", "Cell", "H"),
        ("uint32_t", "count", "Count", "I"),
    ],
    "TPacketCGItemMoveLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "srcWindow", "SrcWindow", "B"),
        ("uint16_t", "srcCell", "SrcCell", "H"),
        ("uint8_t", "dstWindow", "DstWindow", "B"),
        ("uint16_t", "dstCell", "DstCell", "H"),
        ("uint16_t", "count", "Count", "H"),
    ],
    "TPacketCGItemPickUpLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("uint32_t", "dwVID", "Vid", "I"),
    ],
    "TPacketCGTargetLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("uint32_t", "targetVid", "TargetVid", "I"),
    ],
    "TPacketCGEmpireLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "empire", "Empire", "B"),
    ],
    "TPacketCGLogin2Luria": [
        ("uint8_t", "header", "Header", "B"),
        ("char", "szLogin[31]", "Login", "31s"),
        ("uint32_t", "dwLoginKey", "LoginKey", "I"),
        ("uint32_t", "adwClientKeys[4]", "ClientKeys", "16s"),
    ],
    "TPacketCGAuthLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("char", "szLogin[31]", "Login", "31s"),
        ("char", "szPassword[17]", "Password", "17s"),
        ("char", "szPin[5]", "Pin", "5s"),
        ("char", "szToken[33]", "Token", "33s"),
        ("uint32_t", "adwClientKeys[4]", "ClientKeys", "16s"),
        ("char", "szHwidSha256[64]", "HwidSha256", "64s"),
        ("uint8_t", "bNullTerminator", "NullTerminator", "B"),
    ],
    "TPacketCGClientVersionLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("char", "szClientName[62]", "ClientName", "62s"),
        ("uint32_t", "dwTimestamp", "Timestamp", "I"),
    ],
    "TPacketCGKeyAgreementLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("uint16_t", "wType", "Type", "H"),
        ("uint16_t", "wLength", "Length", "H"),
        ("uint8_t", "abData[256]", "Data", "256s"),
    ],
    "TPacketCGHandshakeLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("uint32_t", "dwHandshake", "Handshake", "I"),
        ("uint32_t", "dwTime", "Time", "I"),
        ("int32_t", "lDelta", "Delta", "i"),
    ],
    "TPacketGCMoveLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "bFunc", "Func", "B"),
        ("uint8_t", "bArg", "Arg", "B"),
        ("uint8_t", "bRot", "Rot", "B"),
        ("uint32_t", "dwVID", "Vid", "I"),
        ("int32_t", "lX", "X", "i"),
        ("int32_t", "lY", "Y", "i"),
        ("uint32_t", "dwTime", "Time", "I"),
        ("uint32_t", "dwDuration", "Duration", "I"),
    ],
    "TPacketGCDeadLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("uint32_t", "dwVID", "Vid", "I"),
    ],
    "TPacketGCWarpLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("int32_t", "lX", "X", "i"),
        ("int32_t", "lY", "Y", "i"),
        ("int32_t", "lAddr", "Addr", "i"),
        ("uint16_t", "wPort", "Port", "H"),
    ],
    "TPacketGCTargetHPLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("uint32_t", "dwTargetVID", "TargetVid", "I"),
        ("int64_t", "llCurrentHP", "CurrentHp", "q"),
    ],
    "TPacketGCDamageInfoLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("uint32_t", "dwVID", "Vid", "I"),
        ("uint8_t", "flag", "Flag", "B"),
        ("int32_t", "damage", "Damage", "i"),
    ],
    "TPacketGCAuthSuccessLuria": [
        ("uint8_t", "header", "Header", "B"),
        ("uint32_t", "dwLoginKey", "LoginKey", "I"),
        ("uint8_t", "bResult", "Result", "B"),
    ],

    # MartySama
    "TPacketCGAttackMarty": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "bType", "Type", "B"),
        ("uint32_t", "dwVictimVID", "VictimVid", "I"),
        ("uint8_t", "bCRCMagicCubeProcPiece", "CrcProcPiece", "B"),
        ("uint8_t", "bCRCMagicCubeFilePiece", "CrcFilePiece", "B"),
    ],
    "TPacketCGMoveMarty": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "bFunc", "Func", "B"),
        ("uint8_t", "bArg", "Arg", "B"),
        ("uint8_t", "bRot", "Rot", "B"),
        ("int32_t", "lX", "X", "i"),
        ("int32_t", "lY", "Y", "i"),
        ("uint32_t", "dwTime", "Time", "I"),
    ],
    "TPacketCGItemUseMarty": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "window_type", "WindowType", "B"),
        ("uint16_t", "cell", "Cell", "H"),
    ],
    "TPacketCGTargetMarty": [
        ("uint8_t", "header", "Header", "B"),
        ("uint32_t", "targetVid", "TargetVid", "I"),
    ],
    "TPacketCGCreateCharacterMarty": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "index", "Index", "B"),
        ("char", "name[25]", "Name", "25s"),
        ("uint16_t", "job", "Job", "H"),
        ("uint8_t", "shape", "Shape", "B"),
        ("uint8_t", "con", "Con", "B"),
        ("uint8_t", "int_", "Int", "B"),
        ("uint8_t", "str", "Str", "B"),
        ("uint8_t", "dex", "Dex", "B"),
    ],
    "TPacketCGDestroyCharacterMarty": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "index", "Index", "B"),
        ("char", "privateCode[8]", "PrivateCode", "8s"),
    ],
    "TPacketCGSelectCharacterMarty": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "playerIndex", "PlayerIndex", "B"),
    ],
    "TPacketCGSelectEmpireMarty": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "empireId", "EmpireId", "B"),
    ],
    "TPacketGCMoveMarty": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "bFunc", "Func", "B"),
        ("uint8_t", "bArg", "Arg", "B"),
        ("uint8_t", "bRot", "Rot", "B"),
        ("uint32_t", "dwVID", "Vid", "I"),
        ("int32_t", "lX", "X", "i"),
        ("int32_t", "lY", "Y", "i"),
        ("uint32_t", "dwTime", "Time", "I"),
        ("uint32_t", "dwDuration", "Duration", "I"),
    ],
    "TPacketGCAttackMarty": [
        ("uint8_t", "header", "Header", "B"),
        ("uint32_t", "dwVID", "Vid", "I"),
        ("uint32_t", "dwVictimVID", "VictimVid", "I"),
        ("uint8_t", "bType", "Type", "B"),
    ],
    "TPacketGCDeadMarty": [
        ("uint8_t", "header", "Header", "B"),
        ("uint32_t", "dwVID", "Vid", "I"),
    ],
    "TPacketGCTargetMarty": [
        ("uint8_t", "header", "Header", "B"),
        ("uint32_t", "dwVID", "Vid", "I"),
        ("uint8_t", "bHPPercent", "HpPercent", "B"),
    ],
    "TPacketGCDamageInfoMarty": [
        ("uint8_t", "header", "Header", "B"),
        ("uint32_t", "dwVID", "Vid", "I"),
        ("uint8_t", "flag", "Flag", "B"),
        ("int32_t", "damage", "Damage", "i"),
    ],

    # Beavium
    "TPacketCGAttackBeavium": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "attackType", "AttackType", "B"),
        ("uint32_t", "dwVictimVID", "VictimVid", "I"),
        ("uint16_t", "wSeq", "Seq", "H"),
    ],
    "ProxyPacketCGAttack": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "attackType", "AttackType", "B"),
        ("uint32_t", "dwVictimVID", "VictimVid", "I"),
        ("uint16_t", "wSeq", "Seq", "H"),
    ],
    "TPacketCGMoveBeavium": [
        ("uint8_t", "header", "Header", "B"),
        ("uint8_t", "bFunc", "Func", "B"),
        ("uint16_t", "wArg", "Arg", "H"),
        ("uint32_t", "dwRot", "Rot", "I"),
        ("int32_t", "lX", "X", "i"),
        ("int32_t", "lY", "Y", "i"),
        ("uint32_t", "dwTime", "Time", "I"),
        ("uint32_t", "dwExtra", "Extra", "I"),
        ("uint8_t", "bFlag", "Flag", "B"),
        ("uint32_t", "dwReserved", "Reserved", "I"),
    ],
    "TPacketCGItemUseBeavium": [
        ("uint8_t", "header", "Header", "B"),
        ("uint16_t", "cell", "Cell", "H"),
        ("uint8_t", "window", "Window", "B"),
        ("uint32_t", "vnum", "Vnum", "I"),
        ("uint32_t", "targetVid", "TargetVid", "I"),
    ],
    "TPacketCGTargetBeavium": [
        ("uint8_t", "header", "Header", "B"),
        ("uint32_t", "targetVid", "TargetVid", "I"),
    ],
    "TPacketGCAttackBeavium": [
        ("uint8_t", "header", "Header", "B"),
        ("uint32_t", "dwVID", "Vid", "I"),
        ("uint32_t", "dwVictimVID", "VictimVid", "I"),
        ("uint8_t", "bType", "Type", "B"),
        ("uint16_t", "wSeq", "Seq", "H"),
    ],
    "TPacketGCDeadBeavium": [
        ("uint8_t", "header", "Header", "B"),
        ("uint32_t", "dwVID", "Vid", "I"),
        ("uint8_t", "bReason", "Reason", "B"),
        ("uint8_t", "dwReserved[20]", "Reserved", "20s"),
    ],

    # Standard X64 (4B uniform header)
    "TPacketCGAttack": [
        ("uint16_t", "header", "Header", "H"),
        ("uint16_t", "length", "Length", "H"),
        ("uint8_t", "bType", "Type", "B"),
        ("uint32_t", "dwVictimVID", "VictimVid", "I"),
        ("uint8_t", "bCRCMagicCubeProcPiece", "CrcProcPiece", "B"),
        ("uint8_t", "bCRCMagicCubeFilePiece", "CrcFilePiece", "B"),
    ],
    "TPacketCGMove": [
        ("uint16_t", "header", "Header", "H"),
        ("uint16_t", "length", "Length", "H"),
        ("uint8_t", "bFunc", "Func", "B"),
        ("uint8_t", "bArg", "Arg", "B"),
        ("uint8_t", "bRot", "Rot", "B"),
        ("int32_t", "lX", "X", "i"),
        ("int32_t", "lY", "Y", "i"),
        ("uint32_t", "dwTime", "Time", "I"),
    ],
    "TPacketCGItemUse": [
        ("uint16_t", "header", "Header", "H"),
        ("uint16_t", "length", "Length", "H"),
        ("uint8_t", "window", "Window", "B"),
        ("uint8_t", "cell", "Cell", "B"),
    ],
    "TPacketCGTarget": [
        ("uint16_t", "header", "Header", "H"),
        ("uint16_t", "length", "Length", "H"),
        ("uint32_t", "targetVid", "TargetVid", "I"),
    ],
}


def to_pascal_case(snake_str: str) -> str:
    """Konwertuje SNAKE_CASE na PascalCase."""
    parts = snake_str.lower().split('_')
    return ''.join(p.capitalize() for p in parts if p)


def get_opcode_int(op_info: Dict[str, Any]) -> int:
    """Zwraca wartosc liczbowa opkodu z pola dec lub hex."""
    if "dec" in op_info:
        return int(op_info["dec"])
    hex_val = op_info.get("hex", "0")
    if isinstance(hex_val, int):
        return hex_val
    hex_str = str(hex_val).strip()
    return int(hex_str, 16) if hex_str.lower().startswith("0x") else int(hex_str)


# ============================================================================
# GENERATOR C++23
# ============================================================================

def generate_cpp_profile(profile_key: str, profile_data: Dict[str, Any], output_dir: Path) -> List[Path]:
    """Generuje plik naglowkowy C++23 oraz tablice .inl dla danego profilu."""
    created_files: List[Path] = []
    output_dir.mkdir(parents=True, exist_ok=True)

    profile_names_map = {
        "luria": ("LuriaProtocol_gen.h", "luria_gc_table.inl", "Luria", "uint8_t"),
        "standard_x64": ("StandardX64Protocol_gen.h", "standard_x64_gc_table.inl", "StandardX64", "uint16_t"),
        "beavium": ("BeaviumProtocol_gen.h", "beavium_gc_table.inl", "Beavium", "uint8_t"),
        "martysama": ("MartySamaProtocol_gen.h", "martysama_gc_table.inl", "MartySama", "uint8_t"),
    }

    h_name, inl_name, ns_name, header_type = profile_names_map.get(
        profile_key,
        (f"{to_pascal_case(profile_key)}Protocol_gen.h", f"{profile_key}_gc_table.inl", to_pascal_case(profile_key), "uint8_t")
    )

    header_path = output_dir / h_name
    inl_path = output_dir / inl_name
    header_size = 2 if header_type == "uint16_t" else 1

    opcodes_cg = profile_data.get("opcodes_cg", {})
    opcodes_gc = profile_data.get("opcodes_gc", {})

    # 1. Generuj plik .inl dla GC
    inl_lines: List[str] = []
    # Sortuj wedlug wartosci numerycznej
    sorted_gc = sorted(opcodes_gc.items(), key=lambda item: get_opcode_int(item[1]))
    for op_name, op_info in sorted_gc:
        op_hex = op_info.get("hex", "0x00")
        is_dynamic = bool(op_info.get("is_dynamic", False))
        size = op_info.get("size", 0)
        inl_lines.append(f"case {op_hex}: info = {{ {size}, {'true' if is_dynamic else 'false'} }}; return true;")

    inl_content = "\n".join(inl_lines) + "\n"
    inl_path.write_text(inl_content, encoding="utf-8")
    created_files.append(inl_path)

    # 2. Generuj naglowek .h
    h_lines: List[str] = [
        "#pragma once",
        "",
        "// ============================================================================",
        f"// AUTOGEN PROTOCOL SPECIFICATION - {profile_data.get('display_name', profile_key)}",
        "// Single Source of Truth: docs/AI_PROTOCOL_MAP.json",
        "// Standard: C++23 | Zero Padding | static_assert verified",
        "// ============================================================================",
        "",
        "#include <cstdint>",
        "#include <cstddef>",
        "#include <cstring>",
        "",
        f"namespace Protocol::Generated::{ns_name}",
        "{",
        "    // ========================================================================",
        "    // Opcody CG (Client -> Game Server)",
        "    // ========================================================================",
        "    namespace CG",
        "    {",
    ]

    for op_name, op_info in opcodes_cg.items():
        op_hex = op_info.get("hex", "0x00")
        desc = op_info.get("description", "")
        comment = f" // {desc}" if desc else ""
        h_lines.append(f"        constexpr {header_type} {op_name:<24} = {op_hex};{comment}")

    h_lines.extend([
        "    }",
        "",
        "    // ========================================================================",
        "    // Opcody GC (Game Server -> Client)",
        "    // ========================================================================",
        "    namespace GC",
        "    {",
    ])

    for op_name, op_info in opcodes_gc.items():
        op_hex = op_info.get("hex", "0x00")
        desc = op_info.get("description", "")
        comment = f" // {desc}" if desc else ""
        h_lines.append(f"        constexpr {header_type} {op_name:<24} = {op_hex};{comment}")

    h_lines.extend([
        "    }",
        "",
        "    // ========================================================================",
        "    // Struktury Pakietow (#pragma pack 1)",
        "    // ========================================================================",
        "#pragma pack(push, 1)",
        "",
    ])

    # Generuj struktury CG
    h_lines.append("    // --- Pakiety CG (Client -> Game) ---")
    for op_name, op_info in opcodes_cg.items():
        struct_name = op_info.get("struct") or f"TPacketCG{to_pascal_case(op_name)}{ns_name}"
        size = op_info.get("size")
        is_dyn = bool(op_info.get("is_dynamic", False))

        # Sprawdz czy mamy dedykowane pola
        if struct_name in KNOWN_STRUCT_FIELDS:
            fields = KNOWN_STRUCT_FIELDS[struct_name]
            h_lines.append(f"    struct {struct_name}")
            h_lines.append("    {")
            for f_type, f_name, _, _ in fields:
                if f_name == "header":
                    h_lines.append(f"        {f_type} {f_name}{{ CG::{op_name} }};")
                elif f_name == "length" and profile_key == "standard_x64" and size:
                    h_lines.append(f"        {f_type} {f_name}{{ {size} }};")
                else:
                    h_lines.append(f"        {f_type} {f_name}{{}};")
            h_lines.append("    };")
            if size:
                h_lines.append(f"    static_assert(sizeof({struct_name}) == {size}, \"{struct_name} rozmiar musi wynosic {size} B\");")
            h_lines.append("")
        elif size and size >= header_size:
            h_lines.append(f"    struct {struct_name}")
            h_lines.append("    {")
            if profile_key == "standard_x64":
                h_lines.append(f"        {header_type} header{{ CG::{op_name} }};")
                h_lines.append(f"        uint16_t length{{ {size} }};")
                payload_len = size - 4
                if payload_len > 0:
                    h_lines.append(f"        uint8_t payload[{payload_len}]{{}};")
            else:
                h_lines.append(f"        {header_type} header{{ CG::{op_name} }};")
                payload_len = size - header_size
                if payload_len > 0:
                    h_lines.append(f"        uint8_t payload[{payload_len}]{{}};")
            h_lines.append("    };")
            h_lines.append(f"    static_assert(sizeof({struct_name}) == {size}, \"{struct_name} rozmiar musi wynosic {size} B\");")
            h_lines.append("")
        elif is_dyn:
            # Dynamiczny pakiet
            h_lines.append(f"    struct {struct_name}")
            h_lines.append("    {")
            if profile_key == "standard_x64":
                h_lines.append(f"        {header_type} header{{ CG::{op_name} }};")
                h_lines.append("        uint16_t length{0};")
                expected = 4
            else:
                h_lines.append(f"        {header_type} header{{ CG::{op_name} }};")
                h_lines.append("        uint16_t size{0};")
                expected = 3
            h_lines.append("    };")
            h_lines.append(f"    static_assert(sizeof({struct_name}) == {expected}, \"{struct_name} bazowy rozmiar dynamiczny musi wynosic {expected} B\");")
            h_lines.append("")

    # Generuj struktury GC
    h_lines.append("    // --- Pakiety GC (Game -> Client) ---")
    for op_name, op_info in opcodes_gc.items():
        struct_name = op_info.get("struct") or f"TPacketGC{to_pascal_case(op_name)}{ns_name}"
        size = op_info.get("size")
        is_dyn = bool(op_info.get("is_dynamic", False))

        if struct_name in KNOWN_STRUCT_FIELDS:
            fields = KNOWN_STRUCT_FIELDS[struct_name]
            h_lines.append(f"    struct {struct_name}")
            h_lines.append("    {")
            for f_type, f_name, _, _ in fields:
                if f_name == "header":
                    h_lines.append(f"        {f_type} {f_name}{{ GC::{op_name} }};")
                elif f_name == "length" and profile_key == "standard_x64" and size:
                    h_lines.append(f"        {f_type} {f_name}{{ {size} }};")
                else:
                    h_lines.append(f"        {f_type} {f_name}{{}};")
            h_lines.append("    };")
            if size:
                h_lines.append(f"    static_assert(sizeof({struct_name}) == {size}, \"{struct_name} rozmiar musi wynosic {size} B\");")
            h_lines.append("")
        elif size and size >= header_size:
            h_lines.append(f"    struct {struct_name}")
            h_lines.append("    {")
            if profile_key == "standard_x64":
                h_lines.append(f"        {header_type} header{{ GC::{op_name} }};")
                h_lines.append(f"        uint16_t length{{ {size} }};")
                payload_len = size - 4
                if payload_len > 0:
                    h_lines.append(f"        uint8_t payload[{payload_len}]{{}};")
            else:
                h_lines.append(f"        {header_type} header{{ GC::{op_name} }};")
                payload_len = size - header_size
                if payload_len > 0:
                    h_lines.append(f"        uint8_t payload[{payload_len}]{{}};")
            h_lines.append("    };")
            h_lines.append(f"    static_assert(sizeof({struct_name}) == {size}, \"{struct_name} rozmiar musi wynosic {size} B\");")
            h_lines.append("")
        elif is_dyn:
            # Dynamiczny pakiet
            h_lines.append(f"    struct {struct_name}")
            h_lines.append("    {")
            if profile_key == "standard_x64":
                h_lines.append(f"        {header_type} header{{ GC::{op_name} }};")
                h_lines.append("        uint16_t length{0};")
                expected = 4
            elif "32" in op_name or op_name in ("DYNAMIC_32", "VOICE_CHAT_DATA", "VOICE_CHAT_DATA2"):
                h_lines.append(f"        {header_type} header{{ GC::{op_name} }};")
                h_lines.append("        uint32_t size{0};")
                expected = 5
            else:
                h_lines.append(f"        {header_type} header{{ GC::{op_name} }};")
                h_lines.append("        uint16_t size{0};")
                expected = 3
            h_lines.append("    };")
            h_lines.append(f"    static_assert(sizeof({struct_name}) == {expected}, \"{struct_name} bazowy rozmiar dynamiczny musi wynosic {expected} B\");")
            h_lines.append("")

    h_lines.extend([
        "#pragma pack(pop)",
        "",
        "    // ========================================================================",
        "    // Tabela Informacji o Pakietach GC",
        "    // ========================================================================",
        "    struct GCInfo",
        "    {",
        "        uint32_t size{0};",
        "        bool isDynamic{false};",
        "    };",
        "",
        f"    inline bool GetGCInfo({header_type} header, GCInfo& info)",
        "    {",
        "        switch (header)",
        "        {",
        f"#include \"{inl_name}\"",
        "        default:",
        "            return false;",
        "        }",
        "    }",
        "",
        f"}} // namespace Protocol::Generated::{ns_name}",
        ""
    ])

    header_path.write_text("\n".join(h_lines), encoding="utf-8")
    created_files.append(header_path)

    return created_files


# ============================================================================
# GENERATOR C# (E:\Metin2Clientless\Packets\Generated\LuriaPackets.cs)
# ============================================================================

def generate_csharp_packets(schema_data: Dict[str, Any], output_dir: Path) -> List[Path]:
    """Generuje pliki C# z silnie typowanymi structami [StructLayout(LayoutKind.Sequential, Pack = 1)]."""
    created_files: List[Path] = []
    output_dir.mkdir(parents=True, exist_ok=True)

    # Generujemy LuriaPackets.cs
    profile_luria = schema_data.get("profiles", {}).get("luria", {})
    opcodes_cg = profile_luria.get("opcodes_cg", {})
    opcodes_gc = profile_luria.get("opcodes_gc", {})

    cs_lines: List[str] = [
        "// <auto-generated>",
        "// Generated by tools/generate_protocol_code.py",
        "// Single Source of Truth: docs/AI_PROTOCOL_MAP.json",
        "// </auto-generated>",
        "",
        "using System;",
        "using System.Collections.Generic;",
        "using System.Runtime.InteropServices;",
        "",
        "namespace Metin2Clientless.Packets.Generated;",
        "",
        "// ============================================================================",
        "// Stale Opcodow CG dla Serwera Luria",
        "// ============================================================================",
        "public static class LuriaCG",
        "{",
    ]

    for op_name, op_info in opcodes_cg.items():
        op_hex = op_info.get("hex", "0x00")
        desc = op_info.get("description", "")
        cs_lines.append(f"    /// <summary>{desc}</summary>")
        cs_lines.append(f"    public const byte {op_name} = {op_hex};")

    cs_lines.extend([
        "}",
        "",
        "// ============================================================================",
        "// Stale Opcodow GC dla Serwera Luria",
        "// ============================================================================",
        "public static class LuriaGC",
        "{",
    ])

    for op_name, op_info in opcodes_gc.items():
        op_hex = op_info.get("hex", "0x00")
        desc = op_info.get("description", "")
        cs_lines.append(f"    /// <summary>{desc}</summary>")
        cs_lines.append(f"    public const byte {op_name} = {op_hex};")

    cs_lines.extend([
        "}",
        "",
        "// ============================================================================",
        "// Struktury Pakietow CG i GC Lurii ([StructLayout Pack = 1])",
        "// ============================================================================",
        "",
    ])

    # Generuj struktury CG
    for op_name, op_info in opcodes_cg.items():
        struct_name = op_info.get("struct") or f"TPacketCG{to_pascal_case(op_name)}Luria"
        size = op_info.get("size")
        desc = op_info.get("description", "")

        cs_lines.append(f"/// <summary>{desc} (Rozmiar: {size if size else 'dyn'} B)</summary>")
        cs_lines.append("[StructLayout(LayoutKind.Sequential, Pack = 1)]")
        cs_lines.append(f"public struct {struct_name}")
        cs_lines.append("{")
        if size:
            cs_lines.append(f"    public const int PacketSize = {size};")

        if struct_name in KNOWN_STRUCT_FIELDS:
            for f_type, f_cpp, f_cs, _ in KNOWN_STRUCT_FIELDS[struct_name]:
                if "[" in f_cpp:
                    arr_len = int(f_cpp.split("[")[1].split("]")[0])
                    cs_lines.append(f"    public unsafe fixed byte {f_cs}[{arr_len}];")
                elif f_type in ("uint8_t", "BYTE"):
                    cs_lines.append(f"    public byte {f_cs};")
                elif f_type in ("uint16_t", "WORD"):
                    cs_lines.append(f"    public ushort {f_cs};")
                elif f_type in ("uint32_t", "DWORD"):
                    cs_lines.append(f"    public uint {f_cs};")
                elif f_type in ("int32_t", "int"):
                    cs_lines.append(f"    public int {f_cs};")
                elif f_type == "float":
                    cs_lines.append(f"    public float {f_cs};")
                elif f_type in ("int64_t", "long long"):
                    cs_lines.append(f"    public long {f_cs};")
                elif f_type in ("uint64_t", "unsigned long long"):
                    cs_lines.append(f"    public ulong {f_cs};")
                else:
                    cs_lines.append(f"    public byte {f_cs};")
        elif size and size > 1:
            cs_lines.append("    public byte Header;")
            cs_lines.append(f"    public unsafe fixed byte Payload[{size - 1}];")
        elif size == 1:
            cs_lines.append("    public byte Header;")
        else:
            cs_lines.append("    public byte Header;")
            cs_lines.append("    public ushort Size;")

        cs_lines.append("}")
        cs_lines.append("")

    # Generuj struktury GC
    for op_name, op_info in opcodes_gc.items():
        struct_name = op_info.get("struct") or f"TPacketGC{to_pascal_case(op_name)}Luria"
        size = op_info.get("size")
        desc = op_info.get("description", "")

        cs_lines.append(f"/// <summary>{desc} (Rozmiar: {size if size else 'dyn'} B)</summary>")
        cs_lines.append("[StructLayout(LayoutKind.Sequential, Pack = 1)]")
        cs_lines.append(f"public struct {struct_name}")
        cs_lines.append("{")
        if size:
            cs_lines.append(f"    public const int PacketSize = {size};")

        if struct_name in KNOWN_STRUCT_FIELDS:
            for f_type, f_cpp, f_cs, _ in KNOWN_STRUCT_FIELDS[struct_name]:
                if "[" in f_cpp:
                    arr_len = int(f_cpp.split("[")[1].split("]")[0])
                    cs_lines.append(f"    public unsafe fixed byte {f_cs}[{arr_len}];")
                elif f_type in ("uint8_t", "BYTE"):
                    cs_lines.append(f"    public byte {f_cs};")
                elif f_type in ("uint16_t", "WORD"):
                    cs_lines.append(f"    public ushort {f_cs};")
                elif f_type in ("uint32_t", "DWORD"):
                    cs_lines.append(f"    public uint {f_cs};")
                elif f_type in ("int32_t", "int"):
                    cs_lines.append(f"    public int {f_cs};")
                elif f_type == "float":
                    cs_lines.append(f"    public float {f_cs};")
                elif f_type in ("int64_t", "long long"):
                    cs_lines.append(f"    public long {f_cs};")
                elif f_type in ("uint64_t", "unsigned long long"):
                    cs_lines.append(f"    public ulong {f_cs};")
                else:
                    cs_lines.append(f"    public byte {f_cs};")
        elif size and size > 1:
            cs_lines.append("    public byte Header;")
            cs_lines.append(f"    public unsafe fixed byte Payload[{size - 1}];")
        elif size == 1:
            cs_lines.append("    public byte Header;")
        else:
            cs_lines.append("    public byte Header;")
            cs_lines.append("    public ushort Size;")

        cs_lines.append("}")
        cs_lines.append("")

    # Helper deskryptorow GC
    cs_lines.extend([
        "public static class LuriaGCRegistry",
        "{",
        "    public static readonly Dictionary<byte, (uint Size, bool IsDynamic)> Descriptors = new()",
        "    {",
    ])

    for op_name, op_info in sorted(opcodes_gc.items(), key=lambda x: get_opcode_int(x[1])):
        op_hex = op_info.get("hex", "0x00")
        size = op_info.get("size", 0)
        is_dyn = "true" if op_info.get("is_dynamic") else "false"
        cs_lines.append(f"        {{ {op_hex}, ({size}, {is_dyn}) }},")

    cs_lines.extend([
        "    };",
        "",
        "    public static bool TryGetInfo(byte header, out uint size, out bool isDynamic)",
        "    {",
        "        if (Descriptors.TryGetValue(header, out var val))",
        "        {",
        "            size = val.Size;",
        "            isDynamic = val.IsDynamic;",
        "            return true;",
        "        }",
        "        size = 0;",
        "        isDynamic = false;",
        "        return false;",
        "    }",
        "}",
        ""
    ])

    luria_cs_path = output_dir / "LuriaPackets.cs"
    luria_cs_path.write_text("\n".join(cs_lines), encoding="utf-8")
    created_files.append(luria_cs_path)

    return created_files


# ============================================================================
# GENERATOR PYTHON (m2dev-client-main/assets/root/stubs/protocol_packets.py)
# ============================================================================

def generate_python_stubs(schema_data: Dict[str, Any], output_dir: Path) -> List[Path]:
    """Generuje plik Pythona z klasami i formatami struct.Struct dla pakietow."""
    created_files: List[Path] = []
    output_dir.mkdir(parents=True, exist_ok=True)

    py_lines: List[str] = [
        "#!/usr/bin/env python3",
        "# -*- coding: utf-8 -*-",
        '"""',
        "Stubs struktur protokolow sieciowych Metin2.",
        "Wygenerowano automatycznie z docs/AI_PROTOCOL_MAP.json przez tools/generate_protocol_code.py.",
        '"""',
        "",
        "import struct",
        "from typing import Dict, Tuple, Optional, Any",
        "",
    ]

    for profile_key, profile_data in schema_data.get("profiles", {}).items():
        pascal_prof = to_pascal_case(profile_key)
        opcodes_cg = profile_data.get("opcodes_cg", {})
        opcodes_gc = profile_data.get("opcodes_gc", {})

        py_lines.extend([
            f"# ============================================================================",
            f"# PROFIL: {profile_data.get('display_name', profile_key)}",
            f"# ============================================================================",
            f"class {pascal_prof}CG:",
        ])
        for op_name, op_info in opcodes_cg.items():
            op_hex = op_info.get("hex", "0x00")
            py_lines.append(f"    {op_name} = {op_hex}")

        py_lines.extend([
            "",
            f"class {pascal_prof}GC:",
        ])
        for op_name, op_info in opcodes_gc.items():
            op_hex = op_info.get("hex", "0x00")
            py_lines.append(f"    {op_name} = {op_hex}")

        py_lines.extend([
            "",
            f"{profile_key.upper()}_GC_TABLE: Dict[int, Tuple[int, bool]] = {{",
        ])
        for op_name, op_info in opcodes_gc.items():
            op_hex = op_info.get("hex", "0x00")
            size = op_info.get("size", 0)
            is_dyn = bool(op_info.get("is_dynamic", False))
            py_lines.append(f"    {op_hex}: ({size}, {is_dyn}),")
        py_lines.append("}")
        py_lines.append("")

    # Dodaj klasy pakietow dla Lurii i StandardX64 ze struct.Struct
    py_lines.extend([
        "# ============================================================================",
        "# KLASY PAKIETOW ZE struct.Struct",
        "# ============================================================================",
    ])

    for struct_name, fields in KNOWN_STRUCT_FIELDS.items():
        fmt_parts = ["<"]
        field_names = []
        for _, _, f_cs, f_fmt in fields:
            fmt_parts.append(f_fmt)
            field_names.append(f_cs.lower())

        fmt_str = "".join(fmt_parts)
        calc_size = struct.calcsize(fmt_str)

        py_lines.extend([
            f"class {struct_name}:",
            f'    """Klasa pakietu {struct_name} (rozmiar {calc_size}B)."""',
            f'    FORMAT = "{fmt_str}"',
            f"    STRUCT = struct.Struct(FORMAT)",
            f"    SIZE = {calc_size}",
            "",
            f"    def __init__(self, {', '.join(field_names)}):",
        ])
        for fn in field_names:
            py_lines.append(f"        self.{fn} = {fn}")

        py_lines.extend([
            "",
            "    def pack(self) -> bytes:",
            f"        return self.STRUCT.pack({', '.join(f'self.{fn}' for fn in field_names)})",
            "",
            "    @classmethod",
            "    def unpack(cls, buffer: bytes):",
            f"        vals = cls.STRUCT.unpack(buffer[:cls.SIZE])",
            f"        return cls(*vals)",
            "",
        ])

    # Dodaj uniwersalna funkcje deskryptora
    py_lines.extend([
        "def get_gc_packet_info(profile: str, opcode: int) -> Optional[Tuple[int, bool]]:",
        '    """Zwraca krotke (rozmiar, czy_dynamiczny) dla danego profilu i opkodu GC."""',
        "    table_map = {",
        '        "luria": LURIA_GC_TABLE,',
        '        "standard_x64": STANDARD_X64_GC_TABLE,',
        '        "beavium": BEAVIUM_GC_TABLE,',
        '        "martysama": MARTYSAMA_GC_TABLE,',
        "    }",
        "    tbl = table_map.get(profile.lower())",
        "    if tbl and opcode in tbl:",
        "        return tbl[opcode]",
        "    return None",
        "",
    ])

    py_out_path = output_dir / "protocol_packets.py"
    py_out_path.write_text("\n".join(py_lines), encoding="utf-8")
    created_files.append(py_out_path)

    return created_files


# ============================================================================
# GLOWNY PUNKT WEJSCIA I PARSER CLI
# ============================================================================

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generator kodu struktur protokolow sieciowych na podstawie docs/AI_PROTOCOL_MAP.json."
    )
    parser.add_argument(
        "--schema",
        default="docs/AI_PROTOCOL_MAP.json",
        help="Sciezka do bazy AI_PROTOCOL_MAP.json (domyslnie: docs/AI_PROTOCOL_MAP.json)"
    )
    parser.add_argument(
        "--target",
        choices=["cpp", "csharp", "python", "all"],
        default="all",
        help="Docelowy jezyk generowania (cpp, csharp, python, all; domyslnie: all)"
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="Flaga generowania dla wszystkich celow (alias na --target all)"
    )
    parser.add_argument(
        "--output",
        default=None,
        help="Katalog wyjsciowy (opcjonalny, nadpisuje domyslne sciezki)"
    )

    args = parser.parse_args()

    # Wykryj repo root wzgledem lokalizacji skryptu
    script_dir = Path(__file__).resolve().parent
    repo_root = script_dir.parent

    # Sciezka do schemy
    schema_path = Path(args.schema)
    if not schema_path.is_absolute():
        schema_path = repo_root / schema_path

    if not schema_path.exists():
        print(f"[-] Blad: Plik schemy {schema_path} nie istnieje!")
        return 1

    try:
        schema_data = json.loads(schema_path.read_text(encoding="utf-8"))
    except Exception as exc:
        print(f"[-] Blad parsowania JSON {schema_path}: {exc}")
        return 1

    target = "all" if args.all else args.target

    print("=" * 78)
    print("METIN2 PROTOCOL CODE GENERATOR")
    print(f"Schema (Single Source of Truth): {schema_path}")
    print(f"Target: {target}")
    print("=" * 78)

    generated_files: List[Path] = []

    # 1. Cel C++23
    if target in ("cpp", "all"):
        if args.output and target == "cpp":
            cpp_output_dir = Path(args.output)
        else:
            cpp_output_dir = repo_root / "src" / "Client" / "Network" / "Protocol" / "Generated"

        print(f"[*] Generowanie C++23 w: {cpp_output_dir}")
        for prof_key, prof_data in schema_data.get("profiles", {}).items():
            files = generate_cpp_profile(prof_key, prof_data, cpp_output_dir)
            generated_files.extend(files)
            print(f"    + Profil {prof_key}: {[f.name for f in files]}")

    # 2. Cel C#
    if target in ("csharp", "all"):
        if args.output and target == "csharp":
            csharp_output_dir = Path(args.output)
        else:
            clientless_base = Path(r"E:\Metin2Clientless")
            if clientless_base.exists():
                csharp_output_dir = clientless_base / "Packets" / "Generated"
            else:
                csharp_output_dir = repo_root / "Packets" / "Generated"

        print(f"[*] Generowanie C# w: {csharp_output_dir}")
        files = generate_csharp_packets(schema_data, csharp_output_dir)
        generated_files.extend(files)
        print(f"    + Wygenerowano: {[f.name for f in files]}")

    # 3. Cel Python
    if target in ("python", "all"):
        if args.output and target == "python":
            python_output_dir = Path(args.output)
        else:
            python_output_dir = repo_root / "m2dev-client-main" / "assets" / "root" / "stubs"

        print(f"[*] Generowanie Python w: {python_output_dir}")
        files = generate_python_stubs(schema_data, python_output_dir)
        generated_files.extend(files)
        print(f"    + Wygenerowano: {[f.name for f in files]}")

    print("-" * 78)
    print(f"[SUKCES] Pomyslnie wygenerowano lacznie {len(generated_files)} plikow.")
    for gf in generated_files:
        print(f"  * {gf}")
    print("=" * 78)
    return 0


if __name__ == "__main__":
    sys.exit(main())
