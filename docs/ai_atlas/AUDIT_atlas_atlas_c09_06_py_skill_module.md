---
task_id: "atlas_c09_06_py_skill_module"
cluster: "PY"
module_name: "Modul Pythona 'skill' - Baza Umiejetnosci w UI"
target_files:
- src/UserInterface/PythonSkillModule.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c09_06_py_skill_module.md"
architecture_layer: "Mostek Pythona, Moduly C-API i Skrypty Gry"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### Brak Pliku Zrodlowego i Przekierowanie
Zgodnie ze stanem faktycznym repozytorium, plik `src/UserInterface/PythonSkillModule.cpp` nie istnieje. Jednakze, celem spelnienia priorytetowego wymogu biznesowego (analiza metod C-API dla modulu 'skill' w Pythonie, tj. GetSkillName, GetSkillType, itd.), przeprowadzono pelny audyt funkcjonalnosci, bazujac na bezposrednim zrodle, tj. plikach `src/UserInterface/PythonSkill.cpp` oraz `src/UserInterface/PythonSkill.h`, gdzie rzeczone mostki sa zaimplementowane.

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Modul `skill` stanowi C-API (Python C-API) dostarczajace skryptom UI klienta (Python) dostep do bazy danych umiejetnosci (`CPythonSkill`).
- **Funkcja w architekturze:** Dziala jako posrednik miedzy wczytanymi danymi z bazy klienta (SkillProto, SkillDesc) a interfejsem graficznym, pozwalajac skryptom UI pobierac wlasciwosci (nazwy, opisy, wymagania PM, czasy odnowienia) oraz instancje ikon dla umiejetnosci (np. aktywne, pasywne, gildijne).
- **Przeplyw danych:** Wywolania metod skryptowych z UI przekazuja argumenty (np. `SkillIndex`, `SkillPoint`, `GradeIndex`), ktore sa odkodowywane za pomoca `PyTuple_GetInteger` / `PyTuple_GetFloat`. Modul odpytuje singleton `CPythonSkill::Instance()` (dane ladowane z pamieci VFS), przetwarza wzory matematyczne (np. `CPoly`) lub zwraca wlasciwosci klas, uzywajac `Py_BuildValue`.
- **Cykl zycia obiektow:**
  - `CPythonSkill` ladowany jest podczas fazy ladowania (na glownym watku) przy inicjalizacji aplikacji (w `UserInterface.cpp` przez `initskill()`).
  - Poszczegolne instancje grafik (`CGraphicImageInstance`) moga byc alokowane na zadanie, co wymusza reczne ich usuniecie w UI.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound):** Modul inicjowany jest wywolaniem `initskill()` w glownym procesie (`UserInterface.cpp`). Wywolywany bezposrednio z maszynerii UI w Pythonie w glownej petli podczas bindowania UI, najczesciej przy renderowaniu paska skilli oraz wyswietlaniu tooltipow (np. `uiTooltip.py`, `uiTaskBar.py`).
- **Zaleznosci wyjsciowe (Outbound):** 
  - `CPythonSkill` (repozytorium danych skilli).
  - Wzorce (polynomy) ewaluacji (`EterBase/Poly/Poly.h`, klasa `CPoly`).
  - Zaleznosci systemowe EterLib (`CGraphicImageInstance`, `CGraphicImage`).
  - Stan postaci (pobieranie Casting Speed przez `CPythonPlayer::Instance().GetStatus()`).
- **Drzewo dyrektyw `#include`:** Dolacza m.in. `StdAfx.h`, `PythonSkill.h`, `EterBase/Poly/Poly.h`, `PackLib/PackManager.h`, `InstanceBase.h`, `PythonPlayer.h`.
- **Model pamieciowy:** Wymaga szczegolnej ostroznosci przy alokacji z pamieci operacyjnej (C-API `PyObject*` jako zwroty) - referencje Pythonowe (`Py_BuildNone`, `Py_BuildValue`) oraz w C++ (`CGraphicImageInstance*`, na ktorym operuja wylacznie wskazniki surowe, alokacja `CGraphicImageInstance::New()`).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Glowne Struktury w PythonSkill:**
- `CPythonSkill` - Singleton, przechowuje wszystkie zaladowane umiejetnosci i rozszerzenia w slowniku. Wlasciciel: watek glowny (UI/Main).
- `CPythonSkill::SSkillData` - Posiada m.in. wektory wymagan, dane dot. klasy punktow (`GetSkillCoolTime`, `GetNeedSP`), stringi dla modyfikatorow (`AffectDataVector`). Posiada zmienne mapowe (`ms_StatusNameMap`). Wielkosc zmienna, zalezy od kolekcji `std::vector` i `std::string`.

**Wybrane Metody Modulu Pythonowego:**
- `skillGetSkillName(PyObject* poSelf, PyObject* poArgs)`: Odczytuje `(SkillIndex, [SkillGrade])`. Zwraca string (nazwe).
- `skillGetSkillType(PyObject* poSelf, PyObject* poArgs)`: Odczytuje `SkillIndex`. Zwraca byType (int - wyliczenie z constants).
- `skillGetSkillDescription(PyObject* poSelf, PyObject* poArgs)`: Odczytuje `SkillIndex`. Zwraca opis (string).
- `skillGetSkillAffectDescription(PyObject* poSelf, PyObject* poArgs)`: Odczytuje `SkillIndex, AffectIndex, SkillPoint`. Zwraca wyliczona ze wzoru (przez `CPoly`) wartosc z tekstowym opisem.
- `skillGetGradeData(PyObject* poSelf, PyObject* poArgs)`: Odczytuje `SkillIndex, GradeIndex`. Zwraca informacje o stopniu z `GradeData`.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Modul w Pythonie:** `skill` wywolywany po zadeklarowaniu poprzez `Py_InitModule("skill", s_methods)`.
- **Ekspozycje (s_methods):** 
  - `GetSkillName` -> `skillGetSkillName`
  - `GetSkillDescription` -> `skillGetSkillDescription`
  - `GetSkillType` -> `skillGetSkillType`
  - `GetSkillAffectDescription` -> `skillGetSkillAffectDescription`
  - `GetGradeData` -> `skillGetGradeData` (dla GetSkillGrade)
- Brak bezposrednich zaleznosci sieciowych (np. opcodes) na poziomie modulu - modul dziala read-only w trybie offline jako wyszukiwarka parametrow klienta.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Wszystkie funkcje (jak np. CPoly parse / Py_BuildValue) musza byc realizowane w glownym watku, poniewaz mostek Python C-API w Metinie wykorzystuje Global Interpreter Lock (GIL) nie w pelni wyizolowany wielowatkowo w `s_methods`.
- **Potencjalne punkty awarii:** 
  - Przekazanie nieistniejacego `SkillIndex` zwraca krotkie wyrzucenie `Py_BuildException` z wiadomoscia. Agent w Pythonie musi sie z tym liczyc by nie zcrashowac logiki UI.
  - Generacja nowych tekstur ikon przez `skillGetIconInstance` uzywa puli pamieci (lub manualnej alokacji). Zgubienie refki Python/C++ = Memory Leak (szczegolnie bez uzycia GC).
- **Inne Pulapki:** Metoda `skillGetSkillCoolTime` uwzglednia tzw. `CASTING_SPEED` gracza `CPythonPlayer::Instance().GetStatus(POINT_CASTING_SPEED)`. To oznacza ze zwrocony "Cooldown" w UI zalezy od aktulnego ekwipunku postaci na ktorej zalogowany jest gracz, a nie samej tabeli proto!

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Krok po Kroku (Dodawanie funkcji C-API):**
  1. Zdefiniuj funkcje o sygnaturze `PyObject * skillMojaFunkcja(PyObject * poSelf, PyObject * poArgs)`.
  2. Wydobadz argumenty (np. `PyTuple_GetInteger`).
  3. Znajdz strukture w `CPythonSkill::Instance().GetSkillData()`.
  4. Zwroc przez `Py_BuildValue(...)` dane.
  5. Dodaj wpis `{"MojaFunkcja", skillMojaFunkcja, METH_VARARGS}` do `s_methods[]` na samym dole pliku (funkcja `initskill()`).
- **Testowanie (Headless):** Ten modul latwo za-mockowac ladujac plik tekstowy umiejetnosci i podstawiajac "fake" Singleton CPythonPlayer zwracajacy stale parametry. Do weryfikacji nalezy zaimplementowac interpreter CPython w srodowisku C++.
