---
task_id: "atlas_c03_10_shop_npc_system"
cluster: "ITM"
module_name: "CPythonShop - Sklepy NPC i Transakcje Handlowe"
target_files:
- src/UserInterface/PythonShop.cpp
- src/UserInterface/PythonShop.h
- src/Client/Gameplay/ShopPriceValidator.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c03_10_shop_npc_system.md"
architecture_layer: "Ekwipunek, Przedmioty, Handel i Gospodarka"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Modul `CPythonShop` pelni role menedzera stanow i magazynu danych dla systemow sklepow NPC oraz prywatnych sklepow graczy (tzw. tobolow/Private Shop). Odpowiada za przechowywanie dostepnych w sklepie przedmiotow (TShopItemData), obsluge wielu zakladek asortymentu w sklepach (np. sklep u handlarki posiada rozne kategorie) oraz definiowanie walut uzywanych w transakcjach. `ShopPriceValidator` waliduje po stronie klienta, czy gracza stac na zakup przedmiotow (np. Gold, Cheque, Gaya), zapobiegajac wysylaniu blednych pakietow.
- **Punkt zaczepienia:** Modul `CPythonShop` jest inicjalizowany jako Singleton podczas startu aplikacji. Dane do niego ladowane sa w fazie sieciowej (Network Tick / GC Packets) - pakiety `TPacketGCShop` wypelniaja asortyment i buduja strukture zakladek. Python w fazie Render/Update zaciaga te informacje i wyswietla w UI.
- **Data Flow:** 
  1. Otrzymanie pakietu z serwera przez `CPythonNetworkStream`.
  2. Mapowanie i zapis do wewnetrznej tablicy `m_aShoptabs` lub do `m_PrivateShopItemStock` za pomoca metod `SetItemData` i `AddPrivateShopItemStock`.
  3. UI w Pythonie pyta C++ przez zdefiniowane metody `PyMethodDef` (np. `shopGetItemPrice`, `shopGetItemCount`) o wlasciwosci w danym slocie.
  4. Przy probie zakupu przez uzytkownika, `ShopPriceValidator` upewnia sie, ze posiadane przez niego zasoby z `Core::WorldContext` sa wystarczajace, zwracajac wynik (np. pozostaly bilans) za pomoca `std::expected`.
- **Cykl zycia:** Alokacja odbywa sie przez szablon `CSingleton`. Zmienne sa inicjalizowane/czyszczone przed kazdym nowym otwarciem sklepu metoda `Clear()`. Przy zamknieciu metoda `Close()` resetuje stan zmiennych sklepowych, aby zwolnic wirtualny kontekst sesji sklepowej.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** 
  - Subsystem Python GUI: wywoluje funkcje `shop*` zdefiniowane w `PyMethodDef`.
  - Pakiety Sieciowe z Serwera (Game Server -> CPythonNetworkStream) wypelniajace struktury.
- **Zaleznosci wyjsciowe (Outbound):** 
  - `CPythonNetworkStream::Instance().SendBuildPrivateShopPacket` (do komunikacji GameServer).
  - Skrypty Python (`PyBridge::ExtractArgs`, `Py_BuildValue`) do zwracania informacji do UI.
- **Drzewo dyrektyw `#include`:** 
  - `src/UserInterface/PythonShop.h`: `#include "Packet.h"`, `string`, `map`.
  - `src/UserInterface/PythonShop.cpp`: `#include "stdafx.h"`, `"PythonShop.h"`, `"PythonNetworkStream.h"`, `"EterBase/PyBridge.h"`, `<algorithm>`.
  - `src/Client/Gameplay/ShopPriceValidator.h`: `#include "EterBase/StdAfx.h"`, `<cstdint>`, `<expected>`, `"../Core/WorldContext.h"`, `"../Core/DomainCommands.h"`, `"CurrencyType.h"`.
- **Model pamieciowy:** W CPythonShop glownie proste tablice alokowane statycznie (`SHOP_TAB_COUNT_MAX` * `SHOP_HOST_ITEM_MAX_NUM`) obok `std::map` do dynamicznego przechowywania przedmiotow sklepu graczy. `ShopPriceValidator` pracuje wylacznie na referencji do zewnetrznego kontekstu swiata bez wlasnych alokacji. Brak jawnych smart pointerow.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
| Nazwa | Rola | Wlasciciel Watku |
|-------|------|------------------|
| `CPythonShop` | Singleton obslugujacy sesje kupca (NPC/Gracz). | Main (Update/Python) |
| `ShopTab` (wewnetrzna) | Struktura z danymi o 1 karcie asortymentu (waluta, nazwa, do 40 przedmiotow) | CPythonShop |
| `ShopPriceValidator` | C++23 zero-conflict domain logic walidatora cen i zasobow (Gold, Gaya) | Main (Update) |

**Tabela Metod Publicznych (`CPythonShop`):**
| Sygnatura | Zwraca | Skutki Uboczne i Warunki |
|-----------|--------|--------------------------|
| `void Clear()` | `void` | Zeruje asortyment i stany sesji. Nalezy uzywac przy rozlaczeniu/zamknieciu UI. |
| `void SetItemData(uint8_t tabIdx, DWORD dwSlotPos, const TShopItemData & c_rShopItemData)` | `void` | Zapisuje asortyment na slocie. Moze nadpisac stara strukture bez dealokacji bo operuje na value-types. |
| `BOOL GetItemData(uint8_t tabIdx, DWORD dwSlotPos, const TShopItemData ** c_ppItemData)` | `BOOL` | Modyfikuje `c_ppItemData` by wskazac adres polozony na stercie w obiekcie singletona (nie zwalniac tego ptra). |
| `void AddPrivateShopItemStock(TItemPos ItemPos, uint8_t byDisplayPos, DWORD dwPrice)` | `void` | Moduluje mape `m_PrivateShopItemStock`. Zastepuje istniejacy rekord jesli klucz (`TItemPos`) wystepuje. |
| `void BuildPrivateShop(const char * c_szName)` | `void` | Sortuje asortyment i strzela pakietem `SendBuildPrivateShopPacket` przez Singleton Network. |

**Tabela Metod Publicznych (`ShopPriceValidator`):**
| Sygnatura | Zwraca | Skutki Uboczne i Warunki |
|-----------|--------|--------------------------|
| `std::expected<int64_t, Core::CommandError> ValidatePurchase(CurrencyType currency, int64_t unitPrice, uint32_t count)` | Zwraca pozostala sume waluty albo blad | Weryfikuje czy gracza stac na przedmiot i zapobiega wariacjom przepelnienia (overflow protection przed mnozeniem). Czysta funkcja bez stanow, dziala read-only. |

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
`CPythonShop` dziedziczy z `CSingleton`.
Posiada `m_isShoping`, `m_isPrivateShop`, `m_isMainPlayerPrivateShop` jako ciagle flagi BOOL (czyli int32_t). Nastepnie licznik tabow i ogromna tablice structow `ShopTab`. Kluczowe tablice statyczne gwarantuja contiguous memory i latwe pobieranie do Pythona w C-API bez cache-missow.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Sklep zalezy od pakietow budowania prywatnego sklepu w C++ (wywolywane za pomoca `CPythonNetworkStream::Instance().SendBuildPrivateShopPacket`). Serwer uzywa `TPacketGCShop` oraz `TPacketCGShop` (paczki z opcodami, np. header `HEADER_GC_SHOP`, `HEADER_CG_SHOP`).
- **Mostki Python C-API (`PyMethodDef` w C++ - Eksport do interfejsu klienta):**
  - `shop.Open(isPrivateShop, isMainPrivateShop)`
  - `shop.Close()`
  - `shop.IsOpen()`
  - `shop.IsPrivateShop()`
  - `shop.GetItemID(nPos)`
  - `shop.GetItemCount(iIndex)`
  - `shop.GetItemPrice(iIndex)`
  - `shop.GetItemMetinSocket(iIndex, iMetinSocketIndex)`
  - `shop.GetItemAttribute(iIndex, iAttrSlotIndex)`
  - `shop.GetTabCount()`
  - `shop.GetTabName(bTabIdx)`
  - `shop.GetTabCoinType(bTabIdx)`
  - **Prywatne Sklepy**: `shop.ClearPrivateShopStock()`, `shop.AddPrivateShopItemStock(...)`, `shop.DelPrivateShopItemStock(...)`, `shop.BuildPrivateShop(name)`.
Mapowanie opiera sie na uzywaniu `PyBridge::ExtractArgs` i uzyciu `Py_BuildValue` do zwracania informacji.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Ochrona przed wyciekami C-String:** Upewnij sie, ze Python API kopiuje zwracany string C w `GetTabName` bo zwrocony pointer znika jak sklep jest kasowany (wskazuje na c_str() std::string).
- **Crash Points:** 
  1. Otwieranie sklepu bez inicjalizacji Singletona `CPythonShop` (dostep do NULL Pointer).
  2. Przepelnienie `tabIdx` poza `SHOP_TAB_COUNT_MAX` powodujace SEGFAULT. Klasa w kodzie jednak dodala solidne zabezpieczenia brzegowe w wywolaniach (`if (tabIdx >= SHOP_TAB_COUNT_MAX)` logujace out of index za pomoca TraceError).
- **Zabezpieczenie przed overflow (ShopPriceValidator):** W `ValidatePurchase` jest celowe sprawdzanie, czy mnozenie ilosci i ceny nie obetnie pamieci `int64_t`, wyrzucajac `Core::CommandError::InvalidParameter`.
- **Wielowatkowosc:** Te klasy sa pisane do bycia single-threaded (wywolywane glownie w glownej petli UI CPython / Network Game Phase). Brak mutexow oznacza wylacznie synchroniczne modyfikacje z jednego miejsca - unikalbym pisania modyfikacji w background workerach.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Dodawanie nowej waluty dla sklepow (Step-by-step):**
  1. Dodaj typ do C++ enum `EShopCoinType` (w PythonShop.h) oraz w `Client::Gameplay::CurrencyType`.
  2. Zmodyfikuj `ShopPriceValidator::ValidatePurchase`, dodajac nowy case waluty sprawdzajacy nowy zasob na `Core::WorldContext`.
  3. Wyeksportuj stala w `initshop()` pod koniec `PythonShop.cpp` uzywajac `PyModule_AddIntConstant`.
- **Debug i Logi:** Kazdy wyjatek indeksowania jest wrzucany przez `TraceError`, a wiec wpadnie do glownego logu binarnego gry (najpewniej plik `syserr.txt` lub logi konsolowe z `EterBase::ModernLogger`). Zawsze po otwarciu sklepu sprawdz w debuggerze rozmiar `m_bTabCount`.
- **Testowanie `ShopPriceValidator` bez GUI (Unit Tests):** Kod validatora zostal w pelni odizolowany (Zero Conflict C++23), dlatego wystarczy zainkludowac jego naglowek w narzedziu uzywajacym `doctest` i stworzyc fake/mock dla `Core::WorldContext`.
