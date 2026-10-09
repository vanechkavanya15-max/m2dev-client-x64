---
task_id: "atlas_c10_12_discord_rpc"
cluster: "SYS"
module_name: "Integracja Discord Rich Presence"
target_files:
- src/Discord/Discord.cpp
- src/Discord/Discord.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c10_12_discord_rpc.md"
architecture_layer: "VFS, Szyfrowanie, Audio, Proto i Platforma"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

**Cel biznesowy:**
Modul odpowiedzialny jest za integracje klienta gry z platforma Discord poprzez mechanizm Discord Rich Presence (RPC). Jego zadaniem jest asynchroniczne odczytywanie i wysylanie kluczowych informacji o stanie gry danego uzytkownika (wybrana klasa postaci, przynaleznosc do krolestwa, aktywna mapa, biezacy kanal oraz nazwa gildii) bezposrednio do zewnetrznej aplikacji Discord. Dzieki temu profil gracza na Discordzie na zywo odzwierciedla jego aktywnosc w grze.

**Miejsce i sposob wywolania:**
Logika wysylania statusu zostaje zainicjalizowana glownie na etapie startu gry (`Discord_Initialize`), co skutkuje powolaniem wewnetrznego watku odpowiedzialnego za komunikacje I/O. Sam update statusu nastepuje po stronie warstwy aplikacji - tam struktura stanu jest formatowana (przy uzyciu m.in. przestrzeni nazw `Discord`) i przekazywana do funkcji API RPC (`Discord_UpdatePresence`). Dodatkowo w glownej petli gry aplikacji cyklicznie wywolywana jest funkcja `Discord_RunCallbacks()`, majaca za zadanie procesowanie wezwan asynchronicznych (zdarzenia laczenia, bledy, party requesty).

**Przeplyw danych (Data & Control Flow):**
1. Informacje domenowe wyciagane sa ze struktur Singleton (PythonPlayer, PythonCharacterManager) poprzez warstwe pomocnicza.
2. Formowane sa kluczowe pola Rich Presence (np. `GetNameData` buduje lancuchy "Location: Yongan", "Name: Warrior-Guild: ABC").
3. Pola te trafiaja do C-struktury `DiscordRichPresence`.
4. Glowny watek wolajac `Discord_UpdatePresence()` bezpiecznie rygluje mutex `PresenceMutex`, nadpisujac zbuforowany, gotowy do wyslania stan.
5. Poboczny watek I/O wybudza sie, serializuje obecny stan do formatu JSON za pomoca wewnetrznego `JsonWriteRichPresenceObj` i zapisuje go w systemowym potoku IPC komunikujacym sie z aplikacja kliencka Discord (`RpcConnection::Write`).

**Cykl zycia obiektow:**
Polaczenie IPC (reprezentowane przez `RpcConnection`) alokowane jest za posrednictwem metody fabrykujacej przy starcie (`Discord_Initialize`) i egzystuje az do zakonczenia gry, gdzie niszczone jest funkcja zwalniajaca (`Discord_Shutdown`).
Pakiety asynchroniczne pomiedzy watkami podlegaja statycznej lub dedykowanej puli pamieci kolejkowej (klasa `MsgQueue`), ograniczajac ciagla dynamike alokacji/dealokacji w warstwie I/O.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

- **Zaleznosci wejsciowe (Inbound):**
  System opiera sie na ciaglym pollingu wartosci stanow z core`owych klas klienckich gry (Singletonow). Przede wszystkim:
  - `CPythonPlayer` (dane identyfikacyjne gracza)
  - `CPythonCharacterManager` (stan glownego obiektu aktora - klasa i id)
  - `CPythonBackground` (do odczytu globalnego warunku mapy)
  - `CPythonGuild` (odczyt powiazania gildyjnego po identyfikatorze)

- **Zaleznosci wyjsciowe (Outbound):**
  - Rdzenne API systemowe dla zarzadzania watkami oraz muteksami (standard C++ `std::thread`, `std::mutex`, biblioteka `std::atomic`).
  - Podsiec komunikacyjna, wykorzystujaca abstrakcje `BaseConnection` mapowana w systemach Windows na Named Pipes (lacze nazwane).
  - Mechanizm serializacji JSON do tlumaczenia struktur zdefiniowanych w jezyku C na surowe bajty dla IPC.

- **Drzewo dyrektyw `#include` i zagrozenia:**
  Szczegolnie naglowki domenowe gry, takie jak `"PythonCharacterManager.h"`, `"PythonBackground.h"`, itp., ktore tworza dosc twarda wiez ze stara, legacyjna i uwiklana architektura monolitu Pythona. Narusza to zalozenia czystego rozdzialu architektury.
  Dodatkowo zaleznosc od `StdAfx.h` sprawia, ze prekompilowane headery rzutuja na narzuty i utrudniona izolacje modulow C++ w testowaniu.

- **Model pamieciowy:**
  Silnie oparty na wskaznikach z obiektywnego punktu widzenia C (`const char*`) podczas formatowania zdan i stanow, podczas gdy same klasy watku, obiekty muteksowe uzywane w obsludze pamieci wspoldzielonej (`std::mutex`) stosowane sa w bezpiecznym standardzie RAII (`std::lock_guard`). Brak nowoczesnych, bezpieczniejszych opakowan `std::shared_ptr` w surowych zasobach struktury Rich Presence – alokacje wiadomosci obslugiwane przewaznie we wlasnorecznych buforach tablic char [MaxRpcFrameSize].

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabela Klas i Struktur

| Nazwa | Rola | Wielkosc (b) | Wlasciciel Watku |
|-------|------|--------------|------------------|
| `DiscordRichPresence` | Surowa struktura C przenoszaca formatki danych tekstowych i numerycznych, reprezentujaca pelny aktualny biezacy profil (stan, lokacje, party). | Zmienna, zdefiniowana wskaznikami `const char*` | Stos / Glowny |
| `DiscordUser` | Struktura reprezentujaca szczegoly polaczonego lub wywolujacego uzytkownika (identyfikator konta Discord, nazwa, discriminator i avatar). | Zmienna | Glowny / Callbacki |
| `DiscordEventHandlers` | C-style interfejs zestawu wskaznikow na funkcje reagujace na notyfikacje sieciowe z IPC. | Okolo 48 bytes (arch x64) | Watek Glowny |
| `RpcConnection::MessageFrame` | Naglowek protokolu Discord IPC powiazany ze statycznym blokiem na JSON Payload, do wielkosci 64KB (`MaxRpcFrameSize`). | ~65536 bytes | Watek IO |

### Tabela Metod Publicznych (Warstwa Logiki)

- **`Discord::GetNameData()`**
  - **Sygnatura:** `std::pair<std::string, std::string> GetNameData()`
  - **Efekty:** Zwraca pare stringow. Pierwszy to nazwa/mapa np. "Location: Yongan", drugi to polaczona sciezka imienia postaci i jej gildii np. "Name: Zdzich-Guild: Husaria". Metoda ta maskuje takze systemowe nazwy plikow map (`metin2_map_a1`) w ich ladniejsze formy zdefiniowane w plikowej, zhardcodowanej m_MapName mapie.
  - **Typy:** C++11, rzutowania `std::string`
- **`Discord::GetRaceData()` / `Discord::GetEmpireData()`**
  - **Efekty:** Zwracaja dynamiczne struktury par `std::pair<std::string, std::string>`, tlumaczac enumy na nazwy (np. enum 0-> Warrior) i zwracajac nazwe do identyfikatora ikony w profilu klienta Discorda np. (`race_0`, `empire_2`).
- **`extern "C" Discord_Initialize(...)`**
  - **Sygnatura:** `void Discord_Initialize(const char* appId, DiscordEventHandlers* handlers, int autoRegister, const char* steamId)`
  - **Efekty:** Konfiguruje srodowisko, inicjalizuje mutexy, zapisuje callbacki do bezpiecznych instancji zapasowych, tworzy rure komunikacyjna i wreszcie wybudza niezalezny watek I/O (`IoThread->Start()`). Posiada takze opcjonalna rejestracje wsparcia z klientem Steam.
- **`extern "C" Discord_UpdatePresence(...)`**
  - **Sygnatura:** `void Discord_UpdatePresence(const DiscordRichPresence* presence)`
  - **Efekty:** Glowny punkt aktualizacji interfejsu. Blokuje pamiec biezacej klatki na czas tlumaczenia zawartosci wskaznika `presence` na ustandaryzowany wezel JSON, flaga zglaszajac watkowi IO potrzebe zsynchronizowania tego bufora do gniazda IPC.

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
Architektura w oparciu o silne poleganie na C-struct dla `RpcConnection::MessageFrame`:
- `0x0000`: `Opcode opcode` (4 bajty wymuszajace uzycie typow `uint32_t`, definiujace `Handshake`, `Frame`, itd.)
- `0x0004`: `uint32_t length` (4 bajty wymiaru Payloadu)
- `0x0008`: `char message[]` (Payload zawierajacy zserializowany JSON)
Offset ten pozwala theoretycznie na hookowanie lub wstrzykiwanie wlasnych ramek IPC za pomoca technik zewnetrznego FFI (np. z poziomu zewnetrznego integracyjnego API pythona podpietego pod zewnetrzny program).

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

- **Protokol Polaczenia Sieciowego IPC (Inter-Process Communication):**
  Zamiast komunikacji TCP na pakiety z serwerem gry, uzywany jest asynchroniczny protokol binarny Named Pipe. Do wymiany zewnetrznej wykorzystane sa pakiety opcode'ow definiowane na warstwie biblioteki Discordowej w oparciu o struktury:
  - `0x0`: Handshake (Inicjacja i negocjacja sesji komunikacyjnej po uruchomieniu gry z klientem Discord)
  - `0x1`: Frame (Normalna ramka protokolu, wewnatrz JSON formatujacy `DiscordRichPresence` ze wszystkimi szczegolami lokacji, gildii itp.)
- **Mostki Pythonowe:**
  Brak jakiegokolwiek dedykowanego i oddzielnego C-API (`PyMethodDef`) udostepniajacego modyfikacje zewnetrznych klas z pythona (co oznacza, ze skrypty systemowe Pythona nie musza sie manualnie martwic o wywolywanie update'u Rich Presence). Skrypt pythona operuje niezaleznie we wlasnym swiecie, podczas gdy przestrzen C++ logiki klienckiej odczytuje wskazniki od wewnatrz gry.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Zasady wielowatkowosci (Multi-threading hazards):**
  Aplikacja uzywa wlasnego watku obslugujacego wejscie/wyjscie (IoThread). Kiedy tworzysz wezwanie do update'u upewnij sie, ze bufor z ktorego operujesz jest bezpieczny, a odwolania nie lamia izolacji. Callbacki wywolywane przez serwer IPC (np. error) MUSZA obowiazkowo zostac przetworzone przez `Discord_RunCallbacks` wywolane przez glowny watek (Render/Tick) gry. Nie mozna modyfikowac wewnetrznego HUD GUI poprzez callback pochodzacy prosto z IoThread.
- **Potencjalne Punkty Awarii (Crash Points & Edge Cases):**
  - Ryzyko **Use-After-Free**: Obiekt `DiscordRichPresence` polega na surowych wskaznikach znakowych (`const char*`). Jesli wskazniki te odwoluja sie do buforow na stosie uzytych gdzies podczas `OnUpdate()` pythona i zostana zdealokowane przed tym, az watek asynchroniczny IO serializuje i wypcha JSON do IPC, uzytkownik doswiadczy wylaczenia aplikacji klienta (Access Violation).
  - Skrajny limit pakietu w C to 64KB, w 99% nigdy nie przekraczane podczas JSON RPC, lecz ryzykowne, np. w sytuacji ekstremalnego przepelnienia tekstu (Buffer Overflow w payloadzie "details" lub "state").
- **Zarzadzanie Zasobami (RAII):**
  Pamiec IoThread alokowana jest za posrednictwem wskaznikow i musi byc jawnie zniszczona funkcja biblioteki (braki zaimplementowania logiki `Discord_Shutdown()` na koniec zywotnosci aplikacji poskutkuja narastajacymi wyciekami RAM przy ponownych wywolaniach/restartach wewnetrznych klatek).

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. *Odwzorowanie potrzeb:* Jesli wymog polega na dodaniu nowej funkcji (np. stan "Bycie w lochu"), odnajdz funkcje odpowiedzialna za aktualizacje obiektu `DiscordRichPresence` podczas zmiany biezacej instancji/mapy.
  2. *Definicja helpera:* W naglowku `Discord.h` zdefiniuj nowa metode (np. `GetDungeonData()`), ktora odczyta z core-modulow bezpiecznie flagi stanu lokacji.
  3. *Ustawienia Pol:* Dodaj uzyskane dane do dedykowanych pol struktury `DiscordRichPresence` (np. przypisanie do pola `.state` lub uzycie obslugiwanych partii grup: `.partyId` / `.partySize` bazujac na `CPythonGuild` / Party).
  4. *Serializacja & Test:* C stringi z twojego stringa skopiuj lub zmapuj do zyjacowych na dluzszy czas w scope bufforow (zauwaz na pointery z `c_str()`).
- **Jak debugowac i logowac:**
  Podepnij brejkpointy pod `Discord_UpdatePresence` w pliku implementacyjnym rpc. Jesli wartosci sie zawieszaja, skontroluj wywolanie logiki obslugujacej `SendQueue` lub `RpcConnection::Write`. Podgladaj bufor wynikowy `QueuedPresence.buffer` podczas blokady w srodku dzialania muteksa, zeby zweryfikowac ostateczny ksztalt wyeksportowanego profilu.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  Dla celow testow w ramach CI wyeliminuj problem watku asynchronicznego uruchamiajac kod przy definiowanu makra flagowego `DISCORD_DISABLE_IO_THREAD`. Zastap warstwy wywolywania do IPC (w `connection.h` / `connection_win.cpp`) specjalnymi wrapperami mockow z uzyciem np. biblioteki Google Mock w ramach C++23. W ten sposob mozesz bez problemu i narzutu asertywnie obserwowac obieg informacji, pomijajac warunek, by system uruchamiajacy musial trzymac aktywny proces klienta platformy Discord.
