---
task_id: "atlas_c01_02_phase_handshake"
cluster: "NET"
module_name: "Faza Handshake i Bezpieczenstwo Sesji"
target_files:
- src/UserInterface/PythonNetworkStreamPhaseHandshake.cpp
- src/Client/Network/HandshakeFSM.h
- src/Client/Network/HandshakeFlowAdapter.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c01_02_phase_handshake.md"
architecture_layer: "Silnik Sieciowy, Maszyna Faz i Protokol"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Modul Fazy Handshake zarzadza poczatkowym etapem polaczenia miedzy klientem a serwerem gry. Jego glownym celem jest ustanowienie bezpiecznej sesji, co obejmuje wymiane kluczy kryptograficznych dla szyfrowania pakietow oraz kluczowa synchronizacje czasu (Time Sync) niezbedna do poprawnego dzialania mechanik gry, fizyki i zabezpieczen Anti-Cheat (kompensacja lagow serwera, detekcja speedhacka).

W architekturze klienta, faza ta znajduje sie bezposrednio po ustanowieniu fizycznego polaczenia TCP (gniazdo sieciowe). Jest aktywowana w glownej petli sieciowej klienta (`OnUpdate` podsystemu sieciowego `CNetworkStream`). 

Przeplyw danych (Data Flow & Control Flow):
1. Serwer wysyla poczatkowy pakiet `Handshake` (tzw. `KeyChallenge` w starym nazewnictwie) do klienta z uzyciem struktury `PacketHandshake`.
2. Klient (`HandshakeFlowAdapter`) odbiera pakiet i czyta `server_time` i `delta`.
3. Nastepuje synchronizacja `ClientTime` oraz `ServerTime` (`ELTimer_SetServerMSec` w `CPythonNetworkStream`).
4. Klient odczytuje lokalny czas z kompensacja pingu (delta) i odsyla zmodyfikowany pakiet z powrotem.
5. Maszyna stanow (`HandshakeFSM`) przechodzi miedzy stanami: `Initial` -> `HandshakeReceived` -> `TimeSyncSent`.
6. Po pomyslnej wymianie i wlaczeniu szyfrowania przez `MultiCryptoManager`, FSM przechodzi w stan `Complete` a przelacznik faz `PhaseStateMachine` przechodzi do fazy `Login`.

Cykl zycia:
- Obiekty typu `HandshakeFSM` oraz `HandshakeFlowAdapter` sa zazwyczaj powiazane z czasem zycia fizycznej instancji polaczenia. Naleza do cyklu pojedynczej proby logowania.
- W przypadku rozlaczenia, modul jest destrukcyjnie czyszczony, a faza sieciowa przechodzi z powrotem na `Offline`.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
- Systemy sieciowe Windows / Sockets generujace pakiety TCP przychodzace z serwera.
- `CPythonNetworkStream` z warstwy `UserInterface`, glownie delegujaca odebrane naglowki kluczy kryptograficznych do klasy bazowej `CNetworkStream`. 
- Glowna petla aplikacji w Pythonie wywolujaca inicjalizacje i powiadomienia (np. okienko fazy za pomoca `PyCallClassMemberFunc`).

**Zaleznosci wyjsciowe (Outbound):**
- `PhaseStateMachine`: odpowiedzialna za globalne zarzadzanie stanow (Offline, Handshake, Login, Select, itd.).
- `Core::INetworkPort`: interfejs do fizycznej transmisji danych za pomoca buforow sieciowych.
- `MultiCryptoManager`: odpowiada za przelaczenie sesji w bezpieczny, zaszyfrowany tryb komunikacji ("Secure State").
- `EterBase::ModernLogger`: standardowe, nowoczesne raportowanie bledow (C++20 format).
- `CTimer` (singleton): zmiana bazowego czasu gry po odebraniu `server_time`.
- Okna UI (np. `PHASE_WINDOW_LOGIN` z powiadomieniem Python).

**Drzewo dyrektyw `#include` i Ryzyka:**
- `#include "PhaseStateMachine.h"`, `#include "../../EterBase/Result.h"`, `#include <cstdint>`, `#include <span>` w C++23.
- Obiekt `HandshakeFSM` wymaga wylacznie przedniej deklaracji w adapterze dla przyspieszenia kompilacji, ale sam kompozyt uwaznie importuje `Packet_Handshake.h`.
- Tradycyjnie w UI zachowano inkluzje `#include "StdAfx.h"`, co stanowi powiazanie ze starym modelem kompilacji naglowkow i narzuca ostroznosc w uzyciu dyrektyw preprocesora.

**Model pamieciowy:**
- Modul scisle uzywa surowych wskaznikow dla wstrzykiwania zaleznosci (`HandshakeFSM*`, `MultiCryptoManager*`, `Core::INetworkPort*`), bez nadawania praw wlasnosci. Zarzadzanie cyklem zycia odbywa sie poziom wyzej.
- Pakiety bajtowe (payloads) mapowane na struktury (`std::memcpy`, z obsluga bezpieczenstwa przez `std::span`).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur:**
1. `Client::Network::HandshakeFSM`
   - **Rola:** Zarzadzanie logika maszyny stanow fazy wymiany kluczy.
   - **Wlasciciel Watku:** Watek sieciowy. Brak blokad; nie powinno byc wywolywane wspolbieznie na jednej instancji.
2. `Client::Network::HandshakeFlowAdapter`
   - **Rola:** Laczenie podzespolow, obsluga odebranych pakietow i ich deserializacja. 
   - **Wlasciciel Watku:** Watek sieciowy.
3. `PacketHandshake` (zdefiniowana w `Packet_Handshake.h`)
   - **Rola:** Struktura C-struct o rozmiarze 13 bajtow dla wymiany czasow.
   - **Wielkosc:** 13 bajtow (bez wyrownania).

**Tabela Metod Publicznych:**
- `HandshakeFSM::TransitionToHandshakeReceived(uint32_t clientTime, uint32_t serverTime, int32_t delta)`
  - **Zwraca:** `EterBase::VoidResult<>`
  - **Pre-cond:** Stan to `Initial`.
  - **Side-effects:** Aktualizuje `m_serverTimeDelta`, przechodzi w `HandshakeReceived`.
- `HandshakeFSM::TransitionToComplete()`
  - **Zwraca:** `EterBase::VoidResult<>`
  - **Pre-cond:** Stan to `TimeSyncSent`.
  - **Side-effects:** Wymusza zmiane fazy globalnej na `Phase::Login`.
- `HandshakeFlowAdapter::HandleHandshakePacket(std::span<const uint8_t> payload, uint32_t currentClientTime)`
  - **Zwraca:** `EterBase::PacketResult<void>`
  - **Pre-cond:** Dostepnosc wszystkich zaleznosci z Ctor (FSM, Port, Crypto). Prawidlowy rozmiar strumienia w spanie.
  - **Side-effects:** Wykonuje pelen flow handshake (odbior, obliczanie czasu kompensacji, wlaczenie crypto, wysylka odpowiedzi).

**Pamieciowy Layout Struktur:**
- `PacketHandshake` (`#pragma pack(push, 1)`):
  - `[0x00]` `uint8_t header` (Naglowek)
  - `[0x01]` `uint32_t handshake` (Znak kontrolny/Sequence)
  - `[0x05]` `uint32_t time` (Czas Serwera)
  - `[0x09]` `int32_t delta` (Kompensacja lagow)
- *Total: 13 bajtow.* Idealne miejsce na wstrzykiwanie w Memory Hooking ze wzgledu na brak paddingu kompilatora MSVC.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

**Pakiety Sieciowe:**
- `PacketHandshake` pelni funkcje ujednoliconego protokolu autoryzacji C<->S dla najnizszej warstwy sieciowej, operujac jako pierwsza fizyczna ramka po polaczeniu z portem TCP Auth.
- W starszym kodzie pakiet wystepowal jako komenda `GCKeyChallenge` z podobnym schematem wymiany parametrow, uzywany do synchronizacji `m_kServerTimeSync.m_dwChangeServerTime`.

**Metody Pythona (`PyMethodDef` / Mostek C-API UI):**
W pliku C++ (`src/UserInterface/PythonNetworkStreamPhaseHandShake.cpp`) faza nie definiuje dodatkowych komend skryptowych Python bezposrednio, jednakze komunikuje sie z UI w Pythonie po przejsciu stanow C++ uzywajac istniejacych polaczen:
- `CPythonNetworkStream::SetHandShakePhase()` informuje srodowisko, wywolujac na glownym oknie ekranu logowania wywolanie Pythona: `PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOGIN], "OnHandShake", Py_BuildValue("()"))`.
- `CPythonNetworkStream::HandShakePhase()` dziala w petli C++, rutynowo odprawiajac pakiety `DispatchPacket`.
- `CPythonNetworkStream::__LeaveHandshakePhase()` wiaze w C++ event ulatwiajacy opuszczenie fazy.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

**Zasady wielowatkowosci:**
- Architektura modulu zalezy od wylacznego dostepu (Exclusive Access) do danych podzespolow. FSM ani FlowAdapter nie posiadaja blokad (`std::mutex`). Dlatego caly proces handshake MUSI byc przetwarzany na pojedynczym dedykowanym watku sieciowym, lub zsynchronizowany mechanizmem kolejkowania przed wyslaniem eventu.

**Potencjalne punkty awarii (Crash Points & Edge Cases):**
- **Nullowe wskazniki (Dangling Pointers):** `HandshakeFlowAdapter` zachowuje wylacznie czyste wskazniki (`HandshakeFSM*`, `MultiCryptoManager*`, `Core::INetworkPort*`). Brak instancji `PhaseStateMachine` na wejsciu konstruktora zrzuci blad logiki lub uszkodzi dzialanie. Oczekiwane sa instancje, ktore przezyja ten obiekt.
- **Wyrownanie Pakietow (Struct Alignment):** Struktury z C++ takie jak `PacketHandshake` zostaly ciasno spakowane `#pragma pack(push, 1)`. Zabronione jest zmienianie tego lub proba serializacji do innych platform ze zlym narzutem wyrownania (Alignment Faults, np. na starych ARM).
- **Time Wrap-Around (Przepelnienie Zegara):** Zmienne uzyte do kompensacji `uint32_t` maja cykl rzedu 49 dni. Moze to wplynac na logike obliczania czasu przy dlugotrwalym uptime polaczenia deweloperskiego.

**Zarzadzanie zasobami (RAII):**
- Zastosowanie `std::span` z C++20 w adapterze drastycznie minimalizuje szanse na wycieki i gwarantuje bezpieczenstwo granic buforow podczas odbierania surowych ramek (Zero heap allocations w parserze Handshake).

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
Jesli potrzebujesz rozszerzyc proces autoryzacji o nowe zasady (np. hardware fingerprint):
1. Nie modyfikuj legacy logiki w `UserInterface`. Nowa zasada powinna wejsc w sklad sieciowej maszyny stanow.
2. Wejdz do `HandshakeFSM.h` i rozwaz dodanie kolejnego stanu do enuma `HandshakeState` (np. `AwaitingHwidChallenge`).
3. Rozbuduj funkcje adaptera `HandshakeFlowAdapter::HandleHandshakePacket`, aby dekodowala Twoja wlasna rozszerzona strukture pakietu (dodana wczesniej w `Packet_Handshake.h`), dbajac o precyzyjny `std::memcpy`.
4. Stworz przejscie stanu w `HandshakeFSM` by powiadomic `PhaseStateMachine` o koniecznosci uzycia nowych parametrow sieciowych.

**Jak debugowac i logowac:**
- Punkty wejscia uzywaja `EterBase::ModernLogger` - szukaj bledow z przedrostkiem "HandshakeFlowAdapter: " w logach sieciowych.
- Postaw Breakpoint na linijce `TransitionToTimeSyncSent` aby zauwazyc moment kiedy klient wygenerowal czas serwera pomniejszony o lagi (zmienna lokalna `compensatedServerTime`). 

**Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
- Modul ten jest odseparowany zgodnie z `Zero-conflict rule`. W testach stworz Dummy obiekty (`MockNetworkPortAdvanced`) dziedziczace po `INetworkPort` oraz `MultiCryptoManager`. 
- Utworz reczny pakiet jako `std::vector<uint8_t>`, zamapuj go na `std::span` i wstrzyknij bezposrednio w `HandleHandshakePacket`, nastepnie asertuj za pomoca Doctest wyjscie `HandshakeFSM::GetServerTimeDelta()`. 
- Upewnij sie ze testy dzialaja na Linux CI z zastosowaniem C++23. Wszelkie mockowania nalezy zachowac lokalnie.
