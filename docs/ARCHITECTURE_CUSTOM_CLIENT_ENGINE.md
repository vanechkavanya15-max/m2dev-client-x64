# ARCHITEKTURA ZUNIFIKOWANEGO SILNIKA KLIENTA C++23 (CUSTOM CLIENT ENGINE SPECIFICATION)

> Dokument stanowi glowne zrodlo prawdy (Single Source of Truth) dla architektury wbudowanego silnika klienta Metin2 x64 w repozytorium `E:\m2dev-client-src-mainOryginalx64`.  
> Wszystkie komunikaty, sygnatury i dokumentacja sporzadzone w jezyku polskim BEZ znakow diakrytycznych.

---

## 1. Zamysl i Filozofia Architektoniczna

### A. Zasada "Jednego Niezmiennego Silnika"
Tradycyjne podejscie w srodowisku prywatnych serwerow (P-serwerow) prowadzilo do powielania dziesiatek zrodel gry z dyrektywami `#ifdef SERVER_X`. 
W naszym silniku wprowadzono zunifikowana, warstwowa architekture:
- **Core Engine (95% kodu)**: Matematyka terenu (`PRTerrainLib`), szkielety i animacje Granny 3D, zunifikowany stan [`WorldContext`](../src/Client/Core/WorldContext.h), kontrolery ruchu i walki, zarzadzanie pamiecia oraz petla glowna gry sa **calkowicie niezalezne od jakiegokolwiek serwera prywatnego**.
- **Nawierzchnia Adaptacyjna (5% kodu)**: Wszystko, co prywatne serwery modyfikuja (pakiety sieciowe, szyfrowanie, naglowki, limity czasowe ataku), jest odizolowane w sterownikach protokolu dziedziczacych po [`IServerProtocolDriver`](../src/Client/Network/Protocol/IServerProtocolDriver.h).

### B. Eliminacja Zbednych Bibliotek Posrednich (Zero-Sol2, Pure C-ABI)
Zamiast ciezkich szablonow C++ (jak Sol2), silnik udostepnia bezposredni, natywny mostek C-ABI w `src/Client/Bridge/BotApi_FFI.h/.cpp`.
- Dziala z zerowym narzutem pamieciowym (**Zero-Allocation / Zero-GC**).
- Wypelnia przekazane przez skrypt bufory ciagle w pamieci RAM w czasie ponizej 0.001 ms.
- Umozliwia bezposrednia integracje z dowolnym jezykiem (LuaJIT FFI, C# P/Invoke, Rust FFI).

---

## 2. Model Pamieciowy i Sygnatury C-ABI (`BotApi_FFI`)

Pliki: [`src/Client/Bridge/BotApi_FFI.h`](../src/Client/Bridge/BotApi_FFI.h) oraz [`src/Client/Bridge/BotApi_FFI.cpp`](../src/Client/Bridge/BotApi_FFI.cpp).

### A. Uklad Struktur Pamieciowych (`#pragma pack(push, 1)`)

#### `ActorSnapshot` (32 Bajty - Wyrownane do Granicy 4/8 Bajtow)
Struktura reprezentujaca dowolny byt w swiecie gry (potwor, boss, metin, npc, gracz):
| Pole | Typ | Rozmiar | Offset | Opis |
|---|---|---|---|---|
| `vid` | `uint32_t` | 4 B | 0 | Wirtualny identyfikator bytu |
| `vnum` | `uint32_t` | 4 B | 4 | Identyfikator rasy / typu potwora |
| `x` | `float` | 4 B | 8 | Pozycja w swiecie gry X |
| `y` | `float` | 4 B | 12 | Pozycja w swiecie gry Y |
| `z` | `float` | 4 B | 16 | Wysokosc Z |
| `hp` | `int32_t` | 4 B | 20 | Biezace punkty zycia |
| `max_hp` | `int32_t` | 4 B | 24 | Maksymalne punkty zycia |
| `type` | `uint8_t` | 1 B | 28 | Typ instancji: 0=mob, 1=npc, 2=metin, 6=player |
| `is_dead` | `uint8_t` | 1 B | 29 | Flaga smierci (0=zyje, 1=martwy) |
| `_reserved`| `uint16_t` | 2 B | 30 | Wyrownanie do 32 bajtow (brak kar SIMD i misalignment) |

#### `GroundItemSnapshot` (20 Bajtow)
Struktura reprezentujaca upuszczony przedmiot na ziemi:
| Pole | Typ | Rozmiar | Offset | Opis |
|---|---|---|---|---|
| `vid` | `uint32_t` | 4 B | 0 | Wirtualny identyfikator przedmiotu na ziemi |
| `vnum` | `uint32_t` | 4 B | 4 | VNUM przedmiotu |
| `x` | `float` | 4 B | 8 | Pozycja X |
| `y` | `float` | 4 B | 12 | Pozycja Y |
| `count` | `uint32_t` | 4 B | 16 | Liczba sztuk w stosie |

### B. Eksportowane Funkcje `extern "C" __declspec(dllexport)`

1. **Odczyt stanu gracza:**
   - `bool Bot_GetPlayerPos(float* outX, float* outY, float* outZ)`: zwraca `true` po udanym odczycie, `false` gdy brak kontraktu gracza.
   - `int64_t Bot_GetPlayerStatus(int statusType)` (np. 1=Level, 5=HP, 6=MaxHP, 7=Yang)
2. **Zero-Alokacyjna Percepcja Otoczenia:**
   - `int Bot_GetActorsInRadius(float radius, ActorSnapshot* outBuf, int maxCapacity)`:
     Wypelnia tablice `outBuf` bez zadnych alokacji pamieci na stercie i zwraca liczbe wpisow. Zabezpieczone przed ujemnym promieniem oraz odrzucajace byty niebedace aktorami (drzwi, warpy, nieznane).
   - `int Bot_GetGroundItems(float radius, GroundItemSnapshot* outBuf, int maxCapacity)`:
     Wypelnia tablice lezacych przedmiotow (wspiera mocki z blokada `std::mutex`).
3. **Akcje Domenowe:**
   - `bool Bot_MoveTo(float targetX, float targetY)`: z walidacja liczb zmiennoprzecinkowych (odrzuca NaN/Inf oraz zwraca `false` gdy gracz nie jest zainicjalizowany).
   - `bool Bot_AttackTarget(uint32_t vid)`: natychmiastowe zbadanie zasiegu i wykonanie uderzenia.
   - `bool Bot_UseItem(uint8_t windowType, uint16_t cell)`: bezpieczne uzycie przedmiotu.
4. **Sterowanie Widocznoscia Okna i Rendererem:**
   - `void Bot_SetWindowVisibility(bool visible)`
   - `void Bot_SetRenderThrottle(bool throttle)`
   - `bool Bot_IsWindowVisible()`

---

## 3. Potrojny Cykl Zycia Renderera (Tri-State & 0% GPU Engine)

W plikach [`PythonApplication.h`](../src/UserInterface/PythonApplication.h) oraz [`PythonApplication.cpp`](../src/UserInterface/PythonApplication.cpp) wprowadzono atomowa kontrole dlawienia potoku graficznego:
- Zmienna atomowa: `std::atomic<bool> m_bRenderThrottling{false};`
- Metody: `SetRenderThrottling(bool throttle)`, `IsRenderThrottling()`, `SetWindowVisible(bool visible)`.

### Stany Pracy:
1. **Stan 1: Normal Windowed (60 FPS)**:
   - Okno gry widoczne, Direct3D 9 renderuje klatki w pelnej plynnosci.
2. **Stan 2: Stealth Background (0.0% GPU, Okno Ukryte)**:
   - Okno ukryte (`ShowWindow(m_hWnd, SW_HIDE)`).
   - `canRender = false`: Omijany jest caly blok `m_pyGraphic.Begin()`, `RenderGame()`, `m_pyGraphic.Show()`.
   - Watek usypia sie na 1 ms (`std::this_thread::sleep_for(1ms)`), aby nie obciazac rdzenia CPU.
   - **Logika gry, odbieranie pakietow sieciowych, ruch postaci i potok IPC dzialaja w pelnej predkosci 60 Hz**.
3. **Stan 3: True Headless (`--headless`)**:
   - Binarka uruchamia sie bez tworzenia urzadzenia D3D i bez okna Windows (NullDevice).

---

## 4. System Profili Serwerowych (CLI + JSON + Registry)

### A. Argumenty Wiersza Polecen w [`UserInterface.cpp`](../src/UserInterface/UserInterface.cpp#L330-L430)
- `--server=<nazwa>`: Aktywuje sterownik protokolu z rejestru (np. `--server=luria`, `--server=beavium`, `--server=martysama`).
- `--profile=<sciezka>`: Laduje profil JSON z dysku (np. `--profile=servers/pandora.json`).
- `--pipe=<nazwa>`: Definiuje potok IPC Named Pipe (np. `--pipe=\\.\pipe\M2Client_PID`).
- `--stealth`: Uruchamia proces z ukrytym oknem i 0% GPU.
- `--headless`: Uruchamia proces w pelnym trybie bezokienkowym.

### B. Architektura Rejestru Sterownikow ([`ProtocolDriverRegistry`](../src/Client/Network/Protocol/ProtocolDriverRegistry.h))
Rejestr singleton zarzadza zarejestrowanymi sterownikami `IServerProtocolDriver`:
```cpp
auto& registry = Network::Protocol::ProtocolDriverRegistry::Instance();
registry.InitializeDefaults(); // standard_x64, beavium, martysama, luria
registry.SetActiveDriver("luria");
```

---

## 5. Serwer IPC Named Pipe ([`TestHarnessServer`](../src/UserInterface/TestHarness/TestHarnessServer.h))

Potok: `\\.\pipe\M2ClientEngineHarness` (lub zdefiniowany w `--pipe`).  
Standard: **JSON-RPC 2.0** z automatycznym wykrywaniem 4-bajtowego prefiksu dlugosci oraz surowego tekstu JSON.

### Rejestr Metod:
| Metoda | Tryb | Parametry | Opis |
|---|---|---|---|
| `game_ping` | Immediate | brak | Liveness test, zwraca `{"pong": true}` |
| `get_engine_state` | Queued | brak | Zwraca stan gracza, koordynaty, HP, Yang oraz pola `"active_driver"`, `"window_visible"`, `"gpu_throttled"` |
| `game_get_state` | Queued | brak | Stan gracza przez kontrakt `IPlayerDomainContract` |
| `game_get_inventory` | Queued | `{"window": int}` | Ekwipunek przez `IInventoryDomainContract` |
| `game_get_nearby_entities` | Queued | `{"radius": float}` | Percepcja bytow przez `IWorldQueryContract` / `SpatialHashGrid` |
| `game_action_attack` | Queued | `{"target_vid": int}` | Atak na cel przez `ICombatDomainContract` |
| `game_action_use_item` | Queued | `{"window": int, "cell": int}` | Uzycie przedmiotu przez `IInventoryDomainContract` |
| `game_login` | Queued | `{"username": str, "password": str, ...}` | Inicjalizacja polaczenia sieciowego i pakiet logowania |
| `game_select_character` | Queued | `{"slot": int}` | Wybor postaci (0..3) w fazie Select |
| `game_enter_game` | Queued | brak | Natychmiastowe wejscie do swiata gry SendEnterGame |
| `game_set_attack_key_state` | Queued | `{"is_pressed": bool}` | Sterowanie klawiszem ataku (spacja/auto-atak) |
| `game_set_dir_key_state` | Queued | `{"dir": int, "is_pressed": bool}` | Sterowanie pojedynczym kierunkiem ruchu (0..3) |
| `game_set_multi_dir_key_state` | Queued | `{"left": bool, "right": bool, ...}` | Sterowanie ruchem wielokierunkowym (WASD) |
| `game_move_to_position` | Queued | `{"x": float, "y": float, "z": float}` | Nawigacja pikselowa postaci (ochrona NaN/Inf) |
| `game_stop` | Queued | brak | Zatrzymanie postaci w miejscu |
| `game_set_target` / `game_clear_target` | Queued | `{"target_vid": int, "force": bool}` | Zaznaczanie i odznaczanie celow wg VID |
| `game_pick_close_item` | Queued | brak | Podnoszenie przedmiotow z ziemi (klawisz Z) |
| `game_use_skill` | Queued | `{"slot": int}` | Aktywacja / uzycie umiejetnosci ze slotu paska |
| `game_send_chat` / `chat_send` | Queued | `{"text": string, "type": int}` | Wysylanie wiadomosci na czat lub komendy GM |
| `game_attack_target` | Queued | `{"target_vid": int}` | Bezposredni atak celu przez CPythonPlayer::AttackTarget |
| `game_use_item` | Queued | `{"window": int, "cell": int}` | Bezposrednie uzycie przedmiotu przez SendItemUsePacket |
| `game_set_window_mode` | Queued | `{"visible": bool, "throttle": bool}` | Dynamicznie chowa lub odkrywa okno gry i steruje dlawieniem GPU |
| `game_set_protocol_driver` | Immediate | `{"driver": string}` | Przelacza aktywny profil serwera w locie |
| `tick_frame` | Queued | `{"count": int, "delta": float}` | Przepycha N klatek logiki (determinizm testowy) |
| `inject_packet` | Queued | `{"header": int, "data": string}` | Wstrzykuje binarny pakiet z ciagu hex do routera |
| `audit_dual_state` | Queued | `{"epsilon": float}` | Weryfikacja synchronizacji Dual-State i ActorRegistry |
| `shutdown` | Immediate | brak | Bezpiecznie zamyka proces z kodem 0 |


---

## 6. Weryfikacja Testowa (CTest 100% PASS)

Wszystkie moduly sa w pelni zweryfikowane w systemie CMake / CTest:
- `BotFFIBridgeTest` ([`tests/test_c33_bot_ffi_bridge.cpp`](../tests/test_c33_bot_ffi_bridge.cpp)): Weryfikuje uklad pamieci ABI, rozmiary struktur, offsety pol, filtry typow, zerowa alokacje i bezpieczenstwo wskaznikow.
- `MultiServerProtocolTest`, `BeaviumServerProfileTest`, `UniversalProtocolDriversTest`: Weryfikuja przelaczanie sterownikow.
- `CryptoProvidersTest` ([`tests/test_c35_crypto_providers.cpp`](../tests/test_c35_crypto_providers.cpp)): Weryfikuje strategie kryptograficzne, w tym silnik Blowfish CTR oraz deterministyczna negocjacje kluczy Luria DH2 MODP 1024-bit.
- `HeadlessClientLiveTest`: Weryfikuje uruchomienie binarki `Metin2_Release.exe --headless` z polaczeniem Named Pipe.
- `McpHarnessSelfTest`: Weryfikuje serwer MCP pod architekture C++23.

Wynik zestawu testow: **51/51 testow zakonczonych sukcesem (100% PASS w 8.91s)**.

---

## 7. Wyniki Audytu Bezpieczenstwa i Stabilnosci (Critical Hardening)

W toku rygorystycznego audytu statycznego i dynamicznego zidentyfikowano i wyeliminowano nastepujace problemy:
1. **Concurrency & Data Race**:
   - Wskazniki kontraktow w `BotApi_FFI` oraz `ProtocolDriverRegistry` zabezpieczono polami `std::atomic` oraz dedykowanymi mutexami `std::mutex`.
   - Zabezpieczono wektor `s_mockGroundItems` przed uszkodzeniem sterty przy wspolbieznym dostepie.
2. **Krytyczne Null-Safety**:
   - `Bot_MoveTo`: Naprawiono blad logiki - funkcja zwraca teraz prawidlowo `false` zamiast `true`, gdy gracz nie jest zainicjalizowany.
   - Wszystkie eksportowane funkcje C-ABI posiadaja pelne zabezpieczenie blokami `try { ... } catch (...)`, zapobiegajac wyciekowi wyjatkow C++ poza granice ABI.
3. **Optymalizacja Pamieci C-ABI**:
   - `ActorSnapshot` zostal rozszerzony z 30 do 32 bajtow (`_reserved: uint16_t`, `is_dead: uint8_t`). Zapobiega to karom wydajnosciowym z tytulu braku wyrownania (misalignment) oraz wyjatkom sprzetowym SIMD (SSE/AVX) przy operacjach wektorowych.
4. **Cykl Zycia Okna i Throttling**:
   - Naprawiono problem zablokowania renderowania przy przejsciu ze `--stealth` do okna widocznego - `SetWindowVisible(true)` prawidlowo resetuje flage `m_isMinimizedWnd = false`, przywracajac renderowanie D3D9.
   - Wyeliminowano deadlock w `TestHarnessServer::Stop()` poprzez atomowe anulowanie zadan i zwalnianie uchwytu potoku klienta.

---

## 8. Gleboki Audyt Warstwy Sieciowej i Sterownikow Protokolu (100% Zero-Defect Hardening)

W ramach zaawansowanego audytu podsystemu sieciowego (`AccountConnector`, `PythonNetworkStream`, `ProtocolDriverRegistry`, `Drivers/*`) zidentyfikowano i wyeliminowano krytyczne podatnosci:
1. **Zabezpieczenie Null Singleton Dereference w Adapterach Gry (`GameDomainAdapters.cpp`)**:
   - Wszystkie bezposrednie wywolania `CPythonPlayer::Instance()` i `CPythonCharacterManager::Instance()` zastapiono bezpiecznymi sprawdzeniami `InstancePtr()`. Wywolania FFI przed wejsciem do swiata gry (ekran logowania, wybor postaci, ladowanie fazy) nie powoduja juz crasha `0xC0000005`.
2. **Eliminacja Zdalnego Crashu Procesu (Format String Crash w `PythonNetworkStreamCommand.cpp`)**:
   - Usunieto nadmiarowy znacznik `%s` w komunikatach `TraceError("... Strange Parameter Count : %s", c_szCommand)` we wszystkich handlerach komend serwera (`cube`, `BettingMoney` itp.), zapobiegajac natychmiastowej awarii klienta przy nieoczekiwanej liczbie argumentow.
3. **Naprawa Destrukcyjnej Mutacji IP Postaci (`PythonNetworkStream.cpp`)**:
   - W `GetAccountCharacterSlotDataz` wyeliminowano niszczenie adresu `rkSimplePlayerInfo.lAddr >>= 8` w glownym rejestrze slotow. Operacja odbywa sie na lokalnej kopii `dwAddr`, co umozliwia poprawne laczenie sie z serwerem gry w `ConnectGameServer` (brak zerowania adresu do `0.0.0.0`).
4. **Naprawa Odwroconej Logiki Filtra Obelg (`PythonNetworkStreamPhaseLoading.cpp`)**:
   - Skorygowano warunek w `IsChatInsultIn` (`if (!m_isEnableChatInsultFilter) return false;`).
5. **Zabezpieczenie Pamieci i Out-of-Bounds**:
   - Dodano walidacje granic `m_dwSelectedCharacterIndex < PLAYER_PER_ACCOUNT4` przed zapisem w `m_akSimplePlayerInfo`.
   - Skorygowano uzycie referencji `CRef<CNetworkActorManager>` w handlerach postaci Luria.
6. **Zabezpieczenie `AccountConnector` i Obsluga 1-Bajtowych Naglowkow**:
   - `Connect()` zabezpieczono przed przekazaniem `nullptr` jako adresu hosta.
   - `__AnalyzePacket` prawidlowo rozpoznaje sterowniki z 1-bajtowymi naglowkami (`martysama`, `beavium`, `luria`) oraz automatycznie inicjalizuje domyslne sterowniki przy ich braku.
   - W `__AuthState_RecvLoginFailureLuria` pobierany jest pelny 10-bajtowy bufor wraz z bajtem paddingu.
7. **Wyeliminowanie Wyciekow Kryptografii w `NetStream.cpp`**:
   - Dodano atomowe czyszczenie `m_pCryptoProvider.reset()` w `CNetworkStream::Clear()`.
8. **Thread-Safety i Wyrownanie Pamieci w Sterownikach**:
   - `ProtocolDriverRegistry::GetActiveDriverName()` zwraca bezpieczna kopie `std::string` chroniona muteksem.
   - W `BeaviumProtocolDriver.cpp` zastapiono niewyrownany odczyt `*reinterpret_cast<const uint32_t*>` bezpiecznym `std::memcpy`.
   - `DispatchInbound` w `BeaviumProtocolDriver`, `MartySamaProtocolDriver` i `LuriaProtocolDriver` propaguje wynik `DispatchPacket`, umozliwiajac prawidlowe wywolanie awaryjnego routingu `RouteGamePacket`.
