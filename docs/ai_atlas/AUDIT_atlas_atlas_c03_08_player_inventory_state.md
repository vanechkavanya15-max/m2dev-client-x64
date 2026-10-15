---
task_id: "atlas_c03_08_player_inventory_state"
cluster: "ITM"
module_name: "CPythonPlayer - Ekwipunek Gracza i Sloty Specjalne"
target_files:
- src/UserInterface/PythonPlayer.cpp
- src/UserInterface/PythonPlayer.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c03_08_player_inventory_state.md"
architecture_layer: "Ekwipunek, Przedmioty, Handel i Gospodarka"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu**: `CPythonPlayer` (wraz z wewnetrznym komponentem `Client::Gameplay::InventoryDomain`) stanowi centralne repozytorium stanu postaci po stronie klienta. Przechowuje i zarzadza obiektami reprezentujacymi przedmioty nalezace do gracza, w tym ekwipunek (inventory), zalozone wyposazenie (equipment), pas (belt inventory), oraz alchemie smoczych kamieni (dragon soul inventory).
- **Punkt wywolania**: Klasa ta dziala jako Singleton dostepny globalnie. Zmiany stanu (np. odbieranie danych pakietow sieciowych) inicjowane sa przez system sieciowy (np. `ItemSetHandler`, `ItemDelHandler`), a odczyty stanu nastepuja bezposrednio podczas renderowania interfejsu (UI) poprzez mostek Python C-API (np. `player.GetItemIndex`).
- **Przeplyw danych**: 
  1. Klient odbiera pakiet (np. `HEADER_GC_ITEM_SET` / `0x0511`).
  2. Handler sieciowy odpakowuje dane i uaktualnia dedykowany bufor in-memory w `InventoryDomain`.
  3. `CPythonPlayer` synchronizuje swoje bufory kompatybilnosciowe `m_itemDataCompat` (typu `TItemData`) aby zapewnic wsteczna kompatybilnosc z API Pythona (metoda `UpdateCompatItem`).
  4. Warstwa UI pyta C++ (poprzez modul `player`) o ID i ilosc przedmiotu pod danym indeksem (`TItemPos`).
- **Cykl zycia obiektow**: `CPythonPlayer` alokowany i inicjalizowany jest w fazie ladowania gry. Klasa utrzymuje ciagly stan podczas polaczenia (PhaseGame). Obiekty itemow nie sa bezposrednio niszczone w pamieci sterty, a raczej ulegaja "wyzerowaniu" w `InventoryDomain` przy usunieciu przedmiotu.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound)**: 
  - Architektura sieciowa: `CPythonNetworkStream` (parsuje pakiety i propaguje do domenowych handlerow).
  - Interfejs uzytkownika: pliki .py (UI, ekwipunek), komunikujace sie przez modul C-API `PythonPlayerModule.cpp`.
- **Zaleznosci wyjsciowe (Outbound)**: 
  - `Client::Gameplay::InventoryDomain` do hermetycznego zarzadzania logika pojemnosci przedmiotow.
  - `QuickslotManager` (zarzadzanie podrecznymi przypisaniami skrotow).
  - Warstwa Sieciowa (wysylanie opkadow np. `CG::ITEM_USE`, `CG::ITEM_MOVE` przez `SendItemUsePacket`).
- **Drzewo dyrektyw `#include`**: Obejmuje system domenowy `InventoryDomain.h`, narzedzia pomocnicze typu `TItemPos` oraz interfejs gracza `IAbstractPlayer`. Brak drastycznego ryzyka cyklicznego, zaleznosci skierowane sa "w dol" (w kierunku mechaniki sieci i danych).
- **Model pamieciowy**: Wskazniki surowe stosowane sa glownie podczas obslugi pakietow z sieci. Struktury zewnetrzne `std::optional<ItemData>` oraz wektory (`std::vector`) znajduja sie wewnatrz `InventoryDomain`. Klasa wciaz trzyma kompatybilna statyczna tablice C-style: `mutable TItemData m_itemDataCompat[c_Inventory_Count]`.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur**
- `CPythonPlayer`: Glowny singleton zarzadzajacy graczem. Wlasciciel watku glownym (UI). Reprezentuje centralny kontroler operacji in-game dla gracza.
- `Client::Gameplay::InventoryDomain`: Agregat logiczny przechowujacy bufory ekwipunku (`m_mainInventory`, `m_equipment`, `m_beltInventory`, `m_dragonSoulInventory`).

**Tabela Metod Publicznych w CPythonPlayer**
- `const TItemData* GetItemData(TItemPos Cell) const;`
  - Pobiera odniesienie do obiektu przedmiotu na wskazanym miejscu. Warunek: Prawidlowy index komorki. Skutek: Synchronizuje najnowsze dane z `InventoryDomain` do mapy kompatybilnosci.
- `void SetItemData(TItemPos Cell, const TItemData & c_rkItemInst);`
  - Rejestruje lub usuwa (gdy vnum = 0) przedmiot we wskazanym indeksie, wykonujac zapisy zarowno w `InventoryDomain` (nowa logika) i `m_itemDataCompat` (stara).
- `std::optional<std::pair<Client::Gameplay::InventoryWindow, EterBase::ItemSlot>> MapItemPosToDomain(const TItemPos& Cell) const;`
  - Translatuje stara notacje `TItemPos` na nowy, bezpieczny typ przestrzeni `InventoryWindow` i `ItemSlot`.

**Pamieciowy Layout Struktur (Memory Layout & Offsets)**
- `c_Inventory_Count` (`GameType.h`) determinuje wielkosc bufora dla tablic kompatybilnosciowych. Wartosc zmienia sie na podstawie makra m.in `ENABLE_NEW_EQUIPMENT_SYSTEM` i okresla calkowita pule zwyklego ekwipunku (w tym paskow i ekwipunku klasycznego).

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe (Protokol)**: 
  - Wysylane przez klienta (CG): `ITEM_USE (0x0501 / 0x0512)`, `ITEM_DROP (0x0502)`, `ITEM_MOVE (0x0504)`, `ITEM_PICKUP (0x0505)`, `ITEM_USE_TO_ITEM (0x0506)`.
  - Odbierane z serwera (GC): `ITEM_SET (0x0511)`, `ITEM_DEL (0x0510)`, `ITEM_UPDATE (0x0514)`.
- **Metody Pythona (`PyMethodDef` w PythonPlayerModule.cpp)**: 
  - Oparte o protokoly `METH_FASTCALL` oraz `METH_VARARGS`.
  - `player.GetItemIndex(window, pos)` -> zwraca VNUM przedmiotu w danej komorce.
  - `player.GetItemCount(window, pos)` -> ilosc (count).
  - `player.GetItemMetinSocket(window, pos, index)` -> dane ulepszen/kamieni dusz.
  - `player.GetItemAttribute(window, pos, index)` -> pobiera statystyki bonusow.
  - Operacje aktywne sa przypiete do warstwy strumienia w module np `net.SendItemUsePacket(pos)` wywolujac metody w `CPythonNetworkStream`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci**: Kod `CPythonPlayer` dziala glownie w watku UI (Direct3D). Pakiety sieciowe przetwarzane sa rowniez sekwencyjnie w watku UI w fazie `Update()`, aby zapobiec modyfikacjom danych asynchronicznie miedzy watkiem sieciowym, co chroni bufory in-memory przed zepsuciem (data races).
- **Crash Points (Pulapki)**: 
  - Brak bezpiecznej konwersji ze struktury `TItemPos` (gdzie window == EQUIPMENT a cell jest bardzo wysokie). Do translacji obowiazkowe jest wywolanie funkcji `MapItemPosToDomain`. 
  - Bezposrednie odwolanie do obiektu `GetItemData(TItemPos)` przy nieistniejacym przedmiocie zwroci pointer `NULL`. Python moze spowodowac segfault, jesli nie zabezpieczy sie tych wynikow przed przekazaniem ich do funkcji formatujacych (Py_BuildValue z nullptr).
- **Zarzadzanie zasobami (RAII)**: Tablice `m_itemDataCompat` trzymane sa na stosie obiketu klasy. Pamiec na serwery jest zwolniona prawidlowo dzieki wektorom uzywanym w nowym systemie `InventoryDomain`.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Rozbudowa i mapowanie (Step-by-step extension guide)**: 
  1. Gdy musisz dodac np. "Slot Amuletu" zdefiniuj nowa stala slotu w C++ w `GameType.h`.
  2. Zmodyfikuj mapowanie offsetow w przestrzeni serwerowej z uzyciem stalych `INVENTORY_MAX_NUM` oraz `EQUIPMENT_MAX_NUM`.
  3. Konieczne jest zwiekszenie stalej rozmiaru bufora `InventoryDomain`, badz zmodyfikowanie wektora, jesli jest to nowy `InventoryWindow`.
  4. Dodaj obsluge mapy w `CPythonPlayer::MapItemPosToDomain`.
- **Jak debugowac i logowac**: Logowanie odbywa sie na ogol uzywajac `TraceError` badz nowoczesnego logera `EterBase::ModernLogger`. Warto debugowac przy pomocy printowania parametrow wejsciowych w module w `PythonNetworkStreamPhaseGameItem.cpp` (wysylanie opkodow) i `PythonPlayer.cpp` (synchronizacja pamieci).
- **Jak testowac (Headless)**: Pakiety domenowe moga byc izolowanie wywolywane z pominieciem warstwy Pythona na `Client::Gameplay::InventoryDomain`. Zeby to osiagnac, stworz fikcyjny harness (mock) ktory buduje struktury typu `ItemData` i wywoluj metody jak `AddItem`, `RemoveItem`, `SetItem`, unikajac instancjonowania CPythonApplication.
