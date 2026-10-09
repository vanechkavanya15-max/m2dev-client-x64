---
task_id: "atlas_c09_13_scriptlib_utils"
cluster: "PY"
module_name: "PythonUtils - Bezpieczne Konwersje Typow PyObject <-> C++"
target_files:
- src/ScriptLib/PythonUtils.cpp
- src/ScriptLib/PythonUtils.h
- src/ScriptLib/PythonDebugModule.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c09_13_scriptlib_utils.md"
architecture_layer: "Mostek Pythona, Moduly C-API i Skrypty Gry"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Modul `PythonUtils` pelni role krytycznej warstwy posredniczacej (mostka) miedzy osadzonym interpreterem Pythona (skryptami UI oraz mechanika gry) a natywnym kodem C++ klienta Metin2. Zapewnia zestaw narzedzi do obslugi konwersji typow w obie strony - rozpakowywania argumentow z krotek Pythona (warianty `PyTuple_Get*`) oraz bezpiecznego wywolywania metod klas Pythona z poziomu C++.

Dodatkowo, modul ten zawiera implementacje obslugi bledow (`Py_BuildException`), propagacji rzucanych wyjatkow za posrednictwem interfejsu `IPythonExceptionSender` oraz specyficzne makra pomagajace w ladowaniu modulow, zgodnie z konwencjami C-API. Wystepuje tam rowniez warstwa kompatybilnosci miedzy Python 2 i Python 3 dla ciagow znakow (np. `PyString_Check`).

`PythonDebugModule` to dedykowany modul rozszerzenia eksportowany do Pythona pod nazwa `dbg`. Dostarcza on zestaw funkcji pozwalajacych skryptom na zapisywanie do logow systemowych (np. `LogBox`, `Trace`, `TraceError`) oraz rejestracje logiki zbierania i zrzucania informacji o bledach, co ulatwia diagnostyke.

**Miejsce w petli gry:**
Kod ten nie jest przypisany do pojedynczego punktu w petli gry (OnUpdate czy OnRender). Zamiast tego jest uzywany ad-hoc wszedzie tam, gdzie nastepuje wymiana informacji ze skryptami Pythona - na przyklad podczas obslugi eventow UI, odpowiedzi na pakiety sieciowe generujace logike skryptowa, czy podczas inicjalizacji systemow i ladowania gui.

**Przeplyw danych i Cykl zycia:**
1. Alokacja obiekty (PyObject) zachodza po stronie Pythona (lub w C++ za pomoca `Py_BuildNone`, `PyLong_FromLong`).
2. Parametry z Pythona przylatuja jako krotka (Tuple).
3. Obiekt/Krotka jest poddawany ekstrakcji funkcji z rodziny `PyTuple_Get*`, ktore zwracaja surowe typy C++ (np. `long`, `bool`, `char*`). Posiadaja one bezposrednia walidacje zakresow oraz sprawdzanie typow (zaleznie od typu implementacji).
4. Do zwrotu danych z C++ uzywa sie odpowiednich makr, ew. zwraca sie standardowe `Py_None` przez `Py_BuildNone()`. Dealokacja w wiekszosci przypadkow odbywa sie automatycznie (w Pythone) za posrednictwem wbudowanego licznika referencji, pod warunkiem prawidlowego uzywania `Py_INCREF` / `Py_DECREF`.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
- Systemy UI np. moduly okien UI (PythonWindow, UI, menedzer okien).
- Subsystemy gry powiazane bezposrednio z Pythonem (np. system itemow, gildii, sieci - w skryptach odpowiadaja za to CPythonNetworkStream, CPythonGuild, itp.).
- Kazdy podsystem C++, ktory wykorzystuje makra czy funkcje ulatwiajace operacje na obiekcie PyObject* oraz obiekcie Tuple (jak parser packetow sieciowych jesli deleguje event do UI).
- Obiekty klas zdefiniowanych z uzyciem C-API Pythona (`PyMethodDef`).

**Zaleznosci wyjsciowe (Outbound):**
- Python C-API (np. `PyObject_CallObject`, `PyTuple_GetItem`, `PyLong_AsLong`, `PyErr_Print`).
- Zewnetrzny modul zbierania logow: globalny `IPythonExceptionSender* g_pkExceptionSender`, wykorzystywany do zewnetrznego raportowania wyjatkow.
- Interfejs logowania gry z EterLib lub podobnych (np. `LogBox`, `Trace`, `Tracen`, `TraceError`).

**Drzewo dyrektyw `#include`:**
- `"StdAfx.h"`: Standardowy prekompilowany naglowek projektu, zazwyczaj zawierajacy wszystkie najczesciej uzywane wlaczenia z WinAPI oraz STL, w tym sam naglowek Pythona `Python.h`.
- `"PythonUtils.h"`: Zawiera makra oraz deklaracje helperow. Brak groznego uwiklania, poniewaz bazuje wylacznie na Python C-API.

**Model pamieciowy:**
Zarzadzanie pamiecia odbywa sie bezposrednio na klasycznych wskaznikach typu `PyObject*`. Nie ma tutaj stosowanych nowoczesnych inteligentnych wskaznikow (np. `std::unique_ptr` czy `std::shared_ptr`). Jest to uwarunkowane natywnym interfejsem C interpretera Pythona. Wszelkie obiekty posiadaja logike Reference Counting zarzadzana recznie przez `Py_INCREF` i `Py_DECREF`.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Metod Publicznych i Helperow (w `PythonUtils`):**
- `PyObject* Py_BuildNone()`: Tworzy nowa referencje (Py_INCREF) na Py_None i ja zwraca.
- `PyObject* Py_BuildException(const char* c_pszErr, ...)`: Rzuca RuntimeError w Pythonie z obsluga argumentow formatujacych vargs.
- `bool PyTuple_GetString(PyObject* poArgs, int pos, char** ret)`: Ekstraktuje ciag znakow. Uwaga: Zwraca staly wskaznik do bufora wewnetrznego PyObject, bez kopiowania!
- `bool PyTuple_GetInteger(PyObject* poArgs, int pos, int* ret)` / `bool PyTuple_GetLong(...)`: Konwersje ze sprawdzeniem indeksu.
- `bool PyTuple_GetFloat(...)`, `bool PyTuple_GetDouble(...)`: Odczytywanie wartosci rzeczywistych przez `PyFloat_AsDouble`.
- `bool PyCallClassMemberFunc(PyObject* poClass, const char* c_szFunc, PyObject* poArgs)`: Bezpieczne wywolanie metody na obiekcie klasowym. Przechwytuje bledy i wysyla do g_pkExceptionSender.

**Tabela Metod Publicznych w module `dbg` (w `PythonDebugModule`):**
- `dbgLogBox(PyObject* poSelf, PyObject* poArgs)`: LogBox i message box (do interfejsu graficznego).
- `dbgTrace(PyObject* poSelf, PyObject* poArgs)`: Logowanie (bez znaku nowej linii na koncu).
- `dbgTracen(PyObject* poSelf, PyObject* poArgs)`: Logowanie (ze znakiem nowej linii).
- `dbgTraceError(PyObject* poSelf, PyObject* poArgs)`: Logowanie bledu.
- `dbgRegisterExceptionString(PyObject* poSelf, PyObject* poArgs)`: Rejestracja stringa identyfikujacego wyjatek do sendera wyjatkow.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
Moduly te nie wprowadzaja nowych wlasnych struktur narazonych na bezposrednie wyrownania bitowe - bazuja na hermetycznych, abstrakcyjnych typach C Pythona.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

**Pakiety Sieciowe:**
Sam modul `PythonUtils` nie parsuje bezposrednio danych sieciowych z warstwy Winsock / `CNetworkStream`. Pelni za to istotna posrednia role - sluzy zewnetrznym modulom np. `CPythonNetworkStreamModule` do poprawnej interpretacji obiektow przekazywanych w metodach udostepnianych w Pythonie, za pomoca funkcji `PyTuple_Get*`. To na ich wyjsciu formowane sa dane binarne trafiajace prosto do pakietow wysylanych na serwer.

**Metody Pythona (`PyMethodDef` w `initdbg`):**
W pliku `PythonDebugModule.cpp`, definiowana jest scisla tablica eksportujaca metody dla modulu `dbg`:
- `LogBox` -> C++ `dbgLogBox` (format `METH_VARARGS`)
- `Trace` -> C++ `dbgTrace` (format `METH_VARARGS`)
- `Tracen` -> C++ `dbgTracen` (format `METH_VARARGS`)
- `TraceError` -> C++ `dbgTraceError` (format `METH_VARARGS`)
- `TraceTemp` -> C++ `dbgTraceTemp` (format `METH_VARARGS`)
- `TraceTempn` -> C++ `dbgTraceTempn` (format `METH_VARARGS`)
- `RegisterExceptionString` -> C++ `dbgRegisterExceptionString` (format `METH_VARARGS`)

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Zasady wielowatkowosci:**
  Zewnetrzne biblioteki Python C-API nie sa w pelni thread-safe z uwagi na mechanizm Global Interpreter Lock (GIL). Skrypty i interfejs UI modulu uruchamiane sa glownie w jednym watku gry (Main Thread). Zewnetrzne uzycie z innych watkow wymaga manualnego obchodzenia sie z blokadami, jednak ten modul sam z siebie tego nie robi.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  1. Wywolywanie funkcji z krotki z niepoprawnym indeksem (`pos >= PyTuple_Size(poArgs)`). Helpery bezpiecznie to wychwytuja zwracajac `false`, ale jesli logika wywolujaca nie sprawdzi ret-value, moze nastapic awaria (dane nieustalone/smieci).
  2. Typ w krotce nie zgadza sie z oczekiwanym. Np. dla typu double uzywana jest bezposrednio funkcja `PyFloat_AsDouble` - jezeli to nie byla liczba, Python postawi flage bledu. Nalezy to sprawdzac z poziomu PyErr_Occurred() po powrocie, jesli funkcja tego nie hermetyzuje, zeby zapobiec niekontrolowanemu bledowi typu Type mismatch.
  3. `PyString_AsString` zwraca referencje wewnetrzna bez inkrementacji RC. Zmiana obslugi pamieci (lub Dealokacja) po stronie PyObject usunie ten string z pamieci tworzac natychmiast 'dangling pointer'.
- **Zarzadzanie zasobami (RAII):**
  Wystepuje silny nacisk na makra `Py_DECREF` i `Py_XDECREF` po uzyciu tymczasowych obiektow lub krotek, aby uniknac memory leakow, w szczegolnosci w `__PyCallClassMemberFunc_ByCString`. Uzycie RAII `pybind11` czy smart pointerow mogloby zniwelowac te uciazliwe zaleznosci manualne.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  Aby dodac nowa metode do obiektu klasy lub globalna utilse do pobierania nowych typow np. `vector3`:
  1. Zdefiniuj naglowek w `PythonUtils.h` z sygnatura wzorowana na pozostalych (np. `bool PyTuple_GetVector3(PyObject* poArgs, int pos, D3DXVECTOR3* ret)`).
  2. Napisz implementacje w `PythonUtils.cpp`, ostroznie zarzadzajac PyTuple_GetItem. Pamietaj ze zwraca on "Borrowed Reference", wiec nie uzywaj na nim `Py_DECREF`.
  3. Odczytaj typy skladowe uzywajac makr zdefiniowanych w naglowku i zapisz pod wskaznikiem.
  4. Jezeli w PythonDebugModule dodajesz nowa metode dla skryptu to dodaj ja bezposrednio do tabeli `s_methods` w strukturze `PyMethodDef` uzywajac flag `METH_VARARGS` i pamietajac o koniecznym `Py_BuildNone()` na koncu.
- **Jak debugowac i logowac:**
  Najprostszym wyjsciem na przechwytywanie logow ze skryptow z bledem jest uzycie modulu `dbg`, a z poziomu C++ wypisujac dane na stdout lub za pomoca systemowego `TraceError`. Aby wylapac bledne wywolania, polecam sprawdzanie stosu rzucanych wyjatkow za pomoca `PyErr_Print()`, co ulatwia identyfikacje "Exception in function call".
- **Jak testowac bez interfejsu graficznego:**
  Modul ten mozna testowac uzywajac zwyklych testow jednostkowych w C++ na obiekcie inicjalizujacym instancje Pythona poprzez `Py_Initialize()`. Mockowanie jest proste, bo kod opiera sie jedynie o oficjalne C-API Pythona. Po podpieciu makiet, obiekty `PyTuple` mozna wygenerowac i testowac poprawnosc `PyTuple_GetInteger` z waznymi/blednymi danymi w asercjach.
