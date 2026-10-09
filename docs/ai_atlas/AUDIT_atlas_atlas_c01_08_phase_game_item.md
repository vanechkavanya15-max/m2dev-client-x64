---
task_id: "atlas_c01_08_phase_game_item"
cluster: "NET"
module_name: "Obsluga Pakietow Przedmiotow i Ekwipunku (PhaseGame Item)"
target_files:
- src/UserInterface/PythonNetworkStreamPhaseGameItem.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c01_08_phase_game_item.md"
architecture_layer: "Silnik Sieciowy, Maszyna Faz i Protokol"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Plik `PythonNetworkStreamPhaseGameItem.cpp` stanowi scisly most pomiedzy systemem sieciowym klienta (odbior/wysylka danych) a systemem logiki uzytkownika i zarzadzaniem ekwipunkiem (Python/C++ UI i CPythonItem). Jego glownym celem jest zapewnienie prawidlowej synchronizacji stanu przedmiotow i ich interakcji pomiedzy klientem a serwerem gry w fazie rozgrywki (PhaseGame).

**Rola w architekturze:**
- **Serializacja/Deserializacja:** Konwertuje abstrakcyjne obiekty gry (takie jak podnoszenie przedmiotu, przesuwanie w ekwipunku, zakupy, przenoszenie pomiedzy SafeBox a ekwipunkiem) na konkretne surowe ramki danych z uzyciem stalych opcode'ow sieciowych (GC/CG).
- **Zarzadzanie stanem klienta:** Wywoluje metody zaktualizujace lokalny model danych (np. `IAbstractPlayer::SetItemData`), bezposrednio wplywajac na to, co uzytkownik widzi w oknie ekwipunku, trejdu, sklepu, SafeBox i Mall.
- **Routing pakietow:** Oprocz obslugi klasycznej petli klienta, plik zawiera nowe routingi uzywajace `NetworkStreamPhaseGameBridge` z wykorzystaniem bezpiecznego modulu `std::span` z C++20 w celu weryfikacji i rutowania poszczegolnych pakietow.
- **Interfejsy z UI:** Wywoluje funkcje graficzne Pythona via `PyCallClassMemberFunc` (np. do odswiezenia okien lub grania dzwiekow), czyniac klienta "zywym".

**Przeplyw Danych (Data Flow) i Sterowania (Control Flow):**
1. Akcja uzytkownika (np. klikniecie prawym przyciskiem myszy w celu uzycia przedmiotu) aktywuje funkcje w API Pythona lub C++.
2. Wywolywana jest odpowiednia funkcja wysylajaca (np. `CPythonNetworkStream::SendItemUsePacket`). Przed wyslaniem sprawdzane sa wazne inwarianty (np. czy uzytkownik nie jest w trakcie walki `__IsPlayerAttacking()` albo czy nie wymienia sie / kupuje `CPythonExchange::isTrading()`).
3. Ramka trafia do bufora wyjsciowego (metoda `Send`).
4. Odpowiedz serwera (np. usuniecie przedmiotu po jego uzyciu) wyzwala `CPythonNetworkStream::RecvItemDelPacket`.
5. Bufor z odpowiednimi polami pakietu jest mapowany (deserialize). Modul routera wywoluje odpowiednie bridge handler dla pakietu.
6. Stan gry aktualizuje sie (np. `IAbstractPlayer::GetSingleton().SetItemData()`).
7. Wywolywana jest lokalna funkcja odswiezajaca (np. `__RefreshInventoryWindow()`) ktora wysyla sygnal do zaktualizowania kontrolek na ekranie.

**Cykl zycia (Lifecycle):**
Pakiet sieciowy nie ma swojego wyjatkowego cyklu zycia jako obiekt biznesowy. Pakiety to krotkotrwale struktury (przewaznie structy typu TPacketCGItemMove alokowane na stosie), mapowane do pamieci z recv/send buffora po czym gina. Zmiany dotycza globalnego/singletonowego cyklu zycia jednostek zaleznosci, tj. CPythonNetworkStream (zarzadzany per caly czas gry), IAbstractPlayer, CPythonSafeBox, itp.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
- Modul wywolywany z glownej petli sieci `CPythonNetworkStream` podczas pobierania paczek.
- Wywolywany bezposrednio przez API z Pythona poprzez warstwe tlumaczaca do interakcji GUI -> serwer.
- Serwer Gry: Pakiety sieciowe (naglowki od 0x01 do opcodow jak np. ITEM_SET).

**Zaleznosci wyjsciowe (Outbound):**
- `IAbstractPlayer` (AbstractPlayer.h) - do aktualizacji atrybutow przedmiotow u gracza.
- `CPythonItem` (PythonItem.h) - wyzwalanie eventow dla itemow (np. play sound).
- `CPythonShop` (PythonShop.h), `CPythonExchange` (PythonExchange.h), `CPythonSafeBox` (PythonSafeBox.h).
- `CPythonCharacterManager` (PythonCharacterManager.h) - dla efektow postaci.
- `GameLib/ItemManager.h`.
- `Network::Dispatchers::NetworkStreamPhaseGameBridge` - uzywany do nowszej architektury z C++20 routowania paczek gier.
- Subsystem graficzny okienek UI przez skryptowe wolanie w Pythonie: `m_apoPhaseWnd[PHASE_WINDOW_GAME]`, czyli srodowisko okien Py.

**Drzewo dyrektyw `#include`:**
```cpp
#include "StdAfx.h"
#include "PythonNetworkStream.h"
#include "PythonItem.h"
#include "PythonShop.h"
#include "PythonExchange.h"
#include "PythonSafeBox.h"
#include "PythonCharacterManager.h"
#include "AbstractPlayer.h"
#include "GameLib/ItemManager.h"
#include "Network/Dispatchers/NetworkStreamPhaseGameBridge.h"
```
Nie wykryto naruszen ryzyka cyklicznych zaleznosci, jednak plik polega silnie na Singletonach (GetSingleton(), Instance()).

**Model pamieciowy:**
Silne poleganie na czystych wskaznikach, jednak wykorzystywana jest konwencja struktur alokowanych na stosie podczas pakietowania. Najnowsze elementy uzywaja modulu `std::span` z C++20 w celu weryfikacji i czytania blokow payloadu. Nie wykorzystuje bezposrednionew/delete z C++.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

#### Tabela Metod Publicznych (na podstawie namespace CPythonNetworkStream):

| Metoda C++ | Typ Zwracany | Argumenty | Skutki Uboczne i Pre-conditions |
| :--- | :--- | :--- | :--- |
| `SendSafeBoxMoneyPacket` | `bool` | `BYTE byState, DWORD dwMoney` | Zawiera assert i zawsze uderza `assert(!"CPythonNetworkStream::SendSafeBoxMoneyPacket")`. |
| `SendSafeBoxCheckinPacket` | `bool` | `TItemPos InventoryPos, BYTE bySafeBoxPos` | Gra dzwiek DropSound, wysyla `TPacketCGSafeboxCheckin`. |
| `SendSafeBoxCheckoutPacket` | `bool` | `BYTE bySafeBoxPos, TItemPos InventoryPos` | Gra dzwiek DropSound, wysyla `TPacketCGSafeboxCheckout`. |
| `SendSafeBoxItemMovePacket` | `bool` | `BYTE bySourcePos, BYTE byTargetPos, BYTE byCount` | Wysyla `TPacketCGItemMove` z naglowkiem `SAFEBOX_ITEM_MOVE`. |
| `RecvSafeBoxSetPacket` | `bool` | `()` | Czyta `TPacketGCItemSet`, updatuje SafeBox i odswieza UI `__RefreshSafeboxWindow`. |
| `RecvSafeBoxDelPacket` | `bool` | `()` | Czyta `TPacketGCItemDel`, usuwa w SafeBox, odswieza UI. |
| `RecvSafeBoxWrongPasswordPacket` | `bool` | `()` | Wywoluje funkcje UI `OnSafeBoxError` z poziomu Pythona. |
| `RecvSafeBoxMoneyChangePacket` | `bool` | `()` | Aktualizuje gotowke w `CPythonSafeBox`, wola `RefreshSafeboxMoney` na UI. |
| `SendMallCheckoutPacket` | `bool` | `BYTE byMallPos, TItemPos InventoryPos` | Wysyla `TPacketCGMallCheckout`, odtwarza dzwiek. |
| `RecvMallOpenPacket` | `bool` | `()` | Otwiera UI uzywajac `OpenMallWindow`. |
| `RecvMallItemSetPacket` | `bool` | `()` | Ustawia item i wola `__RefreshMallWindow`. |
| `RecvMallItemDelPacket` | `bool` | `()` | Usuwa z mall i wola `__RefreshMallWindow`. |
| `RecvItemDelPacket` | `bool` | `()` | Routing przez bridge (C++20), czysci `TItemData`, wywoluje `__RefreshInventoryWindow`. |
| `RecvItemSetPacket` | `bool` | `()` | Routing przez bridge, update `TItemData` poprzez `IAbstractPlayer`, odswieza GUI. |
| `RecvItemUsePacket` | `bool` | `()` | Routing przez bridge, pobiera item. |
| `RecvItemUpdatePacket` | `bool` | `()` | Brak jawnego routowania widocznego na zewnatrz w naglowku (trzeba czytac), update itemu. |
| `RecvItemGroundAddPacket` | `bool` | `()` | Routing przez bridge, aktualizuje podloze o item (Drop). |
| `RecvItemOwnership` | `bool` | `()` | Routing przez bridge, obsluga item ownership (dla imion zlodzieja). |
| `RecvItemGroundDelPacket` | `bool` | `()` | Routing przez bridge, usuwa item z podlogi. |
| `RecvQuickSlotAddPacket` | `bool` | `()` | Odbior paczki QuickSlotAdd i odswiezenie inwentarza. |
| `RecvQuickSlotDelPacket` | `bool` | `()` | Odbior paczki QuickSlotDel i odswiezenie inwentarza. |
| `RecvQuickSlotMovePacket` | `bool` | `()` | Odbior paczki QuickSlotMove i odswiezenie inwentarza. |
| `SendShopEndPacket` | `bool` | `()` | Wysylka eventu zamkniecia sklepu. |
| `SendShopBuyPacket` | `bool` | `BYTE bPos` | Wysylka zadania zakupu przedmiotu w danym slocie. |
| `SendShopSellPacket` | `bool` | `BYTE bySlot` | Zglasza proba sprzedazy wg starej wersji (1 packet). |
| `SendShopSellPacketNew` | `bool` | `BYTE bySlot, BYTE byCount` | Nowa proba ze sztukami (count). |
| `SendItemUsePacket` | `bool` | `TItemPos pos` | Blokuje w trakcie handlu (isTrading), sklepu lub w trakcie walki `__IsPlayerAttacking`. Wysyla `TPacketCGItemUse`. |
| `SendItemUseToItemPacket` | `bool` | `TItemPos source_pos, TItemPos target_pos` | Podobne blokady. Zwraca false i wyrzuca error jesli nie `Send()`. |
| `SendItemDropPacket` | `bool` | `TItemPos pos, DWORD elk` | Przestarzaly drop (brak count). Blokuje podczas handlu/sklepu. |
| `SendItemDropPacketNew` | `bool` | `TItemPos pos, DWORD elk, DWORD count` | Wysyla `TPacketCGItemDrop` dla podanej ilosci, chronionesame way as `SendItemUsePacket`. |
| `SendItemMovePacket` | `bool` | `TItemPos pos, TItemPos change_pos, BYTE num` | Weryfikuje stany UI zablokowanych operacji, wysyla powiadomienie `CANNOT_EQUIP_EXCHANGE`. |
| `SendItemPickUpPacket` | `bool` | `DWORD vid` | Podnoszenie. |
| `SendQuickSlotAddPacket` | `bool` | `BYTE wpos, BYTE type, BYTE pos` | CG_QUICKSLOT_ADD |
| `SendQuickSlotDelPacket` | `bool` | `BYTE pos` | CG_QUICKSLOT_DEL |
| `SendQuickSlotMovePacket` | `bool` | `BYTE pos, BYTE change_pos` | CG_QUICKSLOT_SWAP |
| `RecvSpecialEffect` | `bool` | `()` | Czyta pakiet `TPacketGCSpecialEffect`, uzywa `CPythonItem::PlayUsePotionSound`, aplikuje efekty z `SE_*`. |
| `RecvSpecificEffect` | `bool` | `()` | Obsluguje nakladanie plikowych (po plikach) efektow vizualnych do obiektow VID (Instance). |
| `RecvDragonSoulRefine` | `bool` | `()` | Czyta pakiet alchemii z `DragonSoulSub::*`, wola funkcje `BINARY_DragonSoulRefineWindow_RefineFail`, `RefineSucceed`. |

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
Pakietowanie korzysta ze scislych struktur jak `TItemData`. Przeprowadzono refaktoryzacje tak, aby korzystac z `std::span` podczas routowania pakietow `RouteGamePacket(..., std::span<const uint8_t> payload)`, co uniemozliwia ataki typu Buffer Overflow w nowym kodzie odbierajacym, gdzie bridge'owanie mapuje te wartosci.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

**Pakiety Sieciowe (zidentyfikowane po opcodach - enum z przestrzeni CG / GC):**
- **Client -> Game (CG):** `SAFEBOX_CHECKIN`, `SAFEBOX_CHECKOUT`, `SAFEBOX_ITEM_MOVE`, `MALL_CHECKOUT`, `SHOP`, `ITEM_USE`, `ITEM_USE_TO_ITEM`, `ITEM_DROP`, `ITEM_MOVE`, `ITEM_PICKUP`, `QUICKSLOT_ADD`, `QUICKSLOT_DEL`, `QUICKSLOT_SWAP`.
- **Game -> Client (GC):** `SAFEBOX_WRONG_PASSWORD`, `SAFEBOX_MONEY_CHANGE`, `MALL_OPEN`, `ITEM_DEL`, `ITEM_SET`, `ITEM_GET`, `ITEM_USE`, `ITEM_UPDATE`, `ITEM_GROUND_ADD`, `ITEM_OWNERSHIP`, `ITEM_GROUND_DEL`, `QUICKSLOT_ADD`, `QUICKSLOT_DEL`, `QUICKSLOT_SWAP`, `SPECIAL_EFFECT`, `SPECIFIC_EFFECT`, `DRAGON_SOUL_REFINE`.

**Metody Pythona i API UI (`PyCallClassMemberFunc` targets):**
Nie wykryto `PyMethodDef` bezposrednio w tym pliku. C++ API wola jednak UI nastepujacymi stringami zdarzen (ktore UI musi posiadac/nasluchiwac w obiekcie skryptowym `game.py`):
- `"BINARY_DragonSoulRefineWindow_Open"`, `"BINARY_DragonSoulRefineWindow_RefineFail"`, `"BINARY_DragonSoulRefineWindow_RefineSucceed"`
- `"BINARY_Highlight_Item"`, `"BINARY_ItemGet"`, `"BINARY_ItemGetFromParty"`, `"BINARY_ItemDeliverToParty"`
- `"OnSafeBoxError"`, `"RefreshSafeboxMoney"`, `"OpenMallWindow"`
- `"BINARY_AppendNotifyMessage"` - uzywany do bindowania komunikatow jak `"CANNOT_EQUIP_EXCHANGE"` lub `"CANNOT_EQUIP_SHOP"`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

**Zasady wielowatkowosci:**
Calosc logiki dziala na glownym watku, silnie spleciona ze srodowiskiem UI i renderowania (Python GUI calls, `__RefreshInventoryWindow()`). Nie mozna wywolywac tych paczek asynchronicznie poza watkiem Main (ze wzgledu na manipulacje GUI), inaczej doszloby do race conditions i crashy w Python / C++ CPythonNetworkStream map.

**Potencjalne punkty awarii (Crash Points & Edge Cases):**
1. **Atak z falszywymi paczkami UI:** Nie autoryzowane klikniecia wymuszaja blokady przed `SendItemMovePacket` np. `if (CPythonExchange::Instance().isTrading())`. Agent musi absolutnie pamietac aby przed dodaniem akcji wylaczyc je podczas Trade (Zapobieganie kopiowaniu itemow/duping).
2. **Crash w SafeBoxMoney:** `SendSafeBoxMoneyPacket` posiada `assert(!"CPythonNetworkStream::SendSafeBoxMoneyPacket");` ktory po prostu spowowduje zamkniecie aplikacji (lub ignoracje w release, zwracajac z falszem). Funkcja ta jest celowo zamockowana / ukatrupiona.
3. **Nie obsluzone VNUM dla item_get:** Wywala trace error: `TraceError("CPythonNetworkStream::RecvItemGetPacket - Unknown item vnum %u", packet.dwItemVnum);`

**Zarzadzanie zasobami (RAII):**
Kod C-Style strukturalny (TItemData). Uzywane jest `memset(&kItemData, 0, sizeof(TItemData));` Zmiany te musza byc w 100% kompatybilne bajtowo, zero padding miedzy flagami i macierzami (sockets, attrs). Przestarzale `RecvItem*Packet` powoli przemieszczaja swoje logiki przez uzycie `NetworkStreamPhaseGameBridge` na zewnatrz tego modulu uzywajac `std::span` (patrz implementacje bridge dla GC::ITEM_DEL). W zwiazku z tym, nie nalezy juz dodawac bezposrednich handlow payloadu w ciele funkcji RecvItem.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
1. **Protokol:** Jesli serwer wprowadzil nowa operacje (np. "ITEM_LOCK"), nie implementuj obslugi parsera naglowka recznie z rzutowaniemstarego typu `Recv()`. Zamiast tego zaimplementuj nowoczesny C++23 `RouteGamePacket` bridge i przekaz jako `std::span` z payloadem. Zobacz jak zaimplementowano to z `GC::ITEM_SET`.
2. **Restrykcje WUI:** Jesli modyfikujesz mechanike uzywania (`SendItemUsePacket`) itemow, musisz zachowac bloki na obsluge handlu (CPythonExchange) oraz ataki (`__IsPlayerAttacking()`).
3. **UI Sync:** Kazda zmiana stanu ekwipunku musi wymusic odswiezenieekranu po updejcie struktury `IAbstractPlayer::SetItemData`. Wywolaj wtedy `__RefreshInventoryWindow()`.

**Jak debugowac i logowac:**
Uzywaj`TraceError` (lub `Tracen`) jezeli chcesz zrzucic bledydo`syserr.txt`clienta. Metody przesylania zwracajabool, jednak jesli majaproblemrobia `Tracen("SendQuickSlotSwapPacket Error")`. Przy sprawdzaniu czy kod idziepythona, uzyj break pointdlastringow UI tj. `"CANNOT_EQUIP_EXCHANGE"`.

**Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
Jezeli robisz testy jednostkowe: Zmockuj `IAbstractPlayer` i`CPythonExchange` (uzywajpatternu z `#ifndef`). Przechwyc `PyCallClassMemberFunc` poniewaz przy wywolaniu testu Headless te pointery zwroca Nullowe Referencje naUI zPythona (konkretnie`m_apoPhaseWnd[PHASE_WINDOW_GAME]` na brakujacy PyObject bedzie powodowal segmentation faults). Przetestuj deserializacje paczek poprzez generowanie binarnych stringow reprezentujacychpaczke(przykladTItemMove).
