---
task_id: "atlas_c09_08_py_wndmgr_module"
cluster: "PY"
module_name: "Modul Pythona 'wndMgr' - Zarzadzanie Graficznym UI ze Skryptow"
target_files:
- src/EterPythonLib/PythonWindowManagerModule.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c09_08_py_wndmgr_module.md"
architecture_layer: "Mostek Pythona, Moduly C-API i Skrypty Gry"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- Jaka jest dokladna funkcja tego modulu w architekturze klienta.
Modul `wndMgr` pelni role gwnego mostka (Python C-API) pomiedzy warstwa skryptowa (Python UI) a wewnetrznym silnikiem interfejsu uzytkownika C++ w kliencie Metin2. Modul umozliwia zarzadzanie instancjami klas zdefiniowanych w przestrzeni `UI::CWindow` oraz wszystkich jej podklas (np. `UI::CSlotWindow`, `UI::CImageBox`). Dzieki niemu programisci interfejsu moga, za posrednictwem skryptow w jezyku Python, dynamicznie tworzyc, konfigurowac (np. `wndMgr.SetPosition`, `wndMgr.SetDiffuseColor`), aktualizowac i ukrywac elementy GUI ekranu (m.in. ekwipunek, menu postaci, sloty umiejetnosci).

- W jakim momencie petli gry (OnUpdate / OnRender / Network Tick) ten kod jest wywolywany.
Metody tego modulu moga byc wywolywane z dwoch potencjalnych obszarow. Przede wszystkim glowny watyk wykonawczy Pythona wykonuje ten kod podczas inicjalizacji widzetow podczas ladowania gry lub poszczegolnych jej modulow (skrypty `*.py` sluzace za definicje ekranow), a nastepnie kod ten jest uruchamiany glownie w fazie Update interfejsu uzytkownika (reakcja na zdarzenia, np. najechanie myszka - hover). Z uwagi na charakter jednowatkowy glownego wezla Pythona z zasady te sa one obslugiwane synchronicznie bez uzycia asynchronicznych eventow co czyni ze narzuty bezposrednio wplywaja na ogolny klatkaz (framerate).

- Pelny opis przeplywu danych (Control Flow & Data Flow) krok po kroku.
Przeplyw sterowania jest nastepujacy: 
1. Skrypt Pythona wywoluje API modulu z uzyciem nazw metod wyeksportowanych w definicji `PyMethodDef s_methods[]`, np. `wndMgr.Register(...)` lub `wndMgr.Show(...)`. 
2. Przeplyw przechodzi przez API C (CPython), przekazujac wskaznik okna i jego argumenty z warstwy skryptu w postaci krotki (tuple/args) do modulu C++. 
3. Funkcje wrapujace (np. `wndMgrShow`) konwertuja argumenty Pythona przy uzyciu helperow takich jak `PyFastCall_GetWindow` czy `PyTuple_GetWindow`, weryfikujac ich poprawnosc. Instancje okien sa przesylane jako numeryczne wskazniki 64-bitowe.
4. Funkcja wywoluje docelowa metode silnika GUI C++, jak np. `pWin->Show()`.
5. Jezeli konieczne jest zgloszenie wyjatku do Pythona w przypadku blednych parametrow (brak referencji, zly format zmiennych), wykorzystywana jest konwencja `return Py_BuildException();`. W innych przypadkach zwracane sa wartosci konwertowane przez `Py_BuildValue` albo specjalny makrodef `Py_RETURN_NONE`.

- Cykl zycia obiektow (Lifecycle: alokacja, inicjalizacja, reset, dealokacja).
Obiekty okien, ktore modyfikujemy przez API `wndMgr`, sa tworzone glownie poprzez wywolania w przestrzeni `UI::CWindowManager` m.in. `UI::CWindowManager::Instance().RegisterWindow(po, szLayer)`. Zarzadzanie zasobami i instancjami odbywa sie po stronie warstwy C++, przy uwzglednieniu wlasnych klas menadzera (np. hierarchiczne drzowo okien, warstwy - layers). Obiekty zniszczone z Pythona zazwyczaj wywoluja `wndMgrDestroy`, co deallokuje pamiec przez usuniecie zasobow po stronie silnika C++, bez opierania sie na standardowym systemie GC (Garbage Collector) z Pythona. Wskazniki przekazywane sa jako wartosci `unsigned long long` co minimalizuje narzuty, natomiast grozi bledami uzycia zdealokowanej pamieci po zniszczeniu okna.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Skrypty interfejsu uzytkownika i systemy zarzadzajace GUI zaimplementowane w Pythonie (np. `ui.py`, `uiInventory.py`). To one dokonuja bezposrednich callow do metod modulu `wndMgr`. Dodatkowo klasy dziedziczace po Window (UI::CWindow) w EterPythonLib uzywaja go.
- **Zaleznosci wyjsciowe (Outbound):** Zaleznosci to fundamentalne pule C++ z przestrzeni `UI` oraz rdzenne moduly silnika: `UI::CWindowManager` (Singleton dla menagowania hierarchii interfejsu uzytkownika), obiekty konkretnych klas dziedziczacych np. `UI::CWindow`, `UI::CSlotWindow`, `UI::CImageBox`, obiekty graficzne jak `CGraphicTextInstance`. Takze polega bezposrednio na oficjalnym C API Pythona (`Python.h`).
- **Drzewo dyrektyw `#include`:** Dolaczone naglowki to: `"StdAfx.h"`, `"PythonWindow.h"`, `"PythonSlotWindow.h"`, `"PythonGridSlotWindow.h"`. Naglowki te maja tendencje do lancuchowania zaleznosci, wiec nierzadko dolaczane beda inne fundamentalne interfejsy renderingu silnika `EterLib` z powodu hermetycznych powiazan ze stanami okien.
- **Model pamieciowy:** Znaczaca wada architektoniczna jest obsluga i przekazywanie okien do/z Pythona jako typow calkowitych sluzacych za ukryte referencje do golych wskaznikow w pamieci C (Raw Pointers), np. operacje typu castowania `reinterpret_cast<UI::CWindow*>(ullVal)`. Tworzy to silne ryzyko bledow 'Dangling Pointer' poniewaz narzedzia nie wykorzystuja nowoczesnych smart pointerow np. `std::unique_ptr` z C++11+. Wszystko polega na konwencji poprawnego parowania `Register/Destroy`.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
Obiekty znajdujace sie w tym module, ze wzgledu na charakter tego jako modulu funkcji wyeksportowanych do intepretera, nie posiadaja swoich definicji klas (oprocz struktur menagowanych np. CWindow). Glowne typy uzywane przez API:
- `UI::CWindow`: Glowna bazowa klasa okien, zarzadzana przez instancje (wielkosc: dynamiczna), Wlasciciel watku: Main Thread (Graphics).
- `UI::CSlotWindow`: Podklasa UI::CWindow. Okno obslugujace ekwipunek, gniazda itd.
- `UI::CWindowManager`: Glowny singleton kontrolujacy widoki i zdarzenia myszki/klawiatury dla nich wszystkich.

**Tabela Metod Publicznych:** (kluczowe wyeksportowane funkcje PyMethodDef):
- `wndMgrRegister(PyObject* poSelf, PyObject* poArgs) -> PyObject*`: Rejestruje ogolne okno (bazujac na obiekcie Pythona i nazwie warstwy). Zwraca identyfikator 'Handle' dla tego okna do Pythona (rzeczywisty adres C++ jako Integer).
- `wndMgrSetWndPosition(PyObject* poSelf, PyObject* const* poArgs, Py_ssize_t nargs) -> PyObject*`: FastCall, C++ sygnatura PyObject*, przyjmuje tuple zawierajaca identyfikator okna, orac wspolrzedne `X` i `Y` (`PyLong`), Nastepnie uzywa `UI::CWindow::SetPosition(x,y)`.
- `wndMgrShow(PyObject* poSelf, PyObject* const* poArgs, Py_ssize_t nargs) -> PyObject*`: Pokazuje podane okno uzywajac interfejsu `pWin->Show()`.
- `wndMgrHide(PyObject* poSelf, PyObject* const* poArgs, Py_ssize_t nargs) -> PyObject*`: Ukrywa wybrane okno, uzywajac wewnetrznie funkcji `pWin->Hide()`.
- `wndMgrAppendSlot(PyObject* poSelf, PyObject* poArgs) -> PyObject*`: Dodaje konkretny slot do obiektu klasy CSlotWindow uzywajac m.in. `pSlotWin->AppendSlot(iIndex, ixPosition, iyPosition, ixCellSize, iyCellSize)`.
- `wndMgrSetSlot(PyObject* poSelf, PyObject* poArgs) -> PyObject*`: Konfiguruje stan graficzny slotu m.in. za pomoca indexu przedmiotu (`iItemIndex`), szerokosci, wysokosci oraz informacji dla ikon `(CGraphicImage*)` oraz koloru (`diffuseColor`). 
- `wndImageSetDiffuseColor(PyObject* poSelf, PyObject* poArgs) -> PyObject*`: Definiuje uklad nakladania koloru (Diffuse RGBA float parameters) modyfikujace kolor i opacity wybranego okienka CImageBox, co czesto wywolywane jest by symulowac przyciemnienie 'zacienionych' ikon ekwipunku.
- helper `PyFastCall_GetWindow(PyObject* const* poArgs, Py_ssize_t nargs, Py_ssize_t pos, UI::CWindow** ppRetWindow)` oraz `PyTuple_GetWindow`: narzedzia parsujace wejscia umozliwiajace bezpieczne wypakowanie `UI::CWindow*` (w miare mozliwosci) unikajace niepotrzebnych bledow (Exception) o ile sam adres `ullVal` w dalszym ciagu istnieje fizycznie.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
Modul ten nie definiuje swoich wlasnych struktur w pamieci, jest czysto zaleznym zbiorem funkcji narzedziowych dzialajacym na obcych typach. Mapowanie C-Python narzuca to z uwagi na architekture "Modulu Pythona". Jednak najwazniejszym aspektem na ktory Agenci i Hooki powinni zwracac uwage jest koncepcja podawania do skryptu referencji do obiektu w postaci wylacznie jednej 64bitowej wartosci (`PyLong_AsUnsignedLongLong(poArg)`), w systemach 64-bitowych (`x64`), gdzie `reinterpret_cast<UI::CWindow*>(ullVal)` gwarantuje dostep do VTable funkcji uzywanych przy obsludze gui (SetPosition, Show, itp.).  

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Brak bezposredniego polaczenia w protokole sieciowym z modulem interfejsu. Komunikacja na poziomie serwera gry ogranicza sie do logiki np. wysylania/odbioru stanow ekwipunku, co na wyzszym poziomie odbierane jest z instancji 'Network' do 'Python/CPython', a dopiero same skrypty interakcji interfejsu (Python UI Scripts) wydaja zlecenia w celu renderingu dla modulu wndMgr. Takie odseparowanie zapewnia hermetyczna role "Wylacznie warstwy renderujacej wizualnej".
- **Metody Pythona (`PyMethodDef`):** Modul jest zarejestrowany pod wbudowana nazwa `wndMgr` w srodowisku skryptowym. Skrypty uzyskuja dostep do funkcji jak m.in.: `wndMgr.Register`, `wndMgr.SetPosition`, `wndMgr.Show`, `wndMgr.Hide`, `wndMgr.AppendSlot`, `wndMgr.SetSlot`, `wndMgr.SetDiffuseColor`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Calosc zadan GUI jak i zadan modulu `wndMgr` MUSI BYC KATEGORYCZNIE ODSLUGIWANA W GLOWNYM WATKU RENDERINGU I WYKONAWCZYM (Main Thread). Wynika to z faktu iz modyfikacje stanu okien sa mocno polaczone z procesem renderowania D3D, gdzie jakikolwiek Data Race ze strony innych watkow zakloce dzialanie potokow renderowania D3D i wewnetrznych menedzerow `CWindowManager`. Zero synchronizacji (mutex) na polach `UI::CWindow`.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** System jest wyjatkowo podatny na uzycie po zwolnieniu (Use-After-Free) ze wzgledu na brak mechanizmu slabej referencji (weak references). Skrypt Pythona trzyma goly `Handle` wskazujacy pod oryginalny wskaznik. Wywolanie np. `wndMgr.Show(dead_handle)` doprowadzi nieuchronnie do segfaultu / invalid access jezeli wczesniej system C++ zakonczyl dzialanie `pWin`. Funkcje typu `GetWindow` weryfikuja jednie czy podano Integer i konwertuja go od razu castem co nie jest procesem bezpiecznym (Safe C++).
- **Zarzadzanie zasobami (RAII):** Silnik rzuca alokacja surowych wskaznikow (np. nowy CWindow alokowany przez `CWindowManager::RegisterWindow`), zas w teorii powinien sam nimi zarzadzac. Dzieki czemu wycieki moga wynikac po stronie samego Menagera jesli np nie czysci warstw przed odlaczeniem klienta. Brak RAII.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zdefiniuj wewnetrzna docelowa metode (lub modyfikacje zachowania) w odpowiedniej klasie np. w `UI::CWindow` (`PythonWindow.h / cpp`).
  2. Napisz funkcje pomocnicza / proxy w pliku `src/EterPythonLib/PythonWindowManagerModule.cpp`, wykorzytujaca wlasciwy helper, np.: 
  ```cpp
  PyObject* wndMgrSetMyNewProperty(PyObject* poSelf, PyObject* poArgs) {
      UI::CWindow* pWin;
      if (!PyTuple_GetWindow(poArgs, 0, &pWin)) return Py_BuildException();
      // Pobieranie dodatkowych parametrow np. PyTuple_GetInteger
      pWin->SetMyNewProperty();
      return Py_BuildNone();
  }
  ```
  3. Zarejestruj nowa opcje pod koniec pliku w definicji w tablicy `PyMethodDef s_methods[]`, uzywajac prawidlowych wlasciwosci (np. `METH_VARARGS` lub nowoczesnego wywolania `METH_FASTCALL`).
- **Jak debugowac i logowac:** Loguj wywolania metod poprzez istniejacy narzedzia systemowe np. `Sys_Log` jezeli zauwazysz bledne wskazniki. Jesli skrypt zglasza bledy wywolania lub zlego typu, to prawdopodobnie helpery nie byly w stanie prawidlowo sprawdzic wejsciowych formatow argumentow (np. Tuple) na ktorych bazuje C-API. Zwracaj `Py_BuildException(...)` po to, aby wywolac Stack Trace w skrypcie Pythona (sys.err) przy uzyciu modulu Pythona by po stronie developera logiki zauwazyc na zywo bledy.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Brak bezposredniego testowania jest tu typowy. Jednakze dzieki strukturze opartej na obiektach mozna wykorzystac pule wyizolowanych (mock) instancji podajac ich bezposredni cast np. rzutujac numer do `std::intptr_t`. Do powyzszego konieczne jednak jest mockowanie struktury obslugujacej hierarchie z `CWindowManager` badz unikanie testowania zlozonych relacji drzew okien w samej strukturze `PythonWindowManagerModule.cpp`.
