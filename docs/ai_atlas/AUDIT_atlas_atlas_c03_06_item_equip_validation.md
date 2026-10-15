---
task_id: "atlas_c03_06_item_equip_validation"
cluster: "ITM"
module_name: "Walidator Mozliwosci Zakladania Ekwipunku"
target_files:
- src/Client/Gameplay/InventoryItemValidator.h
- src/Client/Gameplay/InventoryCommandHandler.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c03_06_item_equip_validation.md"
architecture_layer: "Ekwipunek, Przedmioty, Handel i Gospodarka"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Dokladna funkcja:** Modul ten pelni role glownego bezpiecznika w systemie ekwipunku, odpowiadajac za walidacje danych przed podjeciem jakiejkolwiek akcji na przedmiocie przez uzytkownika. Rozdziela on odpowiedzialnosc w architekturze na bezstanowa walidacje wlasciwosci przedmiotow (poziom, profesja, slot docelowy) poprzez `InventoryItemValidator` oraz stanowa obsluge komend dzialajacych na ekwipunku poprzez `InventoryCommandHandler`, w tym sprawdzenie stanu gracza (czy nie jest martwy lub czy nie handluje).
- **Miejsce wywolania:** Kod jest wywolywany przewaznie w watku glownym gry, w reakcji na akcje gracza w interfejsie graficznym (UI), lub podczas odbierania informacji (np. poprzez EventBus, dispatchery pakietow po potwierdzeniu akcji ekwipunku przez serwer).
- **Przeplyw danych (Control & Data Flow):**
  1. Gracz inicjuje akcje w interfejsie, np. klikajac prawym przyciskiem myszy by zalozyc przedmiot. Akcja ta jest pakowana w komende (np. `UseItemCommand`).
  2. `InventoryCommandHandler::Handle` otrzymuje wariant komendy i uzywa `std::visit` z C++23 do przekazania jej do konkretnego handlera (np. `HandleUse`).
  3. `HandleUse` (lub inna metoda) zaczyna od sprawdzenia ogolnych warunkow za pomoca `CheckCommonConditions` - czy gracz jest zywy, czy aktualnie nie handluje oraz czy podany slot na pewno posiada przedmiot (komunikacja z `InventoryDomain`).
  4. Nastepnie do dzialania wchodzi `InventoryItemValidator`, ktory bezstanowo weryfikuje w `CanEquipItem`, czy gracz ma prawo zalozyc ten konkretny przedmiot w tym miejscu (sprawdzajac poziom postaci, klase postaci/Anti-Flag, oraz typ docelowego miejsca i flage zakladania przedmiotu).
  5. W przypadku powodzenia (zwracane `std::expected` / `EterBase::Result` z wartoscia pusta lub true), wykonywana jest faktyczna logika modyfikacji i komunikacja z serwerem. W przypadku bledu, dzialanie jest przerywane i zwracany jest enum `CommandError`.
- **Cykl zycia obiektow:** `InventoryItemValidator` posiada wylacznie metody statyczne, wiec nie posiada wlasnego stanu cyklu zycia. Obiekt `InventoryCommandHandler` jest instancjonowany prawdopodobnie wraz z podsystemem Gameplay i operuje na referencjach do istniejacych podsystemow (`InventoryDomain`, `IPlayerConditionProvider`), nie zarzadzajac ich czasem zycia bezposrednio. Komendy to krotko zyjace obiekty typu Value Object (POD), alokowane na stosie i przekazywane przez wartosc/referencje.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Modul jest prawdopodobnie wywolywany przez warstwe sieciowa (po odpowiednim przetworzeniu przez zmodernizowany `Client::Bridge::StranglerFacade` i dispatchery pakietow) oraz sub-system interfejsu uzytkownika/Pythona przekazujacy akcje gracza (most do polecen).
- **Zaleznosci wyjsciowe (Outbound):** 
  - `GameLib/ItemData.h` - dla sprawdzania wlasciwosci przedmiotu (`CItemData`).
  - `EterBase/Result.h` i `EterBase/StrongTypes.h` - dla semantyki zwrotow (`Result` uzywajace std::expected).
  - `Client/Core/DomainCommands.h` i przestrzenie domenowe (np. `InventoryDomain`).
  - `Client/Network/Protocol/GameType.h` - dla definicji stalych typu `c_Equipment_Start`.
- **Drzewo dyrektyw `#include`:**
  - Zewnetrzne/Globalne: `<cstdint>`, `<string_view>`, `<variant>`, `<format>`.
  - Wewnetrzne EterBase: `../../EterBase/Result.h`, `../../EterBase/StrongTypes.h`, `EterBase/StdAfx.h`.
  - Powiazane domeny: `../../Client/Core/DomainCommands.h`, `../../GameLib/ItemData.h`, `Client/Network/Protocol/GameType.h`, `InventoryDomain.h`.
  - *Ryzyko zaleznosci cyklicznych:* Zastosowano `forward declaration` dla `class CItemData;` i `class InventoryDomain;` w naglowkach, co dobrze chroni przed zaleznosciami cyklicznymi.
- **Model pamieciowy:** Dominuja przekazania przez niemodyfikowalne referencje `const IPlayerConditionProvider&` czy proste obiekty na stosie (`UseItemCommand`, inteligentnie zapakowane w `std::variant`). Uzycie zjawisk zarzadzania wskaznikami jest mocno ograniczone - brak uzywania raw pointerow bedacych wlascicielami (wskaznik `const CItemData*` sluzy wylacznie do odczytu danych).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabele Klas i Struktur:**
- `Client::Gameplay::CommandError` (enum class, 1 bajt) - Silnie typowane stany bledow domenowych operacji m.in `PlayerDead`, `PlayerTrading`, `SlotEmpty`.
- `Client::Gameplay::UseItemCommand` (struct, ~3-4 bajty) - Komenda uzycia: `windowType` (uint8_t), `slot` (EterBase::ItemSlot).
- `Client::Gameplay::DropItemCommand` (struct, ~3-4 bajty) - Komenda wyrzucenia.
- `Client::Gameplay::MoveItemCommand` (struct, ~6-8 bajtow) - Komenda przeniesienia (zrodlowy slot/okno i docelowy slot/okno).
- `Client::Gameplay::IPlayerConditionProvider` (interfejs klas) - Ostrzykuje stan gracza dla handlera by uniknac twardej zaleznosci od klasy bytu/gracza.
- `Client::Gameplay::InventoryCommandHandler` (class, wlasciciel watku glownego) - Rozpakowuje komendy `std::variant` i autoryzuje je wobec stanu gry. Posiada dwie referencje do innych domen (prawdopodobnie rozmiar 16 bajtow na architekturze 64-bit).
- `Client::Gameplay::InventoryItemValidator` (class/static utility) - Wykonuje czysta, bezstanowa walidacje noszenia sprzetu.

**Tabela Metod Publicznych:**
- `std::string_view Client::Gameplay::ToString(CommandError err)`
  - *Argumenty:* `CommandError`
  - *Zwraca:* Zreprezentowany ciag znakow bledu, brak skutkow ubocznych.
- `EterBase::Result<bool, Client::Core::CommandError> Client::Gameplay::InventoryItemValidator::CanEquipItem(const CItemData* itemData, uint8_t playerRace, uint8_t playerLevel, uint16_t targetCell)`
  - *Argumenty:* Pointer na dane przedmiotu, rasa, poziom gracza oraz komorka docelowa.
  - *Zwraca:* `true` jesli gracz moze zalozyc przedmiot, `CommandError::InvalidParameter` jako `std::unexpected` jesli ktores wymaganie nie jest spelnione (zly poziom, anti-flagi, niewlasciwy slot ubran).
- `EterBase::Result<void, CommandError> Client::Gameplay::InventoryCommandHandler::Handle(const InventoryCommand& command)`
  - *Argumenty:* Komenda ekwipunku (`std::variant`).
  - *Zwraca:* Sukces (void) lub `CommandError` w przypadku niedozwolonej akcji.
- `virtual bool Client::Gameplay::IPlayerConditionProvider::IsDead() const` oraz `IsTrading() const`
  - Metody zwracajace stan gracza. Posiadaja implementacje w systemach postaci.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
Struktury komend (POD) sa minimalne. `InventoryCommand` to `std::variant`, wiec jego rozmiar wyniesie maksimum rozmiaru wariantow + tag (zwykle ok. 16 bajtow na x64 wlaczajac w to alignment). Klasa `InventoryCommandHandler` ma tylko dwa pola wskaznikowe/referencyjne, wiec jej wielkosc wynosi dokladnie 16 bajtow, z offsetami (0: `InventoryDomain&`, 8: `IPlayerConditionProvider&`), pod warunkiem standardowego layoutu g++ na Linuxie x64.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Kod po udanej walidacji po stronie `InventoryCommandHandler` inicjalizuje docelowo wyslanie pakietow ekwipunku do serwera (np. CG pakiet uzycia lub przeniesienia przedmiotu, takich jak `CG_ITEM_USE`, `CG_ITEM_DROP`, `CG_ITEM_MOVE`). Odpowiedzi serwera w formie pakietow GC trafiaja do sieciowego dispatchera pakietow, np. `PhaseGamePacketDispatcher` przed zmiana wizualna po stronie klienta.
- **Metody Pythona (`PyMethodDef`):** Funkcjonalnosci sa wywolywane z pythona poprzez C-API klienta ekwipunku, np. gdy wywolywane jest klikniecie z `net.SendItemUsePacket()` - Python przesyla inty, ktore w warstwie C++ sa deserializowane do np. `UseItemCommand` przed uzyciem w opisanym CommandHandlerze.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Kod operuje scisle na watku glownym gry, zgodnie z architektura Metin2 dla `CInstanceBase` oraz eventow UI. Z uwagi na obecnosc `std::visit` i bezstanowosc walidacji, nie ma tutaj mutexow. Dzialanie wielowatkowe spowodowaloby "data races".
- **Potencjalne punkty awarii (Crash Points):** Nalezy uwazac na wysylanie nullptr w pole `itemData` dla `CanEquipItem` - jest tam prewencyjny straznik zwracajacy blad, co jest dobra rzecza. Podatnoscia moga byc zle obliczone zakresy docelowych slotow, dlatego uzywane jest mapowanie z `c_Equipment_Start`.
- **Zarzadzanie zasobami (RAII):** Kod powszechnie stosuje monadyczne interfejsy `EterBase::Result` oraz silne typowanie i zwracanie kopii przez wartosc, nie alokujac wskaznikow i omijajac potencjalne wycieki pamieci.
- **Specyfika domenowa Metin2:** System profesji `RaceToJob` dzieli wynik modulo 4 na: `0: Warrior, 1: Assassin, 2: Sura, 3: Shaman`. Nalezy wziac to pod uwage w integracjach AI.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Dodaj nowa strukture np. `SplitItemCommand` do `InventoryCommandHandler.h` z potrzebnymi danymi (slot docelowy, okno, ilosc sztuk).
  2. Rozszerz algebryczny typ sumy danych `std::variant<...>` o nowo dodana komende.
  3. Skompiluj (kompilator wyrzuci blad o niespelnieniu wzorca przez `std::visit` - zmusza to do dodania w `Handle` nowego `else if constexpr(...)` wraz z odpowiednim `HandleSplit`).
  4. Zaimplementuj `HandleSplit`, obowiazkowo sprawdzajac na poczatku `CheckCommonConditions()`.
- **Jak debugowac i logowac:** Do debugowania sprawdzac glownie stan zwracany poprzez `EterBase::Result`. Pomocny okaze sie `std::formatter` w logach oparty na `Client::Gameplay::ToString()`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Utworz prosta implementacje `IPlayerConditionProvider` w tescie doctest uzywajac zmiennych logicznych oraz zmockowane dane klas. Stworz w testach obiekty komend na stosie i uzywaj bezstanowego walidatora (bez zaleznosci UI i instancji gry) do blyskawicznego testowania krawedziowego (edge-case'y blednego poziomu, czy martwej postaci).
