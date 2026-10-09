---
task_id: "atlas_c09_14_py_interned_strings"
cluster: "PY"
module_name: "PythonInternedStrings - Pula Statycznych Symboli Pythona"
target_files:
- src/EterPythonLib/PythonInternedStrings.cpp
- src/EterPythonLib/PythonInternedStrings.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c09_14_py_interned_strings.md"
architecture_layer: "Mostek Pythona, Moduly C-API i Skrypty Gry"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Modul `PythonInternedStrings` odpowiada za ekstremalna optymalizacje wywolan z przestrzeni C++ do skryptow Pythona (glownie interfejsu uzytkownika UI) podczas pracy klienta gry Metin2. Zamiast dokonywac powtarzalnych, kosztownych alokacji, de-alokacji i haszowania lancuchow znakow w kazdej klatce przy tworzeniu obiektow takich jak "OnUpdate" czy "OnRender", modul internuje powszechnie uzywane nazwy metod w zoptymalizowany sposob. 

Przeplyw danych: Metoda `Initialize` jest wywolywana z poziomu `PythonWindowManager` podczas inicjalizacji UI i powoluje globalne stale stringi Pythona (`PyObject*`) poprzez uzycie `PyUnicode_InternFromString`. Od tej pory metody klas takich jak `PythonWindow` bezposrednio posluguja sie przygotowanymi referencjami przy probach odpalenia zdarzen przez uzycie bezpiecznej metody `Call()`. Na sam koniec dzialania aplikacji modul wykorzystuje funkcje `Finalize` usuwajaca wczesniej zarejestrowane referencje poprzez uzycie makra `Py_CLEAR`.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound):** 
  - `src/EterPythonLib/PythonWindowManager.cpp` - steruje cyklem zycia (Initialize / Finalize).
  - `src/EterPythonLib/PythonWindow.cpp` - generuje stale strzaly logiki (np. OnUpdate, OnRender, OnMoveWindow, OnPressEscapeKey).
  - `src/EterPythonLib/PythonSlotWindow.cpp` - zglasza zdarzenia zwiazane ze slotami ekwipunku (np. OnSelectEmptySlot, OnSelectItemSlot, OnUseSlot, OnOverInItem, OnPressedSlotButton).

- **Zaleznosci wyjsciowe (Outbound):** 
  - API C Pythona: makra ref-countingowe (`Py_DECREF`, `Py_XDECREF`, `Py_CLEAR`), makra konwersji (np. `PyLong_FromLong`) oraz funkcje wywolan `PyObject_GetAttr`, `PyObject_Vectorcall`.

- **Drzewo dyrektyw `#include`:** 
  - W `PythonInternedStrings.h`: dolacza `<python/python.h>` (obudowane logika #undef dla deaktywacji _DEBUG na lib-release).
  - W `PythonInternedStrings.cpp`: dolacza `"StdAfx.h"` oraz `"PythonInternedStrings.h"`. Modul wolny od zaleznosci cyklicznych.

- **Model pamieciowy:** 
  Klasa nie zawiera zmiennych niestatycznych (rozmiar samej instancji to 0). Posiada serie statycznych wskaznikow na obiekty surowe C Pythona (`PyObject*`), na ktorych opiera sie obsluga wskaznikow za pomoca dedykowanego ref-countingu.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)
**Tabela Klas i Struktur:**
- `UI::PythonInternedStrings`: Klasa narzedziowa operujaca z wylacznoscia dla glownego watku.

**Tabela Metod Publicznych:**
- `static void Initialize()`: Wywoluje internowanie. Pre-condition: Interpreter Pythona jest uruchomiony.
- `static void Finalize()`: Zwalnia zarejestrowane zasoby wykorzystujac czyszczenie makrem `Py_CLEAR`.
- `static bool Call(PyObject* poInstance, PyObject* poMethodName, PyObject* const* args, size_t nargs, PyObject** ppoRet)`: Rdzen podzespolu do zoptymalizowanego Vectorcall wywolujacego metody w obiekcie poInstance. Bezpiecznie obsluguje usuwanie alokowanych stringow/wyjatkow z call-stacka jesli wywolanie sie nie powiedzie.
- Warianty `Call(...)` (z long, unsigned long itp.): Sluzace do opakowania elementarnych typow w `PyLong_FromLong`, nastepnie redukowanych z referencji. Zwraca flagowe zatwierdzenie istnienia (bool) dla poprawnego strzalu metody.
- `static bool CallWithReturn(PyObject* poInstance, PyObject* poMethodName, long* plRet)`: Wariant `Call` odzyskujacy i rzutujacy na typ C wynik z Pythona dla prymitywow.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
Instancja jest calkowicie pusta. W systemie globalnym istnienie pol jest rejestrowane jako ciag czystych (null-inited) wskaznikow `PyObject*` (m.in OnUpdate, OnRender, OnUseSlot, RefreshStatus). Zadne inne statyczne dane nie istnieja.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Pakiety Sieciowe:** Kod nie obsluguje logiki bezposredniej mapowanej po kodach OP (np. 0x01).
- **Metody Pythona (`PyMethodDef`):** Ten modul C++ sluzy wylacznie do powolywania wywolan zdarzen odgornie nakreslonych po stronie interpretera Pythona. Nie eksportuje do Pythona wlasnego C-API. Mapuje jedynie zachowania narzedzi na standardowy system wywolan UI z warstwy klienta (`ui.py`).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Wszystkie dzialania klas oraz wywolania pod-metod musza odbywac sie w glownym watku trzymajacym obiekt Python GIL (Global Interpreter Lock). Modul padnie jesli sprobujesz uruchomic `Call` z watku pobocznego z powodu naruszen refcountu C-API.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** 
  1) Bledy zwiazane ze stosem. Nalezy bezwzglednie pamietac o fakcie, ze Vectorcall podaje standardowy rozmiar w parametrze `nargs`, jednak nie uzywa flagi `PY_VECTORCALL_ARGUMENTS_OFFSET` aby chronic parametry wejsciowe przed korupcja tablicy `args`. 
  2) Nullowe uchwyty lub zwalnianie metod podczas `Initialize` gdy ktos wywoluje event po zawieszeniu obiektu UI. Kod korzysta tu juz ze straznikow dla `!poInstance` oraz `!poMethodName`.
- **Zarzadzanie zasobami (RAII):** Referencje argumentow (tworzone z `PyLong_FromLong`) sa skrupulatnie uwalniane po wykonaniu strzalu funkcji niezaleznie czy funkcja odniosla zamierzony sukces poprzez instrukcje `Py_DECREF`. 

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Dodaj nowa statyczna nazwe w `PythonInternedStrings.h` w postaci `static PyObject* OnNowyEvent;`.
  2. W pliku `PythonInternedStrings.cpp` zadeklaruj poczatkowa wartosc inicjujaca w ciele klasy: `PyObject* PythonInternedStrings::OnNowyEvent = nullptr;`.
  3. W metodzie `Initialize` doloz alokacje stringu: `OnNowyEvent = PyUnicode_InternFromString("OnNowyEvent");`.
  4. W metodzie `Finalize` zabezpiecz dealokacje poprzez `Py_CLEAR(OnNowyEvent);`.
  5. W klasach wlascicielach (jak `PythonWindow`) wykonuj nowa metode operujac wylacznie poprzez API z modulem (np. `UI::PythonInternedStrings::Call(...)`).
- **Jak debugowac i logowac:** Do wykrycia problemow ze stanami logicznymi przy `PyObject_Vectorcall` uzyj bezposredniego makra `PyErr_Print()`, co automatycznie spowoduje zapis w pliku `sys.stderr`. Ustaw Breakpoint na wchodzeniu do `PyObject_GetAttr(poInstance, poMethodName)` zeby zbadac czy wywolywany instans istnieje w tablicy wirtualnej interpretera.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Kontekst da sie zamockowac uzywajac standardowego pre-buildowanego narzedzia Python w trybie headless, ktore powola wirtualny stan wywolujac `Py_Initialize()` przed uruchomieniem czesci unitowej dla metody `Call`. W testach warto wylacznie uzyc pustego obiektu, do ktorego podpina sie pusta definicje w Pythonie i analizowac wycieki memory-leak (np. w Valgrind) sprawdzajac referencje na poszczegolnych krokach po zakonczeniu testu za posrednictwem `Py_REFCNT(obj)`.
