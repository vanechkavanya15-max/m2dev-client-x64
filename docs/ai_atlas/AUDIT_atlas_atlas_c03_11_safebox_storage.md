---
task_id: "atlas_c03_11_safebox_storage"
cluster: "ITM"
module_name: "CPythonSafeBox - Magazyn Dozorcy i Magazyn Item-Shop"
target_files:
- src/UserInterface/PythonSafeBox.cpp
- src/UserInterface/PythonSafeBox.h
- src/Client/Gameplay/SafeboxCommandHandler.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c03_11_safebox_storage.md"
architecture_layer: "Ekwipunek, Przedmioty, Handel i Gospodarka"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport AI Atlas: CPythonSafeBox - Magazyn Dozorcy i Magazyn Item-Shop

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Modul `CPythonSafeBox` (oraz powiazany nowoczesny `SafeboxCommandHandler`) zarzadza stanem i logika dostepu do Magazynu Dozorcy (SafeBox) oraz Magazynu Item-Shop (Mall) w kliencie gry. Zapewnia:
- Przechowywanie przedmiotow na serwerze z podzialem na komorki (sloty).
- Obsluge autoryzacji haslem dostepu do magazynu.
- Transfer i zarzadzanie waluta (Yang / Money) z i do magazynu.
- Dostep do przedmiotow zakupionych za posrednictwem platformy Item-Shop bezposrednio w grze.
- Mapowanie logiki serwerowej (pakiety z przedmiotami) na struktury pamieci klienta.
- Bezpieczny i ujednolicony dostep do tych stanow za posrednictwem Python C-API dla interfejsu graficznego (UI).

**Przeplyw danych (Data Flow) i Cykl zycia:**
1. **Inicjalizacja:** Obiekt `CPythonSafeBox` dziala jako Singleton, jest tworzony podczas ladowania aplikacji. `SafeboxCommandHandler` to nowoczesna warstwa domenowa (Client::Gameplay), ktora wspolpracuje z zasobami `InventoryDomain` i bazowym `SafeBox` (TradeDomain).
2. **Otwarcie (On Network Event):** Na odebranie pakietow konfiguracyjnych (rozmiar magazynu, haslo zatwierdzone), `CPythonSafeBox` ulega "otwarciu" (metoda `OpenSafeBox` / `OpenMall`), gdzie wektory danych przedmiotow (`std::vector<TItemData>`) sa alokowane i czyszczone zerami (zero-init).
3. **Modyfikacja:** Przychodza pakiety `SAFEBOX_SET`, `SAFEBOX_DEL`, lub `SAFEBOX_MONEY_CHANGE`, uaktualniajac stan przedmiotow lub waluty w kliencie za pomoca metod `SetItemData`, `DelItemData`, `SetMoney`.
4. **Odczyt (On UI Render / Event):** Skrypty Pythona (glownie interfejs w plikach takich jak `uiSafebox.py`) cyklicznie odczytuja stan za pomoca eksponowanych metod API (np. `safebox.GetItemID`, `safebox.GetItemCount`), prezentujac zasoby graczowi. Nowoczesny `SafeboxCommandHandler` realizuje domene transakcyjna (przenoszenie z/do inwentarza) po stronie nowej architektury.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

- **Zaleznosci wejsciowe (Inbound):**
  - Siec: Faza gry (`src/UserInterface/PythonNetworkStreamPhaseGameItem.cpp` - odbiera zdarzenia takie jak set, del, wrong password, money change).
  - Skrypty Pythona: Interfejs UI `uiSafebox.py`, `uiMall.py` uzywajace zaeksponowanego modulu C `safebox`.
  - Architektura domenowa: Nowe systemy transakcji `InventoryDomain`, obsluga komend sieciowych, weryfikacja przez `SafeboxCommandHandler`.

- **Zaleznosci wyjsciowe (Outbound):**
  - Standardowe logowanie: `TraceError` dla blednych slotow (legacy), `EterBase::ModernLogger` w domenie.
  - Wywolywanie metod interfejsu Pythona (callbacks z kodu sieciowego) `OnSafeBoxError`, `RefreshSafeboxMoney`, `RefreshSafebox`.
  - Powiazanie z `Client::Gameplay::TradeDomain` dla bazowego kontenera `SafeBox` w nowej architekturze.

- **Drzewo dyrektyw `#include`:**
  - `PythonSafeBox.h` zawiera standardowe biblioteki (vector) oraz typy dziedziczone z naglowkow silnika. Zalezy od `CSingleton`, `TItemData`.
  - `SafeboxCommandHandler.h` zalezy od `<string_view>`, `<format>`, `<cstdint>`, `<expected>`, `EterBase/Result.h`, `EterBase/StrongTypes.h`, `InventoryDomain.h`, `TradeDomain.h`.
  - **Model pamieciowy:** `CPythonSafeBox` przechowuje zasoby w bezposrednich strukturach liniowych `std::vector<TItemData>`. `SafeboxCommandHandler` trzyma instancje jako czyste referencje (`InventoryDomain&`, `SafeBox&`), wiec jest typowym handlerem bez praw wlasnosci.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### CPythonSafeBox
- **Rola:** Singleton przechowujacy stan magazynu u klienta. Ekspozycja do Pythona.
- **Zmienne uzyteczne:** `SAFEBOX_SLOT_X_COUNT` = 5, `SAFEBOX_SLOT_Y_COUNT` = 9, `SAFEBOX_PAGE_SIZE` = 45.
- **Pola czlonkowskie:**
  - `TItemInstanceVector m_ItemInstanceVector;` - offset wektora z przedmiotami magazynu
  - `TItemInstanceVector m_MallItemInstanceVector;` - offset wektora z przedmiotami Item-Shop
  - `DWORD m_dwMoney;` - zgromadzona gotowka (Yang)
- **Metody Publiczne:**
  - `void OpenSafeBox(int iSize);` - Czysci i alokuje pamiec dla magazynu (X_COUNT * iSize).
  - `void SetItemData(DWORD dwSlotIndex, const TItemData & rItemData);` - Aktualizuje element struktury TItemData, weryfikuje zakres (bounds check).
  - `void DelItemData(DWORD dwSlotIndex);` - Wypelnia zerami TItemData podanym w indeksie.
  - `void SetMoney(DWORD dwMoney);`, `DWORD GetMoney();` - Setter/Getter waluty.
  - Oraz adekwatne metody dla modulu `Mall` (np. `OpenMall`, `SetMallItemData`, `DelMallItemData`, `GetMallSize`).

### Client::Gameplay::SafeboxCommandHandler
- **Rola:** Domena odpowiedzialna za przenoszenie miedzy inwentarzem a magazynem, abstrakcja C++23.
- **Pola czlonkowskie:** `InventoryDomain& m_inventory`, `SafeBox& m_safebox`, `bool m_isOpen`.
- **Metody Publiczne:**
  - `EterBase::Result<void, CommandError> ValidatePassword(std::string_view password);`
  - `EterBase::Result<void, CommandError> MoveItemToSafebox(EterBase::ItemSlot inventorySlot, EterBase::ItemSlot safeboxSlot);` - Przenoszenie z inwentarza do magazynu z zachowaniem ACID dla zasobow.
  - `EterBase::Result<void, CommandError> MoveItemToInventory(EterBase::ItemSlot safeboxSlot, EterBase::ItemSlot inventorySlot);` - Transakcja powrotna z walidacja pojemnosci i obecnosci przedmiotow.
  - `EterBase::Result<void, CommandError> Close();` - Zamyka stan magazynu w domenie.

### Client::Gameplay::CommandError (Enum Class)
- `None, InvalidPassword, ItemNotFound, InventoryFull, SafeboxFull, InvalidSlot, NotOpened, AlreadyOpened`

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

- **Opcody PAKIETOW SIECIOWYCH (Header ID):**
  - GC: `HEADER_GC_SAFEBOX_SET` = 0x39 (w BeaviumProtocol jako 0x39)
  - GC: `HEADER_GC_SAFEBOX_DEL` = 0x3A (w BeaviumProtocol jako 0x3A)
  - GC: `HEADER_GC_SAFEBOX_WRONG_PASSWORD` = 0x3B (w BeaviumProtocol jako 0x3B)
  - GC: `HEADER_GC_SAFEBOX_SIZE` = 0x3C (w BeaviumProtocol jako 0x3C)
  - CG/GC: `SAFEBOX_MONEY_CHANGE` = 0x0834 / `SAFEBOX_MONEY` = 0x0823
  - CG: `SAFEBOX_CHECKIN` = 0x0820, `SAFEBOX_CHECKOUT` = 0x0821, `SAFEBOX_ITEM_MOVE` = 0x0822

- **Python C-API (`PyMethodDef s_methods[]` w module `safebox`):**
  - `safebox.GetCurrentSafeboxSize()` -> `(int)` rozmiar magazynu
  - `safebox.GetItemID(int slot)` -> `(int vnum)`
  - `safebox.GetItemCount(int slot)` -> `(int count)`
  - `safebox.GetItemFlags(int slot)` -> `(int flags)`
  - `safebox.GetItemMetinSocket(int slot, int socketIndex)` -> `(int socketValue)`
  - `safebox.GetItemAttribute(int slot, int attrIndex)` -> `(int type, int value)`
  - `safebox.GetMoney()` -> `(int)`
  - Oraz adekwatne funkcje prefiksowane `GetMall` (np. `GetMallItemID`, `GetMallItemCount`).

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Zasady wielowatkowosci:** Modul nie uzywa wbudowanej synchronizacji (muteksow). Pakiety z sieci trafiaja i sa przetwarzane bezposrednio, zaklada sie, ze dostep do Singletonu `CPythonSafeBox` pochodzi z glownego watku gry, a `SafeboxCommandHandler` to komponent bezstanowy. Uruchamianie tego kodu z watku pobierania danych moze spowodowac Race Condition na std::vector.
- **Potencjalne punkty awarii:** Odwolania poza tablice (out-of-bounds). Przechwytuje je warunek `dwSlotIndex >= m_ItemInstanceVector.size()` wypisujacy `TraceError`. W `SafeboxCommandHandler` blad taki mapowany jest na bezpieczny modern typ `EterBase::Result` i enum `CommandError::InvalidSlot`.
- **Transakcje Domenowe (Zasady SOLID i ACID):** W klasie `SafeboxCommandHandler` zastosowano probny commit/rollback. Jesli inwentarz nie moze wlozyc przedmiotu usunietego uprzednio z magazynu, zachodzi roll-back do magazynu. Operacje te nie zrzucaja programu.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zidentyfikuj pakiet w `src/Client/Network/Protocol/ProtocolOpcodes.h`.
  2. Jesli to wplywa na Python, dodaj argument i offset w wektorze `TItemData`. Dodaj handler w Python API w `src/UserInterface/PythonSafeBox.cpp`.
  3. Zmodyfikuj `SafeboxCommandHandler.cpp` dla logiki lokalnej/domenowej. Dodaj uzycie `EterBase::ModernLogger::Info` jezeli jest to udana transakcja. Zaktualizuj plik `uiSafebox.py` z wykorzystaniem nowego mapowania C-API.

- **Jak debugowac i logowac:**
  - Standardowe logi pojawiaja sie poprzez wbudowany w modern interfejs `EterBase::ModernLogger::Info("SafeboxCommandHandler::... ")`. Sprawdzaj offsety dla blednych komend pakietu (nieudany checkin/checkout objawi sie jako log Error z Handler'a).

- **Jak testowac (Headless / Unit Test Harness):**
  - Poniewaz `SafeboxCommandHandler` oparty jest na Dependency Injection (zalezy od abstrakcyjnych / zewnetrznych refow jak `InventoryDomain&`), wystarczy zmockowac lub zasymulowac instancje obydwu warstw. Mozna symulowac scenariusze wyczerpania przestrzeni inwentarza wywolujac operacje przeniesienia (MoveItemToInventory) i oczekiwac wartosci `unexpected(CommandError::InventoryFull)`. Mozna testowac bez interfejsu graficznego.
