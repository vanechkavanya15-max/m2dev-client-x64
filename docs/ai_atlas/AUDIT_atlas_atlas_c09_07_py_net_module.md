---
task_id: "atlas_c09_07_py_net_module"
cluster: "PY"
module_name: "Modul Pythona 'net' - Interfejs Wysylania Pakietow ze Skryptow"
target_files:
- src/UserInterface/PythonNetworkStreamModule.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c09_07_py_net_module.md"
architecture_layer: "Mostek Pythona, Moduly C-API i Skrypty Gry"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# ZADANIE DLA AGENTA AI: Modul Pythona 'net' - Interfejs Wysylania Pakietow ze Skryptow

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul `src/UserInterface/PythonNetworkStreamModule.cpp` tworzy i eksportuje mostek C-API do srodowiska Pythona pod nazwa modulu `net`.
Stanowi on warstwe posrednia pomiedzy logika interfejsu uzytkownika i logiki biznesowej zdefiniowanej w skryptach Pythona, a C++-owym, sieciowym silnikiem pod spodem, zdefiniowanym przezSingleton `CPythonNetworkStream`.

Kiedy gracz wykonuje akcje w UI (np. klika "Wyslij" na czacie, naciska PPM na przedmiot w ekwipunku, kupuje przedmiot w sklepie lub rozpoczyna handel), funkcja wywolania zwrotnego zdefiniowana w logice Pythona uzywa modulu `net` (np. `net.SendChatPacket(text, type)`). Kod wewnatrz C++ konwertuje obiekty z `PyObject` i przekazuje argumenty jako typy natywne (np. stale, ciagi znakow, identyfikatory VID - Visual ID) bezposrednio do metod instancji Singletona `CPythonNetworkStream::Instance()`. Pakiety te sa nastepnie formatowane zgodnie ze struktura z `Packet.h` / C-API i ostatecznie zrzucane do socketa TCP przez bufor asynchroniczny.

Przeplyw danych dziala w glownym watku gry (Main/UI Thread/Python Thread). Nie sa inicjowane dedykowane watki, alokacja sprowadza sie do parsowania krotek argumentow funkcji oraz ich delegowania.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):**
  - Glowne wywolania pochodza ze skryptow Python UI (zwykle pliki: `uiChat.py`, `uiInventory.py`, `uiShop.py`, `uiExchange.py`, `uiMessenger.py` itd. oraz korowy `networkModule.py` lub `game.py`).
- **Zaleznosci wyjsciowe (Outbound):**
  - `CPythonNetworkStream`: (Glowne wyjscie modulu) Umozliwia fizyczne wysylanie sformatowanych bajtow do sieci i obsluge stanow maszyny stanow gry, a takze inicjalizuje polaczenia gniazd.
  - Podstawowe API `PyObject` C/Python do parsowania argumentow `METH_VARARGS` (`PyTuple_GetString`, `PyTuple_GetInteger`, `Py_BuildNone()`, `Py_BuildValue`, itp.).
- **Drzewo dyrektyw `#include`:**
  - `StdAfx.h` - przedkompilowane naglowki prekompilatora, win32API i stale EterLib.
  - `PythonNetworkStream.h` - serce implementacji klas dla wszystkich wywolywanych w `net` metod do ktorych nastepuje delegacja wysylek asynchronicznych do polaczonego Socket-a gry / obsluga stanow (LOGIN, SELECT, GAME).
  - `AccountConnector.h` - powiazane z `netConnectToAccountServer` dla autoryzacji sesji.
  - `PythonGuild.h` - wymagany dla operacji zwiazanych z odpowiedziami i odswiezaniem operacji gildyjnych.
  - `AbstractPlayer.h` - pobieranie parametrow i instancji do weryfikacji.
- **Model pamieciowy:** Wskazniki surowe (Raw Pointers) charakterystyczne dla API C (`PyObject* poSelf`, `PyObject* poArgs`), ktore sa zarzadzane przez mechanizm `Reference Counting` (Licznik Referencji) w maszynie wirtualnej Pythona. Sam kod sieci nie alokuje duzych buforow heap/inteligentnych wskaznikow poza `std::list<std::string>` dla polecen serwera na poziomie pliku globalnym w C++.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
**Tabela Klas i Struktur**
Plik stanowi mostek bez stanowej klasy w srodku, cala struktura jest agregowana w tabeli z C-API `PyMethodDef s_methods[]`, delegujacej do funkcji-metod operujacych na Singletonie `CPythonNetworkStream`. Wywolania naleza do watku glownego CPython.

**Tabela Metod Publicznych (Kluczowe mostki dla modulu biznesowego):**
- `PyObject* netConnectTCP(PyObject* poSelf, PyObject* poArgs)`
  - *Argumenty*: (string) `szAddr`, (int) `port`.
  - *Zwraca*: `Py_BuildNone()`.
  - *Dzialanie*: Deleguje do `CPythonNetworkStream::Connect(szAddr, port)`. Laczy socket z serwerem logowania/auth/gry.
- `PyObject* netSendChatPacket(PyObject* poSelf, PyObject* poArgs)`
  - *Argumenty*: (string) `szLine`, (int) `iType` (domyslnie CHAT_TYPE_TALKING).
  - *Zwraca*: `Py_BuildNone()`.
  - *Dzialanie*: Deleguje wyslanie tekstu uzytkownika do serwera (na czacie gildyjnym, wolaj, ogolnym).
- `PyObject* netSendItemUsePacket(PyObject* poSelf, PyObject* poArgs)`
  - *Argumenty*: (int) `Cell.cell` badz `Cell.window_type, Cell.cell`.
  - *Zwraca*: `Py_BuildNone()`.
  - *Dzialanie*: Typowe wywolanie podczas podwojnego klikniecia / PPM przedmiotu w Inventory, lub ekwipowania elementow pancerza, potki (potions).
- `PyObject* netSendShopBuyPacket(PyObject* poSelf, PyObject* poArgs)`
  - *Argumenty*: (int) `iCount` - w nowej grze najczesciej numer uzytej srodkowej rubryki lub komorki, rzutowanej tu na Count.
  - *Dzialanie*: Akceptacja tranzakcji z NPC sklepu.
- `PyObject* netSendExchangeStartPacket(PyObject* poSelf, PyObject* poArgs)`
  - *Argumenty*: (int) `vid` - Entity ID / Virtual ID podmiotu-celu.
  - *Dzialanie*: Wysyla zadanie do serwera by zapytac podmiot o oferte bezposredniego handlu wymiany w grze.
- `PyObject* netDisconnect(PyObject* poSelf, PyObject* poArgs)`
  - *Argumenty*: Brak.
  - *Dzialanie*: Wymusza zamkniecie polaczenia sieciowego klienta z serwerem i czyszczenie pod-obslugi fazy.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- API C++ pod modulem udostepnia wiele stalych z C++ do Pythona w srodowisku m.in:  `ERROR_NONE`, `PHASE_WINDOW_GAME`, flagi gildyjne.
- Z punktu widzenia C++, np. wywolanie `net.SendExchangeStartPacket(vid)` z Pythona parsuje parametr `vid` z krotki `poArgs` na typ natywny integer `int vid`, po czym na rzecz silnika wywoluje bezposrednia operacje C++ `rkNetStream.SendExchangeStartPacket(vid)`. Ten silnik obsluzy docelowy opcode dla `CG_EXCHANGE` (np. 0x2A lub analogiczny) mapujac zmienne lokalne do struktury przesylanej `TPacketCGExchange` i rzutujac je do warstwy przesylania socketa TCP `Send()`.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Wszystkie wywolania mostku pochodza z glownej petli gry i maszyny Pythona (Main Thread). Klasa `CPythonNetworkStream` z kolei posiada wlasne mechanizmy wewnetrzne obslugi (prawdopodobnie watki buforow Send/Recv) ale mostek dziala scisle jednowatkowo zaleznie od GIL CPython 2/3.
- **Potencjalne punkty awarii (Crash Points):** Metody zawsze weryfikuja pobranie przez `PyTuple_Get...()`, rzucajac w zamian `Py_BuildException()`, jednak samo powolanie instancji przez `CPythonNetworkStream::Instance()` wymaga wczesniejszej pre-inicjalizacji klasy gdzies podcza startu (z reguly zrobione w PythonApplication). Niewlasciwy stan (NULL) instancji wywolal by SegFault.
- **Bezpieczenstwo pamieci:** W zwiazku z hermetyzacja parametrow w C-API, nalezy rygorystycznie mapowac parsowanie i rzutowac zmienne `iType`, `iSlotNumber` itp na poprawne typy zdefiniowane po drugiej stronie API. Przekroczenie limitow nie ma bezposrednio wplywu na ten modul, pod warunkiem ze serwer poprawnie zvaliduje otrzymany packet sieciowy i np. uzytkownik nie wskaze ujemnego slota do handlu.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zdefiniuj nowa metode np. `PyObject* netSendMyCustomPacket(PyObject* poSelf, PyObject* poArgs)` wewnatrz `PythonNetworkStreamModule.cpp`.
  2. Sparsej docelowe wejsciowe zmienne uzywajac `PyTuple_GetInteger` i `PyTuple_GetString` dla wyciagniecia parametrow wchodzacych. Sprawdz bledy konwersji i obsluz `Py_BuildException()`.
  3. Wywolaj na referencji Singletona, swoja nowa funkcje, ktora wczesniej zaprojektujesz w C++ np: `CPythonNetworkStream::Instance().SendMyCustomPacket(myInt)`.
  4. Na samym koncu zadeklaruj `return Py_BuildNone();`.
  5. Wejdz do stalej globalnej listy `s_methods` i dodaj wezel `{ "SendMyCustomPacket", netSendMyCustomPacket, METH_VARARGS },`. W ten sposob zostaje on zainicjowany we wbudowanym pakiecie "net". Z Pythona wywolaj go `import net; net.SendMyCustomPacket(123)`.
- **Jak debugowac i logowac:** Nalezy dodac debugowe wpisy `Tracenf("netSendMyCustomPacket has been hit")` lub logi przez interfejs `EterBase::ModernLogger`, poniewaz bezposrednio to on zajmuje sie ruchem asynchronicznym.
- **Jak testowac bez interfejsu graficznego:** System C-API jest dosc ciasno zgrany ze zrodlowa maszyna wirtualna Pythona. Modul nalezy testowac poprze gMock na obiekcie i funkcjach klasy `CPythonNetworkStream`, izolujac parsowanie i interfejs UI do surowych wartosci przekazywanych mockowanemu klientowi sieci. Inna sciezka: w malym srodowisku CPython inicjalizacja mockowego zaleznosci wirtualnej `net` i wstrzykniecie wlasnych C API functions.
