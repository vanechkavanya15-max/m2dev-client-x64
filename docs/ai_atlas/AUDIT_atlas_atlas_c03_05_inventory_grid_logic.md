---
task_id: "atlas_c03_05_inventory_grid_logic"
cluster: "ITM"
module_name: "Logika Siatki Ekwipunku 2D i Algorytmy Kolizji Slotow"
target_files:
- src/Client/Gameplay/InventoryGridManager.h
- src/Client/Gameplay/InventoryDomain.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c03_05_inventory_grid_logic.md"
architecture_layer: "Ekwipunek, Przedmioty, Handel i Gospodarka"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
**Funkcja modulu:**
Modul odpowiada za scisla reprezentacje i walidacje asortymentu w posiadaniu gracza (InventoryDomain) oraz matematyke zarzadzania siatka 2D dla glownych okiem (InventoryGridManager). Jest to fundamentalna warstwa domenowa (single source of truth) dla okien: Inventory (plecak 1-4 strony), Equipment (Zalozone przedmioty), Belt (Pas), DragonSoul (Alchemia) i SafeBox (Magazyn). Kod jest w pelni niezalezny od UI.

**Miejsce wywolania:**
Kod jest wolany glownie w momencie przetwarzania pakietow sieciowych (Network Tick), np. odbior informacji o dodaniu przedmiotu, jego zmianie lub usunieciu. Wywoluje sie takze przy akcjach gracza, aby zwalidowac np. lokalne przeniesienie przedmiotu przed lub po potwierdzeniu z serwera.

**Przeplyw danych (Data/Control Flow):**
1. System sieciowy lub kontroler polecen wywoluje metody domeny np. `AddItem`, `RemoveItem`, `SwapItem`.
2. Metody te wykonuja walidacje typu granicznego i kolizji (przy udziale logiki siatki - sprawdzanie rozmiaru itemow wzgledem wierszy i kolumn, zapewnienie spistosci strony).
3. Przy udanej weryfikacji odpowiedni wektor np. `m_mainInventory` lub `m_equipment` jest aktualizowany w pamieci (operacje na `std::optional<ItemData>`).
4. Wywolywany jest callback `m_slotUpdateCallback`, informujacy obiekty wyzej (EventBus / UI) o koniecznosci przerysowania konkretnego slotu.

**Cykl zycia:**
Klasy inicjalizowane na poziomie sesji (po wejsciu do gry). Dane alokowane z gory dzieki `std::vector::resize()` dla znanych okien, czyszczone za pomoca metody `Clear()` miedzy zmianami postaci lub po wylogowaniu gracza.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
**Zaleznosci wejsciowe (Inbound):**
Adaptery sieciowe zajmujace sie pakietami itemow, kontrolery gracza decydujace o operacjach ekwipunku, listenery powiazane z UI, ktore rejestruja swoje akcje w `SetSlotUpdateCallback`.

**Zaleznosci wyjsciowe (Outbound):**
`EterBase::StrongType` (silne typy dla np. `ItemVnum`, `ItemSlot`), `EterBase::Result`, logowanie `EterBase::LogModern`, domeny bledow i eventow `Client::Core::DomainErrors/DomainEvents`. Brak uwiklania w DirectX, Granny czy WinAPI.

**Drzewo dyrektyw `#include`:**
Bazuje na standardowej bibliotece: `<vector>`, `<cstdint>`, `<optional>`, `<expected>`, `<span>`, `<array>`, `<functional>`. Bezpieczne zaleznosci wewnetrzne, nie zaobserwowano ryzyk zaleznosci cyklicznych.

**Model pamieciowy:**
- Silnie zorientowany na struktury danych (Data-Oriented).
- Przechowywanie zawartosci ekwipunku to plaskie `std::vector<std::optional<ItemData>>` dajace ciagly blok pamieci dla minimalizacji cache-misses.
- Brak surowych wskaznikow C. Przekazywanie odbywa sie przez wartosc dla malych struktur lub przez const referencje (`const ItemData&`).
- Dla pobierania wlasciwego modulu dla okna z zachowaniem referencji uzywa sie `std::reference_wrapper`.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

#### Tabela Klas i Struktur
| Nazwa | Rola | Wlasciciel Watku |
|---|---|---|
| `InventoryWindow` (enum uint8_t) | Typ okna (Inventory, Equipment, Belt itp.). | Glowny watek (dane) |
| `ItemSize` | Struktura zawierajaca szerokosc i wysokosc (w siatce). | Glowny watek (dane) |
| `ItemAttribute` | Struktura bonusu (type, value). | Glowny watek (dane) |
| `ItemData` | Pelna reprezentacja przedmiotu (vnum, count, size, sockets, attributes, flags). | Glowny watek (dane) |
| `InventorySlotUpdatedEvent` | Powiadomienie o zmianie zawartosci danego slotu w danym oknie. | Watek wywolujacy |
| `InventoryDomain` | Glowna klasa agregujaca wszystkie plecaki gracza i operacje biznesowe. | Glowny watek klienta |
| `InventoryGridManager` | Implementacja matematyczna zajetosci slotow 1D dla wymiarow 2D. | Glowny watek klienta |

#### Tabela Metod Publicznych
| Sygnatura | Wartosc Zwracana | Efekty i Warunki |
|---|---|---|
| `FindEmptyCell(InventoryWindow, ItemSize)` | `int32_t` lub `std::optional<SlotIndex>` | Zwraca pierwszy wolny slot mieszczacy rozmiar przedmiotu. -1 jesli brak. Brak efektow ubocznych. |
| `SetItem(InventoryWindow, ItemSlot, const ItemData&)` | `std::expected<void, InventoryError>` | Nadpisuje zawartosc slotu podanym `ItemData`. Warunek: slot musi byc na to gotowy. Side effect: odpala callback zmiany slotu. |
| `SwapItem(InventoryWindow, ItemSlot, InventoryWindow, ItemSlot)` | `std::expected<void, InventoryError>` | Realizuje trudna zamiane slotow 2 przedmiotow uwzgledniajac ich potencjalnie rozne rozmiary i nakladanie sie w trakcie operacji. |
| `SplitItem(InventoryWindow, ItemSlot src, ItemSlot dst, uint32_t)` | `std::expected<void, InventoryError>` | Rozdziela stos danego itemu na podana liczbe sztuk do nowego okna/slotu. Zmniejsza ilosc w oryginale. |
| `CanPlaceItem(ItemSlot, uint8_t height)` (GridManager) | `Result<void, CommandError>` | Oblicza na bazie reszty z dzielenia stron czy item wielorzedowy nie wychodzi poza strone i wymiary. |

#### Pamieciowy Layout Struktur (Memory Layout & Offsets)
- W pamieci najwazniejszy wektor: `std::vector<std::optional<ItemData>> m_mainInventory`. Zawiera po kolei dane kazdego slota poczynajac od 0-44 dla pierwszej strony, 45-89 dla drugiej, do maks 180 (Dla INVENTORY_MAX_NUM).
- ItemSize jest rzutowane czesto w `SwapItem`, gdzie dla Equipment, Belt i DragonSoul jest wymuszane `{1, 1}`, redukujac ryzyko wchodzenia na sasiednie sloty dla tych unikalnych pojemnikow.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Pakiety Sieciowe:** Kod stanowi bezposrednia baze do aplikowania stanow z `TPacketGCItemSet`, `TPacketGCItemDel` oraz obslugi operacji (CG_ITEM_USE, CG_ITEM_DROP). Brak parsera pakietow bezposrednio w klasie - dane przychodza juz zdeserializowane przez warstwe Network.
- **Python C-API:** Brak bezposredniego wiazania z Pythonem (typu `PyMethodDef`). Interfejs UI z Pythona reaguje na dane propagowane poprzez event update zglaszany przez zarejestrowany na etapie podnoszenia aplikacji `SlotUpdateCallback`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Kod nie ma zadnych mechanizmow blokad (`std::mutex`). Zalozenie wywolywania synchronicznego na jednym, glownym watku logicznym gry (Main Loop). Wszelkie operacje sieciowe musza byc wczesniej przesuniete na watek glowny z watku IO.
- **Pulapki (Gotchas):**
  - Ochrona przed nadpisywaniem itemow (np. SwapItem usuwa oba w pamieciowym ukladzie, weryfikuje ich wspolne nowe miejsce, by poprawnie umiejscowic nowy rozmiar).
  - Wymuszanie rozmiaru {1, 1} dla Equipment, Belt, DragonSoul w logice domenowej ignoruje natywny rozmiar itemu. Jest to wymuszone by nie zachodzily na inne sloty.
  - Odliczanie wolnego miejsca nie korzysta z dynamicznego cache'a a uzywa petli O(N) przy kazdym strzale sprawdzajac uklad kolizyjny po wektorze stanow - uwazac na czestotliwosc szukania wolnego slota w petlach.
- **Zarzadzanie zasobami (RAII):** Czysta alokacja stosu i w pamieci sterty przez kontenery STL, bezpieczne zarzadzanie pamiecia za sprawa standardowych obiektow bibliotecznych.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Dodawanie nowej logiki okiennej:** W enumie dodajesz id. W `InventoryDomain` deklarujesz kolejny wektor. W `GetWindowSlots` dokladasz case obslugujacy ten wektor. Gotowe! Modul bedzie sam realizowac `ItemSize` pod katem siatki dla zwyklych ekwipunkow, o ile nie uzyjesz nadpisywania na {1, 1}.
- **Debugging:** Skup sie na weryfikacji powrotu z funkcji Result. W `SwapItem` jest specyficzny log w przypadku bledu przy przetasowaniu wnetrznosci, z EterBase::ModernLogger::Error. Jesli nie przechodzi zmiana przedmiotu po restarcie, sprawdz silent unexpected return przy FindEmptyCell.
- **Headless Testing:** Klasa nie zawiera ani jednej linijki DirectX. Wystarczy napisac standardowy blok np. w `doctest` instancjonujac obiekt i sztucznie ladowac pakiety (fake ItemData), co gwarantuje latwosc unit-testowania operacji ekwipunku. Przydatne mockowanie okien by przetestowac graniczne operacje slotow (np. Swap).
