---
task_id: "atlas_c09_11_py_trade_modules"
cluster: "PY"
module_name: "Moduly Pythona 'shop', 'exchange', 'safebox' - Transakcje"
target_files:
- src/UserInterface/PythonShopModule.cpp
- src/UserInterface/PythonExchangeModule.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c09_11_py_trade_modules.md"
architecture_layer: "Mostek Pythona, Moduly C-API i Skrypty Gry"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Moduly Pythona `shop`, `exchange` i `safebox` (ktore w ukladzie zrodlowym znajduja sie w plikach takich jak `PythonExchangeModule.cpp`, `PythonShop.cpp`, `PythonSafeBox.cpp` - pod nazwami modulow C-API) pelnia kluczowa role w interfejsie uzytkownika klienta gry Metin2. Zapewniaja one mostek pomiedzy warstwa silnika C++ a skryptami Python odpowiedzialnymi za wyswietlanie okiem handlu (wymiana pomiedzy graczami), sklepow (NPC i prywatnych) oraz magazynu (skrytka bankowa i magazyn ItemShop - Mall). 

Gdy uzytkownik wchodzi w interakcje ze swiatem gry, np. otwiera okno handlu, silnik C++ aktualizuje wewnetrzne struktury danych (np. `CPythonExchange`), a nastepnie interfejs Python (np. `uiExchange.py`) pobiera biezacy stan korzystajac z funkcji wyeksportowanych w modulach `shop`, `exchange` i `safebox`. Metody te sa w wiekszosci typu "getter" pobierajacymi dane bezposrednio ze struktur, ewentualnie typu "setter/action" (np. `SetElkMode` czy `BuildPrivateShop`) inicjujacymi odpowiednie zlecenia do serwera (wysylajac pakiety po sieci) bacz aktualizujacymi stan lokalny. Cykl zycia obiektow opiera sie na zywotnosci instancji singletonow (np. `CPythonExchange::Instance()`), ktore sa aktywne tak dlugo, jak trwa dzialanie aplikacji lub odpowiednia faza gry. Alokacja danych odbywa sie zazwyczaj po stronie odbierania pakietow, a te moduly pelnia wylacznie role interfejsu do odczytu i drobnych modyfikacji. Przeplyw sterowania przebiega glownie od warstwy UI (Python) poprzez C-API z powrotem do UI lub do wyslania pakietu sieciowego. Kod jest wywolywany asynchronicznie, glownie przez petle zdarzen UI w ramach OnUpdate i obslugi eventow wejsciowych klawiatury/myszy.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

- **Zaleznosci wejsciowe (Inbound):** Moduly sa wywolywane z warstwy Python GUI, a dokladniej ze skryptow z folderu `uiscript` oraz systemowych `ui*.py` (np. `uiExchange.py`, `uiShop.py`, `uiSafebox.py`). Sa one ladowane podczas inicializacji klienta przez wywolanie funkcji `initTrade`, `initshop`, `initsafebox`. Czasami powiadomienia do UI inicjowane sa eventami sieciowymi.
- **Zaleznosci wyjsciowe (Outbound):** Moduly komunikuja sie z systemem okien Pythona (wykorzystuja `Py_BuildValue`, `PyArg_ParseTuple`, itp.). W zaleznosci od funkcji wywoluja tez metody logiki klienta za posrednictwem klas C++ takich jak `CPythonExchange`, `CPythonShop`, i `CPythonSafeBox`. Dodatkowo, operacje budowania sklepu wywoluja komunikacje sieciowa poprzez inne obiekty odpowiedzialne za wysylanie.
- **Drzewo dyrektyw `#include`:** W kodzie widoczne sa `stdafx.h`, `PythonExchange.h`, `PythonShop.h`, `PythonSafeBox.h`, co laczy te moduly z glownymi plikami implementacji stanu, ale i naraza na ewentualne cykle w architekturze wielkiego naglowka (stdafx.h).
- **Model pamieciowy:** Przede wszystkim wzorzec Singleton (`T::Instance()`), rzutowanie przez referencje (`&pos`, `&byType`, `&sValue`) do odbierania danych. Wiekszosc wyluskiwan opiera sie o wyciaganie argumentow z krotek Pythona (`PyTuple_GetInteger`), w ktorych uzywane sa standardowe zmienne (typy prymitywne, raw pointers `const char *`). Brak intensywnego stosowania smart pointerow w obrebie wywolan mostu Pythona.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

Moduly eksportuja zestawy funkcji w C, ktore sa mapowane na funkcje Pythona.
W ponizszym zestawieniu uwzgledniono kluczowe definicje:

### Tabela Klas i Struktur
Te pliki operuja glownie funkcjami statycznymi, nie deklaruja wlasnych klas C++ z wyjatkiem struktury tablicy eksportu:

- `PyMethodDef s_methods[]`: W kazdym module przechowuje pary Nazwa_Python -> Funkcja_C++, uzywana podczas `Py_InitModule()`. Nalezy do zasiegu pojedynczego pliku (lub calej aplikacji poprzez Pythona).

### Tabela Metod Publicznych (C-API)

**Modul `exchange`:**
- `exchangeInitTrading(poSelf, poArgs)` -> PyObject*: Konczy wczesniejszy stan handlu `End()`.
- `exchangeisTrading(poSelf, poArgs)` -> PyObject* (int): Zwraca flage aktywnosci.
- `exchangeGetElkFromSelf(poSelf, poArgs)`, `exchangeGetElkFromTarget(poSelf, poArgs)` -> PyObject* (int): Zwracaja zloto/yang oferowane przez odpowiednia strone.
- `exchangeGetAcceptFromSelf(...)`, `exchangeGetAcceptFromTarget(...)` -> PyObject* (int): Flagi akceptacji obu stron.
- `exchangeGetItemVnumFromSelf(poSelf, poArgs)`, `exchangeGetItemVnumFromTarget(poSelf, poArgs)` -> PyObject* (int): Pobieraja identyfikatory (vnum) przedmiotow z konkretnego okna po indeksie `pos`.
- `exchangeGetItemCountFromSelf(...)`, `exchangeGetItemCountFromTarget(...)` -> PyObject* (int): Ilosci przedmiotow.
- `exchangeGetItemMetinSocketFromSelf(...)`, `exchangeGetItemMetinSocketFromTarget(...)` -> PyObject* (int): Wartosci metin slotow, pobiera dwa inty z `poArgs` (pos, attrPos).
- `exchangeGetItemAttributeFromSelf(...)`, `exchangeGetItemAttributeFromTarget(...)` -> PyObject* (tuple: int, int): Zwracaja wlasciwosci i wartosci atrybutow z konkretnego slotu (pos, attrSlotPos).
- `exchangeGetElkMode(poSelf, poArgs)` -> PyObject* (bool): Zwraca flage trybu wymiany zlota.
- `exchangeSetElkMode(poTarget, poArgs)` -> PyObject*: Aktualizuje stan lokalny trybu wymiany zlota (bool elk_mode).

**Modul `shop`:**
- `shopOpen`, `shopClose`, `shopIsOpen`, `shopIsPrivateShop` itd.: Konfiguracje podstawowe i kontrola cyklu widocznosci sklepu.
- `shopGetItemID`, `shopGetItemCount`, `shopGetItemPrice`, `shopGetItemMetinSocket`, `shopGetItemAttribute`: Pobieranie cech przedmiotow w otwartym sklepie (argumentami zwykle jest `pos`).
- `shopBuildPrivateShop(poSelf, poArgs)` -> PyObject*: Wymaga nazwy okna sklepu w `szName`. Wywoluje zlecenia do gry o stworzenie wlasnego sklepu gracza.

**Modul `safebox`:**
- `safeboxGetCurrentSafeboxSize`, `safeboxGetItemID`, `safeboxGetItemCount`, `safeboxGetItemFlags`: Funkcje gettery dla wlasciwosci depozytu.
- `safeboxGetMallItemID`, `safeboxGetMallSize`, `safeboxGetMallItemCount`: Interfejs dla magazynu ItemShopu.

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
Funkcje C-API nie maja swojego layoutu danych (dzialaja wylacznie na stosie), ale odwolaja sie do pol Singletonow:
- `CPythonExchange` przechowuje prawdopodobnie struktury typu `TExchangeItem` per slot, co agent moze wykorzystac przez offsety bazowe CPythonExchange. Zwykle zjawisko hookowania moze nastapic w funkcjach zwiazanych z `BuildPrivateShop` (do oszukania cen) czy `SetElkMode`.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

- **Pakiety Sieciowe:** Moduly te same z siebie nie odbieraja pakietow, ale funkcja `BuildPrivateShop` generuje do sieci polecenia poprzez moduly CommandEncoder takie jak `SendShopPacket`. Wystepuja tutaj w tle opcody `Packet_Exchange.h`, `Packet_Shop.h` m.in wysylane sa polecenia: EXCHANGE_START, EXCHANGE_ITEM_ADD, EXCHANGE_ACCEPT, SHOP_BUILD.
- **Metody Pythona (`PyMethodDef`):** Eksport API: 
  - `exchange.InitTrading`, `exchange.GetItemVnumFromTarget`, itp.
  - `shop.BuildPrivateShop`, `shop.GetItemPrice`, itp.
  - `safebox.GetCurrentSafeboxSize`, itp.
  Zmapowane sa bezposrednio na C++ poprzez tablice `s_methods`. Ponadto zostaja zarejestrowane stale np. `EXCHANGE_ITEM_MAX_NUM`, `SHOP_SLOT_COUNT`.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Zasady wielowatkowosci:** Wszystkie wywolania tego modulu wykonuja sie na wylacznosc glownego watku Pythona (Main D3D Thread). Ze wzgledu na GIL (Global Interpreter Lock) w Pythonie C-API synchronizacja watkow jest rozwiazana niejawnie, niemniej uzycie `Instance()` i rzutowania globalnych singletonow wymaga, by zadna z sieciowych operacji asynchronicznych (Network Thread) nie modyfikowala wewnetrznych tablic okna w tym samym czasie.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** Brak obslugi zlych wejsc `poArgs`. W wielu miejscach funkcje bezposrednio rzutuja wartosci na dany typ (np. poprzez `PyTuple_GetInteger` i `(char) pos`) - przekroczenie zakresow tablic (`pos > MAX_ITEMS`) w kodzie backendowym (np. w `GetItemVnumFromSelf`) moze spowodowac naruszenie obowiazujacego obszaru pamieci, jezeli metoda obslugujaca (np. `CPythonExchange`) nie wykonuje twardej walidacji granic. 
- **Zarzadzanie zasobami (RAII):** Te pliki pelnia role posrednika, zadna wlasna alokacja dynamiczna (`new`/`malloc`) nie nastepuje, jedynie inkrementacja i dekrementacja referencji w interfejsach Pythona za posrednictwem zwracanych obiektow (np. `Py_BuildValue`, `Py_BuildNone()`). 

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zdefiniuj nowa metode w pliku `PythonExchangeModule.cpp` ze standardowa sygnatura `PyObject * exchangeMyNewMethod(PyObject * poSelf, PyObject * poArgs)`.
  2. Wydobadz parametry przy pomocy `PyTuple_GetInteger` lub `PyBridge::ExtractArgs` w celu bezpiecznego parsowania wejscia.
  3. Wykonaj interakcje z CPythonExchange::Instance() / CPythonShop::Instance().
  4. Zwroc obiekt do Pythona uzywajac `Py_BuildValue` lub `Py_BuildNone()`.
  5. Dodaj wpis do tablicy `s_methods` we wlasciwej funkcji inicjujacej (np. `initTrade`).
- **Jak debugowac i logowac:** Loguj ewentualne komunikaty bezposrednio uzywajac funkcji `TraceError()` (czesto uzywanej w architekturze EterLib). Wystaw breakpointy na metody pobierajace obiekty i sprawdz czy UI nie prosi o sloty powyzej maksymalnej zdefiniowanej liczby (np. `EXCHANGE_ITEM_MAX_NUM`).
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Import tych modulow jest trudny ze wzgledu na statyczne poleganie na silniku. W tym celu uzyj frameworku stubujacego dla `Py_BuildValue` oraz samego `CPythonExchange` (Mock Instance), ewentualnie dolacz odpowiednie makra i pisz testy w obrebie specjalnie wyizolowanego `test_bin`.

