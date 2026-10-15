# AI API CHEATSHEET: SILNIK KLIENTA C++23 (SINGLE SOURCE OF TRUTH)

> Scisly i maszynowo czytelny leksykon API silnika gry dla modeli AI, agentow Jules oraz subagentow.  
> Obowiazuje bezwzgledna zasada jezykowa: jezyk polski BEZ polskich znakow diakrytycznych.

---

## 1. Zasada Podzialu Odpowiedzialnosci (Strangler Fig Principle)

Architektura silnika opiera sie na scislej separacji warstw:
- `src/UserInterface/`: **TYLKO adapter GUI, Python 2.7, Direct3D 9 oraz Win32 API**.
  - ZAKAZ implementacji nowej logiki biznesowej, kalkulacji domenowych, logiki walki czy routingu sieciowego.
  - Sluzy wylacznie do renderowania okna, przechwytywania wejscia Windows oraz wywolywania bindingow Pythona.
- `src/Client/`: **JEDYNE MIEJSCE dla logiki domenowej silnika**.
  - Zawiera podsystemy: `Gameplay`, `Network`, `Contracts`, `Bridge`, `Core`, `Controllers`.
  - Czysty kod C++23 calkowicie odciety od DirectX i Windows API, w 100% testowalny jednostkowo w CTest.

---

## 2. Mostek C-ABI BotApi_FFI (`src/Client/Bridge/BotApi_FFI.h`)

Natywny interfejs binarny (C-ABI) o zerowym narzucie alokacji (Zero-Alloc, Zero-GC), bezpieczny wielowatkowo (`std::atomic`, `try/catch`).

### Uklad Pamieciowy `ActorSnapshot` (32 Bajty, `#pragma pack(push, 1)`)
```cpp
#pragma pack(push, 1)
struct ActorSnapshot {
    uint32_t vid;       // Offset  0 (4B): Wirtualny identyfikator bytu
    uint32_t vnum;      // Offset  4 (4B): ID prototypu / rasy potwora
    float x;            // Offset  8 (4B): Wspolrzedna swiata X
    float y;            // Offset 12 (4B): Wspolrzedna swiata Y
    float z;            // Offset 16 (4B): Wspolrzedna swiata Z (wysokosc)
    float rot;          // Offset 20 (4B): Kat obrotu postaci / HP
    float distance;     // Offset 24 (4B): Dystans do gracza / Max HP
    uint8_t type;       // Offset 28 (1B): Typ (0=mob, 1=npc, 2=metin, 6=player)
    uint8_t is_dead;    // Offset 29 (1B): Flaga smierci (0=zyje, 1=martwy)
    uint16_t _reserved; // Offset 30 (2B): Wyrownanie do 32 bajtow (granica SIMD)
};
static_assert(sizeof(ActorSnapshot) == 32, "ActorSnapshot musi miec dokladnie 32 bajty");

struct GroundItemSnapshot {
    uint32_t vid;       // Offset  0 (4B): VID lezacego przedmiotu
    uint32_t vnum;      // Offset  4 (4B): VNUM przedmiotu
    float x;            // Offset  8 (4B): Pozycja X
    float y;            // Offset 12 (4B): Pozycja Y
    uint32_t count;     // Offset 16 (4B): Liczba sztuk w stosie
};
static_assert(sizeof(GroundItemSnapshot) == 20, "GroundItemSnapshot musi miec 20 bajtow");
#pragma pack(pop)
```

### Sygnatury Funkcji Eksportowanych `extern "C"`
```cpp
// 1. Odczyt tozsamosci i stanu gracza
BOT_API uint32_t Bot_GetPlayerVID();
BOT_API bool Bot_GetPlayerPos(float* outX, float* outY, float* outZ);
BOT_API int64_t Bot_GetPlayerStatus(int statusType); // 1=Lvl, 3=Exp, 5=HP, 6=MaxHP, 7=SP, 11=Yang

// 2. Percepcja swiata gry (Zero-Alloc w bufor ciagly)
BOT_API int Bot_GetEntitiesInRadius(float radius, ActorSnapshot* outBuf, int maxCapacity);
BOT_API int Bot_GetActorsInRadius(float radius, ActorSnapshot* outBuf, int maxCapacity);
BOT_API int Bot_GetGroundItems(float radius, GroundItemSnapshot* outBuf, int maxCapacity);

// 3. Ocena i akcje walki oraz poruszania sie
BOT_API bool Bot_MoveTo(float x, float y);
BOT_API bool Bot_EvaluateAttack(uint32_t targetVid);
BOT_API bool Bot_AttackTarget(uint32_t vid);

// 4. Zarzadzanie ekwipunkiem
BOT_API bool Bot_UseItem(uint8_t windowType, uint16_t cell);
BOT_API bool Bot_GetItem(uint8_t windowType, uint16_t cell, ItemDataSnapshot* outItem);

// 5. Kontrola okna i dlawienia GPU (0% GPU Stealth)
BOT_API void Bot_SetWindowVisibility(bool visible);
BOT_API void Bot_SetRenderThrottle(bool throttle);
BOT_API bool Bot_IsWindowVisible();
```

---

## 3. Kontrakty Domenowe C++23 (`src/Client/Contracts/`)

Wszystkie kontrakty to czyste interfejsy abstrakcyjne pozbawione powiazan z Windows API i D3D.

### `IPlayerDomainContract` (`IPlayerDomainContract.h`)
```cpp
class IPlayerDomainContract {
public:
    virtual ~IPlayerDomainContract() = default;
    virtual uint32_t GetPlayerVID() const noexcept = 0;
    virtual std::string_view GetPlayerName() const noexcept = 0;
    virtual int GetPlayerLevel() const noexcept = 0;
    virtual int64_t GetCurrentHP() const noexcept = 0;
    virtual int64_t GetMaxHP() const noexcept = 0;
    virtual int64_t GetCurrentSP() const noexcept = 0;
    virtual int64_t GetMaxSP() const noexcept = 0;
    virtual int64_t GetYang() const noexcept = 0;
    virtual int64_t GetExp() const noexcept = 0;
    virtual PlayerCoordinates GetCoordinates() const noexcept = 0;
    virtual uint32_t GetTargetVID() const noexcept = 0;
    virtual bool IsInCombat() const noexcept = 0;
    virtual PlayerStatusSnapshot CaptureSnapshot() const = 0;
    virtual int64_t GetStatus(int statusType) const;
    virtual bool RequestMove(float x, float y) = 0;
};
```

### `IWorldQueryContract` (`IWorldQueryContract.h`)
```cpp
enum class EntityType : uint8_t { Unknown=0, Player=1, Monster=2, NPC=3, Warp=4, Door=5, ItemGround=6, Stone=7 };

class IWorldQueryContract {
public:
    virtual ~IWorldQueryContract() = default;
    virtual std::vector<EntitySnapshot> GetEntitiesInRadius(float radius) const = 0;
    virtual std::optional<EntitySnapshot> GetEntityByVID(uint32_t vid) const = 0;
    virtual std::optional<EntitySnapshot> GetNearestHostile() const = 0;
    virtual std::optional<EntitySnapshot> GetNearestItemGround() const = 0;
    virtual int ForEachEntityInRadius(float radius, EntityVisitor visitor, void* userData) const;
};
```

### `ICombatDomainContract` (`ICombatDomainContract.h`)
```cpp
struct CombatVerdict {
    bool canAttack{false};
    float remainingCooldown{0.0f};
    float distanceToTarget{0.0f};
    uint8_t hitChancePercent{100};
    const char* reason{""};
};

class ICombatDomainContract {
public:
    virtual ~ICombatDomainContract() = default;
    virtual CombatVerdict EvaluateAttack(uint32_t targetVid) const = 0;
    virtual float GetAttackCooldown() const noexcept = 0;
    virtual uint32_t GetLastTargetVID() const noexcept = 0;
    virtual bool ExecuteAttack(uint32_t targetVid) = 0;
    virtual void UpdateCombatState(float deltaTime) = 0;
};
```

### `IInventoryDomainContract` (`IInventoryDomainContract.h`)
```cpp
struct ItemCellAddress { uint8_t windowType{1}; uint16_t cellIndex{0}; };

class IInventoryDomainContract {
public:
    virtual ~IInventoryDomainContract() = default;
    virtual std::optional<ItemDataSnapshot> GetItem(const ItemCellAddress& pos) const = 0;
    virtual std::vector<ItemDataSnapshot> GetAllItems(uint8_t windowType) const = 0;
    virtual bool IsCellEmpty(const ItemCellAddress& pos) const noexcept = 0;
    virtual size_t GetFreeSlotCount(uint8_t windowType) const noexcept = 0;
    virtual bool RequestUseItem(const ItemCellAddress& pos) = 0;
    virtual bool RequestMoveItem(const ItemCellAddress& src, const ItemCellAddress& dst, uint32_t count) = 0;
    virtual bool RequestDropItem(const ItemCellAddress& pos, uint32_t count) = 0;
};
```

---

## 4. Sterowniki Protokolu Multi-Server (`ProtocolDriverRegistry`)

Wieloserwerowy rejestr singleton zarzadzajacy adaptacja pakietow sieciowych i kryptografii w runtime.

### Dostepne Profile Serwerowe (4 Wbudowane Nazwy)
1. `"standard_x64"`: Standardowy 64-bitowy profil protokolu bazowego.
2. `"beavium"`: Profil serwera Beavium ze specyficznymi strukturami pakietow.
3. `"martysama"`: Profil MartySama z autorskim ukladem naglowkow.
4. `"luria"`: Profil serwera Luria z 1-bajtowymi naglowkami, Blowfish CTR i wymiana kluczy DH2 MODP 1024-bit.

### Pobieranie i Dynamiczne Przelaczanie w Runtime (C++)
```cpp
#include "Client/Network/Protocol/ProtocolDriverRegistry.h"

// 1. Pobranie instancji singletona
auto& registry = Network::Protocol::ProtocolDriverRegistry::Instance();

// 2. Rejestracja profili domyslnych (jesli jeszcze nie wykonano)
registry.InitializeDefaults();

// 3. Pobranie biezaco aktywnego sterownika
Network::Protocol::IServerProtocolDriver* pActive = registry.GetActiveDriver();
std::string activeName = registry.GetActiveDriverName(); // np. "standard_x64"

// 4. Dynamiczne przelaczenie profilu protokolu w locie
bool success = registry.SetActiveDriver("luria");
if (!success) {
    // Blad: profil o podanej nazwie nie istnieje w rejestrze
}
```

---

## 5. Serwer Testowy IPC Named Pipe (`TestHarnessServer`)

- Sciezka potoku: `\\.\pipe\M2ClientEngineHarness` (konfigurowalna przez parametr `--pipe=...`).
- Format: JSON-RPC 2.0 (wspiera format tekstowy oraz naglowek 4-bajtowy little-endian uint32_t).

### Protokol i Rejestr Metod JSON-RPC 2.0

Wszystkie metody dotykajace obiektow gry i pamieci sa wykonywane w trybie **Queued** (kolejkowane do glownego watku w `ProcessMainThreadQueue()`), co w 100% eliminuje zjawisko Data Race z petla gry `UpdateGame()`. Metody czysto procesowe i sterujace potokiem dzialaja w trybie **Immediate**.

| Metoda | Tryb Wykonania | Parametry | Zwracana Wartosc / Opis |
|---|---|---|---|
| `game_ping` | Immediate | `{}` | `{"pong": true, "timestamp_ms": ...}` - Test dostepnosci i responsywnosci IPC (liveness). |
| `get_engine_state` | Queued | `{}` | Pelny synchroniczny snapshot silnika: `vid`, `name`, `x`, `y`, `z`, `hp`, `max_hp`, `yang`, `phase`, `active_driver`, `window_visible`, `gpu_throttled`. |
| `game_get_state` | Queued | `{}` | Stan gracza przez kontrakt `IPlayerDomainContract` (pozycja, hp, sp, exp, poziom, stan walki). |
| `game_get_inventory` | Queued | `{"window": int}` | Pelny stan ekwipunku przez `IInventoryDomainContract` (sloty, vnum, count, sockety, bonusy). |
| `game_get_nearby_entities` | Queued | `{"radius": float}` | Lista potworow, graczy i NPC w zadanym promieniu przez `IWorldQueryContract` / `SpatialHashGrid`. |
| `game_action_attack` | Queued | `{"target_vid": int}` | Bezpieczny atak na cel przez kontrakt `ICombatDomainContract` z weryfikacja zasiegu i stanu zycia. |
| `game_action_use_item` | Queued | `{"window": int, "cell": int}` | Uzycie przedmiotu w danym slocie przez kontrakt `IInventoryDomainContract`. |
| `audit_dual_state` | Queued | `{"epsilon": float}` | Weryfikacja synchronizacji pamieci: C++ vs Python VM (gracz: pozycja, rotacja, hp, sp, vid) oraz `world_audit` (spojnosc `m_aliveActorsMap` z `ActorRegistry`). Parametr `epsilon` definiuje dopuszczalny prog tolerancji. Uwaga: w Pythonie `chr.GetRotation()` zwraca `360.0f - deg`. |
| `game_login` | Queued | `{"username": str, "password": str, "ip": str, "port": int}` | Inicjalizacja polaczenia sieciowego i logowanie. Wspiera elastyczne klucze (`username`/`account`/`id` oraz `server_ip`/`ip`/`addr`). |
| `game_get_character_slots` | Queued | `{}` | Pobiera tablice wszystkich slotow postaci na koncie (0..4): `is_empty`, `id`, `name`, `level`, `job`, `job_name`, `x`, `y`. |
| `game_select_character` | Queued | `{"slot": int}` lub `{"name": str}` | Wybor postaci (slot 0-4 lub po nazwie) w fazie Select i zgloszenie wyboru do serwera. |
| `game_create_character` | Queued | `{"name": str, "job": int/str, "gender": int/str, "shape": int, "slot": int}` | Tworzenie nowej postaci na pustym koncie z automatycznym wejsciem do swiata po utworzeniu. |
| `game_set_auto_reconnect` | Queued | `{"enabled": bool, "delay_ms": int, "max_attempts": int}` | Konfiguracja automatycznego powrotu do swiata gry po rozlaczeniu / kicku. |
| `game_enter_game` | Queued | `{}` | Natychmiastowe wyslanie pakietu wejscia do gry `SendEnterGame()` z ominieciem opoznien UI. |
| `get_world_megasnapshot` | Queued | `{"radius": float}` | Pojedynczy megasnapshot calego otoczenia, gracza, ekwipunku i przedmiotow na ziemi (1 RTT). |
| `game_set_attack_key_state` | Queued | `{"is_pressed": bool}` | Sterowanie klawiszem ataku postaci (spacja/auto-atak) przez `CPythonPlayer::SetAttackKeyState`. Automatycznie resetowane do `false` przy Warp i smierci postaci. |
| `game_set_dir_key_state` | Queued | `{"dir": int, "is_pressed": bool}` | Sterowanie pojedynczym kierunkiem (0=UP, 1=DOWN, 2=LEFT, 3=RIGHT) przez `CPythonPlayer::NEW_SetSingleDirKeyState`. Walidacja zakresu 0..3. |
| `game_set_multi_dir_key_state` | Queued | `{"left": bool, "right": bool, ...}` | Sterowanie ruchem wielokierunkowym (WASD) przez `CPythonPlayer::NEW_SetMultiDirKeyState`. |
| `game_move_to_position` | Queued | `{"x": float, "y": float, "z": float}` | Ruch do wskazanych koordynatow pikselowych swiata gry przez `CPythonPlayer::NEW_MoveToDestPixelPositionDirection`. Zabezpieczenie przed NaN/Inf (`std::isfinite`) oraz granicami mapy [0, 204800). |
| `game_stop` | Queued | `{}` | Natychmiastowe zatrzymanie ruchu postaci gracza przez `CPythonPlayer::NEW_Stop`. |
| `game_set_target` | Queued | `{"target_vid": int, "force": bool}` | Zaznaczenie celu w swiecie gry przez `CPythonPlayer::SetTarget`. |
| `game_clear_target` | Queued | `{}` | Odznaczenie biezacego celu przez `CPythonPlayer::SetTarget(0, TRUE)`. |
| `game_pick_close_item` | Queued | `{}` | Podniesienie lezacego w poblizu przedmiotu z ziemi (klawisz Z) przez `CPythonPlayer::PickCloseItem`. |
| `game_use_skill` | Queued | `{"slot": int}` | Uzycie / aktywacja umiejetnosci z wybranego slotu przez `CPythonPlayer::ClickSkillSlot`. Walidacja slotu `< SKILL_MAX_NUM` oraz wymaganego celu. |
| `game_send_chat` / `chat_send` | Queued | `{"text": string, "type": int}` | Wyslanie wiadomosci na czacie lub komendy GM przez `CPythonNetworkStream::SendChatPacket`. Walidacja dlugosci tekstu `< 512` znakow. |
| `game_attack_target` | Queued | `{"target_vid": int}` | Bezposredni atak celu przez `CPythonPlayer::AttackTarget`. |
| `game_use_item` | Queued | `{"window": int, "cell": int}` | Bezposrednie uzycie przedmiotu przez `CPythonNetworkStream::SendItemUsePacket`. |
| `game_set_window_mode` | Queued | `{"visible": bool, "throttle": bool}` | Dynamiczne chowanie okna (`SW_HIDE`/`SW_RESTORE`) i przelacznik dlawika 0% GPU. |
| `game_set_protocol_driver` | Immediate | `{"driver": string}` | Przelacza aktywny sterownik protokolu w locie (`"standard_x64"`, `"beavium"`, `"martysama"`, `"luria"`). |
| `tick_frame` | Queued | `{"count": int, "delta_time": float}` | Deterministyczne wykonanie dokladnie N klatek logiki silnika z zadanym krokiem czasowym. |
| `inject_packet` | Queued | `{"header": int, "data": string}` | Wstrzykuje binarny pakiet z ciagu hex bezposrednio do routera pakietow. |
| `shutdown` | Immediate | `{}` | Czyste zakonczenie procesu klienta przez asynchroniczne WM_CLOSE i ExitProcess(0). |

### Przyklad Zadania i Odpowiedzi JSON-RPC 2.0
```json
// Zadanie
{"jsonrpc": "2.0", "id": 1, "method": "game_set_protocol_driver", "params": {"driver": "luria"}}

// Odpowiedz sukcesu
{"jsonrpc": "2.0", "id": 1, "result": {"driver": "luria", "status": "active"}}
```

---

## 6. Fast-Boot & Direct-to-World (Dear ImGui DX9)
- **Architektura**: Pelne pominiecie archaicznych faz Pythona (`introLogin`, `introSelect`, `introLoading`).
- **Kreator Postaci na Pustym Koncie**: Automatyczne wykrycie pustego slotu w `FastBootOverlay` i natywne tworzenie postaci przez `CreateCharacterAndEnter()` lub metode IPC `game_create_character`.
- **Auto-Reconnect**: Samoczynne wznawianie sesji po rozlaczeniu / kicku (`OnRemoteDisconnect()`, flaga `--auto-reconnect`, RPC `game_set_auto_reconnect`).
- **Skrot klawiszowy**: Klawisz **F11** przelacza widocznosc panelu w runtime.
- **Odpornosc sieciowa**: Pelna translacja kodow bledu (`ALREADY`, `WRONGPWD`, `NOID`, `BLOCK`, `TIMEOUT`) z 15s watchdogiem.
- **Dokumentacja**: Szczegoly w [`FAST_BOOT_DIRECT_TO_WORLD_MANUAL.md`](file:///E:/m2dev-client-src-mainOryginalx64/docs/FAST_BOOT_DIRECT_TO_WORLD_MANUAL.md).

---

## 7. Uniwersalny VFS Dyskowy (`CPackManager`) i Ochrona Terenu (`CMapOutdoor`)
- **Uniwersalny VFS**: `CPackManager::GetFileWithPool()` oraz `IsExist()` przeszukuja zamontowane podkatalogi `assets/<Paczka>/<sciezka>` dla **wszystkich rozszerzen** (`.txt`, `.raw`, `.atr`, `.wtr`, `.msenv`, `.dds`, `.tga`, `.sub`, `.mse`, `.prt`, `.prb`, `.glb`, `.gr2`).
- **Fallback `AtlasInfo.txt`**: `CMapManager::__LoadMapInfoVector()` sprawdza kolejno `m_stAtlasInfoFileName`, `"AtlasInfo.txt"`, `"atlasinfo.txt"`, `"root/atlasinfo.txt"`, `"locale/pl/AtlasInfo.txt"`, ladujac 110 definicji map.
- **Eliminacja Crashu `0xC0000005` przy Ruchu**: `CMapManager::GetMapOutdoorRef()` zwraca bezpieczny fallback `s_fallbackMap`, `LoadMap()` automatycznie alokuje `m_pkMap`, a metody pickingu w `CPythonBackground` oraz `CMapOutdoor` weryfikuja `IsMapReady()` oraz `m_pRootNode != nullptr`.

---

## 8. Weryfikacja Jakosciowa (100% PASS)
- **CTest**: **51 / 51 PASS (100%)**.
- **Live Luria & Standard x64 Test**: **PASS** (Handshake DH2 / StandardX64 113B MainCharacter -> DirectEnter -> Wejscie do fazy GAME i zaladowanie mapy w ~1.5s).
- **MCP Chaos & Resilience Suite**: **22 / 22 PASS (100%)**.
- **AI Preflight Check**: **100% PASS (0.44s)**. Zero regresji C-ABI, Singleton i Format String.


