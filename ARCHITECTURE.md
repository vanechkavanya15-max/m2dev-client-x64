# ARCHITECTURE.md - Standard Architektury Silnika Klienta Gry 2026

Ten dokument jest glownym zrodlem prawdy (Single Source of Truth) dla wszystkich inzynierow i Agentow AI (Antigravity, Google Jules, roje Swarm) pracujacych w repozytorium klienta Metin2 x64 (Client2026).

---

## 1. Warstwy Architektoniczne Silnika (Clean / Onion Architecture)

Przeplyw zaleznosci jest scisle jednokierunkowy:

```
┌────────────────────────────────────────────────────────────────────────┐
│ WARSTWA PREZENTACJI & GUI (src/UserInterface/)                         │
│ • Okna Pythona, widoki, rendering DirectX 9 / D3D9                     │
│ • Cienkie fasady: PythonPlayer, PythonCharacterManager, PythonNetworkStream │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ (zalezy wylacznie od domen)
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│ WARSTWA DOMEN STANOWYCH C++23 (src/Client/Gameplay, World, Network)     │
│ • Czysta logika biznesowa, reguly gry, ekwipunek, statystyki, kodeki   │
│ • ZERO PYTHONA, ZERO DIRECTX, ZERO HWND                                │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ (zalezy wylacznie od rdzenia)
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│ WARSTWA RDZENIA & FUNDAMENTOW (src/EterBase/, src/Client/Core/)        │
│ • Silne typy (EntityVid, ItemVnum), PacketResult<T>, Math SIMD, Mutexy │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Pelny Przeplyw Pakietu Sieciowego

```
[ Serwer Gry (TCP) ]
         │
         ▼
[ CPythonNetworkStream (Gniazdo Asynchroniczne) ]
         │
         ▼
[ PacketDispatcher / PhaseGameBridge (Mostek GUI) ]
         │ (Przekazuje std::span<const uint8_t>)
         ▼
[ Client::Network::*PacketCodec (Czysta Deserializacja C++23) ]
         │ (Zwraca EterBase::PacketResult<TPacketData>)
         ▼
[ Client::Gameplay / Client::World (Aktualizacja Stanu Domenowego) ]
         │ (Bezpieczna mutacja chroniona std::shared_mutex)
         ▼
[ UserInterface::Core::EventBus (Rozgloszenie Zdarzenia) ]
         │ (Powiadomienie subskrybentow)
         ▼
[ Python UI / CPythonPlayer (Odswiezenie Elementow Okna Gry) ]
```

---

## 3. Podsystem Routerow Sieciowych Fazy Gry (src/UserInterface/Network/Routers/)

W architekturze Client2026 obsluga pakietow fazy gry (`PhaseGame`) zostala odseprarowana od monolitycznych metod przetwarzania strumienia sieciowego. Logika dystrybucji zdeserializowanych danych spoczywa na wyspecjalizowanym podsystemie routerow domenowych.

### A. Centralny Dyspozytor: PhaseGamePacketDispatcher
Klasa `PhaseGamePacketDispatcher` (`src/UserInterface/Network/Routers/PhaseGamePacketDispatcher.h`) pelni role glownego rozdzielacza pakietow:
1. **Dwuetapowa tablica skokow (Jump Table O(1)):**
   - Dla standardowych 8-bitowych naglowkow Ymir (0..255) dyspozytor wykorzystuje prealokowana tablice wskaznikow `std::array<IPacketRouter*, 256> m_jumpTable8`, gwarantujac natychmiastowe przekierowanie pakietu w czasie stalym O(1).
   - Dla nowoczesnych 16-bitowych opkodow (m2dev/x64) wykorzystywany jest deterministyczny, zoptymalizowany przez kompilator switch O(1).
2. **Kontrakt Zero-Desync (Zero-Desync Contract):**
   - Dyspozytor ani podlegle routery **nigdy nie dotykaja surowego strumienia TCP** i **nigdy nie wywoluja metody `Recv()`**.
   - Pakiety trafiaja do dyspozytora wylacznie po uprzednim zbadaniu dlugosci ramki, walidacji payloadu i deserializacji do gotowych struktur POD (`const TPacketGC*&`) lub buforow ze zweryfikowanym rozmiarem `payloadSize`.
3. **Determinizm Jednowatkowy (Single-Threaded Execution):**
   - Caly podsystem routerow dziala wylacznie w glownym watku gry. Eliminuje to koniecznosc stosowania blokad wielowatkowych (mutexow) na poziomie dyspozycji, wykluczajac ryzyko deadlockow i zapewniajac zerowy narzut synchronizacji.

### B. Wyspecjalizowane Routery Domenowe
Kazdy router implementuje interfejs bazowy `UserInterface::Contracts::IPacketRouter` i odpowiada za waska, scisle zdefiniowana domene rozgrywki:

1. **`NetCombatRouter` (`NetCombatRouter.h/.cpp`):**
   - Obsluguje pakiety walki: `GC_ATTACK` (0x0C/0x0401), `GC_DAMAGE_INFO` (0x10/0x0410), `GC_CREATE_FLY` (0x13), `GC_FLY_TARGETING` (0x2C), `GC_DUEL_START` (0x1D).
   - Publikuje zdarzenia domenowe: `CombatAttackDomainEvent`, `CombatDamageInfoDomainEvent`, `CombatFlyDomainEvent`, `CombatFlyTargetingDomainEvent`, `CombatDuelStartDomainEvent`.
   - Zapewnia powiadomienie odbiornikow `IGameEventSink` oraz szyny `EventBus`.

2. **`NetItemRouter` (`NetItemRouter.h/.cpp`):**
   - Obsluguje pakiety przedmiotow i paska szybkiego wyboru: `GC_ITEM_SET` (0x12/0x0511), `GC_ITEM_DEL` (0x14), `GC_ITEM_GROUND_ADD` (0x13), `GC_ITEM_GROUND_DEL` (0x14), `GC_QUICKSLOT_ADD` (0x15), `GC_QUICKSLOT_DEL` (0x16), `GC_QUICKSLOT_SWAP` (0x17).
   - Przekazuje zaktualizowane pozycje ekwipunku i slotow do domenowego menedzera inwentarza.

3. **`NetActorRouter` (`NetActorRouter.h/.cpp`):**
   - Obsluguje pakiety zarzadzania bytami w swiecie gry: `GC_CHARACTER_ADD` (0x01/0x0205), `GC_CHARACTER_ADDITIONAL_INFO` (0x65), `GC_CHARACTER_DELETE` (0x02), `GC_OBSERVER_MOVE` (0x22), `GC_SYNC_POSITION`.
   - Rejestruje pojawienie sie postaci, synchronizuje interpolowane wspolrzedne oraz mapuje zdarzenia smierci na `ActorDeadEvent` i przemieszczenia na `ActorMovedEvent`.

4. **`NetShopRouter` (`NetShopRouter.h/.cpp`):**
   - Obsluguje pakiety handlu z NPC: `GC_SHOP` (0x26/0x0810), `GC_SHOP_SIGN` (0x32/0x0811), `GC_SHOP_START`, `GC_SHOP_UPDATE_ITEM`, `GC_SHOP_UPDATE_PRICE`.
   - Emituje `ShopEvent` oraz `ShopSignEvent`.

5. **`NetExchangeRouter` (`NetExchangeRouter.h/.cpp`):**
   - Obsluguje bezpieczna wymiane miedzy graczami (trade p2p): `GC_EXCHANGE` (0x19/0x051C) z podnaglowkami START, ITEM_ADD, ELK_ADD, ACCEPT, END.
   - Emituje `ExchangeEvent` powiadamiajacy subskrybentow o akcjach obu stron handlu.

6. **`NetQuestRouter` (`NetQuestRouter.h/.cpp`):**
   - Obsluguje zadania i dialogi: `GC_QUEST_INFO` (0x25/0x0912), `GC_QUEST_CONFIRM` (0x0911).
   - Emituje `QuestInfoEvent` oraz `QuestConfirmEvent`.

7. **`NetPartyRouter` (`NetPartyRouter.h/.cpp`):**
   - Obsluguje grupy i druzyny: `GC_PARTY_INVITE` (0x2E/0x0710), `GC_PARTY_ADD` (0x2F/0x0711), `GC_PARTY_UPDATE` (0x30/0x0712), `GC_PARTY_REMOVE` (0x31/0x0713), `GC_PARTY_PARAMETER`.
   - Emituje zdarzenia `PartyAddEvent`, `PartyUpdateEvent`, `PartyRemoveEvent`.

8. **`NetGuildRouter` (`NetGuildRouter.h/.cpp`):**
   - Obsluguje struktury i wojny gildyjne: `GC_GUILD` (0x33/0x0730), `GC_GUILD_WAR`, `GC_GUILD_WAR_POINT`.
   - Emituje `GuildEvent` zawierajacy podnaglowki informacyjne i stan wojen.

### C. Rejestracja i Cykl Zycia Routerow
Inicjalizacja routerow nastepuje poprzez metode `PhaseGamePacketDispatcher::RegisterDefaultRouters(pEventSink)`. System umozliwia takze podmienianie poszczegolnych routerow w locie (`SetCombatRouter`, `SetItemRouter` itd.) na potrzeby testow jednostkowych w pamieci RAM (mock sinks).

---

## 4. Wzorzec Strangler Fig i Przejscie do Standardu C++23

Wielkie repozytoria kodu legacy nie moga byc przepisywane metoda "Wielkiego Wybuchu" (Big Bang Rewrite). W projekcie Client2026 zastosowano sprawdzony wzorzec architektoniczny **Strangler Fig (Dusil Drzewo)**, umozliwiajacy bezpieczna, iteracyjna modernizacje silnika do standardu C++23 przy zachowaniu 100% stabilnosci.

```
┌──────────────────────────────────────────────────────────┐
│ STARY KOD LEGACY (C++03 / Python UI)                     │
│ CPythonNetworkStreamPhaseGame / CPythonCharacterManager │
└────────────────────────────┬─────────────────────────────┘
                             │
            Przekierowanie przez cienkie fasady
                             ▼
┌──────────────────────────────────────────────────────────┐
│ FASADY STRANGLER FIG (src/Client/Bridge/)                │
│ • StranglerFacade (akcje uzytkownika, komendy domenowe)   │
│ • StranglerInstanceFacade (cykl zycia bytow, handles)    │
└────────────────────────────┬─────────────────────────────┘
                             │
             Delegacja do czystych domen C++23
                             ▼
┌──────────────────────────────────────────────────────────┐
│ NOWOCZESNY RDZEN C++23 (Domeny Stanowe)                   │
│ • WorldContext, GameSession, ActorRegistry, SpatialGrid   │
│ • StrongTypes, GenerationalRegistry, Zero-Copy Buffers   │
└──────────────────────────────────────────────────────────┘
```

### A. Zasada Dzialania Strangler Fig w Kliencie
1. **Stopniowe oplecenie fasada:** Wokol starych struktur tworzone sa nowoczesne fasady mostkujace (`StranglerFacade`, `StranglerInstanceFacade`).
2. **Przekierowanie przeplywu:** Stare metody w `UserInterface` przestaja bezposrednio modyfikowac wewnetrzne tablice wskaznikow, a zamiast tego deleguja operacje do fasady.
3. **Przeniesienie stanu gry do C++23:** Nowoczesne domeny (`WorldContext`, `ActorRegistry`, `SpatialHashGrid`) staja sie jedynym zrodlem prawdy (Single Source of Truth).
4. **Odciecie i usuniecie (Strangling):** Gdy wszystkie wywolania danej domeny przejda przez fasade, stary monolit (np. `PythonNetworkStreamPhaseGame.cpp`) zostaje bezpowrotnie usuniety z kompilacji.

### B. Fasady Mostkujace w Systemie
1. **`StranglerFacade` (`src/Client/Bridge/StranglerFacade.h`):**
   - Mostkuje akcje gracza i pakiety wychodzace: `ExecuteAttack`, `ExecuteMove`, `ExecuteUseSkill`, `ExecuteUseItem`, `ExecuteDropItem`, `ExecutePickupItem`, `ExecuteChat`, `ExecuteWhisper`.
   - Przekazuje parametry do bezpiecznej sesji `GameSession` i wysyla zserializowane dane przez `NetworkStreamPort` lub mocki testowe (`MockNetworkPortAdvanced`).
2. **`StranglerInstanceFacade` (`src/Client/Bridge/StranglerInstanceFacade.h`):**
   - Mostkuje zarzadzanie instancjami bytow: rejestracja (`RegisterInstance`), wyrejestrowanie (`UnregisterInstance`), aktualizacja pozycji (`UpdatePosition`), status zycia (`SetDead`), gracz glowny (`SetMainInstance`).
   - **Ochrona przed problemem ABA i wiszacymi wskaznikami:** Wykorzystuje silny identyfikator generacyjny `GenerationalRegistry<uint32_t>` oraz uchwyty `EntityHandle`. Gdy byt znika ze swiata gry, jego generacja ulega zwiekszeniu, co natychmiast uniewaznia wszystkie stare referencje w pamieci.
   - **Funkcyjna iteracja C++23:** Udostepnia `for_each_actor()` eliminujace koniecznosc kopiowania list postaci i bezpieczne pod katem wspolbieznosci (`std::shared_mutex`).

---

## 5. Universal Server Profiles Architecture (UPM - Universal Protocol Matrix)

Nowoczesny silnik Client2026 jest w pelni agnostyczny wzgledem konkretnego serwera. Architektura **Universal Protocol Matrix (UPM)** umozliwia uruchomienie klienta na dowolnym serwerze Metin2 (Standard x64, Beavium, PandoraMT2, Azyrah, Classic Ymir 2004) bez koniecznosci ingerencji w kod rdzenia i bez rekompilacji.

Architektura opiera sie na wstrzykiwaniu trzech wymiennych interfejsow:

```
┌────────────────────────────────────────────────────────────────────────┐
│                        SERVER PROFILE INJECTION                        │
│                                                                        │
│   ┌─────────────────────┐ ┌───────────────────┐ ┌──────────────────┐  │
│   │  IProtocolProfile   │ │   ICombatPolicy   │ │ ITransportDriver │  │
│   │ (ramkowanie, kodeki,│ │ (CRC, interwaly,  │ │ (krypto TEA/AES/ │  │
│   │  opkody CG/GC)      │ │  predkosc ataku)  │ │  Sodium, IPC)    │  │
│   └──────────┬──────────┘ └─────────┬─────────┘ └────────┬─────────┘  │
└──────────────┼──────────────────────┼────────────────────┼─────────────┘
               ▼                      ▼                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                     SILNIK RDZENIOWY (Client2026)                      │
│     WorldContext • GameSession • PhaseGamePacketDispatcher • EventBus   │
└────────────────────────────────────────────────────────────────────────┘
```

### A. Filar 1: Sterownik Protokolu (IProtocolProfile / IServerProtocolDriver)
Interfejs `IServerProtocolDriver` (`src/Client/Network/Protocol/IServerProtocolDriver.h`) hermetyzuje wszystkie roznice binarne w przesyle danych:
- **`GetDriverName()`**: Identyfikator profilu (np. `"standard_x64"`, `"beavium"`, `"pandora"`).
- **`InspectFrame(std::span<const uint8_t>)`**: Wykrywa dlugosc ramki i opkod w buforze TCP. Obsluguje zarowno 1-bajtowy framing Ymir, jak i 4-bajtowy naglowek x64 (`FrameHeaderInfo`).
- **`EncodeAttack(const Domain::AttackCommand&)`**: Serializuje czysta domenowa komende ataku do formatu binarnego serwera docelowego (np. 11 bajtow w x64, 8 bajtow w Beavium).
- **`EncodeMove(const Domain::MoveCommand&)`**: Serializuje komende ruchu, konwertujac wspolrzedne i rotacje na wymagany format (np. stopnie/5, mikrostopnie 1e6 czy kompresje kwantowa).
- **`DispatchInbound(uint16_t unifiedOpcode, std::span<const uint8_t> payload, IGameEventSink* pSink)`**: Rozpoznaje zdeserializowany pakiet i publikuje odpowiednie zdarzenie w sinku kontraktowym.
- **Rejestracja i przelaczanie w locie:** `ProtocolDriverRegistry::Instance()` zarzadza zarejestrowanymi sterownikami i pozwala na dynamiczna zmiane aktywnego profilu w runtime.

### B. Filar 2: Polityka Domenowa Walki i Gry (ICombatPolicy)
Polityka walki definiuje reguly biznesowe i mechanizmy bezpieczenstwa charakterystyczne dla danego serwera:
- **`CombatSequenceGuard`**: Zapewnia poprawne obliczanie sumy kontrolnej CRC ataku oraz sekwencji `dwSeq`, zapobiegajac odrzucaniu ciosow przez serwer.
- **`MovementRateLimiter` & `AttackSpeedEmulator`**: Odpowiada za zachowanie scislych limitow czasowych wysylania pakietow ruchu (np. 250 ms) oraz synchronizacje uderzen z animacjami broni.
- **Walidacja trajektorii pociskow**: Oblicza wektory celowania (`FlyTargeting`) i zasiegi aggro zgodnie ze specyfika serwera.

### C. Filar 3: Sterownik Transportu i Kryptografii (ITransportDriver)
Odpowiada za bezpieczny przesyl danych przez siec lub lokalne procesy:
- **`ICryptoProvider` / `PluggableCryptoProvider`**: Wymienny modul szyfrowania strumienia obslugujacy:
  - `Plain`: Brak szyfrowania (dla testow lokalnych).
  - `TeaCryptoProvider`: Standardowy algorytm TEA/XTEA z blokami 16-bajtowymi (klasyczny Metin2).
  - `AesCryptoProvider`: Szyfrowanie blokowe AES-128 / AES-256 (np. PandoraMT2).
  - `SodiumCryptoProvider`: Nowoczesny handshake X25519 oraz szyfrowanie ChaCha20-Poly1305.
- **Buforowanie zero-copy:** `PacketRingBuffer` i `ZeroCopyRingBuffer` zapewniaja bezpieczne zarzadzanie strumieniem TCP bez nadmiarowych alokacji i zbednego `memcpy`.
- **Komunikacja miedzyprocesowa:** `IPCTransportPipe` umozliwia sterowanie klientem w trybie headless bezposrednio przez rury systemowe.

### D. Deklaratywne Profile Serwerowe (JSON)
Dane konfiguracyjne profilu ladowane sa z plikow JSON w katalogu `servers/` (np. `servers/m2dev.json`, `servers/pandora.json`, `servers/classic_ymir.json`) za posrednictwem `ServerProfileManager` i `ServerProfileRegistry`. Profil deklaruje m.in. adresy IP i porty kanalow, framing, typ kryptografii oraz interwaly akcji.

---

## 6. Standardy Inzynierskie i Zasady C++23 dla Agentow AI

1. **Czyste Nazewnictwo (Clean Naming):**
   - Zakaz notacji wegierskiej (`m_kVct_pkInstAlive` -> `m_aliveInstances`, `dwVID` -> `vid`).
2. **Silne Typowanie (Strong Types):**
   - Zakaz uzywania surowego `DWORD` dla identyfikatorow:
     - Identyfikator encji w swiecie -> `EntityVid`
     - Numer prototypu przedmiotu -> `ItemVnum`
     - Indeks slotu w ekwipunku -> `SlotIndex`
     - Identyfikator czaru -> `SkillIndex`
3. **Semantyka Wyniku (`Result<T, Error>`):**
   - Metody domenowe zwracaja `EterBase::PacketResult<T>` lub `std::optional<T>` z precyzyjnym enumem bledu zamiast -1 czy nullptr.
4. **Wstrzykiwanie Zaleznosci (Dependency Injection):**
   - Klasy przyjmuja referencje do potrzebnych domen przez konstruktor, umozliwiajac testy jednostkowe w pamieci RAM (in-memory) bez uruchamiania okna gry czy karty graficznej.
5. **Kompilacja i Testy:**
   - Kompilacja ze wsparciem cache: `sccache` zintegrowany w `CMakeLists.txt`.
   - Polecenie budowania: `cmake --build build --config Release --target Client2026 UserInterface --parallel`.
   - Weryfikacja: `ctest -C Release --output-on-failure` (100% PASS - 51/51 testow jednostkowych w czasie < 3s).

---

## 7. Podsystem Testowy IPC Named Pipe, Tryb Headless i MCP 2026

W celu umozliwienia w pelni autonomicznego sterowania silnikiem gry przez agentow AI (Antigravity, Jules Swarm, Cursor, Claude Code) bez narzutu graficznego GPU, zaimplementowano bezglowa warstwe testowo-sterujaca:

```
[ Agent AI / Narzedzie MCP (tools/mcp/) ]
               │ JSON-RPC 2.0 (stdio lub IPC Pipe)
               ▼
   [ TestHarnessServer ] (\\.\pipe\M2ClientEngineHarness)
               │ (Kolejkowanie zadan - brak Data Race)
               ▼
[ CPythonApplication / ActorRegistry / StranglerFacade ]
```

1. **Tryb Headless (`--headless`)**:
   - Wyciszenie okien modalnych Win32 (`MessageBoxW` zastepowane cichym logiem `TraceError`).
   - Ukrycie glownego okna procesu (`SW_HIDE`).
   - Pominiecie potoku renderingu klatek GPU (`canRender = false`) z dynamicznym dlawikiem CPU (15 ms sleep w petli glownej, redukcja obciazenia CPU do poziomu tla).
   - Bezpieczna atrapa urzadzenia audio (`SoundEngine` stub) odporna na brak DirectSound.
2. **Serwer Testowy IPC Named Pipe (`TestHarnessServer`)**:
   - Potok nazwany: `\\.\pipe\M2ClientEngineHarness`.
   - Protokol: JSON-RPC 2.0 z ramkowaniem 4-bajtowym naglowkiem dlugosci (`uint32_t LE`).
   - Architektura thread-safe: Metody dotykajace struktur gry sa kolejkowane do glownego watku (`ProcessMainThreadQueue()`), eliminujac zjawisko Data Race z `UpdateGame()`. Metody procesowe i potokowe wykonuja sie natychmiast (`isImmediate = true`).
   - Eliminacja Ghost Execution dzieki atomowej maszynie stanow zadan `HarnessTaskState` (`PENDING`, `EXECUTING`, `COMPLETED`, `TIMED_OUT`).
3. **Audyt Stanu Gry (`audit_dual_state`)**:
   - Weryfikacja synchronizacji koordynatow X/Y/Z, rotacji oraz HP gracza pomiedzy strukturami C++ a maszyna Pythona.
   - `world_audit`: Porownanie mapy `m_aliveActorsMap` z `ActorRegistry` gwarantujace brak desynchronizacji i eliminacje zjawiska duchow/zombie.
4. **Weryfikacja Integracyjna**:
   - `HeadlessClientLiveTest` oraz `McpHarnessSelfTest` (51/51 testow CTest PASS).
   - Skrypt `tools/mcp/test_auto.py` weryfikuje pelne wejscie do gry na zywo w polaczeniu z serwerem gry.
