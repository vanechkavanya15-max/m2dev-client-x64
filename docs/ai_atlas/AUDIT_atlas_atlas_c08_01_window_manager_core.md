---
task_id: "atlas_c08_01_window_manager_core"
cluster: "UI"
module_name: "CWindowManager - Silnik Okien UI i Propagacja Zdarzen"
target_files:
- src/EterPythonLib/PythonWindowManager.cpp
- src/EterPythonLib/PythonWindowManager.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c08_01_window_manager_core.md"
architecture_layer: "Interfejs Uzytkownika, Okna i System Tekstu"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Modul `CWindowManager` pelni role glownego zarzadcy cyklu zycia, hierarchii renderingu oraz wejscia uzytkownika dla graficznego interfejsu klienta (GUI).
Jest on scentralizowanym Singletonem, ktory tlumaczy surowe zdarzenia wejscia Win32 / DirectInput z glownej petli C++ (`PythonApplicationProcedure` / `PythonPlayerInputMouse`) na ustrukturyzowane wywolania do konkretnych, renderowanych obiektow graficznych (np. Box, TextLine, Button, SlotWindow).

**Kiedy jest wywolywany:**
- **OnUpdate / OnRender:** W petli gry wewnatrz `CPythonApplication` modul wywoluje kolejno `Update()` oraz `Render()` od najwyzszego wezla korzenia (`m_pRootWindow`), kaskadowo propagujac polecenia az do dzieci.
- **Input Tick:** Funkcje takie jak `RunMouseMove`, `RunMouseLeftButtonDown`, czy eventy IME klawiatury sa triggerowane bezposrednio ze srodowiska Win32 (komunikaty HWND) poprzez `PythonApplicationProcedure`.

**Data Flow i Control Flow:**
1. Zdarzenie myszy (np. wcisniecie LPM) wchodzi do managera (`RunMouseLeftButtonDown(x, y)`).
2. Manager przelicza globalne koordynaty wzgledem obecnej rozdzielczosci i aspektu.
3. Oblicza "PointWindow" uzywajac metody `__PickWindow(x, y)` (skaner zderzen 2D, sprawdza Z-order - warstwy renderowane najwyzej maja pierwszenstwo).
4. Przekazuje komende wcisniecia guzika do trafionego okna (`OnMouseLeftButtonDown`).
5. Jesli okno obsluguje flagi specjalne (np. drag&drop `FLAG_DRAGABLE` w oknach typu `CDragButton`), manager przypisuje ten element do strumienia uchwytu (`m_pLeftCaptureWindow`) by kontrolowac przesuwanie.

**Cykl Zycia Obiektow (Lifecycle):**
- **Alokacja:** Nastepuje poprzez interfejs C API / Pythona. Skrypty interfejsu .py wola metody rejestracji (np. `RegisterTypeWindow` mapujace do `__NewWindow(po, WT_...)`), na co C++ odpowiada alokacja operatora `new`.
- **Inicjalizacja:** Okno zostaje przypisane do odpowiedniej "Warstwy" ("GAME", "UI_BOTTOM", "UI", "TOP_MOST", "CURTAIN") w kontenerze `m_LayerWindowMap`.
- **Reset/Dealokacja:** Usuwanie obiektu realizowane jest bezpiecznym modelem odroczonym (lazy delete). Skrypt Pythona wywoluje prosbe zniszczenia, manager dodaje obiekt do setu `m_ReserveDeleteWindowList`, a realne wolanie operatora `delete` odbywa sie na poczatku nowej ramki wewnatrz `__ClearReserveDeleteWindowList()`. Zapewnia to ochrone przed problemami typu Use-After-Free jesli eventy klawiatury celowaly w niszczony element.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound - Kto to wola?):**
- **`CPythonApplication` (UserInterface):** Przekazuje HWND window messages, uaktualnienia ramki czasowej (Render/Update).
- **`PythonPlayerInputMouse` / `PythonPlayerInput` (UserInterface):** Aktualizuje kamere lub kursor, kiedy klikniecia nie naleza do UI.
- **`PythonPlayer` / `CSlotWindow` itp. (EterPythonLib / UserInterface):** Operacje zapytan o zasoby chlodzenia ikon (cooldown) wywoluja delegacje np. `ClearStoredSlotCoolTimeInAllSlotWindows`.
- **Skrypty Pythona (via C-API):** Modul jest silnie "obsadzony" metodami `PyCallClassMemberFunc` (komunikacja C++ do Pythona w warstwie `CWindow`/`CSlotWindow`).

**Zaleznosci wyjsciowe (Outbound - Co to wola?):**
- **Srodowisko klas okiennych `UI::...`:** Alokuje obiekty i zarzadza `CWindow`, `CSlotWindow`, `CGridSlotWindow`, `CButton`, `CTextLine`, `CImageBox`, `CAniImageBox`.
- **Wydarzenia Pythona:** Wolanie zdarzen interfejsu via Callbacks, np. `OnMouseOverIn()`, `OnSelectEmptySlot()`, itp.

**Drzewo dyrektyw `#include`:**
- `StdAfx.h`, `PythonWindow.h`, `PythonSlotWindow.h`, `PythonGridSlotWindow.h`, `PythonWindowManager.h`, `PythonInternedStrings.h`
**Ryzyka cykliczne:** Brak widocznych krytycznych petli na poziomie naglowkow, aczkolwiek wzajemne zaleznosci w drzewie pointerow `CWindow` wzgledem `CWindowManager` oznaczaja ze nie wolno rozrywac pamieci poza managerem.

**Model pamieciowy:**
- Architektura oparta na czystych (surowych) wskaznikach C++ alokowanych na stercie i rzutowanych pomiedzy C++ i silnikiem Python (przez `PyObject *`).
- Ograniczenia i zarzadzanie pamiecia zarzadza glownie wlasciciel (Manager), czesc obiektow dziala w architekturze wezlow. Brak nowoczesnego RAII (std::unique_ptr/shared_ptr), co implikuje ryzyko wyciekow pamieci mitygowane przez mechanizm `#define __WINDOW_LEAK_CHECK__`.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

#### Klasy i Struktury

| Nazwa symbolu | Typ | Rola w module | Wlasciciel pamieci/watku |
|:--------------|:----|:--------------|:-----------------------|
| `UI::CWindowManager` | `class` | Glowny nadzorca drzewa DOM interfejsu klienta, singleton (CSingleton) | Watek Main / Global (Singleton) |
| `UI::CWindowManager::TLayerContainer` | `std::map` | Mapuje nazwe stalej (np. "UI") na glowny obiekt `CLayer` trzymajacy drzewo | Wnetrze Managera |
| `UI::CWindowManager::TWindowContainer`| `std::list` | Przechowuje sekwencje wskaznikow na obiekty UI do przeskakiwania w eventach | Wnetrze Managera |
| `UI::CWindowManager::TKeyCaptureWindowMap` | `std::map` | Przechwytuje virtual keys i przypina do danego konkretnego okna (Input focus) | Wnetrze Managera |

#### Enumeracje
| Nazwa typu okna | Wartosc w kodzie (Enum WT_...) | Oczekiwana instancja klasy okna (Alokacja) |
|:----------------|:--------------------------------|:-------------------------------------------|
| `WT_NORMAL`     | `0`                             | `CWindow` |
| `WT_SLOT`       | `1`                             | `CSlotWindow` |
| `WT_GRIDSLOT`   | `2`                             | `CGridSlotWindow` |
| `WT_TEXTLINE`   | `3`                             | `CTextLine` |
| `WT_MARKBOX`    | `4`                             | `CMarkBox` |
| `WT_IMAGEBOX`   | `5`                             | `CImageBox` |
| `WT_EXP_IMAGEBOX`| `6`                            | `CExpandedImageBox` |
| `WT_ANI_IMAGEBOX`| `7`                            | `CAniImageBox` |
| `WT_BUTTON`     | `8`                             | `CButton` |
| `WT_RATIOBUTTON`| `9`                             | `CRadioButton` |
| `WT_TOGGLEBUTTON`| `10`                           | `CToggleButton` |
| `WT_DRAGBUTTON` | `11`                            | `CDragButton` |
| `WT_BOX`        | `12`                            | `CBox` |
| `WT_BAR`        | `13`                            | `CBar` |
| `WT_LINE`       | `14`                            | `CLine` |
| `WT_BAR3D`      | `15`                            | `CBar3D` |
| `WT_NUMLINE`    | `16`                            | `CNumberLine` |

#### Kluczowe Metody Publiczne (CWindowManager)

- `CWindow * RegisterWindow(PyObject * po, const char * c_szLayer);`
  - Rejestruje wezel bazowy dla podanej wirtualnej reprezentacji `po` i dolacza do drzewa danej warstwy (`c_szLayer`).
- `void DestroyWindow(CWindow * pWin);`
  - Wycofuje element z drzewa renderujacego, zglasza dealokacje dodajac go do `m_ReserveDeleteWindowList`. Zabezpiecza focusy usuwajac odniesienia (np `m_pPointWindow`).
- `void SetMouseHandler(PyObject * poMouseHandler);`
  - Ustanawia powiazanie zdarzen globalnych myszy do skryptu w Pythonie (sluzace np. do attach icon).
- `void RunMouseMove(long x, long y);`
  - Odswieza globalna pozycje myszy, uruchamia algorytm PickWindow w celu detekcji nakierowania (Hover/MouseOver), zajmuje sie rowniez ograniczeniami w przesuwaniu elementow (Drag bounding boxes) via okno zapisane w `m_pLeftCaptureWindow`.
- `void ClearStoredSlotCoolTimeInAllSlotWindows(DWORD dwKey, DWORD dwSlotIndex);`
  - Rekurencyjne czyszczenie chlodzenia (cooldown timer) zadeklarowanych gniazd umiejetnosci. Obchodzi cale drzewo klas i filtruje poprzez sprawdzanie rzutowania `IsType(UI::CSlotWindow::Type())`.

#### Pamieciowy Layout Struktur (Offsety / Hooking considerations)
Obiekt singletonu jest ulokowany w stalej globalnej czesci pulek aplikacyjnych. Pola moga byc odnajdywane przez heurystyke na `m_lMouseX`, `m_lMouseY`, ktore sa modyfikowane przez wejscie Win32. Czasowy blok na ignorowanie clickow rezyduje we fladze `m_bOnceIgnoreMouseLeftButtonUpEventFlag` i `m_iIgnoreEndTime`.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

- **Mostki sieciowe (Network Protocol):** Modul nie przetwarza bezposrednio danych sieciowych; zjawiska opoznien (lagow) moga jednak wplywac na wywolywanie resetow okien (Cooldowns/Sloty) na podstawie pakietow GC.
- **Python C-API:**
  Instancje uzywaja `PyCallClassMemberFunc` by wysylac powiadomienia do skryptow (.py). Modul sam w sobie uzytkowany jest przez `PythonApplication` ktore eksponuje w module `ui` Pythona wywolania natywne C++ (poprzez warstwe EterPythonLib bindings). `po` czyli wewnetrzne `m_poHandler` to wskaznik PyObject obslugujacy konkretna klase Pythona. Do nich kierowane sa zdarzenia: `OnRender`, `ShowToolTip`, `HideToolTip`, `OnMouseOverIn`, `OnEndFrame` (dla animacji).
  `PyObject * BuildEmptyTuple()` zabezpiecza wielokrotne tworzenie pustych argumentow uzywanych przy notyfikacjach C++ do interfejsu (redukcja napiec na pamieci GC Pythona).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

1. **Zasada jedno-watkowosci (Single Thread Binding):**
   `CWindowManager` JEST NIE BEZPIECZNY W WATKACH. Zaklada w pelni jedno-watkowa prace, zsynchronizowana z glowna petla gry (D3D thread). Wszelkie zapytania o rozmiar (resolutions), mouse eventy, lub iteracje na mapach/listach MOGA prowadzic do korupcji pamieci jesli dotkniete przez asynchroniczny kod C++ z sieci.
2. **Niedozwolony natychmiastowy delete:**
   Nie uzywaj bezposrednio operatora `delete` na okienkach instancjowanych przez CWindowManager z powodu referencji istniejacych w pointerach m_pPointWindow czy m_ActiveWindowList. Bezwzglednie wylacznie uzywaj `DestroyWindow()`, ktora stosuje proces z czyszczeniem sladow (przez `NotifyDestroyWindow`).
3. **Puste lub martwe Handler'y (Python Crash point):**
   Wywolanie metody w klasie okna, ktorego `poHandler` nie posiada funkcji lub stalo sie martwe (Dangling python object), to klasyczny problem architektury EterLib. System zapobiega jednak najgorszym bladom uzywajac wczesnych weryfikacji.
4. **Scrapping / Focus Deadlocks:**
   Istnieje szansa ze `m_pLockWindow` "zawiesi" wejscia innych okien jesli instancja zablokowana nie dostanie prosby o release. Logika "UnlockWindow()" wpycha focus poprzedniego lidera wiersza.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Dodawanie nowej logiki (nowego typu okna):**
Aby dodac nowa, customowa kontrole graficzna dla klienta gry (np. `CRenderingModelBox` do renderingu pelnych modeli postaci w UI):
1. Zadeklaruj klase wywodzaca sie od `UI::CWindow` np. w `PythonWindow.h`.
2. Dodaj wpis enumeracji w `CWindowManager::WT_MODELBOX` zaraz przed klauzula zamykajaca.
3. W `CWindowManager::RegisterTypeWindow` lub w specyficznym `Register...` podepnij nowe alokacje z instrukcja obslugujaca ten typ widgetu.
4. Eksponuj interfejs uzywajac klasycznego bindowania Pythona w module generujacym metody u klienta (np. w `ui.py` + bindings w zrodlach).

**Srodowisko testowe Headless (Headless Testing UI):**
Manager okien pozwala na stymulowanie calego systemu bez renderu. Przetestuj swoj flow w ten sposob:
1. Zainicjalizuj `CWindowManager`.
2. Sztucznie ustaw resolution poprzez `SetResolution`.
3. Wywolaj sztuczne cykle `RunMouseMove(x, y)` i sprawdz stan fokusu przez `GetPointWindow()`. Omija to bezposrednio input Win32.

**Zmienne i flagi do Debugowania:**
W razie problemow z pozycjonowaniem i detekcja kolizji kursora na obiektach, wlacz (zmien w C++ lub w IDE runtime) flage preprocesora na `g_bShowOverInWindowName = TRUE`. Spowoduje to nadawanie w formacie "Tracef" ciaglego strumienia logow z informacja o nazwie najechanego aktualnie pointerem okna. Przydatne dla AI hookujacego sie pod syserr.txt by mapowac DOM zewnetrznie.
Odblokowanie `#define __WINDOW_LEAK_CHECK__` wydrukuje kazdy wyciek podczas wylaczania gry. Zdecydowanie zalecane.
