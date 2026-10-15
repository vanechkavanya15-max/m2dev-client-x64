# AI Agent Manual: Architektura Klienta Metin2 x64 (Standard 2026)

Dokument stanowi oficjalne, najwyzsze zrodlo prawdy (Single Source of Truth) dla wszystkich agentow AI (Antigravity, Jules Swarm, subagenci MCP i skrypty autonomiczne) pracujacych z silnikiem klienta C++23 w `E:\m2dev-client-src-mainOryginalx64`.

---

## 1. Misja Architektury 2026

Tradycyjny klient Metin2 z lat 2004-2014 posiadal gigantyczne wady architektoniczne:
1. Skrajnie powolne ladowanie oparte na 13-klatkowym skrypcie Pythona (`introloading.py`).
2. Silne zakleszczenie zaleznosci: modele 3D, animacje i pliki `npclist.txt` byly parsowane w Pythonie, przez co pominiecie interfejsu powodowalo brak modeli w grze (ladowanie samej mapy).
3. Brak deterministycznego sterowania bezglowego (Headless) z fizyka i kolizjami.
4. Sztywny, jednoserwerowy protokol sieciowy uniemozliwiajacy podlaczanie do serwerow z customowym szyfrowaniem (np. DH2) i zmienionymi rozmiarami struktur pakietowych.

W architekturze 2026 wszystkie te wady zostaly bezwzglednie wyeliminowane.

---

## 2. Trzy Tryby Uruchomieniowe Silnika (Runtime Execution Modes)

Klient obsluguje 3 scisle zdefiniowane profile dzialania:

### A. Tryb Headless Client (`--headless` / `--turbo-ticks=<N>`)
- **Cel:** Pelna symulacja swiata gry w pamieci RAM na potrzeby agentow AI, testow integracyjnych CTest oraz harnessu MCP bez monitora i karty graficznej.
- **Cechy architektoniczne:**
  - Zero narzutu GPU: brak okna Windows, dummy device DirectX 9 (brak prezentacji klatek).
  - **Dlawik CPU 60 Hz:** watek glowny wykonuje `Sleep(15)` zamiast `Sleep(1)`, ograniczajac zbedna petle 1000 Hz do dokladnie 60 tickow na sekunde (uzycie CPU < 0.5%).
  - **Pelna matematyka i fizyka:** aktywny `CPythonBackground`, siatka wysokosci terenu `GetHeight(x, y)`, sprawdzanie kolizji scian, wektory ruchu `PlayerMovementController`.
  - **Dual-State Engine:** stan bytow swiata jest w czasie rzeczywistym synchronizowany pomiedzy `CPythonCharacterManager` a natywnym `ActorRegistry` oraz siatka przestrzenna `SpatialHashGrid`.
  - **Odciecie zbednych podsystemow:** `CFontManager` (FreeType) oraz `SoundEngine` sa calkowicie pomijane w headless.
  - **Sterowanie IPC:** sterowany przez nazwany potok `\\.\pipe\m2_harness_pipe_<PID>`.

### B. Tryb ImGui FastBoot (`--fastboot` / `--auto-connect` / domyslny tryb okienkowy)
- **Cel:** Nowoczesny, blyskawiczny interfejs logowania dla gracza manualnego lub testow graficznych.
- **Cechy architektoniczne:**
  - Wektorowy overlay Dear ImGui DX9 renderowany od pierwszej klatki (`FastBootOverlay`).
  - Wybor profilu serwera (Luria, Beavium, Lokalny Dev) i slotu postaci (0..3).
  - Automatyczne ukrywanie: po osiagnieciu fazy `Game` nakladka automatycznie znika z ekranu (`m_bVisible = false`).
  - **Zero-Lag Input:** gdy nakladka jest ukryta, `ProcessWndProc` natychmiast zwraca `false`, eliminujac jakikolwiek narzut na mysz i klawiature.
  - Mozliwosc przelaczenia widocznosci w dowolnym momencie gry klawiszem `VK_F11`.

### C. Tryb Pure Domain Mode (`--no-python`)
- **Cel:** Ekstremalnie odchudzone srodowisko testowe uruchamiane calkowicie bez inicjalizacji interpretera Pythona 3.14 (`Py_Initialize` nie jest wywolywane).

---

## 3. Native Instant-World Pipeline (Eliminacja Powolnego Pythona)

Wejscie do swiata gry w 2026 roku odbywa sie w czasie ponizej **1 sekundy** dzieki potokowi natywnemu:

```
[ Start Procesu ]
       │
       ▼
[ CRaceManager::PreloadCoreRaces() ] ───► Natychmiastowa rejestracja w C++ 8 ras bazowych,
       │                                  modeli .msm i sciezek zasobow (< 2 ms)
       ▼
[ CRaceManager::LoadNPCList("npclist.txt") ] ──► Zero-Copy parser C++ dla wszystkich mobow
       │
       ▼
[ TCP Connect & Handshake ] ────────────► Czyste C++ (np. LuriaDH2 negocjacja 16-bajtowego klucza Blowfish)
       │
       ▼
[ Autoryzacja & Wybor Postaci ] ────────► DirectEnterMode: natychmiastowy SendSelectCharacterPacket
       │                                  (w Pythonie: FastBootLoginWindow i FastBootSelectWindow
       │                                   pomijaja 1201 linii introLogin i introSelect!)
       ▼
[ RecvMainCharacter ] ──────────────────► Pobranie (lX, lY, VID)
       │
       ├──► Warp(lX, lY) & SetGlobalCenterPosition(lX, lY)
       ├──► Czyszczenie bytow starej mapy (DeleteAllInstances, reset ActorRegistry)
       ├──► SendEnterGame() & m_isStartGame = TRUE
       ▼
[ Faza GAME ] ──────────────────────────► Swiat 3D aktywny. W trybie GUI: GameWindow.Open()
                                          synchronicznie wyswietla HUD bez zadnego zamrazania.
```

---

## 4. Rejestr Sterownikow Protokolu (Multi-Server Protocol Registry)

Wszystkie operacje sieciowe sa odseparowane od twardych struktur za pomoca interfejsu `IServerProtocolDriver`:
Katalog: `src/Client/Network/Protocol/Drivers/`

1. **`LuriaProtocolDriver` (`"luria"`):**
   - Handshake DH2 (Diffie-Hellman 2) z weryfikacja sum kontrolnych CRC i rotacja klucza Blowfish CBC.
   - Zredukowany pakiet spawnu gracza/moba: `0x01` (CharacterAdd, 37 bajtow) bez wymogu `CHAR_ADDITIONAL_INFO`.
   - Zmapowana tablica opcodow w `luria_gc_table.inl`.
2. **`StandardX64ProtocolDriver` (`"standard_x64"`):**
   - Klasyczny standard serwerowy 40k x64 z 4-bajtowymi naglowkami `[Header:2][Length:2]` i tablicowym dispatchem $O(1)$.
   - **Ramkowanie x64 ze Stalym lub Dynamicznym Naglowkiem (`TPacketGCMainCharacter` `0x0071`):**
     W `StandardX64ProtocolDriver::InspectFrame()` oraz `CPythonNetworkStream::RecvMainCharacter()` zaimplementowano scisle ramkowanie x64 oparte na zadeklarowanej dlugosci pakietu w naglowku `[Header:2][Length:2]` (lub tabeli GC/dynamicznym naglowku profilu). Eliminuje to jakiekolwiek przesuniecie strumienia TCP (TCP Stream Desync), a pelna struktura `TPacketGCMainCharacter` (ze wszystkimi polami `szName[25]`, `wRaceNum`, `byEmpire`, `bySkillGroup`, `lX, lY, lZ`) jest prawidlowo deserializowana do `WorldContext`.
3. **`BeaviumProtocolDriver` (`"beavium"`):**
   - Profil dostosowany do serwera Beavium.

---

## 5. Standard Kontraktu IPC i TestHarness (JSON-RPC 2.0)

Serwer testowy `TestHarnessServer` nasluchuje na potoku Named Pipe:
`\\.\pipe\M2ClientEngineHarness` (lub `\\.\pipe\m2_harness_pipe_<PID>` przy uruchomieniu wielu instancji).

### Kontrakt JSON-RPC 2.0:
- **Zadanie Sukces:**
  `{"jsonrpc": "2.0", "ok": true, "result": { ... }, "id": 1}`
- **Zadanie Blad:**
  `{"jsonrpc": "2.0", "ok": false, "error": {"code": -32600, "message": "Opis bledu"}, "id": 1}`

### Kluczowe Metody Dostepne dla Agentow AI (42 Narzedzia MCP):
| Metoda | Parametry | Opis |
|---|---|---|
| `game_ping` | `{}` | Sprawdzenie czy serwer IPC odpowiada (`"status": "pong"`). |
| `game_login` | `{"account": "...", "password": "...", "profile": "luria", "slot": 0}` | Pelny automat logowania i wejscia do gry z C++. |
| `game_get_character_slots` | `{}` | Pobranie pelnej tablicy postaci na koncie (slot 0..4, nazwy, klasy, levele). |
| `game_select_character` | `{"slot": 0}` lub `{"name": "..."}` | Wybor postaci na slocie lub bezposrednio po nazwie postaci. |
| `game_create_character` | `{"name": "...", "job": 0, "gender": 0, "shape": 0, "slot": 0}` | Tworzenie nowej postaci na pustym koncie (Job: 0=Warrior, 1=Ninja, 2=Sura, 3=Shaman). |
| `game_set_auto_reconnect` | `{"enabled": true, "delay_ms": 3000, "max_attempts": 10}` | Konfiguracja samoczynnego powrotu do gry po rozlaczeniu. |
| `game_enter_game` | `{}` | Wymuszenie wejscia do gry po fazie Loading. |
| `game_get_state` | `{}` | Pobranie aktualnej fazy gry, VID gracza, nazwy, pozycji (x, y, z), rotacji, HP/SP, auto_reconnect. |
| `get_world_megasnapshot` | `{"radius": 5000.0}` | Jednozapytaniowy megasnapshot calego otoczenia, gracza, ekwipunku i przedmiotow na ziemi (1 RTT dla AI). |
| `game_get_inventory` | `{}` | Pobranie pelnego stanu slotow ekwipunku gracza (vnum, count, sockety). |
| `game_get_nearby_entities` | `{"radius": 5000.0}` | Szybkie zapytanie przestrzenne z `SpatialHashGrid` zwracajace potwory, NPC i graczy. |
| `game_action_attack` | `{"target_vid": 12345}` | Wykonanie ataku na cel z uwzglednieniem animacji i czasu odnowienia. |
| `game_move_to_position` | `{"x": 1000.0, "y": 2000.0}` | Rozpoczecie naturalnego biegu na wskazana pozycje z kalkulacja kolizji. |
| `game_use_skill` | `{"skill_slot": 1, "target_vid": 0}` | Uzycie umiejetnosci. |
| `m2_audit_dual_state` | `{}` | Weryfikacja spojnosci stanu pamieci pomiedzy silnikiem C++ a Pythonem/ActorRegistry. |
| `tick_frame` | `{"delta_ms": 16.6}` | Deterministyczny krok klatki w trybie symulacji. |

---

### 5.1. Obsluga Pustego Konta i Kreatora Postaci (Brak stworzonej postaci):
Gdy konto jest nowo zarejestrowane i nie posiada zadnej postaci:
1. **Tryb FastBoot ImGui (Manualny):**
   - W `FastBootOverlay::UpdateNetworkStatus()` klient w fazie Select sprawdza `rkNetStream.IsCharacterSlotEmpty(m_characterSlot)`.
   - Jesli slot jest pusty, overlay automatycznie aktywuje wektorowy formularz ImGui z polami: Nazwa postaci, Klasa (Wojownik, Ninja, Sura, Szaman), Plec (Mezczyzna / Kobieta) oraz Ksztalt pancerza (0 / 1).
   - Klikniecie przycisku `>> STWORZ POSTAC I WEJDZ DO GRY <<` wywoluje `CreateCharacterAndEnter()`, ktora oblicza identyfikator rasy i statystyki bazowe w C++, wysyla pakiet tworzenia postaci do serwera, a po otrzymaniu potwierdzenia sukcesu (`__RecvPlayerCreateSuccessPacket`) natychmiast automatycznie wybiera postac i przechodzi do swiata gry w < 100 ms.
2. **Tryb Headless / AI Agent (Automatyczny):**
   - W przypadku braku postaci agent AI otrzymuje w `game_get_state` faze `Phase=Select` oraz `PlayerName=""` / brak VID.
   - Agent wola metode IPC `game_create_character` z parametrami `name`, `job`, `gender`, `slot`.
   - Silnik C++ tworzy postac i automatycznie wchodzi do gry bez angazowania UI.

---

### 5.2. Auto-Reconnect po Zerwaniu Sesji (Kick / Disconnect / Restart CH):
W przypadku utraty polaczenia TCP podczas gry (np. restart kanalu serwera CH1, kick anty-afk, zerwanie gniazda):
1. **Wykrycie w C++:**
   - Metoda `CPythonNetworkStream::OnRemoteDisconnect()` powiadamia `FastBootOverlay::Instance().OnRemoteDisconnect()`.
   - Jesli klient byl uprzednio w grze (`m_bWasInGame == true`) oraz wlaczony jest auto-reconnect (`m_bAutoReconnect == true`), wlacza sie bezpieczny licznik czasu (domyslnie 3 sekundy).
2. **Konfiguracja:**
   - **Flagi startowe:** `--auto-reconnect` oraz `--reconnect-delay=<sekundy>`.
   - **Interfejs ImGui:** Checkbox `[x] Auto-Reconnect po zerwaniu` z mozliwoscia anulowania odliczania przyciskiem `Anuluj`. Ustawienie utrwalane w pliku `fast_boot_config.json`.
   - **API IPC Named Pipe:** Metoda `game_set_auto_reconnect` (`{"enabled": true, "delay_ms": 3000, "max_attempts": 10}`).
3. **Plynny powrot do swiata:**
   - Po uplywie opoznienia klient automatycznie odnawia sesje (DH2 Handshake -> Auth -> Select -> Loading -> Game) i wchodzi do gry bez restartu procesu aplikacji!

---

### 5.3. Uniwersalny VFS Dyskowy (`CPackManager`), Ladowanie Map (`AtlasInfo.txt`) i Ochrona Terenu (`CMapOutdoor`):
W celu wyeliminowania bledow brakujacych map (czarne tlo), szarych modeli bez tekstur oraz zrzutow pamieci przy poruszaniu sie wprowadzono nastepujace gwarancje w silniku C++:
1. **Pelna Obsluga Wszystkich Rozszerzen w Katalogu `assets/` (`PackManager.cpp`):**
   - W metodach `CPackManager::GetFileWithPool()` oraz `CPackManager::IsExist()` przeszukiwanie zamontowanych podkatalogow `assets/<Paczka>/<sciezka>` dziala dla **wszystkich typow plikow** (nie tylko `.glb`/`.gr2`), w tym:
     * Plikow definicji map i srodowiska: `AtlasInfo.txt`, `Setting.txt`, `Town.txt`, `.msenv`.
     * Siatek wysokosci, wody i atrybutow kolizji terenu: `height.raw`, `tile.raw`, `attr.atr`, `water.wtr`.
     * Tekstur modeli 3D, nieba i podloza: `.dds`, `.tga`, `.sub`, `.jpg`.
     * Efektow i obiektow otoczenia: `.mse`, `.prt`, `.prb`, `.prd`.
2. **Wielokierunkowy Fallback `AtlasInfo.txt` (`MapManager.cpp`):**
   - Metoda `CMapManager::__LoadMapInfoVector()` przeszukuje kolejno sciezki: `m_stAtlasInfoFileName`, `"AtlasInfo.txt"`, `"atlasinfo.txt"`, `"root/atlasinfo.txt"`, `"locale/pl/AtlasInfo.txt"`, ladujac wszystkie 110 definicji map niezaleznie od ukladu paczek.
3. **Eliminacja Dereferencji `nullptr` i Crashu przy Ruchu (`MapManager.cpp`, `PythonBackground.cpp`, `MapOutdoor.cpp`):**
   - `CMapManager::GetMapOutdoorRef()` posiada statyczna instancje awaryjna (`static CMapOutdoor s_fallbackMap;`), dzieki czemu nigdy nie zwraca referencji do adresu `0x0000000000000000` (co wczesniej powodowalo crash `0xC0000005` pod offsetem `0x12F6`).
   - `CMapManager::LoadMap()` automatycznie alokuje `m_pkMap = (CMapOutdoor*)AllocMap()`, jesli mapa nie zostala wczesniej zainicjalizowana przez `Create()`.
   - Metody `CPythonBackground::GetPickingPoint`, `GetPickingPointWithRay`, `GetPickingPointWithRayOnlyTerrain`, `CreateTargetEffect`, `CreateSpecialEffect` oraz `CMapOutdoor::GetPickingPointWithRay` weryfikuja `if (!m_pkMap || !IsMapReady() || !m_pRootNode) return false;`.
4. **Bezpieczna Kolejnosc Inicjalizacji Flagi `--direct` (`UserInterface.cpp`):**
   - `CrashSentinel::Instance().Initialize()` uruchamiany jest na samym poczatku `WinMain`.
   - Flaga `--direct <slot>` zapisywana jest do `g_iDirectEnterSlot` i aplikowana do `CPythonNetworkStream::Instance().SetDirectEnterMode()` dopiero po utworzeniu obiektu `CPythonApplication` w `Main()`.

---

### 5.4. Bootstrap Danych Gry w Pythonie (`EnsureGameDataLoaded`) oraz Parsowanie `npclist.txt`:
Gdy `introloading.py` zostaje pominiete w trybie Instant-World (lub przy natychmiastowym wejsciu do gry):
1. W `assets/root/game.py` na samym poczatku `GameWindow.Open()` (przed `interface.MakeInterface()`) wywolywana jest funkcja pomocnicza `playerSettingModule.EnsureGameDataLoaded()`.
2. Rola `EnsureGameDataLoaded()` to integracja po stronie Pythona: funkcja ta gwarantuje jednokrotna, bezpieczna inicjalizacje danych srodowiskowych, map lochow (`RegisterDungeonMapName`), tytulow rang (`chrmgr.RegisterTitleName`), kolorow nazw graczy i rang (`colorInfo`) oraz kluczowych efektow wizualnych, jesli faza IntroLoading nie zostala uruchomiona.
3. Wszelkie definicje potworow, NPC oraz kamieni Metin nie posiadaja sztucznego, zahardcodowanego fallbacku w C++ - metoda `CRaceManager::LoadNPCList("npclist.txt")` wczytuje i parsuje dane wylacznie na podstawie pliku `npclist.txt` (wyszukiwanego elastycznie w VFS oraz na dysku pod sciezkami `npclist.txt`, `root/npclist.txt`, `assets/root/npclist.txt`).

---

## 6. Zasady Pracy dla Agentow AI

1. **Zasada Jezykowa:**
   Wszystkie komentarze, dokumentacja, logi i odpowiedzi MUSZA byc formulowane w **jezyku polskim BEZ polskich znakow diakrytycznych** (uzywac a, c, e, l, n, o, s, z, z zamiast ogonkow).
2. **Zakaz Modyfikacji `C:\scripts`:**
   Folder `C:\scripts` jest zrodlem referencyjnym tylko do odczytu. Calkowity zakaz wprowadzania w nim zmian!
3. **Weryfikacja Kompilacji i Testow (100% PASS):**
   Przed zakonczeniem dowolnego zadania agent MUSI wykonac:
   ```powershell
   cmake --build build --config Release -j8
   ctest -C Release --output-on-failure
   ```
   Wszystkie **51/51 testow CTest musza miec status PASS**.
4. **Test na Zywo:**
   Po modyfikacjach sieciowych nalezy uruchomic test weryfikacyjny:
   `python tools/mcp/test_luria_live_harness.py`.

