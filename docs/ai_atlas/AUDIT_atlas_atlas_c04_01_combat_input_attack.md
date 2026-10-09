---
task_id: "atlas_c04_01_combat_input_attack"
cluster: "CBT"
module_name: "Przechwytywanie Wejscia Walki i Kolejkowanie Atakow"
target_files:
- src/UserInterface/PythonPlayerInputKeyboard.cpp
- src/UserInterface/PythonPlayerInputMouse.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c04_01_combat_input_attack.md"
architecture_layer: "System Walki, Umiejetnosci i Fizyka Pociskow"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport Audytu: Przechwytywanie Wejscia Walki i Kolejkowanie Atakow

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Modul odpowiedzialny za przechwytywanie bezposredniego wejscia uzytkownika (myszy i klawiatury) i mapowanie go na wysokopoziomowe akcje w grze, takie jak poruszanie sie w odpowiednim kierunku, ustawienie kamery, kolejkowanie ataku automatycznego czy podejscie do wskazanego obiektu (tzw. "Smart Input"). Dziala bezposrednio jako kontroler postaci gracza.

W strukturze petli gry system wejsc reaguje natychmiastowo na wcisniecie (np. komunikaty OS mapowane z WinAPI / DirectInput do klas Pythonowych `game.py` a nastpnie do `CPythonPlayer`), jednak jego zasadnicza ewaluacja (reakcja na stale wektory wejscia, ruch do wyznaczonego celu, sprawdzanie dystansu) zachodzi w kluczowej funkcji `NEW_RefreshMouseWalkingDirection()`, ktora jest wywolywana w glownej petli klienta przy kazdej ramce `OnUpdate()`.

**Przeplyw danych (Data & Control Flow):**
1. **Zdarzenie OS:** Uzytkownik naciska "Spacje" lub klika "PPM".
2. **Rejestracja wejscia:** Przechwycenie zdarzenia (np. `SetAttackKeyState`, `NEW_SetMouseSmartState`) i ustawienie flag trybu w module gracza (np. `m_isAtkKey = true`, `m_isSmtMov = true`).
3. **Konwersja:** Ruchy klawiszowe wektorowe WASD/strzalki konwertowane sa na stopnie (kat obrotu postaci) wzgledem kamery przez `GetDegreeFromDirection` czy `NEW_GetMultiKeyDirRotation`. 
4. **Zarezerwowane akcje:** Klikniecie z uzyciem myszy na potwora/NPC rezerwuje dzialanie w specjalnym polu (np. `m_eReservedMode = MODE_USE_SKILL`, `MODE_CLICK_ITEM`).
5. **Ewaluacja (Tick):** W `NEW_RefreshMouseWalkingDirection()` sprawdzane jest polozenie w przestrzeni w stosunku do zarezerwowanego celu. Jesli postac jest zbyt daleko, nastepuje wydanie polecenia ruchu do instancji (`NEW_MoveToDestInstanceDirection`). Jezeli cel znajduje sie blisko, odpalany jest pakiet na serwer sieciowy, a gracz realizuje faktyczne uzycie zaklecia, atak lub podniesienie przedmiotu (`SendClickItemPacket`).

**Cykl zycia:**
Ten system opiera sie na ciaglej akomodacji stanow boolowskich i rezerwowanych indeksow (VID potwora). Wszystkie zarzadzania odbywaja sie w czasie trwania calej sesji gry, w polach Singletona `CPythonPlayer`. Zresetowanie stanow (np. przy zakonczeniu czynnosci lub kliknieciu w inne miejsce ekranu) realizowane jest m.in. funkcja `__ClearReservedAction()`. Brak tu pamieci dynamicznie zwalnianej lub zarzadzania czasem zycia (obiekty zalezne pobierane sa z systemow zewnetrznych via VirtualID - VID).

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
- System Python/UI: Modul `game.py` i eventy wejscia uzytkownika wywolujace funkcje C++ zarejestrowane przez interfejs Python API (`PythonPlayerModule.cpp`).
- Glowna petla aplikacji klienta (`CPythonApplication`, `OnUpdate`), ktora aktywuje odswiezanie kierunku poruszania sie i obsluge podpiecia interfejsu klawiszy CPythonPlayerEventHandler.

**Zaleznosci wyjsciowe (Outbound):**
- **EterLib (Render/Camera):** Interakcja z menedzerem okien `UI::CWindowManager` celem pobrania koordynatow kursora (X, Y) do proporcji ekranu. Uzycie `CCameraManager::Instance()` i `CCamera` do transformowania widoku kamery przy przeciaganiu MMB/PPM.
- **InstanceBase:** Komunikacja z glownym aktorem (`NEW_GetMainActorPtr()`) w celu wykonania atakow (`NEW_Attack`), ruchu (`NEW_MoveToDirection`), pobierania koordynatow i rotacji.
- **CPythonNetworkStream:** Wysylanie pakietow po sfinalizowaniu ruchu - pakiety np. o stanie bohatera (`SendCharacterStatePacket`), chwyceniu przedmiotu.

**Drzewo dyrektyw `#include`:**
`PythonPlayerInputKeyboard.cpp`
- `"StdAfx.h"`
- `"PythonPlayer.h"`
- `"InstanceBase.h"`

`PythonPlayerInputMouse.cpp`
- `"StdAfx.h"`
- `"PythonPlayer.h"`
- `"PythonApplication.h"`
- `"EterLib/Camera.h"`

Brak groznych potencjalnych redefinicji, jednak polega w znacznym stopniu na istnieniu zintegrowanego srodowiska globalnego precompiled header (StdAfx.h).

**Model pamieciowy:**
Bazuje calkowicie na uzyciu surowych wskaznikow (raw pointers `CInstanceBase*`, `CCamera*`) pobieranych poprzez globalne Singletony. Nalezy obslugiwac weryfikacje za kazdym razem (np. `if (!pkInstMain) return`), by zapobiec Null Reference.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabela Klas i Struktur
| Nazwa (Klasa/Typ) | Rola | Wielkosc/Znaczenie | Wlasciciel Watku |
| --- | --- | --- | --- |
| `CPythonPlayer` | Implementuje funkcje sterowania klawiszami/mysza oraz system stanow (m.in. m_isAtkKey, m_isDirMov). | Singleton | Main Thread (UI/Render) |

### Tabela Metod Publicznych
| Sygnatura Metody (C++) | Wartosc Zwracana | Warunki Wstepne (Pre-conditions) | Skutki Uboczne / Opis Akcji |
| --- | --- | --- | --- |
| `void SetAttackKeyState(bool isPress)` | `void` | Gracz jest zespawnowany. | Przerywa czynnosci (np. lowienie ryb). Zapala globalna flage uderzenia `m_isAtkKey`. |
| `void NEW_SetSingleDIKKeyState(int eDIKKey, bool isPress)` | `void` | Kod podany w DIK_*. | Aktywuje stan (wcisniety) odpowiedniego kierunku m_isUp, m_isDown, itp. |
| `void NEW_SetMultiDirKeyState(bool isLeft, bool isRight, bool isUp, bool isDown)` | `void` | Zgloszony zamiar ruchu kierunkami. | Zatrzymuje gracza (`NEW_Stop`) albo nakazuje ruch `NEW_MoveToDirection`. |
| `float GetDegreeFromDirection(int iUD, int iLR)` | `float` | Stany kierunku to wartosci KEYBOARD_UD_* oraz LR_*. | Przeksztalca wypadkowy wektor z klawiszy WASD na stopnie (0-360) dla rotacji. |
| `void NEW_SetMouseMoveState(int eMBS)` | `void` | eMBS to np. MBS_PRESS. | Wlacza obsluge poruszania w strone kursora (m_isDirMov). |
| `bool NEW_MoveToMouseScreenDirection()` | `bool` | Wskaznik ma ustalone koordynaty. | Sprawdza wspolrzedne okna i przelicza ruch w dany punkt na podstawie rzutu perspektywy ekranu na swiat gry. |
| `void NEW_SetMouseCameraState(int eMBS)` | `void` | Dzialajaca i prawidlowa kamera `CCamera`. | Inicjuje/Zamyka Drag&Drop na kamerze. Wysyla polecenie rotacji / podswietlenia do kamery. |
| `void NEW_SetMouseSmartState(int eMBS, bool isAuto)` | `void` | Sprawny glowny gracz w swiecie. Nie w sklepie. | Aktywuje rozliczenie klikniecia "Smart": Interakcja w to, co kliknieto. |
| `void NEW_RefreshMouseWalkingDirection()` | `void` | Gra trwa, petla `OnUpdate` aktywna. | Niezwykle wazny zarzadca kolejkowania zadan. Aktualizuje stany podejscia MODE_CLICK_ITEM, MODE_CLICK_ACTOR, az zrealizuje operacje na koncu (gdy dojdzie do dystansu zerowego/wymaganego). Nastepnie resetuje rezerwacje w pamieci. |

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
Wewnatrz `CPythonPlayer` kluczowe rezerwy (offsets naleza do domeny aplikacji kompilowanej 32/64):
- Flagi boolowskie kierunkow ruchu: `m_isUp`, `m_isDown`, `m_isLeft`, `m_isRight`, `m_isDirKey`, `m_isAtkKey` trzymane obok siebie z flagami inteligentnego celu `m_isSmtMov`, `m_isDirMov`.
- Kontrola bufora: `m_eReservedMode` (enum trybu) determinuje, co zostanie uwolnione. Rezerwowane sa `m_dwVIDReserved` oraz `m_dwIIDReserved` w bliskim ukladzie offsetowym do szybkiego porownania ze swiatem VID/IID. Zmiana ich wymusza hook na przerywanie akcji bota AI.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

**Pakiety Sieciowe:**
- Sieciowa warstwa operuje poprzez pakiet TCP, obslugujac powiadomienia o podniesieniu/ruchu na granicy strefy buforowej.
- `CPythonNetworkStream::SendCharacterStatePacket(kPPosCur, fCurRot, CInstanceBase::FUNC_WAIT, 0)` – po ukonczeniu ruchu (dystans osiagniety) wysyla synchronizacje pozycyjna i katowa postaci by skorygowac polozenie z serwerem.
- `SendClickItemPacket(m_dwIIDReserved)` – wysyla potwierdzenie probkowania chwycenia przedmiotu po dojsciu do niego, skutkujace wyslaniem pakietu (prawdopodobnie HEADER_CG_ITEM_PICKUP).
- Wysylanie ataku i walki pakowane jest w oddzielne pakiety, m.in. w implementacji funkcji celowania do potwora `MODE_CLICK_ACTOR` -> `__ReserveProcess_ClickActor()`.

**Metody Pythona (`PyMethodDef`):**
Metody w C++ podlegaja rygorystycznemu bridge'owi. Klasa `CPythonPlayer` eksportuje dostep dla Game.py w pliku `PythonPlayerModule.cpp`, gdzie udostepnione beda m.in. odpowiedniki:
- `player.SetAttackKeyState(isPress)` - rejestracja bitow uderzenia bezposrednio z eventu Python API.
- `player.SetSingleDIKKeyState(key, isPress)` - wstrzykiwanie symulacji wejsc (np. symulator makro Pythona).

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Zasady wielowatkowosci:** Wszystkie obliczenia dla rezerwowanego wejscia znajduja sie w Main Thread (watek aplikacji glownej). Interakcja modulu `PythonPlayerInput*` z obcymi watkami takimi jak np. Socket Network Thread przyjmowana jest jako skrajnie niebezpieczna bez wlasciwej wymiany eventowej, dlatego bezposrednie modyfikacje kolejek wejsciowych w innych watkach spowoduja wylom, Race Conditions oraz krasz instancji `CInstanceBase` (ktora zabrania manipulacji spoza MainThread).
- **Potencjalne punkty awarii (Crash Points):**
  - Brak bezpieczenstwa map. Jesli serwer rozlaczy w srodku interakcji podejscia (MODE_CLICK_ACTOR), instancja zapamietana w postaci VID moze byc juz niewazna. Przed egzekucja wywolania np. `SetTarget`, zawsze potrzebny jest `NEW_FindActorPtr(m_dwVIDReserved)` by sprawdzic czy dany pointer zostal uzyty. Nigdy nie przechowuj bezposrednich wskaznikow, zawsze dzialaj przez VID (Virtual ID).
  - Brak uzywania NULL checks. Uzycie `CPythonPlayer::NEW_GetMainActorPtr()` musi zawsze nastepowac po sprawdzeniu `if (!pkInstMain) return;`.
- **Zarzadzanie zasobami (RAII):** Kod nie tworzy i nie niszczy instancji aktora. Wirtualny zasob (VirtualID, TargetID) to tylko numer mapowany przez menedzery, a obsluga dziala wylacznie w oparciu o stan odpytywania podczas ramki gry.
- **Edge Cases:** Proba walki i nacisniecia wejsc w trakcie stanu Emocji `__IsProcessingEmotion()`, Ogluszenia `AFFECT_STUN` i w trybie prywatnego sklepu `IsOpenPrivateShop()`. Ignorowanie logiki (np. hook od strony cheata/hacka pominawszy blokade stanu sklepu) spowoduje desynchronizacje pamieciowa serwera (zablokuje sie socket na serwerze ze wzgledu na brak wlasciwego op-kodu ruchu).

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Instrukcja dodawania nowej funkcji wejsciowej (np. Automatyczne chodzenie za sojusznikiem (Follow)):**
1. Zdefiniuj w `CPythonPlayer` (np. enum `EMode`) nowa flage, np. `MODE_FOLLOW_ACTOR`.
2. Dodaj zmienne w `CPythonPlayer` zapamietujace VID osoby sledzonej `m_dwFollowVID`.
3. Przechwyc komende Pythona w API z UI: Np. dodaj w module UI przycisk, ktory zwroci `player.SetFollowActorMode(vid)`.
4. Umiesc logike wewnatrz `NEW_RefreshMouseWalkingDirection()`, tworzac nowy przypadek `case MODE_FOLLOW_ACTOR:` - oblicz dystans do aktora, wywolaj `NEW_MoveToDestInstanceDirection()`, i gdy odleglosc spadnie ponizej pewnego progu to `NEW_Stop()`. Pakiety state-movement odpalaja sie po czesci automatycznie, gdy sie zatrzymujesz przez `NEW_Stop()`.

**Jak debugowac i logowac:**
W przypadku rezerwacji wejsc problematyczne sa nienaturalne przerwania ruchu. Sprawdzic warto logi wpisujac funkcje systemowe `TraceError` badz `Tracen`. Konieczne punkty zatrzymania (Breakpoints) umieszczaj na wstepie metody `NEW_RefreshMouseWalkingDirection()` oraz na funkcji rezerwacji `__OnPressSmart`. Mozna podgladac `m_eReservedMode` by miec pewnosc jaki tryb rezerwacji zostal aktywowany.

**Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
Poniewaz funkcje klawiatury reaguja na czyste eventy z DIK_*, testy automatyczne moga symulowac ruch wywolujac po kolei: `player.NEW_SetSingleDIKKeyState(DIK_UP, true)`, przepuszczajac sztuczna ramke `OnUpdate()` (symulujac czas). Poniewaz `CInstanceBase` jest w tej instancji uzywane, moduly testowe (np. `doctest`) dla testow jednostkowych musza implementowac interfejs `mock` do Singletonu `CPythonCharacterManager` badz bazowac na zredukowanych testach jednostkowych samego konwertera (np. sprawdzenie poprawnego rzutowania katow w funkcji `GetDegreeFromDirection` do stopnia precyzji zmiennoprzecinkowej, co wykonuje sie bez uruchamiania symulacji silnika Direct3D).
