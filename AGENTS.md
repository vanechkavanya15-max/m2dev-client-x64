# INSTRUKCJE DLA AGENTOW AI (JULES SWARM)

## 1. ZASADY GLOWNE I PRIORYTETY
1. **GRAFIKI NIE RUSZAMY NA RAZIE:** Zgodnie z wytycznymi, podsystemy graficzne (DirectX, renderery, modele) pozostaja nienaruszone.
2. **AUDIOLIB WYTNIETY:** `AudioLib` zostal zastapiony lekka, bezkosztowa atrapa (stub). Zadne moduly dzwieku (miniaudio, Miles) nie sa kompilowane ani inicjalizowane.
3. **GLOWNY CEL NA ROK 2026:** Porzadek, modularyzacja, refaktoryzacja do standardu C++20/C++23 oraz stworzenie **maksymalnej czytelnosci dla agentow AI** w kodzie sieci, logiki i stanu gry.

---

## 2. KOMPENDIUM WIEDZY: KATALOG `docs/`
W katalogu `docs/` znajduje sie kompletna dekonstrukcja architektury klienta:
* `docs/CLIENT_SYSTEM_MAP.md` - centralna mapa systemu, cykl zycia, petla glowna oraz podsystemy domenowe 2026.
* `docs/AI_API_CHEATSHEET.md` - scisly kontrakt C-ABI (BotApi_FFI) oraz kompletny rejestr metod JSON-RPC 2.0.
* `tools/mcp/MCP_TOOL_SPEC.md` - specyfikacja narzedzi Agent MCP (@server.tool) i sterowania bezglowym klientem.
* `REPO_MAP.md` - deterministyczna mapa klas, struktur i 51 testow CTest (kompresja 95%).
* `docs/01_architecture/` - gleboka analiza warstwowa i integracja z serwerem.
* `docs/02_subsystems/` - dekonstrukcja bibliotek pomocniczych.
* `docs/03_network_protocol/`:
  * `01_network_stream_core_handshake.md` - handshake, time-sync, ping-pong.
  * `02_phases_login_select_loading.md` - przebieg faz logowania i wyboru postaci.
  * `03_phase_game_actors_items.md` - obsluga ruchu, encji i przedmiotow.
  * `04_packets_cg_client_to_server.md` - KOMPLETNY rejestr pakietow wychodzacych.
  * `05_packets_gc_server_to_client.md` - KOMPLETNY rejestr pakietow przychodzacych.
* `docs/04_python_bindings/` - powiazania C++ do modulow Pythona (`player`, `chrmgr`, `item`).

ZANIM zaczniesz zmieniac dany pakiet lub modul, przeczytaj odpowiedni plik w `docs/`!

---

## 3. KLUCZOWA PRAWDA ARCHITEKTONICZNA: STAN GRY ZYJE W C++
Python w kliencie Metin2 jest **wylacznie warstwa widoku/interfejsu (View)**. Caly krytyczny stan gry zyje w czystym kodzie C++:
* **`CPythonPlayer`** -> przechowuje tablice ekwipunku, aktualne HP/MP/EXP, sloty umiejetnosci i cele ataku.
* **`CPythonCharacterManager`** -> trzyma instancje wszystkich potworow, graczy i NPC w zasiegu widzenia (`CInstanceBase`).
* **`CPythonItem`** -> zarzadza przedmiotami lezacymi na ziemi.

Oznacza to, ze bot i agenci AI operuja bezposrednio na strukturach C++, bez koniecznosci polegania na skryptach graficznych UI.

---

## 4. STANDARDY MODERNIZACJI NA ROK 2026 (STANDARD C++23 & AI READABILITY)

Projekt dziala w pelnym standardzie **C++23** (MSVC 2022 x64, CMake). Wszyscy agenci Jules Swarm musza przestrzegac nastepujacych zasad:

### A. Eliminacja wskaznikow wyjsciowych (std::expected<T, E>)
Nigdy nie piszemy funkcji w starym stylu: `bool Parse(Data* out)`.
Wszystkie nowe metody i handlery zwracaja scisle `std::expected` lub `EterBase::PacketResult<T>`:
```cpp
// PRAWIDLOWO (Standard 2026 C++23):
EterBase::PacketResult<void> ProcessDamagePacket(std::span<const uint8_t> buffer);
```
Dla modelu LLM/AI typ zwracany niesie albo pelne dane, albo precyzyjny powod bledu (`PacketError::BufferUnderflow`, `InvalidHeader`). Zero domyslow i zero ukrytych efektow ubocznych!

### B. Likwidacja piramid "if" (Monadyczny std::optional)
Zamiast 5 zagniezdzen `if (pActor) { if (pItem) { if (pSkill) ... } }`, uzywamy czystych potokow:
```cpp
auto weaponVnum = EntityManager::Find(vid)
    .and_then(&Actor::GetEquipment)
    .transform(&Equipment::GetWeaponVnum)
    .value_or(ItemVnum(0));
```
Oszczedza to tokeny kontekstu i wyklucza bledy brzegowe przy analizie kodu przez agentow AI.

### C. Silne Typy Domenowe (StrongTypes.h)
Zakaz uzywania generycznego `uint32_t` na wszystko. Uzywaj typow z `src/EterBase/StrongTypes.h`:
* `EntityId` / `EntityVid` -> identyfikator aktora, moba, gracza (VID).
* `ItemVnum` -> numer typu przedmiotu (VNUM).
* `ItemSlot` -> indeks slotu w ekwipunku/depozycie.
* `SkillId` -> numer umiejetnosci.
Dzieki temu kompilator fizycznie uniemozliwia pomylenie VID celu z VNUM miecza.

### D. Bezpieczne Formatowanie Pamieci (std::format i ModernLogger)
Calkowity zakaz uzywania niebezpiecznych funkcji `sprintf`, `vsprintf` i buforow `char[1024]`.
Stosujemy `std::format` oraz `EterBase::ModernLogger::Info(...)`, `ModernLogger::Error(...)`.

### E. Wielowymiarowy operator dostepu operator[](x, y)
Dla siatek kolizji i map (`CollisionGrid`, `MapAttr`) uzywamy C++23: `grid[x, y]` zamiast starego `grid[y * width + x]`.

---

## 5. ZASADY PRACY W REPOZYTORIUM
* Ignorujemy `m2dev-client-main/`, `build/`, `bin/`.
* Kazdy agent tworzy lub modyfikuje swoj dedykowany modul bez kolizji z innymi agentami (zasada Zero-Conflict).
* Przed implementacja pakietu ZAWSZE sprawdz specyfikacje w `docs/03_network_protocol/04_packets_cg_client_to_server.md` oraz `05_packets_gc_server_to_client.md`.

---

## 6. STANDARD ROZSZERZEN I PROFILI SERWEROWYCH DLA AGENTOW AI

Nowa uniwersalna architektura silnika opiera sie na macierzy protokolow Universal Protocol Matrix (UPM) oraz podsystemie routerow domenowych. Wszyscy agenci realizujacy zadania zwiazane z profilami serwerow, transportem sieciowym lub nowymi pakietami musza bezwzglednie stosowac sie do ponizszych regul.

### A. Zelazne Reguly Architektoniczne dla Agentow AI

1. **Zakaz Monolitow (Zero Monolith Policy):**
   - Kategoryczny zakaz pisania wielkich klas i wielotysiecznych plikow skupiajacych cala logike sieciowa (takich jak stary 4 234-liniowy `PythonNetworkStreamPhaseGame.cpp`).
   - Obowiazuje zasada pojedynczej odpowiedzialnosci (Single Responsibility Principle - SRP):
     - **1 zadanie = 1 dedykowany pakiet/handler/sterownik = 1 para plikow (.h / .cpp) + 1 plik testu jednostkowego w `tests/test_c26_*.cpp`**.
   - Budzet kodu na modul wynosi zazwyczaj 100-300 linii, z nieprzekraczalnym limitem 500-1500 linii dla zlozonych monolitycznych struktur biznesowych.
   - Kazdy handler lub router operuje wylacznie na gotowych zwalidowanych payloadach POD (`std::span<const uint8_t>`) lub strukturach domenowych. ZERO odpytywania soketow przez `Recv()` w handlerach!

2. **Obowiazkowe Stosowanie REPO_MAP.md:**
   - ZABRANIA SIE zgadywania metod, sygnatur funkcji, klas i przestrzeni nazw z pamieci modelu AI.
   - ZANIM agent przystapi do pisania kodu, MUSI odpytac lub wygenerowac `REPO_MAP.md` (zwiezly deterministyczny wyciag sygnatur AST z calego repozytorium z narzedzia `jules_cli/repo_map.py`).
   - Uzywaj wylacznie istniejacych typow domenowych (`EntityVid`, `ItemVnum`, `PacketResult<T>`, `WorldContext`, `IGameEventSink`) bez wymyslania duplikatow. Oszczedza to tokeny i eliminuje halucynacje kontekstowe w rojach Swarm.

3. **Wymog 100% PASS na ctest (Zero-Regression Policy):**
   - Zaden kod nie moze zostac uznany za ukonczony (Definition of Done), dopoki caly zestaw testow jednostkowych nie zwroci statusu **100% PASS**:
     ```powershell
     ctest -C Release --output-on-failure
     ```
   - Kazda nowa klasa profilu, sterownika czy routera MUSI zawierac pelny suite testowy pokrywajacy:
     - Sciezke standardowa (happy path).
     - Testy blednych danych i ucietych buforow (payload size underflow).
     - Testy nieznanych naglowkow (odrzucanie bez awarii / zero-crash).
     - Weryfikacje rejestracji i przelaczania w locie (runtime switching).
   - W przypadku niepowodzenia chociazby 1 testu w ctest, kod podlega natychmiastowemu rollbackowi.

---

### B. Procedura Dodawania Nowego Profilu Serwerowego (np. Beavium, Azyrah, Standard)

W celu dodania obslugi nowego serwera docelowego agent AI musi zrealizowac nastepujaca 5-krokowa procedure:

#### Krok 1: Definicja Deklaratywnego Profilu JSON (`servers/<nazwa>.json`)
Stworz plik konfiguracyjny profilu w katalogu `servers/`, deklarujac parametry polaczenia, framing oraz flagi anty-cheat:
```json
{
  "name": "azyrah",
  "auth": {
    "ip": "127.0.0.1",
    "port": 11002
  },
  "channels": [
    { "ip": "127.0.0.1", "port": 13000 },
    { "ip": "127.0.0.1", "port": 13001 }
  ],
  "framing": "Ymir1B",
  "crypto": "TEA",
  "header_size_bytes": 1,
  "move_interval_ms": 250,
  "pickup_interval_ms": 100,
  "attack_crc_required": true
}
```
Upewnij sie, ze profil laduje sie poprawnie przez `Client::Network::ServerProfileManager`.

#### Krok 2: Implementacja Sterownika Protokolu (`IServerProtocolDriver`)
Stworz dedykowany sterownik w katalogu `src/Client/Network/Protocol/Drivers/`:
- `src/Client/Network/Protocol/Drivers/<Nazwa>ProtocolDriver.h`
- `src/Client/Network/Protocol/Drivers/<Nazwa>ProtocolDriver.cpp`

Zaimplementuj wszystkie metody czystego interfejsu `IServerProtocolDriver`:
1. `GetDriverName()`: zwraca unikalny identyfikator profilu (np. `"azyrah"`).
2. `InspectFrame(std::span<const uint8_t> buffer)`: bada poczatek bufora TCP, zwracajac zunifikowany opkod, calkowity rozmiar pakietu do pobrania ze strumienia oraz rozmiar naglowka (`FrameHeaderInfo`), badz `std::nullopt` w razie niekompletnych danych.
3. `EncodeAttack(const Domain::AttackCommand& cmd)`: koduje domenowa komende ataku do binarnego rekordu CG wymaganego przez serwer docelowy (np. `TPacketCGAttack` z sekwencja i CRC).
4. `EncodeMove(const Domain::MoveCommand& cmd)`: serializuje komende ruchu, przeliczajac koordynaty oraz rotacje (np. konwersja do stopni / 5, mikrostopni lub kwantyzacji).
5. `DispatchInbound(uint16_t unifiedOpcode, std::span<const uint8_t> payload, IGameEventSink* pSink)`: rozpakowuje specyficzny pakiet serwera i publikuje czyste zdarzenie domenowe (`OnAttackExecuted`, `OnActorDead`, `OnActorMoved`, `OnItemReceived` itd.) na interfejsie `IGameEventSink`.

#### Krok 3: Rejestracja w Rejestrze Sterownikow (`ProtocolDriverRegistry`)
W klasie `Network::Protocol::ProtocolDriverRegistry`:
- Zarejestruj nowa instancje sterownika w metodzie `InitializeDefaults()` lub dynamicznie:
  ```cpp
  auto driver = std::make_unique<Drivers::AzyrahProtocolDriver>();
  ProtocolDriverRegistry::Instance().RegisterDriver(std::move(driver));
  ```
- Umozliw przelaczenie aktywnego profilu w runtime:
  ```cpp
  ProtocolDriverRegistry::Instance().SetActiveDriver("azyrah");
  ```

#### Krok 4: Konfiguracja Routerow Fazy Gry (`PhaseGamePacketDispatcher`)
Jezeli serwer posiada unikalne pakiety domenowe, rozszerz odpowiedni router w `src/UserInterface/Network/Routers/`:
- `NetCombatRouter` dla niestandardowych pakietow walki lub obrazen.
- `NetItemRouter` dla specyficznych struktur inwentarza lub przedmiotow na ziemi.
- `NetActorRouter` dla alternatywnych pakietow spawnu postaci lub synchronizacji pozycji.
Upewnij sie, ze naglowki sa zarejestrowane w Jump Table `PhaseGamePacketDispatcher` dla czasu O(1).

#### Krok 5: Utworzenie Testow Jednostkowych i Weryfikacja ctest
Napisz kompletny plik testowy `tests/test_c26_<nazwa>_protocol.cpp`:
1. Test poprawnego i blednego rozpoznawania ramek w `InspectFrame`.
2. Test kodowania komend ataku i ruchu w `EncodeAttack` / `EncodeMove`.
3. Test dyspozycji przychodzacych pakietow na obiekcie `MockGameEventSink`.
4. Test rejestracji i przelaczania aktywnego sterownika w `ProtocolDriverRegistry`.
5. Zarejestruj test w glownym `CMakeLists.txt` i uruchom:
   ```powershell
   cmake --build build --config Release --target test_c26_<nazwa>_protocol
   ctest -C Release --output-on-failure
   ```
Wszystkie testy w projekcie musza bezwzglednie zakonczyc sie wynikiem **100% PASS**.

---

### C. Detekcja i Ramkowanie Pakietu TPacketGCMainCharacter (Standard x64)

W architekturze klienta x64 detekcja pakietu wejscia do gry `TPacketGCMainCharacter` (`0x0071` / `GC::MAIN_CHARACTER`) opiera sie scisle na mechanizmie ramkowania strumienia sieciowego, a nie na arbitralnym zgadywaniu:
1. **Ramkowanie w Sterownikach UPM (`InspectFrame`)**:
   - W profilu `standard_x64` obowiazuje 4-bajtowy staly naglowek `[Header:2][Length:2]`. Kazda ramka przychodzaca ze strumienia TCP posiada zdefiniowana w naglowku calkowita dlugosc pakietu (`packetLength`), co gwarantuje prawidlowy podzial strumienia TCP na granice komunikatow.
   - W profilach o naglowkach 1-bajtowych lub dynamicznych (np. `luria`, `beavium`, `martysama`), dlugosc ramki wyznaczana jest na podstawie statycznej tablicy GC lub pola dlugosci ramki dynamicznej.
2. **Obsluga Payloadu w Handlerze Domenowym (`LoadingPacketHandler`)**:
   - Po odebraniu pelnej ramki wyznaczonej przez sterownik protokolu, payload przekazywany jest jako bezpieczny wycinek pamieci `std::span<const uint8_t>`.
   - `LoadingPacketHandler::HandleMainCharacter` weryfikuje poprawnosc rozmiaru bufora (`payload.size() >= sizeof(TPacketGCMainCharacter)`), a nastepnie deserializuje dane gracza glownego do `WorldContext` (ustawiajac `localPlayerVid` oraz koordynaty `lX, lY, lZ`).
3. **Odpornosc na Desynchronizacje Strumienia TCP**:
   - Klient w fazie Loading konsumuje ze strumienia dokladnie tyle bajtow, ile wynosi zadeklarowany rozmiar ramki ze stalym lub dynamicznym naglowkiem, co calkowicie eliminuje powstawanie przesuniec bufora (TCP stream desync).

