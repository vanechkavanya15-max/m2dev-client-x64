---
task_id: "atlas_c03_09_exchange_flow"
cluster: "ITM"
module_name: "CPythonExchange - System Wymiany Miedzy Graczami"
target_files:
- src/UserInterface/PythonExchange.cpp
- src/UserInterface/PythonExchange.h
- src/Client/Gameplay/ExchangeStateGuard.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c03_09_exchange_flow.md"
architecture_layer: "Ekwipunek, Przedmioty, Handel i Gospodarka"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu**: Klasa `CPythonExchange` pelni role menedzera stanu i glownego interfejsu (Singleton) po stronie klienta do obslugi handlu miedzy graczami (wymiany przedmiotow oraz waluty Elk/Yang). Obejmuje inicjalizacje sesji handlowej, aktualizacje danych obu stron handlu (siebie samego oraz partnera handlowego), w tym informacji o dodanych i usunietych przedmiotach, wlasciwosciach przedmiotow (atrybuty i sockety), kwotach Yang oraz statusie zatwierdzenia transakcji (Accept). Z kolei `ExchangeStateGuard` sluzy do walidacji sesji pod katem regul gry, pilnujac spojnosci stanu i stanowiac zabezpieczenie architektoniczne.
- **Punkt w petli gry**: Logika ta dziala asynchronicznie, glownie jako reakcja na pakiety sieciowe przychodzace (network tick) za posrednictwem mostkow fazy gry (np. `PhaseGameExchangeBridge`). Z kolei w UI operacje polegaja na modyfikacji stanow zaleznie od interakcji uzytkownika i biezacych klatek renderowania.
- **Przeplyw danych**:
  1. *Otwarcie (START)*: Pakiet `ExchangeSub::GC::START` aktywuje stan handlu. Nastepuje wyczyszczenie pol (`Clear()`) w `CPythonExchange` i ustawienie flagi `m_isTrading` oraz pobranie imienia partnera. Mostek Python wywoluje `StartExchange` z poziomu okna UI.
  2. *Dodawanie przedmiotow (ITEM_ADD)*: Gdy klient lub partner dodaja przedmioty (albo Elk/Yang) wysylane sa do klienta pakiety np. `ExchangeSub::GC::ITEM_ADD`. Struktury pamieci (`m_self`, `m_victim`) w `CPythonExchange` zapisywane sa przez dedykowane setery (np. `SetItemToSelf`, `SetItemMetinSocketToSelf`).
  3. *Akceptacja (ACCEPT)*: Gdy ktos klika "Akceptuj", dochodzi pakiet `ExchangeSub::GC::ACCEPT`. `SetAcceptToSelf` / `SetAcceptToTarget` zapisuje akceptacje transakcji.
  4. *Finalizacja (END) lub przerwanie (CANCEL)*: Po obu stronach zgoda prowadzi do wymiany itemow; transakcja jest konczona pakietem z subheaderem END, gdzie wywoluje sie `End()` na instancji wymiany. Po zamknieciu wysylany jest event `EndExchange` w UI.
- **Cykl zycia obiektow**:
  - `CPythonExchange` jest Singletonem. Alokacja odbywa sie na starcie klienta. Inicjalizacja `Clear()` nastepuje przy kazdym nowym handlu (na `START`). Stan konczy sie na `End()` resetujacym flage `m_isTrading = false`. Obiekty struktur `TExchangeData` sa przypisywane do predefiniowanych macierzy dla przedmiotow z ograniczeniem `EXCHANGE_ITEM_MAX_NUM = 12`.
  - `ExchangeStateGuard` jest wywolywane zaleznie od modulu domenowego `Client::Gameplay` do jednorazowej lub prewencyjnej walidacji biezacego stanu sesji uzytkujac lekkie obiekty bledu (`Client::Core::Result`).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound)**:
  - Klasa wywolywana w module klienta m.in. przez: `PhaseGameExchangeBridge` (z `src/UserInterface/PythonNetworkStreamPhaseGameExchange.cpp`) oraz handlers.
  - Okna handlowe Pythona (przez wyeksponowane C-API `PythonExchangeModule`).
  - Sprawdzenia w `PythonNetworkStreamPhaseGameItem.cpp` (np. `CPythonExchange::Instance().isTrading()`).
- **Zaleznosci wyjsciowe (Outbound)**:
  - `PythonPlayer`, `PythonCharacterManager` do pobierania Vnum, imion i instancji postaci.
  - Zglaszanie zmian w oknach interfejsu m.in. poprzez funkcje `PyCallClassMemberFunc(...)`.
  - Korzysta bezposrednio z `ExchangeErrors.h`, a `ExchangeStateGuard` opiera sie na `Client/Gameplay/TradeDomain.h` i modulu `Client/Core/Result.h`.
- **Drzewo dyrektyw `#include`**:
  - W `PythonExchange.cpp`: `"stdafx.h"`, `"PythonExchange.h"`, `"ExchangeErrors.h"`.
  - W `PythonExchange.h`: `"Packet.h"`. 
  - W `ExchangeStateGuard.h`: `<expected>`, `<cstdint>`, `"Client/Core/Result.h"`, `"Client/Core/DomainCommands.h"`, `"Client/Gameplay/TradeDomain.h"`. Brak oczywistych naruszen cyklicznych zaleznosci.
- **Model pamieciowy**:
  - Pola przechowywane jako byty (primitive types / arrays), operacje sa na kopiach (value semantics) z tablic prealokowanych. Brak wskaznikow (wylaczajac std::span/inteligentne polaczenia w warstwach wyzszych, mostkach) - z perspektywy samego handlu struktura z uzyciem pre-definiowanych dlugosci typu macierze dwuwymiarowe (`[12][MAX_NUM]`). Wszelkie odwolania znakowe na sztywno zaalokowane (`char name[CHARACTER_NAME_MAX_LEN + 1]`).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur**:
  - `CPythonExchange` | Singleton zarzadzajacy stanem handlu | Wielkosc zalezy od struct | Glowny watek logiki
  - `CPythonExchange::TExchangeData` | Wewnetrzna struktura przetrzymujaca stan gracza (self) i partnera (victim) podczas wymiany | Stale alokowana | Glowny watek
  - `Client::Gameplay::ExchangeStateGuard` | Stateless validator logiczny regul handlu na serwerze/kllencie | N/A | Glowny watek gry.
- **Tabela Metod Publicznych**:
  - `void CPythonExchange::Start()` i `End()` | C++ flag setter | void | Brak warunkow, setuje m_isTrading na boolean. 
  - `void CPythonExchange::SetItemToSelf(DWORD pos, DWORD vnum, BYTE count)` | Ustawianie przedmiotu na dany slot (self). | void | Sprawdza `pos >= EXCHANGE_ITEM_MAX_NUM`. | Nadpisuje `m_self.item_vnum` i `m_self.item_count`.
  - `void CPythonExchange::SetElkToTarget(DWORD elk)` | Zmiana kasy u partnera | void | - | Nadpisuje kwote Elku/Yang.
  - `void CPythonExchange::SetAcceptToSelf(BYTE Accept)` | Flaga zgody. | void | - | Odwzorowuje logike false/true jako binarne.
  - `[[nodiscard]] Client::Core::Result<void, Client::Core::CommandError> ExchangeStateGuard::Validate(const PlayerExchange& exchange, EterBase::EntityId initiator, EterBase::EntityId target) const` | Sprawdzenie stanu handlu | Result | Oczekuje dostepnej instancji. | Wyrzuca Result (Void / Error).
- **Pamieciowy Layout Struktur (Memory Layout & Offsets)**:
  - `TExchangeData`: `name[CHARACTER_NAME_MAX_LEN + 1]` -> `item_vnum[12]` (DWORD) -> `item_count[12]` (BYTE) -> `item_metin[12][SOCKET_MAX]` (DWORD) -> `item_attr[12][ATTR_MAX]` (TPlayerItemAttribute) -> `accept` (BYTE) -> `elk` (DWORD). Struktury podlegaja paddingom C/C++. W razie Hookingu podmienic offsety pamietajac ze Singleton oparty z uzyciem bazowej instancji EterLib/CSingleton.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe**:
  - CG (Client -> Game): Pakiet `TPacketCGExchange` (Header np. `CG::EXCHANGE`). Subheadery dla akcji `ExchangeSub::CG::START`, `ITEM_ADD`, `ITEM_DEL`, `ELK_ADD`, `ACCEPT`, `CANCEL`.
  - GC (Game -> Client): Pakiet `TPacketGCExchange` (Header np. `GC::EXCHANGE`). Subheadery obslugiwane to m.in. `START`, `ITEM_ADD`, `ITEM_DEL`, `ELK_ADD`, `ACCEPT`, `END`, `ALREADY`, `LESS_ELK`.
  - Odkodowywane przez `ExchangePacketCodec::DecodeExchangePacket`.
- **Metody Pythona (`PyMethodDef`)**:
  - Interfejs znajduje sie prawdpodobnie w innym pliku, jednakze pakiety triggeruja od razu sub-hooki bezposrednio w GUI jak np.: `PyCallClassMemberFunc(..., "StartExchange", ...)`, `"EndExchange"`. Wartosci same pobierane i odpytywane poprzez singleton przez dostarczone wrappery z C API takie jak przypisane property.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci**:
  - Singletony w UI gry dzialaja glownie w obrebie glownego watku D3D i watek sieciowy przekierowujacy asynchronicznie polecenia. Nie uzywano specjalnych mutexow na handlu z powodu wbudowanych procedur event busa / synchronizacji watku, brak bezposredniej wspolbieznosci na danych w interfejsie.
- **Potencjalne punkty awarii (Crash Points & Edge Cases)**:
  - Najwieksze niebezpieczenstwo: `pos >= EXCHANGE_ITEM_MAX_NUM` – kazdy accessor i setter jak np. `SetItemMetinSocketToSelf` lub `DelItemOfTarget` ma obowiazkowy guard chroniacy przed Out-of-Bounds array write.
  - Zgubienie synchronizacji UI i backendu przy nieoczekiwanym zerwaniu lacza (m_isTrading pozostanie w blednym stanie), jezeli PhaseGame nie odpali poprawnego Clear. Brak clear dla flagi `m_elk_mode` podczas operacji Start().
- **Zarzadzanie zasobami (RAII)**:
  - Brak dedykowanych RAII, stan zarzadzany jest tradycyjnym alokowaniem macierzy typu POD. Zabezpieczenia na poziomie `EterBase::Result` uzywane w nowoczesnym `ExchangeStateGuard`. Do resetu pol uzywa klasycznego `memset` (nalezy pilnowac rozmiarow TExchangeData).

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide)**:
  1. Zmiana liczby pol: Ograniczenie `EXCHANGE_ITEM_MAX_NUM` (obecnie 12) jest globalna stala w `PythonExchange.h` bindowana do Pythona w C-API (np. jako stala w module). Zmiana tego wymaga przeliczenia offsetow pociagajacych pakiety, ew. dostosowanie wielkosci `m_self.item_vnum[x]`. 
  2. Implementujac zaleznosci sprawdzania np. blokady w `ExchangeStateGuard`, rozbuduj metode `Validate`, dodajac odpowiednie `CommandError`. Nastepnie zapewnij obsulge logowania poprzez nowe bledy.
- **Jak debugowac i logowac**:
  - Klient korzysta ze standardowych loggerow np. `TraceError` badz nowoczesnego `EterBase::ModernLogger`. Sledzenie zrzutow ze zmiennych jak `m_isTrading` czy biezace numery w obrebie pol struktury np. po blednym zgloszeniu pakietu w `PhaseGameExchangeBridge::HandleExchange`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness)**:
  - Klasa `ExchangeStateGuard` jest odizolowana (stateless i posiada nowoczesne error handle `expected`), mozna stworzyc instancje mocka w GTest podajac atrapy structu `PlayerExchange`. Do testow Singletona trzeba upewnic sie co do cyklu powolywania i zwalniania z pamieci (lub uzyc izolowanego modulu podpinanego pod pusta instancje `CSingleton` z prealokowana tablica `TExchangeData`).
