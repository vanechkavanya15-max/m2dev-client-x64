---
task_id: "atlas_c09_05_py_item_module"
cluster: "PY"
module_name: "Modul Pythona 'item' - Baza Danych Przedmiotow w UI"
target_files:
- src/UserInterface/PythonItemModule.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c09_05_py_item_module.md"
architecture_layer: "Mostek Pythona, Moduly C-API i Skrypty Gry"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Plik `PythonItemModule.cpp` sluzy jako mostek C-API pomiedzy silnikiem klienta Metin2 (w szczegolnosci `CItemManager` i `CPythonItem`) a warstwa skryptowa Pythona w UI. Pozwala skryptom na odpytywanie bazy danych przedmiotow (np. statystyki, ceny, ikony) oraz zarzadzanie przedmiotami upuszczonymi na ziemie w swiecie gry.
- **Punkt w petli gry:** Funkcje takie jak `Update` (ktora wola `CPythonItem::Update`) i `Render` sa wywolywane z glownej petli aplikacji (odpowiednio w fazie OnUpdate i OnRender). Pozostale funkcje to bezstanowe zapytania wywolywane na zadanie z UI Pythona podczas interakcji z oknami (np. tooltipy, inwentarz).
- **Przeplyw danych:** Skrypt Pythona podaje identyfikator przedmiotu (lub uzywa uprzednio wybranego przez `SelectItem`). Kod C++ wywoluje odpowiednie zapytanie z `CItemManager::Instance().GetSelectedItemDataPointer()`. Wynik (np. cena, nazwa, flaga) jest pakowany do obiektu Pythona za pomoca `Py_BuildValue` lub nowszych odpowiednikow i zwracany.
- **Cykl zycia obiektow:** Modul nie zarzadza wlasnymi zasobami, polegajac na architekturze singletonow (`CItemManager`, `CPythonItem`). Jednak zaimplementowana jest funkcja `GetIconInstance`, ktora dynamicznie alokuje `CGraphicImageInstance` i zwraca wskaznik do Pythona (rzutowany na 64-bitowego inta). Dealokacja tego zasobu musi byc wywolana z Pythona przez `DeleteIconInstance`, co generuje ryzyko wycieku pamieci jesli UI zapomni o zniszczeniu ikony.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Modul jest wywolywany glownie przez pliki w Pythonie z przedrostkiem `ui` (np. `uiInventory.py`, `uiToolTip.py`, `uiExchange.py`, `uiSafebox.py`), ktore uzywaja `import item` do tworzenia interfejsow.
- **Zaleznosci wyjsciowe (Outbound):** Modul odwoluje sie do `CItemManager` (z `GameLib`), `CPythonItem` (odpowiada m.in. za przedmioty upuszczone, np. 3D modele), `IAbstractApplication` (pobieranie pozycji myszki) i `CGraphicImageInstance`.
- **Drzewo dyrektyw `#include`:**
  - `StdAfx.h` (Precompiled headers)
  - `PythonItem.h` (Deklaracje instancji przedmiotu w pythonie)
  - `GameLib/ItemManager.h` (Dostep do cache bazy danych przedmiotow)
  - `InstanceBase.h`
  - `AbstractApplication.h` (Interfejs aplikacji - pobieranie pozycji myszy w `Update`)
- **Model pamieciowy:** Kod w wiekszosci dziala na czystych wskaznikach C (`CItemData *`) pozyskiwanych z singletona i zaklada, ze pamiec bazy danych przedmiotow jest read-only po zaladowaniu.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur:**
  - `initItem()` - Funkcja inicjalizujaca modul `item` dla interpretera Pythona. Rejestruje funkcje C++ w strukturze `PyMethodDef` i dodaje liczne stale numeryczne. Zwraca void. Zawsze odpalana w glownym watku przy starcie aplikacji.
- **Tabela Metod Publicznych (C-API do Pythona):**
  - `itemSelectItem` (PyObject* poSelf, PyObject** poArgs, Py_ssize_t nargs) -> Zwraca Py_None. Wybiera dany przedmiot wg VNUM do cache w `CItemManager`. Jesli nie istnieje, wybiera default (np. 60001).
  - `itemGetItemName`, `itemGetItemDescription`, `itemGetItemSummary`, `itemGetIconImage` -> Odpytuja `CItemData` aktywnie wybranego przedmiotu i zwracaja wartosci string, krotke lub liczbe uzywajac formatowania Pythona.
  - `itemGetItemType`, `itemGetItemSubType` -> Zwracaja typ (np. WEAPON, ARMOR) i podtyp. Maja zaktualizowane sygnatury `METH_FASTCALL`.
  - `itemGetLimit`, `itemGetValue`, `itemGetAffect` -> Funkcje uzywajace `METH_FASTCALL`. Biora jeden parametr (indeks limitu/apply/wartosci), zwracaja dane potrzebne do renderingu tooltipow w grze.
  - `itemGetIconInstance` (PyObject* poSelf, PyObject* poArgs) -> Wywoluje `CGraphicImageInstance::New()` i ustawia na nim grafike przedmiotu. Zwraca krotke wskaznikow (w rzutowaniu na long long - "K"). Wymaga wywolania z `itemDeleteIconInstance` zeby zwolnic zasoby.
  - `itemUpdate`, `itemRender` -> Bindowane funkcje petli renderujacej. `Update` pobiera kursor myszy przez `IAbstractApplication::GetSingleton().GetMousePosition()`.
  - `itemCreateItem`, `itemDeleteItem` -> Interakcje z obiektami w swiecie 3D. Generuja obiekt reprezentujacy dropniety item na podanych koordynatach.
- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  Brak lokalnych struktur - wszystkie operacje na modelach bazuja na pamieci zewnetrznej `CItemManager` oraz `CItemData`.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Modul item bezposrednio nie parsuje pakietow sieciowych; ta rola delegowana jest do mostkow z fazy Network (np. pakiety podnoszenia przedmiotu). Modul jednak odpowiada za lokalne wylapanie klikniecia na upuszczony przedmiot (metoda `itemPick` ktora zwraca ID przedmiotu, przekazywane nastepnie w pakiecie do serwera).
- **Metody Pythona (`PyMethodDef`):** Eksportuje modul `item`. Wiele kluczowych metod zostalo przeniesionych do `METH_FASTCALL` w celu zmniejszenia alokacji krotek argumentow: `SelectItem`, `GetItemName`, `GetItemType`, `GetItemSubType`, `GetLimit`, `GetLimitType`, `GetLimitValue`, `GetAffect`, `GetValue`. Inne metody wciaz uzywaja `METH_VARARGS` i musza parsowac argumenty przez `PyTuple_Get*`.
Modul zawiera tez ponad 150 stalych eksportowanych przez `PyModule_AddIntConstant` (np. stale ITEM_TYPE, APPLY_*, LIMIT_*, ARMOR_*, itp.).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Wszystkie funkcje dzialaja na watku glownym (UI / Python GIL). Brak blokad wielowatkowych. Wywolanie C-API z innego watku bez wziecia blokady GIL spowoduje crasha.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Funkcje odpytujace `CItemData` bez wczesniejszego zdefiniowania `CItemManager::Instance().GetSelectedItemDataPointer()` sprawdzaja czy wskaznik nie jest null, ale jesli `CItemManager` padnie lub zostanie niezainicjalizowany - program wyrzuci wyjatek i python zaloguje wyjatek `Py_BuildException("no selected item data")`.
  - Zarzadzanie wskaznikami C++ wewnatrz Pythona: `itemGetIconInstance` alokuje obiekt `CGraphicImageInstance`. Przekazanie falszywego/zwalidowanego adresu do `itemDeleteIconInstance` lub zapomnienie o zniszczeniu stworzy ucieczke pamieci / crash.
  - Zle argumenty z Pythona: `PyTuple_GetInteger` nie dziala we funkcjach oznaczonych jako `METH_FASTCALL`. Musza one uzywac bezposrednio `PyLong_Check` oraz `PyLong_AsLong(poArgs[0])`.
- **Zarzadzanie zasobami (RAII):** Brak wzorca RAII dla interakcji C++ / Python C-API. Zalezno na jawnej alokacji/dealokacji narzuconej logice UI. Utrzymanie integralnosci sterty zalezy od wlasciwych skryptow uzytkownika.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zdefiniuj nowa metode w pliku `src/UserInterface/PythonItemModule.cpp` np. `PyObject * itemMyNewFunction(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)`.
  2. W zaleznosci od standardu nowej metody uzywaj `PyLong_Check()` dla METH_FASTCALL, a `PyTuple_GetInteger()` dla METH_VARARGS.
  3. Zdobadz wskaznik przedmiotu uzywajac `CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();`.
  4. Dodaj obsluge null-pointera (zwroc Py_BuildException lub Py_BuildValue w zaleznosci od wymogow obslugi blendow w UI).
  5. Zwroc wynik przez konwersje makrem np. `PyLong_FromLong()`.
  6. Dodaj metode do tabeli `s_methods` przy uzyciu `METH_FASTCALL` lub `METH_VARARGS`.
- **Jak debugowac i logowac:**
  Do debugowania uzywac logowania np. `TraceError("Cannot find item by %d", iIndex);`. Wartosci bledow w rzutowaniach Python API narazaja system na silent-errors - wiekszosc operacji po prostu zglosi do UI wyjatek.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  Zbuduj mock dla instancji Singletonowej `CItemManager` (w pliku testowym `#include "GameLib/ItemManager.h"`) przed kompilacja z Python C-API. Nalezy wywolac `initItem()` aby przetestowac rejestracje i zmockowac strukturze tablicowej, nastepnie przekazac recznie obiekt `PyObject` ze stosownymi parametrami wejsciowymi do sprawdzanych metod. Pamietac o dodaniu sciezki `Python.h`.
