---
task_id: "atlas_c01_01_netstream_core"
cluster: "NET"
module_name: "CPythonNetworkStream - Rdzen Petli Sieciowej i Socket I/O"
target_files:
- src/UserInterface/PythonNetworkStream.cpp
- src/UserInterface/PythonNetworkStream.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c01_01_netstream_core.md"
architecture_layer: "Silnik Sieciowy, Maszyna Faz i Protokol"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

**Funkcja Modulu:**
`CPythonNetworkStream` to glowny hub sieciowy i maszyna stanow (Phase Machine) w architekturze klienta Metin2. Odpowiada za utrzymanie polaczenia z serwerami (Logowanie, Gra), odbieranie pakietow GC (Game-to-Client), walidacje ramkowania pakietow (framing), serializacje pakietow CG (Client-to-Game) oraz delegacje i wysylke eventow do warstwy UI i logiki gry. W najnowszej iteracji implementuje takze Fasade Dusiciela (StranglerFacade), ktora przelacza stary system na zmodernizowane rutery C++23.

**Cykl Wywolywania:**
Klasa dziedziczy z `CNetworkStream`. W zaleznosci od aktywnej fazy (Offline, Handshake, Login, Select, Loading, Game), kod modulu wykonuje sie glownie w glownej petli gry (`CPythonApplication`). Funkcja `OnProcess()` wywoluje aktualizacje stanow poszczegolnych faz poprzez funkcyjne delegaty w `m_phaseProcessFunc`.

**Przeplyw Danych (Control & Data Flow):**
1. Odbior danych z warstwy Winsock przez `CNetworkStream` do wewnetrznego bufora (64KB).
2. `DispatchPacket` przetwarza naglowki (Header: 2 bytes) i wielkosci z ramki `TDynamicSizePacketHeader`.
3. Pakiety delegowane sa albo do starych handlerow przez `PacketHandlerMap` (np. `m_gameHandlers`), albo do nowoczesnego rutera `Network::Dispatchers::NetworkStreamPhaseGameBridge::RouteGamePacket()`.
4. Handlery odczytuja pakiety przy pomocy `Recv()` lub mapuja je bezposrednio do nowej struktury `std::span` i informuja mostki Pythona o zmianach.

**Cykl Zycia Obiektow:**
Alokacja klasy nastepuje jako Singleton. W konstruktorze bufory ustawiane sa na 65536 bajtow, mapy handlerow sa rejestrowane z podzialem na fazy, a takze nastapuje inicjalizacja `StranglerFacade`. Wraz ze zmiana fazy, np. `SetGamePhase()`, czyszczone sa bufory tymczasowe a zmienne stanu sa przelaczane.

---

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci Wejsciowe (Inbound):**
- Glowna petla aplikacji klienta (`CPythonApplication`) wola metode `Process()`.
- Rownolegle interfejs Pythona (PythonNetworkStream module) oraz skrypty z przestrzeni `ui*.py` komunikuja sie uzywajac eksponowanych komend.
- Dane przychodzace (pakiety i heartbeat) od `Server`a via standard TCP sockets (Winsock).

**Zaleznosci Wyjsciowe (Outbound):**
- Siec Bazowa: `CNetworkStream` z EterLib.
- System Mostow (Bridges): Klasy `PhaseGame*Bridge` (np. `PhaseGameGuildBridge`, `PhaseGameSyncBridge`, `NetworkStreamPhaseGameBridge`) – implementujace dekompozycje logiki.
- Rejestr Aktorow i Obiektow: `CNetworkActorManager`, delegaty do `CInstanceBase`, `CPythonPlayer`, `CPythonCharacterManager`.
- System UI i Moduly C++23: `Network::PacketDispatcher`, `Client::Bridge::StranglerFacade`.

**Drzewo dyrektyw `#include` i Ryzyka:**
- Standardowe C++: `<unordered_map>`, `<vector>`, `<span>`.
- EterLib: `"EterLib/FuncObject.h"`, `"EterLib/NetStream.h"`.
- Moduly Zalezne: `"Packet.h"`, `"PythonNetworkStreamPhaseGame*.h"` - silna zaleznosc od starych definicji struktur pakietow i ich rozbijania na sub-moduly. Ryzyko dotyczy nadmiarowych inkluzji naglowkow pythonowych w `PythonNetworkStreamPhaseGame.cpp` - moga prowadzic do konfliktow i zapetlen zaleznosci, jezeli C++23 refaktor bedzie postepowal w mostkach.

**Model Pamieciowy:**
Mieszany. Rdzen klasy i bufory sa czescia Singletonu - nie sa uwalniane do zamkniecia.
Modul wykorzystuje czyste wskazniki `CInstanceBase *` do pobierania celow, `CRef<CNetworkActorManager>` dla powiazan instancji na mapie oraz stosuje nowe narzedzia C++23 (`std::vector`, `std::span`) do przekazywania buforow pakietow.

---

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur:**
- `CPythonNetworkStream` (Klasa): Glowny silnik sieciowy. Wlasciciel: Watek glowny. Dziedziczy po `CNetworkStream` (I/O sieciowe) i `CSingleton`.
- `PacketHandlerEntry` (Struktura): Slot w mapie dla opcodow, zawiera handler funkcji (`handler`), wariant ramkowania (`minSize`), i bool o przerwaniu przetwarzania w fazie (`exitPhase`).
- `SMarkAuth` (Struktura): Pamiec dla autoryzacji serwera emblematow (Mark/Symbol).
- `PacketLogEntry` (Struktura): Wpis na bufor kolowy logowania przychodzacych pakietow. Zawiera `seq`, `header`, `length`. Rozmiar: 8 bajtow.

**Tabela Metod Publicznych:**
- `bool DispatchPacket(const PacketHandlerMap& handlers)`: Serce rutera sieciowego. Wola wlasciwe funkcje. Zwraca true jesli pomyslnie sciagnieto dane. Side-effect: modyfikuje wskaznik odczytu danych bufora pakietow.
- `void SetHandler(PyObject* poHandler)`: Wstrzykuje instancje skryptowa obslugujaca zdarzenia i alarmy.
- `bool RecvPhasePacket()`: Centralny dekoder fazy z serwera, przelaczajacy np. pomiedzy Loading a Game.
- `void Register*Handlers()`: Funkcje ladujace mape opcodow dla odpowiednich stanow w tablice `m_gameHandlers`, `m_loginHandlers`, etc.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
`CPythonNetworkStream`:
- Wskazniki i Kolekcje (dynamiczna rzesza): `m_offlineHandlers` ... `m_gameHandlers` na koncu instancji.
- Tablice pre-alokowane (statyczna rzesza): `m_aRecentRecvPackets[32]` offsetowana blizej tylu, zarzadzana przez `m_dwRecvPacketSeq`.
- Parametry ID/Pozycji gracza (`m_dwMainActorVID`, `m_dwMainActorRace`) znajduja sie obok stalych klasowych stanow fazowych (`m_dwChangingPhaseTime`, `m_strPhase`).

---

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

**Pakiety Sieciowe:**
- Handshake / Core: `GC::PHASE`, `GC::PING`, `GC::KEY_CHALLENGE`, `GC::KEY_COMPLETE`.
- Select Phase: `GC::EMPIRE`, `GC::PLAYER_CREATE_SUCCESS`, `GC::PLAYER_DELETE_SUCCESS`.
- Game Phase (GC): `GC::CHARACTER_ADD`, `GC::CHARACTER_UPDATE`, `GC::ITEM_SET`, `GC::SHOP`, `GC::EXCHANGE`, `GC::TARGET`, `GC::SKILL_LEVEL`.
- Format Ramki: [2 bajty opcodu] -> [2 bajty dlugosci (lub wiecej w zaleznosci od opcodu), `TDynamicSizePacketHeader`] -> [dane].

**Metody Pythona (`PyMethodDef`):**
Obiekty takie jak `PyCallClassMemberFunc(m_poHandler, "SetGamePhase", ...)` wystepuja jako alarmy do Pythona. Python jest wywolywany zwrotnie w momencie zakonczenia konkretnego handlera:
- `"RefreshAlignment"`, `"RefreshTargetBoard"`, `"SetGamePhase"`, `"SetLoginPhase"`.

Zamiast zalezec bezposrednio od Pythona, system zostal zrefaktoryzowany na serie uzytkowych fasad np. `PhaseGameShopBridge`, `PhaseGameSkillsBridge`, izolujac brudny kod UI.

---

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

**Zasady Wielowatkowosci:**
Pomimo ze to modul sieciowy, jest podpiety w 100% pod Glowny Watek Gry (Update Tick). Gniazda (Winsock) rezyduja w implementacji asynchronicznej/nieblokujacej pollowanej w trybie select/I/O (co robi glowna klasa bazowa), zatem tutaj w `CPythonNetworkStream` brak bezposrednich lockow (`std::mutex`). Cale zarzadzanie pamiecia UI dzieje sie bezkolizyjnie z D3D.

**Potencjalne Punkty Awarii (Crash Points):**
- Desynchronizacja Ramkowania Pakietow (Framing): Jezeli opcod podaje zla wielkosc, handler przerwie odczyt. Zmienna `packetFrame.length` jest walidowana miedzy `PACKET_HEADER_SIZE` a 65000 bajtow. Niespelnienie testu resetuje klienta `PostQuitMessage(0)`.
- Zaleznosci Faz: Przetwarzanie pakietu Game bedac w fazie Loading zniszczy maszyne stanow, poniewaz mapy (`PacketHandlerMap`) nie dziela wszystkich funkcji.

**Zarzadzanie Zasobami (RAII):**
W celu unikniecia wyciekow z powtarzajacych sie reallocow, wiekszosc pakietow jest parsowana przez statyczne offsetowanie `Peek` zamiast `malloc`. Jednakze tablice handlerow (HashMaps) nie sa czyszczone miedzy mapami by oszczedzic dealokacji stertowych. Nowe integracje uzywaja `std::vector` jako smart buffer.

---

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Instrukcja Dodawania Nowej Funkcji:**
1. Nowy naglowek serwerowy i wielkosc okresl w `Packet.h` (np. `GC::PET_UPDATE`, rozmiar X).
2. Dodaj wpis do handlera uzywajac wlasciwej metody fazowej w konstruktorze lub odpowiedniej metodzie `Register*Handlers()` (np. `RegisterGameHandlers()`). Wzor: `h[GC::PET_UPDATE] = { &CPythonNetworkStream::RecvPetUpdatePacket, sizeof(TPacketGCPet), false };`
3. Napisz nowa metode `bool CPythonNetworkStream::RecvPetUpdatePacket()` – obsluge przeprowadz odczytujac pakiet (np. `Recv()`).
4. NIE lacz sie bezposrednio z `PythonPlayer`! Uzyj klas typu `PhaseGame*Bridge`, zeby oddelegowac dane na interfejs UI zachowujac zerowa powierzenie dekompozycji C++23.

**Jak Debugowac i Logowac:**
Uzywaj wbudowanych makr klasy, szczegolnie funkcji z debug trackera powiazanych ze strumieniami: `LogRecvPacket(header, packetFrame.length)` oraz rzucania wyjatkow do pinu: `TraceError("Unknown packet header... recv_seq: %u", m_dwRecvPacketSeq)`. Podatne pakiety sa cachowane w oknie 32 wpisow (`m_aRecentRecvPackets`), ulatwiajac root cause.

**Jak Testowac (Headless / Unit Test Harness):**
Skorzystaj z systemow testowych C++23 dla mostow (e.g. `NetworkStreamPhaseGameBridge`).
- Uzywajac `run_isolated_test.sh` zbuduj symulator nadawcy `std::vector<uint8_t>` i wyslij do `DispatchPacket()`.
- Unikaj uruchamiania DirectX, mockuj wirtualne maszyny pakietow zamiast laczyc instancje sieciowe po `127.0.0.1`. Zamiast socketow odzywaj sie prosto w API `Peek` / `Recv`.
