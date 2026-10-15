---
task_id: "atlas_c01_03_phase_login"
cluster: "NET"
module_name: "Faza Logowania i AccountConnector"
target_files:
- src/UserInterface/PythonNetworkStreamPhaseLogin.cpp
- src/UserInterface/AccountConnector.cpp
- src/UserInterface/AccountConnector.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c01_03_phase_login.md"
architecture_layer: "Silnik Sieciowy, Maszyna Faz i Protokol"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):

**Funkcja modulu:** Modul ten odpowiada za obsluge kluczowej dla klienta gry fazy logowania. Obejmuje polaczenie z serwerem autoryzacji (Auth), wymiane kluczy szyfrujacych, handshake, uwierzytelnienie hasla uzytkownika, a w przypadku sukcesu uzyskanie klucza logowania (Login Key) pozwalajacego na nawiazanie polaczenia z wlasciwym serwerem gry (Game Server). Nastepnie przechodzi w faze obslugi listy postaci (Select/Create/Delete).

**Miejsce wywolywania:** 
Kod jest zintegrowany z maszyna stanow sieciowych w obiekcie `CPythonNetworkStream`. Glowna praca odbywa sie asynchronicznie w czasie cyklu `Network Tick` (wywolywanym przez `Process()` strumienia, pochodzacym z petli `OnUpdate`). Ponadto modul jest silnie podpiety pod maszyne stanow w UI (fazy w skryptach Pythona) poprzez warstwe `AccountConnector`.

**Przeplyw danych (Control & Data Flow):**
1. Skrypt UI poprzez mostek C-API przekazuje login i haslo uzywajac `netSendLoginPacket` i powiazanych funkcji, po wczesniejszym wywolaniu `netConnectToAccountServer`.
2. `CAccountConnector` inicjalizuje stan `STATE_HANDSHAKE` - nastepuje wymiana `TPacketGCKeyChallenge` oraz `TPacketGCKeyComplete` z serwerem Auth.
3. Klient otrzymuje pakiet `TPacketGCPhase` ze stanem `PHASE_AUTH`.
4. `CAccountConnector` wysyla `TPacketCGLogin3` z loginem i haslem i przechodzi w stan `STATE_AUTH`.
5. Serwer weryfikuje dane. W przypadku niepowodzenia przychodzi `TPacketGCLoginFailure` i UI jest o tym powiadamiane. W przypadku sukcesu przychodzi `TPacketGCAuthSuccess` zawierajacy identyfikator `dwLoginKey`.
6. Po otrzymaniu sukcesu, `CAccountConnector` sie rozlacza, a `CPythonNetworkStream` zapisuje klucz (`dwLoginKey`) i laczy sie z Game Server.
7. Otrzymujac faza `LoginPhase`, instancja Network Stream wysyla pakiet `TPacketCGLogin2` przekazujac otrzymany `dwLoginKey`.
8. Gra autoryzuje wejscie i serwer odsyla pakiety z danymi postaci (`TPacketGCLoginSuccess3` / `TPacketGCLoginSuccess4`), listujac dostepne postacie na ekranie wyboru bohatera. 

**Cykl zycia obiektow:**
- `CAccountConnector`: Alokowany zazwyczaj statycznie / raz podczas uruchomienia klienta jako instancja (dziedziczy po `CSingleton`). Resetuje swoj wewnetrzny status do `STATE_OFFLINE` przez funkcje `__OfflineState_Set()`.
- Obiekty pakietow C-style (`TPacketGCPhase`, `TPacketGCLoginFailure`, itd.): Tworzone przejsciowo na stosie wewnatrz metod takich jak `__AuthState_RecvPhase()`, inicjowane przez polecenie pobrania zawartosci z bufora sieciowego (`Recv()`), a ich czas zycia konczy sie po zamknieciu scopu funkcji.


### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):

**Zaleznosci wejsciowe (Inbound):**
- System Python C-API (np. `netConnectToAccountServer`, `netSetLoginInfo`, `netSendLoginPacket` w `PythonNetworkStreamModule.cpp`).
- Maszyna Faz Python UI w module Network.
- Serwer Auth wysylajacy pakiety (AuthSuccess, LoginFailure, Phase, KeyChallenge).
- Game Server wysylajacy pakiety sukcesu wyboru bohatera (LoginSuccess3, LoginSuccess4).

**Zaleznosci wyjsciowe (Outbound):**
- Strumienie Sieciowe (NetworkStreams): `CNetworkStream`, narzedzia biblioteki `EterLib`.
- Python API: Konstrukcja zwrotna odpowiedzi (tzw. callbacks) uzywajaca `PyCallClassMemberFunc` ze zlodziejstwem argumentow z uzyciem `Py_BuildValue()`.

**Drzewo dyrektyw `#include`:**
- `#include "StdAfx.h"`: Standardowy naglowek zawierajacy typowe globalne dyrektywy Windows (istnieje ryzyko silnego i czestego polaczenia z `d3d9.h` oraz funkcjami OS).
- `#include "AccountConnector.h"`
- `#include "Packet.h"`
- `#include "PythonNetworkStream.h"`
- *AccountConnector.h* wlacza `#include "EterLib/NetStream.h"` i `#include "EterLib/FuncObject.h"`.

**Model pamieciowy:**
Zgodnie z konwencja w kodzie starszej wersji silnika dominuja "gole" wskazniki C na obiekty Pythona (`PyObject * m_poHandler`), oraz alokacja struktur bezposrednio na stosie (stack-allocated structures). Ponadto wykorzystywane sa struktury i stringi z wbudowanej biblioteki standardowej (`std::string`). Zarzadzanie zasobami to poleganie na wbudowanych mechanizmach dealokacji z poziomu warstwy CNetworkStream i Pythona.


### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
1. `CAccountConnector` (Dziedziczy po `CNetworkStream` i `CSingleton<CAccountConnector>`)
   - Rozmiar: Obiekt singletonowy, rozmiar nie do konca statyczny poprzez wektory, ale zarzadzany globalnie.
   - Wlasciciel watku: Glowny watek obslugi sieciowej i update gry (Main/Network Thread).
   - Cel: Bezposrednio steruje faza `AUTH` i procesem handshake z serwerem logowania.

2. `CPythonNetworkStream` (Maszyna stanu faz gry)
   - Przetwarza wlasciwe wejscie po udanym handshake, np. autoryzujac faza logowania (stan `LoginPhase`).

**Tabela Metod Publicznych w `CAccountConnector`:**
- `void SetHandler(PyObject* poHandler)`: Brak typow zwracanych, przyjmuje i zapisuje na goly wskaznik - ryzyko braku refcountingu (`Py_INCREF` nie wystepuje, trzeba uwazac na dangling pointery).
- `void SetLoginInfo(const char * c_szName, const char * c_szPwd)`: Otrzymuje czyste tablice znakow C. Kopiuje je do obiektow `std::string`.
- `void ClearLoginInfo(void)`: Bezpiecznie czysci zachowane haslo ustawiajac na wartosc pusta.
- `bool Connect(const char * c_szAddr, int iPort, const char * c_szAccountAddr, int iAccountPort)`: Nawiazuje polaczenie bazowe (dziedziczone) uprzednio resetujac stan do OFFLINE.

**Tabela Metod Publicznych w `CPythonNetworkStream` (z `PythonNetworkStreamPhaseLogin.cpp`):**
- `void LoginPhase()`: Glowna metoda wysylajaca zadania na strumien.
- `void SetLoginPhase()`: Funkcja zmieniajaca etap dzialania u klienta, obslugujaca takze direct mode login.
- `bool SendLoginPacketNew(const char * c_szName, const char * c_szPassword)`: Wrzuca nowy pakiet CG do bufora wewnetrznego (bezposrednio wywoluje `#pragma pack` payload).

**Pamieciowy Layout Struktur sieciowych (Zaleznosci Layoutowe offsetow C-style):**
- `TPacketCGLogin3`: `uint8_t` (header) -> `uint8_t`/`uint16_t` (length) -> `char[ID_MAX_NUM]` (login) -> `char[PASS_MAX_NUM]` (pwd).
- `TPacketGCLoginSuccess4`: Obsluga tablic zawierajacych po 4 pozycje ID gildii (`uint32_t guild_id[4]`), nazw gildii (`char guild_name[4][32]`), i `akSimplePlayerInformation`. Wymaga zgodnosci 1:1 na biezace offsety z serwerem przy uzywaniu np. FFI lub Arthion.


### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):

**Pakiety Sieciowe:**
- `GC::PHASE` (`TPacketGCPhase`): Serwer deklaruje krok fazy (np. HANDSHAKE lub AUTH).
- `GC::PING`, `GC::KEY_CHALLENGE`, `GC::KEY_COMPLETE`: Narzedzia negocjacji CNetworkStream i zabezpieczen kluczy.
- `CG::LOGIN3` (`TPacketCGLogin3`): Wyslanie nieszyfrowanych danych logowania.
- `GC::AUTH_SUCCESS` (`TPacketGCAuthSuccess`): Potwierdzenie przyjscia AuthSuccess, niosace m.in. token `dwLoginKey`.
- `GC::LOGIN_FAILURE` (`TPacketGCLoginFailure`): Niepoprawne dane. Zwraca takze char ze statusem powiadomienia (np. `WRONGPWD`).
- `CG::LOGIN2` (`TPacketCGLogin2`): Drugi login (z samym `login_key`), idacy na serwer docelowy (Game).
- `GC::LOGIN_SUCCESS3` (`TPacketGCLoginSuccess3`) / `GC::LOGIN_SUCCESS4` (`TPacketGCLoginSuccess4`): Sukces i odbior pakietu kont z postaciami na wybor bohatera.
- `GC::EMPIRE` (`TPacketGCEmpire`): Przychodzi stan z wybranym obecnie krolestwem bohatera.
- `GC::LOGIN_KEY` (`TPacketGCLoginKey`): Pakiet narzucajacy zmiane/aktualizacje klucza w ramach ponowienia, rzadko spotykany w czystym logowaniu.

**Metody Pythona (`PyMethodDef` w `PythonNetworkStreamModule.cpp`):**
- `netSetLoginInfo(PyObject* poSelf, PyObject* poArgs)`: Odbiera login (string) i haslo (string), mapuje do metody w `CPythonNetworkStream::Instance().SetLoginInfo` oraz `CAccountConnector::Instance().SetLoginInfo`.
- `netSendLoginPacket(PyObject* poSelf, PyObject* poArgs)`: Odbiera 2 stringi, mapuje do `CPythonNetworkStream::Instance().SendLoginPacketNew`.
- `netConnectToAccountServer(PyObject* poSelf, PyObject* poArgs)`: Odbiera IP (string) i port (int), ustawia adres Auth przez `CPythonNetworkStream`.
- `netSetAccountConnectorHandler`: Przypina instancje w Pythonie do obslugi handlerow callbackow.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):

**Zasady wielowatkowosci:**
Kod klienta Metin2 domyslnie uruchamia polaczenia sieciowe na osobnym watku Winsock, ale faza procesu UI i wymiany faz w maszynie dziala jako glowny watek wywolywany metoda `Process()`. Nalezy byc szczegolnie ostroznym przy modyfikowaniu struktur `m_apoPhaseWnd` - funkcje `PyCallClassMemberFunc` **musza** byc wylonowane z watku glownego z powodu Global Interpreter Lock (GIL) Pythona i braku srodkow asynchronicznej ochrony GIL z poziomu `CNetworkStream`. W nowym standardzie (Zero-Conflict, C++23) bedzie wymagane opieranie powiadomien na systemie `EventBus`.

**Potencjalne punkty awarii (Crash Points & Edge Cases):**
1. **Brak wskaznika `m_poHandler`:** W `CAccountConnector` jesli polaczenie zostanie przerwane (np. uzycie metody `OnConnectFailure`) a handler nigdy nie zostal ustawiony, mozna spodziewac sie potencjalnych naruszen przy niejawnych wywolaniach. Zwykly `if (m_poHandler)` chroni w wiekszosci metod, ale wciaz ryzyko "stale pointer" pozostaje, jesli obiekt pythona ulegnie wczesniejszemu zniszczeniu, bo `m_poHandler` to czysty wskaznik bez podnoszenia referencji.
2. **Buffer Overflows w `strncpy`:** W operacjach zapisu (np. `strncpy(LoginPacket.name, m_strID.c_str(), ID_MAX_NUM); LoginPacket.name[ID_MAX_NUM] = '\0';` w `TPacketCGLogin3`), brak pelnej weryfikacji poczatkowej dlugosci `m_strID` sprawia ze jesli przekracza limit, to jest twardo scinany (truncate). AI powinno zawsze upewnic sie, iz string limitowane sa w miejcu zrodla (UI klienta).
3. **Nieobsulgiwany Stan (Invalid State Exception):** `m_dwLoginKey` jesli jest `0` resetuje proces uwierzytelnienia. Modyfikujac proces, upewnij sie by nigdzie go recznie nie nadpisac lub wyzerowac na poziomie polaczenia z wlasciwym Game Serverem.

**Zarzadzanie zasobami (RAII):**
Hasla tymczasowe (`m_strPassword`) sa alokowane jako wartosc std::string. Zawsze czyszczone metoda `ClearLoginInfo()` podczas opuszczania fazy by zapobiec atakom ze sterty. Wszelkie implementacje AI musza powtarzac to zachowanie - tzw. zeroing credentialow po udanym lub wygaslym logowaniu.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):

**Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
Chcac dodac np. nowa warstwe pakietu w C++23 dla obslugi dodatkowego loginu (np. MFA - Multi-Factor Authentication):
1. Przestrzegaj **Zasady Zero-Conflict**. Nie edytuj `AccountConnector.cpp`!
2. Zdefiniuj wlasne handlery rozszerzajace dzialajace na podstawie odrebnego interfejsu lub systemu hookowego na module `NetworkStream` - np. wprowadz w katalogu `src/Client/Network/Handlers/` pliki dedykowane do np. MFA Auth, tak samo jak `LoginPacketHandler.cpp`.
3. Podepnij wywolania przez C++ `Core::EventBus` by uniknac uderzen bezposrednio o wywolywanie Pythona, emitujac nowy silny typ eventu (dziedziczacego po `UserInterface::Core::IEvent`) po odebraniu opcodu w buforze `CAccountConnector`.

**Jak debugowac i logowac:**
Uzywaj `EterBase::ModernLogger::Error` lub `Tracef("...")` wbudowanego w srodowisko do obslugi logow tekstowych. Pakiety autoryzacji sa newralgiczne - **nie wolno** umieszczac wartosci `m_strPassword` bezposrednio w trace/logerach (nawet na etapie debugowania u klienta, chron przed inwigilacja sterty). 

**Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
Rozdzial ten dziala glownie w obrebie stanu, wiec `CAccountConnector` mozna sfalszowac (zmockowac) podczas Doctest symulujac odpowiedzi sieciowe (tworzac tablice danych opartych o zrzuty payloadow). Dla `PyCallClassMemberFunc` nalezaloby napisac pusty atrapowy (dummy) handler `PyObject` lub skompilowac unit z wylaczeniem naglowkow narzucajacych API Pythona uzywajac makra `-DTEST_MODE_DISABLE_STDAFX` i tworzac reczny `MockStateManager`. Ustaw stany sztucznie poprzez np. `__AuthState_Set()` i przepusc tablice bajtow by zweryfikowac, czy state machine klienta przeszedl do GamePhase.
