---
task_id: "atlas_c03_15_currency_economy"
cluster: "ITM"
module_name: "Wielo-Walutowy System Gospodarki Gry"
target_files:
- src/Client/Gameplay/CurrencyType.h
- src/Client/Gameplay/TradeDomain.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c03_15_currency_economy.md"
architecture_layer: "Ekwipunek, Przedmioty, Handel i Gospodarka"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Modul `TradeDomain` i `CurrencyType` implementuje rdzenna logike domenowa operacji gospodarczych i handlowych w kliencie gry Metin2. Decyduje o obsludze sklepow u NPC, bezpiecznej wymiany pomiedzy graczami (Player Exchange) oraz banku depozytowego (SafeBox).

Kod realizuje nowoczesne podejscie (Domain-Driven Design) izolujace logike gry od warstwy prezentacji (UI) i sieci. Operacje walutowe chronione sa przez silne typowanie (uzywajace mechanizmu `EterBase::StrongType`), co zapobiega bledom mieszania walut oraz bledom przepelnienia wartosci (dzieki 64-bitowemu buforowaniu na typach zdefiniowanych m.in. dla zlota i ceny). Cykl zycia obiektow domenowych jest scisle zwiazany z wystapieniem sesji handlu (np. inicjalizacja instancji `PlayerExchange` w momencie rozpoczecia wymiany i niszczenie jej po zatwierdzeniu/anulowaniu).

Kod jest wywolywany na biezaco przez obsluge pakietow sieciowych (np. podczas walidacji po otrzymaniu pomyslnej zgody od serwera) i bezposrednio przed aktualizacja widoku UI. 

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

- **Zaleznosci wejsciowe (Inbound):** Moduly warstwy interfejsu uzytkownika (EventBus aktualizujacy widok), menedzery inwentarza (np. InventoryDomain sprawdzajace warunki obrotu walut) oraz moduly przetwarzania pakietow sieciowych (Game Packet Handlers).
- **Zaleznosci wyjsciowe (Outbound):** 
  - `EterBase/StrongTypes.h` - gwarantuje silne typowanie wartosci.
  - `EterBase/Result.h` - struktury dla deterministycznej obslugi bledow zamiast wyjatkow.
  - Standardowe biblioteki C++ (`<vector>`, `<unordered_map>`, `<optional>`, `<cstdint>`).
- **Drzewo dyrektyw `#include`:** Dolaczenia naglowkow EterBase wskazuja na gleboka integracje z systemem zarzadzania stanem/zwracania bledow bez zaleznosci od interfejsu UI w samym naglowku domenowym. Brak cyklicznych zaleznosci.
- **Model pamieciowy:** W przewazajacej mierze oparty na stosie w operacjach. Przechowywanie danych wykorzystuje optymalne kontenery `std::unordered_map` (dla szybkiego mapowania numeru przedmiotu na jego wlasciwosci w sklepie czy bezpiecznej skrzynce). Do przekazywania stanow wykorzystywane sa stale i zmienne referencje oraz surowe wskazniki dla podgladu danych (`const ExchangeParticipant*`). Brak jawnego uzycia inteligentnych wskaznikow (shared_ptr).

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Enum:
- `CurrencyType` (`uint8_t` z `CurrencyType.h`): Zdefiniowane typy walut (Gold, Cheque, Gaya).
- `ExchangeState`: Maszyna stanow dla wymiany pomiedzy graczami (Start, AddItem, AddGold, Lock, Accept, Cancel).

### Type Aliases & Tags:
- `Gold`: Alias na `EterBase::StrongType<GoldTag, uint64_t, 0>`. Hermetyzacja 64-bitowej wartosci.
- `Price`: Alias na `EterBase::StrongType<PriceTag, uint64_t, 0>`. Hermetyzacja 64-bitowej ceny.

### Struktury (Memory Layout & Offsets):
- `ShopItem`:
  - `vnum` (`EterBase::ItemVnum`) - ID przedmiotu
  - `count` (`uint8_t`) - Ilosc
  - `buy_price` (`Price`) - Cena kupna w sklepie
  - `sell_price` (`Price`) - Otrzymywana cena za sprzedaz
- `ExchangeParticipant`:
  - `id` (`EterBase::EntityId`) - ID podmiotu wymiany
  - `gold` (`Gold`) - Zaproponowane zloto (domyslnie 0)
  - `items` (`std::vector<std::pair<EterBase::ItemSlot, EterBase::ItemVnum>>`) - Kolekcja oferowanych przedmiotow
  - `is_locked` (`bool`) - Zablokowanie wprowadzania zmian
  - `is_accepted` (`bool`) - Akceptacja wymiany
- `SafeBoxItem`: Przechowuje wlasciwosci przedmiotu w magazynie (`vnum`, `count`).

### Klasy C++:
- **`NpcShop`:**
  - `RegisterItem(const ShopItem& item)` -> void
  - `GetItem(EterBase::ItemVnum vnum) const` -> `std::optional<ShopItem>`
  - `BuyItem(EterBase::ItemVnum vnum, uint8_t count, Gold current_gold) const` -> `EterBase::Result<Gold, std::string_view>` (operacja zakupu)
  - `SellItem(EterBase::ItemVnum vnum, uint8_t count) const` -> `EterBase::Result<Gold, std::string_view>` (operacja sprzedazy)
- **`PlayerExchange`:**
  - Konstruktor inicjujacy instancje podmiotow wymiany w oparciu o ich EntityId.
  - Zestaw operacji modyfikujacych stan z sygnatura zwracajaca `EterBase::VoidResult<std::string_view>`: `AddItem`, `AddGold`, `Lock`, `Accept`, `Cancel`.
  - Akcesory stanu: `GetState`, `GetParticipant`.
- **`SafeBox`:**
  - Konstruktor tworzacy bezpieczny magazyn (SafeBox) dla gracza.
  - Operacje: `SetItem`, `RemoveItem`, `GetItem`.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

Z racji bycia czysta klasa domenowa, te naglowki nie exportuja bezposrednio definicji pakietow sieciowych (`struct TPacketCG...`) ani nie rejestruja metod poprzez `PyMethodDef` w Pythonie. 
Zaleznosci od strony wywolan PyAPI beda bazowaly na funkcjach `app.BuyItem()`, `app.ExchangeItem()`, ktore wewnetrznie posluza jako punkty wejscia do aktualizacji stanu instancji `PlayerExchange` oraz `NpcShop`. Pakiety moga uzywac opcodow (np. pakiet akcji handlu) do transportowania Vnum i ilosci towarow z sieci do tych struktur domenowych.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Pulapki silnego typowania:** Poniewaz `Gold` oraz `Price` korzystaja z `EterBase::StrongType`, proba dodania zwyklej liczby calkowitej (`int` czy `uint64_t`) spowoduje blad kompilacji. Nalezy explicite owijac wartosci lub korzystac ze zdefiniowanych operatorow.
- **Bezpieczenstwo w pamieci (String_view lifetime):** Blad zwracany z typu `EterBase::Result` to `std::string_view`. Agenci AI musza upewnic sie, ze teksty bledu zwracane w implementacjach to literaly ciagow znakow (static storage duration). Dynamicznie skomponowane stringi zwrocone jako `string_view` stana sie zawieszone (dangling reference), zwiastujac powazne usterki.
- **Limit zlota i przepelnienia:** Uzyto bezpiecznego typu `uint64_t` w podkladzie `StrongType`, dlatego kod operujacy ze starym systemem sieciowym (`int` / `DWORD`) narazony jest na uciecie bitow (truncation), jesli ktos probuje zrzutowac go wstecz. Nalezy przestrzegac rozszerzania typow.
- **Bezpieczenstwo Stanu (State Safety):** System zabezpiecza transakcje wymiany. Wymiana nie moze przejsc w stan akceptacji `Accept`, dopoki stan poszczegolnego uczestnika nie zostanie zatwierdzony `is_locked == true`. Konieczna jest stala weryfikacja stanow i stanowiska uzytkownika przed aktualizacja globalnego zasobu.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

- **Dodawanie Nowego Typu Waluty (Step-by-step):**
  1. Dodaj wpis do enum class `Client::Gameplay::CurrencyType`.
  2. Utworz nowy strukturalny tag, np: `struct WonTag {};` w `TradeDomain.h`.
  3. Zdefiniuj alias na `EterBase::StrongType`: `using Won = EterBase::StrongType<WonTag, uint64_t, 0>;`.
  4. Wzbogac logike transakcji w `PlayerExchange` dodajac pole z nowa waluta do structury `ExchangeParticipant` i adekwatne metody `AddWon`.
- **Jak Testowac (Headless / Unit Test Harness):**
  - Ten podsystem logiki nie ma zadnych zaleznosci od systemu renderingu, co oznacza, ze nadaje sie doskonale do testow jednostkowych w trybie Headless.
  - Skonfiguruj Doctest i napisz testy weryfikujace `NpcShop` i caly proces `PlayerExchange`. Zamockuj system sieci, zeby symulowac przychodzace akcje dodawania przedmiotow.
- **Debugging & Logowanie:** Najlepiej polegac na punktach zrzutu (breakpoints) lub sprawdzeniu logiki zwracania surowej wartosci podczas debuggingu poprzez wewnetrzna wartosc np. `gold.Get()` z interfejsu `StrongType`.
