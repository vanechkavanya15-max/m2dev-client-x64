---
task_id: "atlas_c09_04_py_chrmgr_module"
cluster: "PY"
module_name: "Modul Pythona 'chrmgr' - Zarzadzanie Postaciami w Skryptach"
target_files:
- src/UserInterface/PythonCharacterManagerModule.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c09_04_py_chrmgr_module.md"
architecture_layer: "Mostek Pythona, Moduly C-API i Skrypty Gry"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Modul `chrmgr` (implementowany m.in. w `PythonCharacterManagerModule.cpp`) dziala jako natywny most C-API pomiedzy srodowiskiem skryptowym Pythona (skrypty UI, logika gry) a sercem zarzadzania instancjami postaci w C++ (`CPythonCharacterManager` i `CRaceManager`). Pozwala na rejestracje zasobow (animacje, modele, rasy), zarzadzanie afektami i wplywa na rendering nickow nad postaciami.
- **Wywolywanie (Game Loop):** Modul ten jest inicjowany jednorazowo (`initchrmgr`) na poczatku dzialania klienta gry. Funkcje w nim zawarte (jak rejestrowanie kolorow nazw) sa wywolywane glownie przez skrypty inicjujace konfiguracje, podczas gdy wlasciwosci pojedynczych postaci (np. `SetAffect`, `SetEmoticon`) sterowane sa pakietami sieciowymi na biezaco przez dispatcher fazy gry (np. `CPythonNetworkStream`).
- **Control Flow & Data Flow:** Z punktu widzenia przeplywu, skrypt Pythona wywoluje zarejestrowane z API metody (np. `chrmgr.SetEmoticon(vid, eft)`). W bloku C++ nastepuje rozpakowanie elementow ze sterty (np. `PyTuple_GetInteger`), weryfikacja (C++ nie wyrzuca std::exception do Pythona, lecz stosuje standardowe `Py_BuildException` / `Py_BadArgument`), a na koncu zadanie trafia z reguly do metody docelowego Singletona: `CPythonCharacterManager::Instance().SetEmoticon(vid, eft)`.
- **Cykl zycia (Lifecycle):** Rejestrowanie klas C-API odbywa sie statycznie z definicji tabeli `PyMethodDef`. Sam `chrmgr` dziala na rzecz obiektow, ktore zyja przez cala gre (wzorce Singleton), natomiast pojedyncze podmioty - wywolywane z `VID` - moga juz nie istniec. Uzywa opoznionej destrukcji i map asocjacyjnych, nie przechowujac na Pythonie wskaznikow. Metody pokrewne dedykowane scisle konkretnej wybranej postaci, takie jak `SelectInstance`, `ShowInstance`, `SetMotion`, `SetArmor` znajduja sie logicznie w module `chr` (z pliku `PythonCharacterModule.cpp`), a `chrmgr` skupia ogol postaci.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):**
  - **Skrypty Klienta:** `playerSettingModule.py`, `ui.py`, inne skrypty ladujace gre.
  - **Klient Metin2:** `PythonApplicationModule` odpala `initchrmgr` podczas startu gry.
- **Zaleznosci wyjsciowe (Outbound):**
  - `CPythonCharacterManager`: Glowny arbiter zarzadzania postaciami.
  - `CRaceManager` / `CRaceData`: Singletony odpowiadajace za meta-dane rasy (Modele 3D, zbroje, kosci, animacje GR2).
  - `CInstanceBase`: Klasa instancji 3D, udostepnia stale statyczne (np. kolory nazw `NAMECOLOR_MOB`) do eksportu.
  - Python C-API: Modul `Python.h`.
- **Drzewo dyrektyw `#include`:** 
  - `StdAfx.h` (Glowny punkt, Windows.h, makra, stale i podstawy Pythona).
  - `PythonCharacterManager.h` (Kluczowy dla calej komunikacji).
  - `PythonBackground.h` (Potrzebne np. do konwersji koordynatow swiata dla `GetVIDInfo`).
  - `InstanceBase.h` (Uzycie enumow takich jak `EFFECT_HIT`).
  - `GameLib/RaceManager.h`
- **Model pamieciowy:** Brak alokacji pamieci ze sterty przez sam modul `chrmgr`. Modul rzutuje po prostu C-pointery referencyjne na referencje Singletonow. Od strony powiazanej (uzywanie VID) mapuje sie id numeryczne by uniknac wskaznikow-sierot (dangling pointers) (np. szukanie na liscie aktywnych `GetInstancePtr(nVID)` jest wysoce preferowane nad trzymaniem CInstanceBase*).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur:**
  - `CPythonCharacterManager`: Rejestruje encje w obiekcie rejestru aktorow i mapuje ID sieciowe. Posiada struktury ECS na nowszych standardach. Singleton z watku glownego.
  - `CInstanceBase`: Encja postaci w grze z podpieta warstwa fizyki, renderu (SpeedTree / Granny) i combat systemu.
- **Tabela Metod Publicznych (Mapowanie C-API):**
  - `chrmgrSetEmpireNameMode`: Przelacza tryb flag krolestw.
  - `chrmgrRegisterTitleName` / `chrmgrRefreshAllPCTextTail` / `chrmgrRegisterNameColor` / `chrmgrRegisterTitleColor`: Odpowiada za UI nickow (tzw. TextTail).
  - `chrmgrGetVIDInfo` / `chrmgrGetPickedVID`: Narzedzia zwrotne / debugowe dot. instancji.
  - `chrmgrSetPathName` / `chrmgrCreateRace` / `chrmgrSelectRace` / `chrmgrLoadRaceData` / `chrmgrSetShapeModel` / `chrmgrAppendShapeSkin`: Ladowanie folderu i zasobow (pliki msa/msm, gr2, tekstury) modeli ras.
  - `chrmgrRegisterAttachingBoneName` / `chrmgrRegisterMotionMode` / `chrmgrSetMotionRandomWeight` / `chrmgrRegisterMotionData` / `chrmgrRegisterCacheMotionData`: Zarzadzanie ruchem/animacjami do przypisanej rasy. (Cache opcja dla pre-loadingu przy ladowaniu).
  - `chrmgrSetAffect` / `chrmgrSetEmoticon` / `chrmgrIsPossibleEmoticon` / `chrmgrRegisterEffect` / `chrmgrShowPointEffect`: Mechanika ikonek efektow nad glowa czy stanow bufow.
  - Oczekuja formy tuple wylapywanej jako (`PyObject* poSelf`, `PyObject* poArgs`). Wszystkie zwaracaja obiekty PyObject (glownie `Py_BuildNone()`).
- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  Modul operuje na C-API i nie definiuje wlasnego ukladu dla modyfikacji, wykorzystuje jedynie stalych. Stale dostepne po stronie pythona np. `chrmgr.NAMECOLOR_EMPIRE_PC` (wartosc = 5) sa eksportowane bezposrednio ze stalych `CInstanceBase::NAMECOLOR_*`.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Sam modul C-API pythona siecia bezposrednio nie zarzadza, ale serwer uzywajac np. `TPacketGCCharacterAdditionalInfo` wysyla zjawiska i powiazane do aktora tytuly/kolory/efekty (np. opcody z handlerami w `NetworkActorManager`), a te z kolei po aktywacji w C++ uaktulaniaja stan lub w skryptach odpala metody z tego mostka (np. `chrmgr.SetEmoticon`).
- **Metody Pythona (`PyMethodDef`):** Eksport API z uzyciem flagi `METH_VARARGS`. Eksportuje metody do pre-loadingu m.in. cache (`chrmgrRegisterCacheMotionData`), rasy, oraz wysoce specyficzne efekty w tabeli m.in. dla uzycia mikstur autoleczenia: `EFFECT_AUTO_HPUP`. Moduly powiazane dla pojedynczego obiektu, tj. `SelectInstance`, `ShowInstance`, `SetArmor` i `SetMotion` znajduja sie w zaleznym module o nazwie "chr" (plik: `PythonCharacterModule.cpp`), eksportujac na poziomie pojedynczego zaznaczenia.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Python-C API musi dzialac wspolbierznie tylko pod ochrona Global Interpreter Lock (GIL). Wykonuje sie z reguly na watku glownym (UI + Direct3D). Uzywanie np. ladowania nie-cachiowanego pliku modelu w trakcie gry moze zablokowac pipeline renderingu glownego.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Brak zabezpieczenia wezwan rasy. Funkcje takie jak `chrmgrLoadRaceData`, `chrmgrRegisterComboAttackNew` pobieraja pointer rasy (`CRaceManager::Instance().GetSelectedRaceDataPointer()`). Jesli skrypt wczesniej nie wolal `SelectRace()`, zwracany jest NULL co konczy sie bezbolesnie wylacznie rzuceniem wyjatku przez system pythona `Py_BuildException("RaceData has not selected!")`. Omija to bezposrednie naruszenie pamieci (Segfault).
  - Buffer overflow w `chrmgrGetVIDInfo` ze wzgledu na uzycie archaicznego `char szInfo[1024]` wraz ze starym silnikowym `sprintf()`.
- **Zarzadzanie zasobami (RAII):** Parsowanie argumentow (`PyTuple_GetInteger`) w moduly PythonCharacterManagerModule.cpp nie zwieksza reference-counta tuple'a, wiec nie trzeba pamietac o uzyciu `Py_DECREF`. Stosowane jest rygorystyczne zwracanie bledu: jesli zwrocone false, leci `Py_BadArgument()`.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Stworz docelowa nowa funkcje o typowym layoucie C-API np. `PyObject * chrmgrNowyFiczer(PyObject* poSelf, PyObject* poArgs)`.
  2. Rozpakuj parametry za pomoca `PyTuple_GetInteger` badz `PyTuple_GetString`. W razie bledow return `Py_BadArgument()`.
  3. Odwolaj sie do wlasciwego singletona poprzez Instance (np. `CPythonCharacterManager::Instance()`). Jesli dzialasz na rasach, sprawdz czy rase przedtem zaznaczono (!pRaceData -> wyrzuc exception).
  4. Dodaj strukture do `s_methods[]` w podsekcji `initchrmgr()`, mianujac sygnature i flage np. `{"NowyFiczer", chrmgrNowyFiczer, METH_VARARGS}`.
- **Jak debugowac i logowac:** Unikac drukowania bezposrednio do konsoli `printf`. W `chrmgrLoadRaceData` logowany jest nieudany ladowanie: `TraceError("Failed to load race data : %s\n", c_szFullFileName);`. Modernizujac nalezy skorzystac z wbudowanego `EterBase::ModernLogger`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Mockowanie tego modulu jest zalezne od mocka Singletonow. W Linux sandbox mozna zaincludowac tylko glowne naglowki uzywajac podstawienia makr dla Direct3D, zbudowac tablice pythonowa wirtualnie bez zaleznosci od interfejsu wizualnego i testowac C++ zachowanie metod API, dostarczajac dummy instancje poprzez mockowany Singleton. Stale moga byc testowane narzedziem Pythona (`pytest`).

