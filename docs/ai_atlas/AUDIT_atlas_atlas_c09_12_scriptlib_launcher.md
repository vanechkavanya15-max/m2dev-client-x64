---
task_id: "atlas_c09_12_scriptlib_launcher"
cluster: "PY"
module_name: "PythonLauncher - Start i Konfiguracja Interpretera C-API"
target_files:
- src/ScriptLib/PythonLauncher.cpp
- src/ScriptLib/PythonLauncher.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c09_12_scriptlib_launcher.md"
architecture_layer: "Mostek Pythona, Moduly C-API i Skrypty Gry"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Modul `CPythonLauncher` pelni role menedzera cyklu zycia interpretera Pythona w srodowisku C++ klienta gry. Zapewnia on inicjalizacje srodowiska (`Py_Initialize`), zakonczenie dzialania (`Py_Finalize`), konfiguracje podstawowych modulow oraz bezpieczne wykonywanie skryptow (np. plikow z kodem, ciagow znakow czy skompilowanego kodu bajtowego). Pelni takze kluczowa role w systemie debugowania i przechwytywania wyjatkow rzucanych w Pythonie.
- **Punkt w petli gry:** Klasa nie dziala bezposrednio w glownej petli gry (`OnUpdate` / `OnRender`). Zamiast tego jej inicjalizacja odbywa sie raz podczas uruchamiania klienta (np. w `RunMainScript`), a dzialanie konczy w momencie wylaczania aplikacji. Wykorzystywana jest glownie do uruchamiania glownych skryptow w fazie bootowania gry.
- **Przeplyw danych:** 
  1. Wywolanie konstruktora inicjuje standardowe moduly i srodowisko interpretera.
  2. Wywolanie `Create()` tworzy przestrzen nazw dla modulu `__main__`, dodaje wbudowane stale `TRUE` i `FALSE` do slownika `builtins` oraz laduje glowny modul systemowy `sys`.
  3. Nastepnie uruchamiane sa zrodlowe pliki skryptowe (np. `system.py` czy `prototype.py`) wczytywane najczesciej poprzez system Wirtualnego Systemu Plikow (`CPackManager`).
  4. Wykonanie przebiega bezposrednio za pomoca interfejsu C-API Pythona (`PyRun_String`, `PyEval_EvalCode`).
  5. Jesli w skrypcie pojawiaja sie bledy, sa one zglaszane i obrabiane przez mechanizmy wewnetrzne, rzutowane uzywajac interfejsu trace'ow Pythona, a ostatecznie zrzucane do logu w C++ przez `Traceback()`.
- **Cykl zycia:** Alokacja odbywa sie na ogol raz w glownym watku klienta C++. Konstruktor alokuje srodowisko i flage izolacji (np. `Py_FrozenFlag = 1`), a `Py_Initialize()` stawia instancje w pamieci. Wywolanie destruktora badz bezposrednie wywolanie metody `Clear()` zwalnia interfejs interpretera C-API za pomoca wywolania `Py_Finalize()`. Instancja zyje przez caly okres dzialania gry i dziedziczy z `CSingleton`, rejestrujac siebie pod publiczny wskaznik singletonu.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):**
  - Funkcje z glownego zarzadcy logiki rozruchu UI (np. `src/UserInterface/UserInterface.cpp`, z wnetrza procedury uruchamiania i `src/UserInterface/PythonApplicationModule.cpp` ktory bezposrednio moze wywolywac wykonanie plikow skryptowych `RunFile()`).
  - System inicjalizacji podzespolow uzywa bazowego `CSingleton<CPythonLauncher>`.
- **Zaleznosci wyjsciowe (Outbound):**
  - **Python C-API:** Pliki naglowkowe np. `python/frameobject.h` oraz biblioteka standardowa C-API pozwalajaca wykonac `Py_Initialize`, kompilacje (`Py_CompileString`) i ocene kodu (`PyEval_EvalCode`), obsluge wyjatkow (`PyErr_Fetch`, `PyErr_NormalizeException`).
  - **VFS (Virtual File System):** `PackLib/PackManager.h` (metoda `CPackManager::Instance().GetFile`) dla ekstrakcji plikow Pythonowych (.py i .pyc) z zahaslowanych lub spakowanych archiwow gry.
  - **Rejestrowanie (Logging):** Wywolania obslugi C++ i bledow (`LogBoxf`, `Tracef`).
  - **System Kodowania (Encoding):** `utf8.h` uzywane np. do konwersji znakow i parsowania w wywolaniach Windowsa podczas `RunCompiledFile` (`Utf8ToWide`).
- **Drzewo dyrektyw `#include`:** 
  - `StdAfx.h`
  - `python/frameobject.h`
  - `PackLib/PackManager.h`
  - `PythonLauncher.h`
  - `PythonModules/frozen_modules.h`
  - `utf8.h`
  - Uwaga: W naglowku klasy znajduje sie hack `#ifdef BYTE ... #undef BYTE` obchodzacy problem kolizji typow prostych narzucanych ze strony API Windows oraz API C.
- **Model pamieciowy:** Dominuja surowe wskazniki obiektow Pythona C-API (`PyObject*`), dla ktorych uzywane jest reczne zliczanie referencji (`Py_DECREF`, `Py_XDECREF`). Wskazniki pamieci VFS ze srodowiska C++ ladowane sa glownie bezposrednio na wektory `std::string` a nastepnie kopiowane przed inicjalizacja modulu Pythona. Modul posiada stale referencje z wlascicielskim slownikiem: `m_poModule` i `m_poDic`. Brak inteligentnych wskaznikow C++ do zasobow z wnetrza intepretera.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
| Nazwa / Typ | Rola | Wielkosc w pamieci | Wlasciciel Watku |
| --- | --- | --- | --- |
| `CPythonLauncher` | Glowny zarzadca uruchamiania srodowiska interpretowalnego, lacznik srodowiska Python z C++. Obsluguje zarzadzanie paczkami oraz hooki stack trace'a interpretera. | ~24-32 Bajtow dla klasy 64-bit bazowej singletonu i dwoch wskaznikow zewn. obiektu modulu. | Glowny watek aplikacji (Main Thread) |

**Tabela Metod Publicznych (`CPythonLauncher`):**
| Sygnatura | Typy argumentow / Zwracany typ | Opis |
| --- | --- | --- |
| `CPythonLauncher()` | `void` -> `void` | Konstruktor. Odpala stale globalne w `CPythonLauncher` (w tym `InitStandardPythonModules`) oraz tworzy srodowisko bazowe interpretera poprzez `Py_Initialize`. |
| `~CPythonLauncher()` | `void` -> `void` | Destruktor. Bezpiecznie wywoluje funkcje czyszczaca (`Clear()`). |
| `void Clear()` | `void` -> `void` | Zwalnia pamiec intepretera oraz finalizuje jego dzialanie (`Py_Finalize()`). |
| `bool Create()` | `void` -> `bool` | Glowna inicjalizacja logiczna. Deklaruje `__main__`, laduje do niego i srodowiska `builtins` aliasy liczbowe (`TRUE=1`, `FALSE=0`), laduje globalnie modul `sys`. Zwraca flage operacji. |
| `void SetTraceFunc(...)`| Wskaznik funkcyjny. | Ustawia uzytkownika do callbacku sledzacego frame stack podczas wykonywania kodu Pythona (przydatne do debugowania kazdej uzytej lini). |
| `bool RunLine(...)` | `const char*` -> `bool` | Pozwala na proste i szybkie zinterpretowanie dowolnego stringu jako linijki i wiersza w C-API (`PyRun_String`). Zwraca `false` na bledzie w konsoli. |
| `bool RunFile(...)` | `const char*` -> `bool` | Laduje zewnetrzny kod skryptu (tekstu plain `.py`) prosto z Wirtualnego Systemu Plikow (`CPackManager`), parsuje usuwajac zbedne `\r` (carriage returns) oraz natychmiast go kompiluje i ewaluuje (PyEval). |
| `bool RunMemoryTextFile(...)`| `const char*`, `UINT`, `const VOID*` -> `bool` | Wrapuje podany surowy blok pamieci `c_pvFileData` ze znakami jako wieloliniowy string, tworzac i wolajac specjalne makro interpretera `exec(compile(...))` aby zrealizowac jego tresc "w locie". |
| `bool RunCompiledFile(...)`| `const char*` -> `bool` | Odczytuje kod binarnego `.pyc`, weryfikujac tzw. Magic Number wersji Pythona i ewentualne offsety czasu oraz wagi. Odczytany marszalizowany obiekt jest puszczony do eval'a. |
| `const char* GetError()` | `void` -> `const char*` | Wyciaga biezace bledy interfejsu (jezeli istnieja). Przeksztalca rzucony tam obiekt Pythonowy w prostego sformatowanego stringa C++. |

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
Instancja `CPythonLauncher` jest Singletonem dziedziczacym z zewnetrznego szablonu (przejmuje wlasciwosci klas bazowych i ewentualnie pamiec wskaznika statycznego `ms_singleton`). Nalezy zalozyc obecnosc kluczowych offsetow dla 64-bitowego kodu:
- **`m_poModule`** (offset: zaraz po obiektach bazowych) - typu `PyObject*`, sluzy jako instancja C-API przestrzeni modulow main aplikacji.
- **`m_poDic`** - typu `PyObject*`, instancja slownika (`dict`) w srodowisku C-API dla przestrzeni srodowiskowej (czesto przypisana na to samo srodowisko modulow `__main__`).
Wewnatrz pliku `.cpp` deklarowane sa bufory zrzutow trace, takie jak publiczny, pamieciowy global `g_stTraceBuffer` (wielkosci 512 komorek std::string) oraz indykator stanu ramki logowej `g_nCurTraceN`.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Brak opcode'ow, nie przetwarza sieciowych pakietow ani zadan z wnetrza struktury (jest systemowym srodowiskiem hosta dla skryptow).
- **Mostkowanie C-API:**
  Instancja w bezposredni sposob podaje srodowisku Python globalne i wbudowane aliasy poprzez:
  ```cpp
  PyModule_AddIntConstant(builtins, "TRUE", 1);
  PyModule_AddIntConstant(builtins, "FALSE", 0);
  PyDict_SetItemString(m_poDic, "__builtins__", builtins);
  ```
  Obrabianie rzuconych bledow w C++ za pomoca integracji rzutujacej obiekt `traceback` (import na locie wewnetrznie i wykonywanie modulu w razie `Traceback()` do wylapania formatu exception stacka uzywajac `PyObject_CallFunction`).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Obiekt modulu ORAZ WSZELKIE wywolania Pythona MUSZA byc wykonywane wYLACZNIE przez glowny watek programu. Interfejs zaden nie wymusza blokad miedzywatkowych (GIL – `Global Interpreter Lock` stanowy w C-API nie jest chroniony tu jako `PyGILState_Ensure`). Wywolanie jakiegokolwiek modulu interpretera na bocznym watku doprowadzi od reki do fatalnego bledu programu.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Brak lub usterki archiwow z VFS. Puste stringi ladowane za pomoca bezposrednich pakietow wyrzuca pusty blad (brak pliku lub odczytu).
  - Skompilowany skrypt (.pyc) z zupelnie inna sygnatura magiczna wersji Pythona niz ta aktualnie kompilowana obok zrodla. Wyrzuca on wowczas twardy RuntimeError: "Bad magic number in .pyc file" - odpalany przez wewnetrzne `PyMarshal_ReadLongFromFile`.
  - Przepelnienia globalnej ilosci zarejestrowanych tablic logowych, aczkolwiek wylapane prewencyjnie w `TraceFunc` dzieki sprawdzaniu limitu `>= 512` na `g_nCurTraceN`.
- **Zarzadzanie zasobami (RAII):** Silna opartosc i bezwzglednosc interfejsu referencyjnego z C-API Pythona. Obiekty alokowane lub przekazywane z interpretera do srodowiska C++ (`code`, `result`, `exc`, `v`, `tb` itd.) musza zostac zawsze recznie usuniete za pomoca interfejsow redukujacych wskaznik referencyjny (`Py_DECREF` lub sprawdzone warunkowo bezpieczne `Py_XDECREF`). Brak stosowania std::unique_ptr narzaza instancje na wycieki pamieci operacyjnej w przypadku wyjsc awaryjnych z sekwencji czy rzucania twardych bledow wczesniej na warunku.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zaimportuj naglowek nowej funkcjonalnosci jesli trzeba (najpewniej nowy parser dla stringa).
  2. W pliku `PythonLauncher.h` deklaruj swoja klase i metode pod modyfikatorami public/protected w klasie `CPythonLauncher`.
  3. Realizujac plik w `PythonLauncher.cpp`, przed bezposrednim zwrotem wezlem decyzyjnym pamietaj o sprzataniu referencji wyciagnietych na surowych wskaznikach obiektow PyObject i zawsze uwzgledniaj wariant wylapanej zmiennej bledu (PyErr_Occurred).
  4. Nie wolno ci wprowadzac lockow miedzywatkowych lub przenosic czesci inicjalizacji interpretera na odmienne watki asynchroniczne.
- **Jak debugowac i logowac:** Przechwycenie wyjatku i sledzenie linii wyzwalanej w trybie uruchomieniowym powinno byc robione poprzez przeglad wywolan w terminalu SysErr badz C++ Log Console - klasa bazowa posiada uformowany i ladnie sformatowany rzut logu `Traceback()` lapiacy wyjatek krok po kroku wykorzystujac zaszyta klase loga na modulu tracebacku systemowego C-API. Zmienna logu stosowego jest latwa do sprawdzenia w locie.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Jezeli chcesz wykonywac jednostkowe testy operacji i nie budowac zbednych zaleznosci (takich jak ladowanie pelnych podsystemow silnika gry z UserInterface i calym silnikiem EterLib lub VFS PackManager), mozesz wywolac konstruktor `CPythonLauncher`, spiac srodowisko uzywajac wczesnego `Create()`, a potem bezposrednio sprawdzac wtryski instrukcji na pamieci poslugujac sie jedynie zewnetrznie funkcja `RunMemoryTextFile` lub wlasnym interfejsem dla ciagu lini `RunLine()`. Odpadaja wszystkie mechanizmy ladowania skryptu gry z paczki dyskowej, unikajac w ten sposob zaleznosci do ladowarki VFS.
