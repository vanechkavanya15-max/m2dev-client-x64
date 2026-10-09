---
task_id: "atlas_c09_03_py_player_module"
cluster: "PY"
module_name: "Modul Pythona 'player' - Akcje Gracza i Stan Postaci"
target_files:
- src/UserInterface/PythonPlayerModule.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c09_03_py_player_module.md"
architecture_layer: "Mostek Pythona, Moduly C-API i Skrypty Gry"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul CPython API `player` (zaimplementowany w `PythonPlayerModule.cpp`) dziala jako interfejs (mostek) pomiedzy silnikiem klienta gry (C++) a logika interfejsu uzytkownika oraz skryptami napisanymi w Pythonie. 
Jego glownym celem biznesowym jest umozliwienie dostepu z poziomu skryptow UI do stanow postaci, statystyk (HP, SP, statystyki poboczne), zawartosci ekwipunku, drzewka umiejetnosci oraz obslugi akcji gracza (uzycie przedmiotu, nacisniecie umiejetnosci, atak).
Wywolania odbywaja sie bezposrednio podczas dzialania skryptow Pythona, ktore z kolei sa wykonywane wewnatrz petli gry, przewaznie w obsludze zdarzen myszy/klawiatury lub metodach OnUpdate/OnRender okien UI. Przeplyw danych opiera sie na konwersji typow Pythona (Tuple/Int/Dict) na typy C++, wykonaniu odpowiedniej logiki biznesowej w singletonie `CPythonPlayer` lub na glownej postaci (`CInstanceBase`), a na koniec opakowaniu wyniku z powrotem w obiekty Pythona (czesto poprzez funkcje pomocnicze `Py_BuildValue`, `Py_BuildNone`, `PyLong_FromLong`). Obiekty nie sa tutaj manualnie niszczone; polega to glownie na zarzadzaniu pamiecia poprzez Python C-API (np. kradziez referencji i garbage collection).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):**
  Modul ten jest wywolywany glownie przez pliki `*.py` interfejsu uzytkownika gry (m.in. `uiInventory.py`, `uiCharacter.py`, `uiTaskBar.py`, systemy quick slotow, oknach grupy, ekwipunku, itp.).
- **Zaleznosci wyjsciowe (Outbound):**
  Zalezy bezposrednio od modulu `CPythonPlayer` (serce klienta gry obslugujace logike gracza), klas `CInstanceBase` oraz `CPythonApplication`. Wchodza z nim w interakcje rowniez pomniejsze systemy takie jak `CItemData` i narzedzia do komunikacji miedzy modulowej (np. system zdarzen i punktow postaci).
- **Drzewo dyrektyw `#include`:**
  - `StdAfx.h` (Prekompilowane naglowki, w tym bazowe makra).
  - `PythonPlayer.h` (Glowny singleton, logika interakcji z ekwipunkiem i graczem).
  - `PythonApplication.h` (Stan aplikacji).
  - `<utf8.h>` (Obsluga kodowania tekstu/ciagow znakow).
- **Model pamieciowy:**
  Kod wykorzystuje surowe wskazniki wchodzace w sklad Python C-API (np. `PyObject*`) oraz mechanizmy kontroli zycia Pythona, co oznacza ryzyko dla bezpieczenstwa jesli referencje z `PyLong_FromLong` itp. beda zle zarzadzane w kontekscie skryptu. Odwoluje sie rowniez za pomoca referencji/wskaznikow pobieranych z Singletonow (np. `CPythonPlayer::Instance()`).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur:**
  Ten plik jest czystym zbiorem funkcji (proceduralnym mostkiem). Funkcje zawarte to pojedyncze exporty Python C-API. Wszelkie struktury i typy sa przechowywane w `CPythonPlayer`.
- **Tabela Metod Publicznych (wybrane kluczowe metody analizowane):**
  - `playerGetItemIndex(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)`
    Argumenty: Opcjonalnie zlozona krotka podajaca indeks okna `window` (TItemPos) oraz okna glownego lub prosty indeks w postaci int/long `iSlotIndex`.
    Skutki: Wywoluje `CPythonPlayer::Instance().GetItemIndex()`, zamienia wynik za pomoca `PyLong_FromLong()`.
  - `playerGetStatus(PyObject* poSelf, PyObject* poArgs)`
    Argumenty: `iType` typu Integer, przekazane poprzez `PyTuple_GetInteger`.
    Skutki: Odczytuje statystyke z `CPythonPlayer::Instance().GetStatus64()` (uwzgledniajac modyfikatory bronia/zbroja).
  - `playerClickSkillSlot(PyObject * poSelf, PyObject * poArgs)`
    Argumenty: Indeks umiejetnosci `iSkillSlot`.
    Skutki: Wywoluje `CPythonPlayer::Instance().ClickSkillSlot()`. Uzywane przez QuickSlot oraz Skill UI.
  - `playerGetPlayTime(PyObject* poSelf, PyObject* poArgs)`
    Argumenty: Brak / `poArgs` puste.
    Skutki: Zwraca `CPythonPlayer::Instance().GetPlayTime()` (w sekundach lub minutach zaleznie od konwersji po stronie Pythona).
  - `playerIsSkillCoolTime(PyObject* poSelf, PyObject* poArgs)`
    Argumenty: Indeks slota umiejetnosci.
    Skutki: Sprawdza, czy gracz moze uzyc umiejetnosci uzywajac `CPythonPlayer::Instance().IsSkillCoolTime(iSlotIndex)`
  - Atak i akcje powiazane (np. oczekiwane `SendAttack` realizowane przez inne metody takie jak `ComboAttack` lub delegowane nizej):
    `playerComboAttack` wywoluje `CPythonPlayer::Instance().NEW_Attack()`. Metoda `SetAttackKeyState` ustala flage wcisniecia guzika ataku. Bezposrednie wysylanie pakietow ataku do serwera (sieciowe SendAttack) moze znajdowac sie w modulach sieciowych (`net`).
- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  Brak bezposrednich definicji struktur pamieciowych wewnatrz modulu. Parametry stale rejestrowane sa za pomoca `PyModule_AddIntConstant` i pokrywaja sloty, zaleznosci grup (PARTY_STATE) czy umiejetnosci i statusy PK.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:**
  Brak bezposredniej obslugi pakietow sieciowych (GC/CG). Wysylanie akcji i logiki sieciowej delegowane jest podspodem w singletonie `CPythonPlayer` badz przez instancje (np. `CPythonNetworkStream`).
- **Metody Pythona (`PyMethodDef`):**
  - `{"GetItemIndex", (PyCFunction)playerGetItemIndex, METH_FASTCALL}`
  - `{"GetStatus", playerGetStatus, METH_VARARGS}`
  - `{"ClickSkillSlot", playerClickSkillSlot, METH_VARARGS}`
  - `{"GetPlayTime", playerGetPlayTime, METH_VARARGS}`
  - `{"IsSkillCoolTime", playerIsSkillCoolTime, METH_VARARGS}`
  - `{"ComboAttack", playerComboAttack, METH_VARARGS}`
  - `{"SetAttackKeyState", playerSetAttackKeyState, METH_VARARGS}`

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:**
  Calosc kodu Pythona (i Python C-API w kliencie Metin2) powinnna byc odpalana WYLACZNIE w glownym watku, poniewaz Global Interpreter Lock (GIL) nie jest obslugiwany odpowiednio dla wielowatkowosci UI.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Typowe pulapki w `METH_VARARGS` i `PyTuple_GetInteger` - jezeli wartosci wejsciowe ze skryptu .py (np. index slota w Inventory) beda wieksze niz zalozone, API Pythona nie zabezpiecza same w sobie w tym miejscu kodu C przed wyjsciem za bufor. Nalezy polegac na zabezpieczeniach w warstwie ponizej (`CPythonPlayer`).
  - Roznice w konwersjach pomiedzy `PyLong_FromLong` a `Py_BuildValue("i")` moga prowadzic do problemow z wydajnoscia, jak tez w zaleznosci od kompilacji x64, zwracaniem zlych wartosci przy duzych intach.
- **Zarzadzanie zasobami (RAII):**
  Wycieki pamieci poprzez Py_BuildException badz Py_BuildNone rzadko w tym module wystepuja, poniewaz Metin uzywa systematycznie stalych wskaznikow od singletonow gry, jednakze przy modernizacjach nalezy zachowac nowa polityke powrotu.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zaprojektuj funkcje jako statyczna np. `PyObject * playerMyNewAction(PyObject* poSelf, PyObject* poArgs)` lub korzystajac z szybkiego parsowania zmiennych na stosie przy wsparciu `METH_FASTCALL`.
  2. Sparsej argumenty poprzez odpowiednia funkcje API Pythona.
  3. Wywolaj pozadana logike z warstwy EterLib / `CPythonPlayer`.
  4. Dodaj strukture rejestracji na samym dole pliku w tablicy `s_methods[]`.
- **Jak debugowac i logowac:**
  Jezeli uzywa sie modern logger (EterBase::ModernLogger), uzywaj bezposrednio `ModernLogger::Error()` z formatowaniem `{}` zamiast starego API do wypisywania bledow z blednych parametrow wejsciowych w PyObjectach.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  Ten modul jest mocno zalezny od API pythona oraz interfejsow singletonow jak `CPythonPlayer` oraz `CInstanceBase`. Testy headless wymagaja w pelni zasymulowanego srodowiska interfejsu pythona i mockowania obslugi gracza oraz symulowanego main instancingu (tworzenie Main Playera).
