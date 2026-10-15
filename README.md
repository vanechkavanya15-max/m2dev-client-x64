# Metin2 x64 Modern Client Engine (C++23 AI-Native)

[![Standard](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Architektura](https://img.shields.io/badge/Architektura-x64%20Native-brightgreen.svg)]()
[![System Budowania](https://img.shields.io/badge/Build-CMake%203.26%2B-orange.svg)]()
[![Zestaw Testow](https://img.shields.io/badge/CTest-51%2F51%20PASS%20(100%25)-success.svg)]()
[![MCP Harness](https://img.shields.io/badge/MCP%20Tools-42%20Natywne%20Narzedzia-blueviolet.svg)]()
[![Grafika RHI](https://img.shields.io/badge/Render-Direct3D%209%20%2F%20glTF%202.0-yellow.svg)]()

Zmodernizowany silnik klienta gry Metin2 x64 stworzony w oparciu o standard **C++23**, paradygmat **Data-Oriented Design (DOD)** oraz wzorzec **AI-First Architecture**. Repozytorium stanowi kompletna ewolucje kodu zrodlowego gry, eliminujaca historyczne monolity z lat 2004–2014 na rzecz modulow domenowych o scislym SRP, transparentnego ladowania zasobow z dysku, uniwersalnej matrycy protokolow sieciowych oraz pelnej integracji z autonomicznymi agentami AI za posrednictwem protokolu MCP i szyny Named Pipe IPC.

---

## Spis Tresci

1. [Glowne Filary Architektoniczne](#1-glowne-filary-architektoniczne)
2. [Universal Protocol Matrix (UPM)](#2-universal-protocol-matrix-upm)
3. [Virtual File System (VFS) i Potok glTF](#3-virtual-file-system-vfs-i-potok-gltf)
4. [Silnik Headless i Mostek IPC / MCP (42 Narzedzia)](#4-silnik-headless-i-mostek-ipc--mcp-42-narzedzia)
5. [Renderer Diagnostyczny (DebugRenderer ESP)](#5-renderer-diagnostyczny-debugrenderer-esp)
6. [Struktura Repozytorium](#6-struktura-repozytorium)
7. [Wymagania i Kompilacja](#7-wymagania-i-kompilacja)
8. [Flagi Uruchomieniowe CLI](#8-flagi-uruchomieniowe-cli)
9. [Zestaw Testow i Weryfikacja Jakosci](#9-zestaw-testow-i-weryfikacja-jakosci)

---

## 1. Glowne Filary Architektoniczne

Projekt odrzuca przestarzaly model hybrydowy i wielotysieczne pliki kodu na rzecz czystej, modulowej architektury:

- **Standard C++23 (MSVC v143 / v145)**: Pelne wykorzystanie nowoczesnych cech jezyka: `std::expected` / `PacketResult<T>` do obslugi bledow bez wyjatkow, `std::span<const uint8_t>` dla operacji zero-copy na buforach sieciowych, silne typowanie domenowe z naglowka [`StrongTypes.h`](file:///E:/m2dev-client-src-mainOryginalx64/src/EterBase/StrongTypes.h) (`EntityVid`, `ItemVnum`, `SlotIndex`, `Money64`).
- **Dekonstrukcja Monolitow Kodowych**:
  - Historyczny monolit sieciowy `PythonNetworkStreamPhaseGame.cpp` (10 713 linii) zostal zastapiony przez tablicowy router $O(1)$ [`ModernPacketDispatcher`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/ModernPacketDispatcher.h) oraz odizolowane handlery domenowe (Party, Guild, Quest, Refine, Exchange).
  - Monolit gracza `PythonPlayer.cpp` (7 523 linie) zostal podzielony na zbalansowane domeny DDD: [`WorldContext`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Core/WorldContext.h), [`InventoryDomain`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Gameplay/InventoryDomain.h), [`SkillDomain`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Gameplay/SkillDomain.h) oraz [`PlayerStatsDomain`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Gameplay/PlayerStatsDomain.h).
  - Monolit instancji aktora `InstanceBase.cpp` zostal podzielony na komponenty logiczne: montowanie, walka, wyglad, tytuly i rangi.
- **Bezpieczenstwo Pamieci i Zapobieganie Problemom ABA**: Zastosowanie generacyjnego rejestru bytow [`GenerationalRegistry`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Actor/EntityHandle.h) oraz funkcyjnego rejestru [`ActorRegistry`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/World/ActorRegistry.h) chroniacego pamiec przed Use-After-Free i wiszacymi wskaznikami przy dynamicznym respawnie potworow.
- **Odpornosc Maszyny Stanow Direct3D 9**: Wszystkie operacje modyfikujace deklaracje wierzcholkow, shadery i potok stalofunkcyjny (FFP) sa zabezpieczone straznikami RAII z [`StateManagerGuards.h`](file:///E:/m2dev-client-src-mainOryginalx64/src/EterLib/StateManagerGuards.h).

---

## 2. Universal Protocol Matrix (UPM)

Klient eliminuje sztywne powiazanie ze specyficznym serwerem gry dzieki heksagonalnej architekturze sterownikow protokolu implementujacych interfejs `IServerProtocolDriver`:

| Profil Sterownika | Identyfikator | Format Ramkowania | Obsluga Kryptograficzna | Zastosowanie |
|---|---|---|---|---|
| **Standard x64** | `standard_x64` | 4-bajtowe `[Header:2][Length:2]` | Libsodium / SecureCipher | Nowoczesne serwery x64 (40k+), natywne struktury zunifikowane |
| **Luria** | `luria` | 1-bajtowy opkod + lookup GC (137 pakietow) | Diffie-Hellman DH2 MODP + Blowfish CTR | Serwery oparte na silniku Luria z dynamiczna autoryzacja |
| **Beavium** | `beavium` | 1-bajtowy opkod + lookup w tabeli GC | AES-256-CTR | Serwery z kompresja katow do mikrostopni |
| **MartySama** | `martysama` | 1-bajtowy opkod + dynamiczne dlugosci 2B | TEA / Security / MagicCube CRC | Klasyczne galezie mainline |

Architektura **Pluggable Crypto Strategy** ([`INetCryptoProvider`](file:///E:/m2dev-client-src-mainOryginalx64/src/EterLib/NetCryptoProvider.h)) calkowicie odcina implementacje gniazda sieciowego od konkretnych algorytmow szyfrowania, pozwalajac na plynne przelaczanie profilu w czasie rzeczywistym.

---

## 3. Virtual File System (VFS) i Potok glTF

### Dynamiczny i Transparentny VFS
Tradycyjna, sztywna lista ponad 90 zahardcodowanych archiwow `.epk`/`.eix` w C++ zostala zastapiona transparentnym podsystemem VFS:
- Dynamiczne skanowanie i montowanie paczek `.pck` oraz otwartych katalogow zasobow `assets/` za posrednictwem `std::filesystem::directory_iterator`.
- Uniwersalny mechanizm wyszukiwania `CPackManager::GetFileWithPool` oraz `IsExist` obsluguje wszystkie formaty plikow z dysku:
  - Definicje map i srodowiska: `AtlasInfo.txt`, `Setting.txt`, `Town.txt`, `.msenv`.
  - Dane wysokosci terenu, wody i kolizji: `height.raw`, `tile.raw`, `attr.atr`, `water.wtr`.
  - Tekstury i grafiki: `.dds`, `.tga`, `.sub`, `.png`, `.jpg`.
  - Modele i animacje: `.glb`, `.gltf`, `.gr2`, `.msa`, `.msm`, `.mse`.
- Wielosciezkowy fallback ladowania `AtlasInfo.txt` w `CMapManager` gwarantuje poprawne wczytanie wszystkich 110 stref gry niezaleznie od struktury katalogow.

### Hybrydowy Potok Modeli 3D (glTF 2.0 / PBR)
Klient wspiera nowoczesny otwarty standard **glTF 2.0** (`.glb` / `.gltf`) bezposrednio w silniku renderujacym:
- Pelne parsowanie hierarchii kosci, wag wierzcholkow (`JOINTS_0`, `WEIGHTS_0`) oraz wierzcholkow pozycji.
- Obsluga zaczepow kosci wyposazenia (np. `Bip001 R Hand` dla broni) oraz linkowania kosci fryzur do szkieletu postaci.
- Wspolistnienie natywnego potoku glTF z historycznym rendererem Granny 3D (`.gr2`).

---

## 4. Silnik Headless i Mostek IPC / MCP (42 Narzedzia)

Klient zostal przystosowany do dzialania jako w pelni autonomiczny agent gry w chmurze lub srodowisku CI/CD:

### Potok IPC Named Pipe i Optymalizacje Headless
- **Potok Komunikacyjny**: `\\.\pipe\M2ClientEngineHarness` (protokol JSON-RPC 2.0 z 4-bajtowym ramkowaniem little-endian).
- **Tryb Headless (`--headless`)**:
  - Dlawik petli glownej z czasem ramki 15 ms (~60 Hz), redukujacy bezuzyteczne zuzycie procesora z 1000 Hz / 15% rdzenia do **<0.5% rdzenia**.
  - Zastepcza atrapa podsystemu audio (`SoundEngine`), eliminujaca crash przy braku fizycznej karty dzwiekowej.
  - Calkowite zablokowanie modalnych okien dialogowych Windows (`MessageBoxW`).
- **Kolejkowanie Zadan Watku Glownego**: Wszystkie komendy IPC modyfikujace stan gry sa kolejkowane i przetwarzane synchronicznie w petli glownej (`TestHarnessServer::ProcessMainThreadQueue()`), co wyklucza wyscigi pamieci z renderowaniem Direct3D 9.

### Zestaw 42 Narzedzi MCP (`m2-client-test` / `m2_agent_harness_mcp.py`)
Model Context Protocol umozliwia agentom AI (takim jak Antigravity czy Jules Swarm) precyzyjne sterowanie gra bez modyfikacji bajtow procesu:

1. **Zarzadzanie Procesem i Zegarem (5 narzedzi)**:
   - `m2_harness_launch`, `m2_harness_stop`, `m2_tick_frame`, `m2_get_engine_state`, `m2_audit_dual_state`.
2. **Diagnostyka i Odpornosc na Awaryjnosc (3 narzedzia)**:
   - `m2_inject_packet`, `m2_fuzz_router`, `m2_get_crash_report` (raport CrashSentinel ze stosem PDB).
3. **Faza Sieciowa, Kreator Postaci i Auto-Reconnect (9 narzedzi)**:
   - `m2_game_ping`, `m2_game_login`, `m2_game_get_character_slots`, `m2_game_select_character`, `m2_game_create_character`, `m2_game_enter_game`, `m2_game_set_auto_reconnect`, `m2_game_set_window_mode`, `m2_game_set_protocol_driver`.
4. **Odczyt Domenowy i Megasnapshot (4 narzedzia)**:
   - `m2_game_get_state`, `m2_game_get_inventory`, `m2_game_get_nearby_entities`, `m2_get_world_megasnapshot` (kompletny zrzut stanu gracza, aktorow i otoczenia w 1 zapytaniu RTT).
5. **Natywne Sterowanie Postacia i Walka (12 narzedzi)**:
   - `m2_game_set_attack_key_state`, `m2_game_set_dir_key_state`, `m2_game_set_multi_dir_key_state` (WASD), `m2_game_move_to_position` (nawigacja pikselowa), `m2_game_stop`, `m2_game_set_target`, `m2_game_clear_target`, `m2_game_pick_close_item`, `m2_game_use_skill`, `m2_game_send_chat`, `m2_game_attack_target`, `m2_game_use_item`.
6. **Automatyzacja Okna i Systemu Operacyjnego (9 narzedzi)**:
   - `m2_client_launch`, `m2_client_status`, `m2_client_screenshot`, `m2_client_mouse_click`, `m2_client_mouse_move`, `m2_client_send_text`, `m2_client_send_key`, `m2_client_get_logs`, `m2_client_close`.

---

## 5. Renderer Diagnostyczny (DebugRenderer ESP)

Dla potrzeb bota autonomicznego, wizualizacji diagnostycznej oraz testow automatycznych wprowadzono modul `DebugRenderer`:
- **Architektura RHI i Straznik RAII**: [`ScopedDebugRenderGuard`](file:///E:/m2dev-client-src-mainOryginalx64/src/EterLib/StateManagerGuards.h) izoluje macierz swiata (`D3DTS_WORLD`), odcina etapy tekstur i blokuje zapis Z-Buffera (`D3DRS_ZWRITEENABLE = FALSE`), eliminujac z-fighting ze splattingiem terenu.
- **Obslugiwane Prymitywy 3D**:
  - Rysowanie linii swiatowych (`DrawLine`), tracerow do celow (`DrawTracer`), ramek AABB wokol aktorow (`DrawBox`, `DrawAABB`), okregow zasiegu na podlozu (`DrawCircle`, `DrawCircleActor`) oraz krzyzykow pozycji (`DrawCross`).
  - Przeliczanie wspolrzednych 3D na plaszczyzne ekranu 2D (`WorldToScreen`, `GetActorScreenPos`).
- **Mostek Python C-API**: Dostepny w srodowisku skryptowym przez modul `debugRender`.

---

## 6. Struktura Repozytorium

```
E:\m2dev-client-src-mainOryginalx64\
├── CMakeLists.txt                 # Glowna konfiguracja budowania CMake (C++23)
├── AGENTS.md                      # Instrukcje operacyjne dla roju agentow AI
├── README.md                      # Dokumentacja glowna projektu
├── src/
│   ├── Client/                    # Nowoczesne warstwy domenowe silnika
│   │   ├── Core/                  # WorldContext, DomainEvents, silne typy
│   │   ├── Gameplay/              # Domeny: Inventory, Skills, PlayerStats
│   │   ├── Network/               # Dispatchery O(1), sterowniki UPM, protokoly
│   │   ├── Graphics/              # DebugRenderer, RHI, pipeline glTF
│   │   └── IPC/                   # TestHarnessServer, potok Named Pipe
│   ├── UserInterface/             # Punkt wejscia WinMain, mostki Pythona, FastBoot
│   ├── GameLib/                   # Aktorzy, RaceManager, zarzadzanie mapa Outdoor
│   ├── EterLib/                   # StateManager D3D9, kryptografia, gniazda NetStream
│   ├── EterBase/                  # LogModern, StrongTypes, Result, Memory
│   └── PackLib/                   # CPackManager, dynamiczny VFS
├── docs/                          # Kompletna specyfikacja architektury i sieci
│   ├── CLIENT_SYSTEM_MAP.md       # Centralna mapa podsystemow klienta 2026
│   ├── AI_INSTANT_WORLD_ARCHITECTURE_2026.md # Architektura Instant-World
│   └── 03_network_protocol/       # Rejestry pakietow CG i GC oraz sterowniki
├── tools/
│   └── mcp/                       # Serwer m2_agent_harness_mcp.py i testy bota
└── tests/                         # Zestaw 51 testow jednostkowych i integracyjnych
```

---

## 7. Wymagania i Kompilacja

### Wymagania Srodowiskowe
- **System operacyjny**: Microsoft Windows 10 lub Windows 11 (64-bit).
- **Kompilator**: Visual Studio 2022 (MSVC v143) lub nowszy z zestawem narzedzi C++23.
- **Windows SDK**: Windows SDK 10 (10.0.19041+) lub Windows SDK 11.
- **System budowania**: CMake 3.26 lub nowszy.
- **Srodowisko uruchomieniowe testow**: Python 3.10+ (dla testow MCP i bota).

### Budowanie ze Wiersza Polecen (PowerShell)

```powershell
# 1. Konfiguracja projektu CMake dla architektury x64
cmake -S . -B build -A x64

# 2. Kompilacja w konfiguracji Release (zoptymalizowana)
cmake --build build --config Release -j8

# 3. Uruchomienie zestawu testow CTest
ctest --test-dir build -C Release --output-on-failure
```

Pliki binarne generowane sa w katalogu `build/bin/Release/`:
- `Metin2_Release.exe` — glowny plik wykonywalny klienta gry.
- `DumpProto.exe` — narzedzie serializacji struktur Item/Mob Proto.
- `PackMaker.exe` — narzedzie pakowania archiwow VFS.

---

## 8. Flagi Uruchomieniowe CLI

Klient wspiera zestaw zaawansowanych argumentow wiersza polecen ulatwiajacych automatyzacje:

```powershell
# Uruchomienie w trybie bezglowym (Headless) dla bota lub testow
.\build\bin\Release\Metin2_Release.exe --headless

# Byskawiczne wejscie do gry z pominieciem ekranow logowania (Direct-to-World)
.\build\bin\Release\Metin2_Release.exe --direct 0

# Aktywacja automatycznego wznawiania polaczenia po zerwaniu sesji TCP
.\build\bin\Release\Metin2_Release.exe --auto-reconnect

# Wymuszenie profilu sterownika sieciowego (np. luria, standard_x64, beavium)
.\build\bin\Release\Metin2_Release.exe --server standard_x64
```

---

## 9. Zestaw Testow i Weryfikacja Jakosci

Stan techniczny silnika podlega ciaglej weryfikacji w procesie testow wielopoziomowych:

- **CTest (51/51 PASS - 100%)**:
  - Testy ABI struktur pakietowych, indeksowania przestrzennego `SpatialHashGrid`, rejestru `ActorRegistry`.
  - Testy kryptografii: Libsodium, Blowfish CTR, wymiana kluczy Diffie-Hellman Luria DH2.
  - Testy sterownikow sieciowych UPM (`StandardX64`, `Luria`, `Beavium`, `MartySama`).
  - Testy live klienta bezglowego (`HeadlessClientLiveTest`) oraz pomostu C-ABI (`BotFFIBridgeTest`).
- **Testy Chaosu i Skrajnych Obciazen MCP (22/22 PASS - 100%)**:
  - Testy fuzzingu routera, przerwania potoku IPC, natychmiastowego reconnectu i obciazenia do 63.7 zapytan/s (`tools/mcp/test_mcp_all_possibilities.py`).
- **Testy Sterowania Postacia na Zywo z Serwerem (11/11 PASS - 100%)**:
  - Weryfikacja ruchu, ataku, uzycia skilli, podnoszenia lootu oraz synchronizacji pamieci `m2_audit_dual_state` na dzialajacym serwerze gry x64 (`tools/mcp/test_live_character_actions.py`).
- **Preflight AI Linter (100% PASS)**:
  - Pelna poprawnosc format stringow `std::format`, brak wiszacych referencji do singletonow i zgodnosc C-ABI (`tools/ai_preflight_check.py`).
