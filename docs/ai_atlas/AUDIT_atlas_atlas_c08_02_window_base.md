---
task_id: "atlas_c08_02_window_base"
cluster: "UI"
module_name: "CWindow - Klasa Bazowa Kontrolki Interfejsu"
target_files:
- src/EterPythonLib/PythonWindow.cpp
- src/EterPythonLib/PythonWindow.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c08_02_window_base.md"
architecture_layer: "Interfejs Uzytkownika, Okna i System Tekstu"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Modul ten stanowi fundament dla calego systemu interfejsu uzytkownika w kliencie gry (UI). Klasa bazowa `UI::CWindow` oraz klasy pochodne (takie jak `CBox`, `CBar`, `CTextLine`, `CImageBox`, `CButton` i inne) sa uzywane do tworzenia i zarzadzania strukturami UI (okna, przyciski, obrazki, teksty).
W kliencie gry UI jest oparte o drzewo (parent-child relationship). Modul aktualizuje pozycje globalne i lokalne okien, uwzglednia wyrownanie, zarzadza interakcjami (zdarzenia myszy i klawiatury za posrednictwem eventow wysylanych do/z Pythona) oraz obsluguje renderowanie korzystajac bezposrednio z `CPythonGraphic`.

Proces (Data & Control Flow):
1. **Alokacja:** Obiekty CWindow sa czesto tworzone po stronie C++ za sprawa scriptow interfejsu uzytkownika ladowanych poprzez interfejs Pythona (czesto zarzadzane w `PythonWindowManager`).
2. **Inicjalizacja:** Wspolrzedne, rozmiary, wlasciwosci (flagi widocznosci, ograniczanie i pozycja) sa wstepnie ustawiane metodami.
3. **OnUpdate & OnRender:** Okna sa przeliczane co ramke (obliczanie polozen z uwzglednieniem orientacji np. RTL/LTR) i nastepnie renderowane przez glownego renderera 2D (`CPythonGraphic`). Obcinanie prostokata (ScissorTest) jest dostepne przez modyfikacje stanow DirectX.
4. **Zdarzenia wejscia:** Ruch myszy, klikniecia i klawiatura sa kierowane przez managera do poszczegolnych okien na zasadzie z-order, po czym czesto sa routowane do Pythona uzywajac np. funkcji `PyCallClassMemberFunc`.
5. **Dealokacja / Reset:** Dealokacja (zniszczenie) usuwa wszystkie dzieci za pomoca funkcji `Clear()`. Obiekty graficzne pod maska maja dedykowane zarzadzanie pamiecia zasobow (`CResourceManager`).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound):** 
  - Subsystemy renderowania oraz obslugi wejsc z uzytkownikiem (klawiatura, mysz). 
  - Mostek Pythona (`CWindowManager` mapujacy obiekty skryptow w C++), wtyczki i skrypty Python.
  - Okna dzieci/rodzice aktualizujace relacje.

- **Zaleznosci wyjsciowe (Outbound):** 
  - `EterBase/CRC32.h` - haszowanie do identyfikacji typu (np. "CWindow", "CBar3D").
  - `EterLib/StateManager.h` - kontrola widocznosci przy uzyciu D3DRS_SCISSORTESTENABLE dla stref przyciecia (ScissorRect).
  - `PythonSlotWindow.h`, `PythonWindowManager.h` - wspolpraca z menedzerem okien UI.
  - `PythonInternedStrings.h` - przyspieszona integracja wywolan metod Python-owych (bez wyszukiwania nazw w czasie wykonywania).
  - CPythonGraphic (bezposrednie wywolania: `RenderBox2d`, `RenderBar2d`, `RenderLine2d`).
  - Python C-API (wywolywanie zdarzen skryptow Python np. `OnMouseLeftButtonDown`).
  - `UserInterface/Locale_Interface.h` - dla obslugi wyrownania stron od prawej (RTL) np. dla jezykow bliskowschodnich.

- **Drzewo dyrektyw `#include`:** 
  `#include "StdAfx.h"`, `#include "EterBase/CRC32.h"`, `#include "PythonWindow.h"`, `#include "PythonSlotWindow.h"`, `#include "PythonWindowManager.h"`, `#include "PythonInternedStrings.h"`, `#include "EterLib/StateManager.h"`, `#include "UserInterface/Locale_Interface.h"`.
  Brak widocznych ryzyk cyklicznych; kod odpowiednio zalezy od headerow interfejsow.

- **Model pamieciowy:** 
  Klasy operuja w wiekszosci na surowych wskaznikach (`CWindow * m_pParent`). Zbiory dzieci to `std::list<CWindow *> m_pChildList` bez dedykowanego wlascicielstwa RAII - dealokacje czesto zaleza od zdarzen (Pythona i okien). Instancje zasobow graficznych alokowane bywaja przez `New()` i zwalniane przez dedykowane metody statyczne `Delete()`.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur**
- `UI::CWindow` - klasa bazowa UI (32-bit lub 64-bit wielkosc struktury zalezy od kompilatora, przechowuje obramowanie i struktury wspolrzednych), dedykowane do dzialania glownie w watku glownego loopa.
- `UI::CLayer` - warstwa widoku (brak implementacji IsWindow(), czysto strukturalna okno).
- `UI::CBox`, `UI::CBar`, `UI::CLine`, `UI::CBar3D` - prymitywy renderujace D3D w 2D (pudelka, linie, trjwymiarowe obiekty pseudo 3D na bazie paskow gradientu).
- `UI::CTextLine`, `UI::CNumberLine` - obiekty do zaprezentowania tekstow uzywajace instancji fontow (CGraphicTextInstance). CNumberLine uzywa zasobow pojedynczych liter.
- `UI::CImageBox`, `UI::CExpandedImageBox`, `UI::CAniImageBox`, `UI::CMarkBox` - okna kontenerowe prezentujace CGraphicImageInstance i tekstury (tez animowane dla AniImageBox).
- `UI::CButton`, `UI::CRadioButton`, `UI::CToggleButton`, `UI::CDragButton` - okna obslugujace logike myszki (klikniecia, najazdy, wcisniecia) i podmieniajace renderowany stan (Up/Down/Over).

**Tabela Metod Publicznych (wybrane najwazniejsze dla CWindow)**
- `void Update()` i `void Render()` - cykl zycia na klatke (wywoluje `OnUpdate` / `OnRender`). Brak skutkow ubocznych na swiat logiczny, zarzadza tylko UI.
- `long UpdateRect()` - przelicza globalna i lokalna pozycje dla zagniezdzonego okna UI wraz z jego dziecmi (wykorzystuje RTL i orientacje).
- `CWindow * PickWindow(long x, y)` - metoda zwraca wskaznik na okno ktore znajduje sie pod wplywem danych koordynat. Iteruje dzieci okna wg kolejnosci by wspierac Z-order.
- `BOOL OnKeyDown(int ikey)`, `BOOL OnMouseLeftButtonDown()` - obsluga akcji, po ktorej kod przewaznie wywoluje analogiczne wywolanie Pythona przypisane do C-API by obsluzyc wlasciwy kod logiczny skryptu okna.
- `void AddChild(CWindow * pWin)`, `void DeleteChild(CWindow * pWin)` - zarzadzanie dziecmi. (DeleteChild czesto stosuje tzw. m_pReserveChildList jesli petla m_isUpdatingChildren jest aktywna zeby zapobiec iterator invalidation).

**Pamieciowy Layout Struktur (Memory Layout & Offsets)**
Kluczowe zmienne czlonkowskie CWindow (od poczatku instancji): `std::string m_strName;`, `EHorizontalAlign m_HorizontalAlign`, `EVerticalAlign m_VerticalAlign`, `long m_x, m_y`, `long m_lWidth, m_lHeight`, `RECT m_rect;`, `RECT m_limitBiasRect;`, `bool m_bMovable;`, `bool m_bShow;`, `DWORD m_dwFlag;`, `PyObject * m_poHandler;`, `CWindow * m_pParent;`, `std::list m_pChildList;`. Bezpieczny Hook FFI musi rozwazyc offset vtable (poniewaz CWindow ma metody wirtualne np. OnRender) oraz mapowanie wskaznika na PyObject by przechwycic metody skryptowe UI.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Pakiety Sieciowe:** Modul bezposrednio NIE generuje ani nie dekoduje zadnych pakietow sieciowych (0x0). Obsluga jest realizowana w kodzie Pythona poprzez API `net` i eventy aplikacyjne, ktore powoduja dzialania z `CWindow`.
- **Metody Pythona (`PyMethodDef`):** Modul jest glownie uzywany PRZEZ Pythona i eksponuje dzialania do `PyCallClassMemberFunc`. Uzywane wywolania Pythona to np.: `"OnMouseDrag"`, `"OnSetFocus"`, `"OnKillFocus"`, `"OnDrop"`, `"OnTop"`, `"OnIMEUpdate"`, `"OnIMETab"`, `"OnIMEReturn"`, `"OnIMEKeyDown"`, `"OnIMEOpenCandidateList"`, `"OnIMECloseCandidateList"`, `"OnIMEOpenReadingWnd"`, `"OnIMECloseReadingWnd"`, `"OnKeyDown"`, `"OnKeyUp"`, `"OnPressExitKey"`, `"OnMouseLeftButtonDown"`, `"OnMouseLeftButtonUp"`, `"OnMouseLeftButtonDoubleClick"`, `"OnMouseRightButtonDown"`, `"OnMouseRightButtonUp"`, `"OnMouseRightButtonDoubleClick"`, `"OnMouseMiddleButtonDown"`, `"OnMouseMiddleButtonUp"`. Szybkie wywolania optymalizowane sa przez `PythonInternedStrings::Call`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Wszystkie dzialania na instancjach dziedziczacych po `CWindow` POWINNY byc wykonywane jedynie na glownym watku renderujacym (main logic thread). Modul operuje bezposrednio na DirectX (przez StateManager/ScissorRect i CPythonGraphic) oraz strukturach Pythona (`PyObject *`), oba te srodowiska nie wspieraja tutaj bezpieczenstwa wielowatkowego.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Invalid Iterator (Iterator Invalidation): `DeleteChild` wykorzystuje mechanizm powolnego usuwania i przenoszenia do rezerw (`m_pReserveChildList`), gdy system jest w srodku petli `Update()`. Omijanie tego mechanizmu doprowadzi do awarii.
  - Wycieki: Zmiana rodzica albo zniszczenie parenta musi poprawnie wolac `Clear()`, ktora odczepia i likwiduje obiekty i reference od/do instancji Python-owych. Ale bezposrednio w C++ `m_pChildList` zawiera surowe wskazniki bez scislej gwarancji usuniecia, liczy na prawidlowe zniszczenie "garbage collection" ze skryptu UI po wywolaniu `DestroyHandle()`.
  - Przepelnienia zasobow D3D: Bledne okna z brakiem ScissorRect i StateManager resetow. Konstrukt `ScopedScissorRect` z `std::max`/`std::min` poprawnie zarzadza granicami w zagniezdzeniach, uniemozliwiajac bledne stany na karcie graficznej.
- **Zarzadzanie zasobami (RAII):** CTextLine uzywa `m_TextInstance.Destroy()`, ale pozostale jak CAniImageBox jawnie czyszcza swoje pule za pomoca `std::for_each` i wlasnych metod `Delete()`. W CWindow brak mechanizmu RAII w odniesieniu do list dzieci (`m_pChildList`).

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Jesli chcesz nowa kontrolke (np. `CMySliderBox`), zadeklaruj nowa klase dziedziczaca po `UI::CWindow` w pliku `PythonWindow.h`. Zadeklaruj unikalna funkcje statyczna `Type()` zwracajaca CRC32 dla np. `"CMySliderBox"`.
  2. Zaimplementuj nadpisania np. `OnUpdate` oraz `OnRender` z logika bezposrednio na wspolrzednych `m_rect.left / m_rect.top`.
  3. Koniecznie utworz logike Python'a po stronie modulu `PythonWindowManager.cpp`, ktory dolaczy ta nowa klase jako ekspozycje w wbudowanym interfejsie C-API `ui.MySliderBox()`.
- **Jak debugowac i logowac:** Przechwytywanie flagi `DEBUG_dwCounter` ze zdefiniowanego preprocesora `_DEBUG` w konstruktorze `CWindow` potrafi pomagac logowac identyfikatory dla instancji bez unikalnej nazwy. Zmienna `m_strName` przechowuje nazwe biezacego modulu/okna, uzywaj `GetName()`. Uruchom zmienna `g_bOutlineBoxEnable` (w ustawieniu debug) zeby automatycznie renderowac bialy kwadrat 2D wokol kazdego okna `RenderBox2d()` co bardzo pomaga w dostosowaniu i wizualizacji problemow ze stylem Scissor i Limit Bias.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Nie da sie wywolac petli Render w headless bez zmockowania modulu renderujacego `CPythonGraphic`. Mozesz jednak przeprowadzic jednostkowe testy operacji hierarchii dzieci, wywolan i metod matematycznych takich jak `UpdateRect()`, co udowadnia jak offsety ukladaja sie dla flagi `UI::CWindow::FLAG_RTL` mockujac puste obiekty PyObject i unikajac zawolania na `Render()`.
