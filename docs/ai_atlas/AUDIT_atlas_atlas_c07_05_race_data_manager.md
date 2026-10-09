---
task_id: "atlas_c07_05_race_data_manager"
cluster: "MOD"
module_name: "CRaceData i CRaceManager - Rejestr Ras, Kosci i Ksztaltow"
target_files:
- src/GameLib/RaceData.cpp
- src/GameLib/RaceData.h
- src/GameLib/RaceManager.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c07_05_race_data_manager.md"
architecture_layer: "Modele 3D, Szkielety Granny, Drzewa i Efekty"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Ten modul w architekturze klienta Metin2 pelni kluczowa role w zarzadzaniu danymi o rasach, potworach i NPC. 
`CRaceData` przechowuje informacje takie jak mapowanie kosci dla modeli 3D, efekty wizualne (np. dym), dane kolizyjne, sciezki do plikow z modelami i animacjami, oraz informacje o combo i uderzeniach. 
`CRaceManager` sluzy jako globalny (Singleton) punkt dostepu, ktory laduje te dane z plikow i zarzadza ich pamiecia podreczna, aby nie duplikowac wczytywania modeli.
Jest on wywolywany na etapie inicjalizacji aplikacji (wczytywanie modeli i systemow), podczas wczytywania konkretnego bytu (np. gdy postac loguje sie na mape, gdy pojawia sie nowy mob).
Przeplyw danych: `CRaceManager` pobiera id rasy. Jezeli ta jest w cache, to po prostu zwraca wskaznik do `CRaceData`. W przeciwnym razie `__LoadRaceData` probuje wczytac dane: wczytuje pliki konfiguracyjne (msm, txt) parsujac je z wykorzystaniem `CPackManager`.
Zarzadzanie cyklem zycia obiektow odbywa sie czesciowo manualnie, czesciowo uzywajac pul pamieci z `CDynamicPool`. Obiekty laduja sie w odpowiedzi na potrzebne byty, zas po ukonczeniu ich istnienia mozna je recznie dealokowac uzywajac `Destroy`.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound):**
  Modul ten najczesciej wolany jest z klas takich jak `CActorInstance`, `CInstanceBase` oraz `CPythonCharacterManager` gdy tworzone sa instancje bytowe.
- **Zaleznosci wyjsciowe (Outbound):**
  Zaleznosci Outbound wlaczaja `EterGrnLib` (struktury modeli Granny 3D jak `CGraphicThing`), `EterLib` (menedzery zasobow `CResourceManager`), pule watkow `GameThreadPool`, moduly asynchroniczne, i VFS gry (`PackLib/PackManager`).
- **Drzewo dyrektyw `#include`:**
  - `RaceData.h`: `<EterGrnLib/Thing.h>` - bazowe klasy
  - `RaceData.cpp`: `"StdAfx.h"`, `"EterLib/ResourceManager.h"`, `"EterLib/AttributeInstance.h"`, `"EterBase/Utils.h"`, `"RaceData.h"`, `"RaceMotionData.h"`, `"EterBase/Filename.h"`
  - `RaceManager.cpp`: `"StdAfx.h"`, `"RaceManager.h"`, `"RaceMotionData.h"`, `"PackLib/PackManager.h"`, `"EterLib/GameThreadPool.h"`, `<future>`, `<vector>`, `<set>`, `<algorithm>`
  Potencjalne ryzyka cyklicznych zaleznosci moga zachodzic z `RaceMotionData.h` (wspoltworza one dane o ruchu modelu).
- **Model pamieciowy:**
  Kod korzysta glownie z czystych wskaznikow C. Przechowuje zasoby na bazie `std::map`, `std::vector`, np. `typedef std::map<DWORD, CRaceData*> TRaceDataMap;`. Posiada muteksy (np. `std::mutex m_RaceDataMapMutex`) do zarzadzania wspolbieznoscia, co sugeruje ze system bywa wzywany asynchronicznie.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur:**
- `CRaceData`: Przechowuje sciezki do plikow, informacje o kosciach (bone attaching), uderzeniach combo. Rozmiar: zalezny od instancji klas map; wlasciciel watku glowny, badz pool zarzadzajacy wczytywaniem.
- `CRaceManager`: Singleton, globalny rejestr wczytanych ras i zasobow im przypisanych. Uzywa muteksow do ladowania wielowatkowego.
- `CRaceData::TMotionModeData`: Przechowuje id trybu animacji oraz przypisana mape wktorow animacji.
- `CRaceData::SSkin`, `SHair`, `SShape`: Struktury z informacjami o plikach podmieniajacych/doczepiajacych czesci skory.

**Tabela Metod Publicznych:**
- `CRaceData::GetMotionKey(WORD wMotionModeIndex, WORD wMotionIndex, MOTION_KEY * pMotionKey)`: Oblicza finalny klucz (np. horse/general motion), zwraca bool statusu. Bez efektow ubocznych, odczyt.
- `CRaceData::LoadRaceData(const char * c_szFileName)`: Wczytuje informacje o kosciach, bryle z dysku. Side-effects: aktualizuje cache ras, wysyla zapytania do menedzera zasobow.
- `CRaceManager::GetRaceDataPointer(DWORD dwRaceIndex, CRaceData ** ppRaceData)`: Bezpieczne wielowatkowo, pobiera lub wczytuje rase asynchronicznie jesli nieistnieje. Side-effects: jezeli rasa w trakcje ladowania asynchronicznego - synchronizuje ladowanie. Dodaje dane do pamieci, dealokuje w razie problemow.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
`CRaceData`: Posiada na poczatku duzo uzywanych pol mapowych (wskazniki kontenerow) jak `TModelDataMap`, pola dla combo `TComboAttackDataMap`, i predefiniowanych tablic C `DWORD m_adwSmokeEffectID[SMOKE_NUM]`. Brak oczywistych wyrownan C-style.
Podpiecia pod boty (FFI): Glownym entry-point by sprawdzic kosci (do np aimbota czy ESP) bedzie pobieranie `m_AttachingBoneNameMap` ( offset w wariancie x64 mapy w `CRaceData` ).

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Pakiety Sieciowe:** 
  Sam modul bezposrednio nie parsuje pakietow (GC/CG). Operuje na id bytow i indeksach (np. nr rasy pobranej wczesniej z pakietu od serwera). Modul ten jest warstwa obslugujaca modele wizualne na polecenie logiki klienta (nie ma bezposrednich opcode pakietu).
- **Metody Pythona (`PyMethodDef`):** 
  Ten podsystem moze byc wzywany niejawnie przez metody Pythona takie jak (zwykle definiowane w systemach nadrzednych `PythonCharacterModule` itp) np. podczas logowania nowej rasy czy zakladania kostiumow (chrladuj, set_shape). Bezposrednich C-API bindingow w tych konkretnych plikach nie ma - one narzedziem bazowym.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Wczytywanie nowej rasy (via `CRaceManager::GetRaceDataPointer`) jest chronione muteksami: `m_RaceDataMapMutex`, `m_LoadingRacesMutex`. Mozliwe ladowanie czesci w oddzielnych watkach (`GameThreadPool`). Istotne zeby zwrocic uwage czy obiekty `CGraphicThing` (grafika 3D z uzyciem DirectX 9) nie wczytuja sie na watku pobocznym zamiast glownego D3D (moze to powodowac crashe D3D context). Zabezpieczeniem przed tym sa opoznione alokacje w `GetBaseModelThing`.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  Pola przechowujace pliki np. _lod_01.gr2 laduja plik przez `NoExtension`. W przypadku pustych scierzek (Nullowe pointery), menedzery zglaszaja TraceError ale moga rzucic nullptr jako wynik `GetRaceDataPointer` - co moze zcrashowac wolajace funkcje, jezeli nie uzyto checku `if(pRaceData)`. 
- **Zarzadzanie zasobami (RAII):** Kod mocno uzywa pamieci przydzielanej klasycznie z C-style `New()` / `Delete()` dla klas alokowanych przez `CDynamicPool`. Zignorowanie wywolania `DestroySystem` zleakuje powazne ilosci danych.
  Pointers do pamieci w mapach nalezy recznie niszczyc. Zastapienie na `std::unique_ptr` zniwelowaloby wycieki. Wskazane unikanie cyklowych wyciekow (zaleznosci w animacjach).

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  Jezeli konieczne bedzie dodanie np. trybow uderzen hybrydowych. Zlokalizuj tablice `s_kMap_stType_dwIndex` w `CRaceManager.cpp`. Dodaj definicje (np. "HYBRID_ATTACK") uzywajac typow dodanych uprzednio do `CRaceMotionData.h`. Nastepnie upewnij sie, iz dane o plikach msm (np. motlist.txt) maja dodane obslugi dla "HYBRID_ATTACK".
- **Jak debugowac i logowac:**
  Modul powszechnie uzywa `TraceError` oraz `Tracenf`. Mozna zalozyc breakpoint bezposrednio w `CRaceManager::GetRaceDataPointer` zeby sprawdzic jak identyfikowane jest konkretne `dwRaceIndex`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  Mockowac `CPackManager::Instance()` i przekazac fake plik motlist.txt bez interakcji z pakietami wlasciwymi. Symuluj `CRaceManager::Instance().CreateRace()` podajac mu statyczne wygenerowane zrodlo zeby ominac zaleznosci wzgledem zrodel dyskowych.

