---
task_id: "atlas_c09_10_py_mini_chat_modules"
cluster: "PY"
module_name: "Moduly Pythona 'miniMap' i 'chat' - Radar i Komunikacja"
target_files:
- src/UserInterface/PythonMiniMapModule.cpp
- src/UserInterface/PythonChatModule.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c09_10_py_mini_chat_modules.md"
architecture_layer: "Mostek Pythona, Moduly C-API i Skrypty Gry"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# RAPORT AUDYTU AI: Moduly Pythona 'miniMap' i 'chat' - Radar i Komunikacja

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Analizowane pliki (`PythonMiniMapModule.cpp` oraz `PythonChatModule.cpp`) tworza pomost pomiedzy silnikiem klienta C++ a systemem skryptowym Python dla dwoch kluczowych elementow interfejsu uzytkownika: Minimapy (Radaru) oraz systemu Czatu (w tym Szeptu). 

Ich glownym zadaniem jest wyeksportowanie Singletonow zarzadzajacych UI, takich jak `CPythonMiniMap` oraz `CPythonChat`, do skryptow Pythona. Dzieki temu programisci tworzacy UI moga m.in:
- W przypadku **Minimapy**: Kontrolowac ladowanie zasobow atlasu, zarzadzac skala (przyblizaniem i oddalaniem mapy), aktualizowac srodkowa pozycje na podstawie Vnum postaci, renderowac UI na podstawie koordynatow ekranowych, jak rowniez obslugiwac tzw. Waypointy (punkty docelowe).
- W przypadku **Czatu**: Tworzyc zestawy czatu, modyfikowac tryby czatu (np. wolanie, gildia), przypisywac kolory dla uzytkownikow i komunikatow systemowych, ustalac linie ignorowanych uzytkownikow, zarzadzac systemem "szeptu" (Whisper) oraz parsowac linki URL z hiperlaczy umieszczonych na czacie.

W petli gry metody renderujace (np. `miniMapRender`, `chatRender`) oraz aktualizujace (`miniMapUpdate`, `chatUpdate`) sa wywolywane per-klatka z wartwy UI. Cykl zycia powiazanych komponentow opiera sie na inicjalizacji w bloku `initChat()` i `initMiniMap()`, ktore tworza moduly i lacza funkcje poprzez struktury `PyMethodDef`.  Dealokacja, z racji na wykorzystanie singletonow i srodowiska Pythona, najczesciej ogranicza sie do czyszczenia buforow lub okien na zadanie z poziomu C++ i rzadko zwalnia same struktury modulow przed calkowitym wylaczeniem gry.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound)**: Interfejs UI zdefiniowany glownie w Pythonie (`root/`, `uiscript/`). Funkcje modulu sa wywolywane ze skryptow do pozycjonowania i renderowania minimapy i linii czatu. System wprowadzania z klawiatury dziala jako glowne urzadzenie wejsciowe wpisujace chat, natomiast pakiety z serwera odbierane przez warstwe sieciowa ostatecznie korzystaja z tych okien przez C++ wywolujac Python.
- **Zaleznosci wyjsciowe (Outbound)**: Kod C++ korzysta glownie z silnika gry i globalnych zasobow przez API `CPythonMiniMap::Instance()` oraz `CPythonChat::Instance()`. Rozszerza on typowe mechaniki graficzne (Renderowanie 2D / okna czatu). Narzedzia C API np. `Py_BuildNone`, `PyTuple_GetInteger` i obiekty wyjatkow.
- **Drzewo dyrektyw `#include`**: Najczesciej pliki zaczynaja sie od globalnego prekompilowanego naglowka `StdAfx.h`, ktory posiada najczesciej wlaczane struktury okien i platformy.
- **Model pamieciowy**: Wymiana danych miedzy C++ i Pythonem odbywa sie za pomoca surowych wskaznikow PyObject*. Kod operuje na alokacjach narzuconych przez silnik interpretera CPython i globalne obiekty singletonow (typu Singleton Pattern w C++). Moduly te nie sa bezposrednio wlascicielami zadnej dodatkowej pamieci bez delegacji w interfejsy `CPythonMiniMap` oraz `CPythonChat`. 

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Modul 'miniMap' (PythonMiniMapModule.cpp)
| Sygnatura | Rola | Parametry |
| :--- | :--- | :--- |
| `miniMapSetScale` | Zmienia skale minimapy. | `(float fScale)` |
| `miniMapSetCenterPosition` | Ustala polozenie na mapie srodka radaru. | `(float fCenterX, float fCenterY)` |
| `miniMapSetMiniMapSize` | Ustala wymiary fizyczne w UI. | `(float fWidth, float fHeight)` |
| `miniMapDestroy` | Niszczy/Resetuje instancje. | Brak |
| `miniMapCreate` | Inicjalizuje struktury rendera minimapy. | Brak |
| `miniMapUpdate` | Aktualizuje stan i podazanie kamery. | `(float fCenterX, float fCenterY)` |
| `miniMapRender` | Rysuje grafike na ekranie. | `(float fScrrenX, float fScrrenY)` |
| `miniMapShow` / `miniMapHide` | Widocznosc modulu. | Brak |
| `miniMapisShow` | Sprawdza status widocznosci. | Zwraca wartosc bool. |
| `miniMapScaleUp` / `miniMapScaleDown`| Zwieksza / Zmniejsza skalowanie. | Brak |
| `miniMapGetInfo` | Pobiera info na koordynatach myszy. | `(float fScrrenX, float fScrrenY)` Zwraca int, string, int, int, long |
| `miniMapLoadAtlas` | Wczytuje wielka mape i render atlasu. | Brak |
| `miniMapUpdateAtlas` / `miniMapRenderAtlas` | Aktualizacja/Render mapy Atlas. | Opcjonalnie podaje ekran `(fScrrenX, fScrrenY)` |
| `miniMapShowAtlas` / `miniMapHideAtlas`| Pokazuje i chowa atlas. | Brak |
| `miniMapIsAtlas` | Sprawdza ladowanie plikow. | Zwraca bool |
| `miniMapGetAtlasInfo` | Dane gildii, id, graczy przy koordynatach.| `(fScrrenX, fScrrenY)` Zwraca string, poz x, y, kolor i gildie |
| `miniMapGetAtlasSize` | Wymiary Atlasu na ekranie. | Zwraca X i Y. |
| `miniMapAddWayPoint` / `miniMapRemoveWayPoint`| Zaznaczanie celow / usuwanie. | ID (int), (opcjonalnie x, y i handler) |

### Modul 'chat' (PythonChatModule.cpp)
| Sygnatura | Rola | Parametry |
| :--- | :--- | :--- |
| `chatSetChatColor` | Ustawienie koloru danego trybu. | `(int iType, r, g, b)` |
| `chatClear` | Czystka tablicy i linii okna czatu. | Brak |
| `chatClose` | Zamkniecie interfejsu. | Brak |
| `chatCreateChatSet` | Tworzy bufor na zestaw czatu. | `(int iID)` |
| `chatUpdate` | Funkcja per klatke. | `(int iID)` |
| `chatRender` | Wyswietlanie tekstu i tla na ekran. | `(int iID)` |
| `chatSetBoardState` | Stan aktywacji i przewijania. | `(int iID, int iState)` |
| `chatSetPosition` | Pozycja w oknie UI. | `(int iID, int ix, int iy)` |
| `chatSetWidth` / `chatSetHeight` | Wymiary bloku czatu. | `(int iID, int iWidth / iHeight)` |
| `chatToggleChatMode` | Przelacza np z Wolania na Zwykly | `(int iID, int iMode)` |
| `chatEnableChatMode` / `chatDisableChatMode` | Filtracja odbieranych w okno typu msg| `(int iID, int iMode)` |
| `chatSetEndPos` | Ustawienie kropki scrolla czatu | `(int iID, float fPos)` |
| `chatGetLineCount` | Ilosc znakow/lini w oknie. | `(int iID)` |
| `chatGetVisibleLineCount` | Co jest renderowane.| `(int iID)` |
| `chatAppendChat` | Najwazniejsza - dodaje wiadomosc do logu. | `(int iType, string szChat)` |
| `chatAppendChatWithDelay` | Dodanie tekstu opoznionego. | `(int iType, string szChat, int iDelay)` |
| `chatIgnoreCharacter` | Dodanie do listy blokowanych. | `(string szName)` |
| `chatIsIgnoreCharacter` | Sprawdzenie czy zablokowany. | `(string szName)` Zwraca bool |
| `chatCreateWhisper` / `chatAppendWhisper`| Interfejsy dla trybu 1v1 PM / PW. | ID okna i wiadomosc. |
| `chatRenderWhisper` | Funkcja per klatke okna szeptu. | `(string szName)` |
| `chatGetLinkFromHyperlink`| Tlumaczenie linku HTML w chatcie. | Zwraca parowane elementy w HTML stringu. |

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
Te moduly to typowe elementy Mostku Pythona. Rejestruja strukture z metoda "s_methods" przy uzyciu `Py_InitModule("miniMap", s_methods)` i `Py_InitModule("chat", s_methods)`.
Z punktu widzenia architektonicznego, NIE sa to obiekty odpowiedzialne za budowanie pakietow w C++. Ich zadaniem jest przekierowanie polecen wizualnych pomiedzy interpretem Pythona wywolujacym kody na Singletonach a kodem w silniku C++. 
Jesli przychodzi wiadomosc po pakiecie (np `HEADER_GC_CHAT`), system sieciowy na poziomie kodu C++ przekazuje zawartosc prosto do `CPythonChat::Instance()->AppendChat()` - Python moze tez zainicjowac dodanie czatu wolajac funkcje C API o tej samej architekturze uzywajac interfejsu modulu "chat".

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Oczekiwane parametry i ich typy:** Wszystkie moduly operujace z systemami uzywaja np. `PyTuple_GetInteger()` i `PyTuple_GetFloat()`. Z uwagi na bezpieczenstwo, zawsze kiedy parsowanie parametru (arg) sie nie uda (np brakuje elementu lub jest innego typu), aplikacja poinformuje poprzez Python Exception (`return Py_BuildException();`). Nigdy nie wywolywac metod z pominieciem sprawdzania wstepnego narzedziami silnika API z Pythona.
- **Bezpieczenstwo Watkowe:** Modul uzywany wylacznie w watku glownym uzywanym do interfejsu (Direct3D Thread / Main Thread). UI Pythona i render sa asynchroniczne tylko z punktu widzenia ladowania zasobow z EterPack/MilesSoundSystem. Nalezy bezwzglednie unikac wywolywania ich ze specyficznych watkow sieci bez upewnienia sie, ze nie sa blokowane dostepem do PyObject i renderowania tekstury okien (DirectX).
- **Globalny Singleton (Instance):** Funkcje korzystaja z ukladow na zasadzie `CPythonMiniMap::Instance().X()`. Agent musi sie upewnic w glownej petli ze singleton dla chat i minimap zostal wlasciwie skonstruowany przed uzywaniem API z poziomu klienta, poniewaz mozna spowodowac wyciek pamieci / nullowy dereference przy starcie uzywajac funkcji Pythona bez zaladowanych klas interfejsu silnika gry.
- **Zarzadzanie pamiecia w WhisperChat:** Dodawanie duzych ilosci pamieci RAM (whisper buffer, chat lines). Limity zazwyczaj sa wymuszone w kodzie bazowym, np max 300 lini historii (zaleznie od EterPythonLib). 

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Krok po kroku zeby dodac nowa wbudowana funkcje:** 
    1. Dodaj nowa, statyczna i bezklasowa metode C `PyObject * modMojaFunkcja(PyObject * poSelf, PyObject * poArgs)`. 
    2. Parsuj argumenty makrami (np `PyTuple_GetInteger` dla `int`, `PyTuple_GetString` dla `stringa`). 
    3. Przekaz zawartosc poprzez metody do `CPythonChat::Instance().MojaFunkcja()`.
    4. Pamietaj dolozyc referencje metody pod nazwa pythonowa w tabeli `s_methods[]`.
    5. Zwracaj zawsze `Py_BuildNone()` jesli wywolanie jest czystym poleceniem `void`.
- **Debugowanie i logi:** Jezeli dane przychodzace od UI nie zgadzaja sie, warto uzywac `TraceError("Tekst Bledu");` do bezposredniego pisania do pliku syserr.txt lub log.txt w kliencie. Dla stringow `const char *` parsowanych, zachowaj uwage na zycie wskaznika wzgledem PyString z Python. 
- **Zasada Headless:** Moduly stricte sa elementami C API dla interpretera skryptow, bardzo polegaja na instancjach singletonowych Render. Podczas mockowania uzywaj "zastepczych" klas Singleton z pominieta logika DirectX dla bezbolesnych i bezpiecznych od strony CI testow.
