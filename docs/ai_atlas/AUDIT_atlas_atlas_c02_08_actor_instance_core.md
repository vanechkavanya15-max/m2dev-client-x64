---
task_id: "atlas_c02_08_actor_instance_core"
cluster: "ACT"
module_name: "CActorInstance - Silnik Aktora w Warstwie GameLib"
target_files:
- src/GameLib/ActorInstance.cpp
- src/GameLib/ActorInstance.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c02_08_actor_instance_core.md"
architecture_layer: "Maszyna Stanow Postaci i Aktorzy"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

**Funkcja modulu:**
Klasa `CActorInstance` stanowi serce silnika postaci w warstwie GameLib. Jest to glowny wezel (hub) zarzadzajacy stanem kazdego aktora w grze - zarowno gracza (PC), potworow (Enemy/NPC), jak i obiektow takich jak kamienie Metin, czy bramy. Modul ten jest "super-struktura" grupujaca wiele pomniejszych podsystemow: system ruchu (Motion), kolizje, system walki (Battle), zarzadzanie bronia i efektami (Attaching/WeaponTrace), oraz renderowanie i transformacje przestrzenne. 

`CActorInstance` nie tylko trzyma dane, ale dziala jako maszyna stanow dla aktora w swiecie gry. Mostkuje logike gry (GameLib) z silnikiem renderowania (EterLib/DirectX 9) i animacji (Granny 3D - poprzez agragacje obiektu klasy nadrzednej lub podobnej w zaleznosci od dokladnej struktury, zazwyczaj powiazane z `CGraphicThingInstance` jesli `CActorInstance` dziedziczy z klas z EterGrnLib).

**Cykl zycia (Lifecycle):**
- **Alokacja/Inicjalizacja:** Obiekty sa tworzone, a nastepnie inicjowane poprzez `__Initialize()` (w tym dane stanow, ruchu, rotacji, pozycji, kolizji). 
- **Zarzadzanie stanem:** Aktor wchodzi w zycie uzywajac np. `SetRace()`, `SetVirtualID()`, a nastepnie przechodzi w stany takie jak Wait, Move, Attack. 
- **Wykonanie w petli gry (Update/Render):** W kazdej klatce wolana jest funkcja `OnUpdate()` (w tym `Update()`, `TransformProcess()`, `Transform()`, `UpdatePointInstance()`, `ShakeProcess()`, `UpdateBoundingSphere()`, `UpdateAttribute()`).
- **Usuwanie/Dealokacja:** Usuwanie z uzyciem metody `Destroy()`, ktora czysci zalezne zasoby (np. `__ClearAttachingEffect()`, niszczy drzewa kolizji). Nalezy jednak uwazac na dangling pointers, gdyz z innych miejsc do aktora odnosza sie inteligentne i czyste wskazniki.

**Przeplyw danych (Control Flow):**
Wejscia sterujace (np. pakiety z serwera odbierane przez warstwe sieci, a nastepnie wysylane przez `CPythonCharacterManager`) przekladaja sie na wolanie funkcji np. `Move()`, `Stop()`, `NormalAttack()`, `ComboAttack()`. Nastepnie `CActorInstance` dodaje ruch (Motion Queue - `TMotionDeque`), przetwarza go (`CurrentMotionProcess()`), aktualizuje fizyke (`PhysicsProcess()`), obsluguje klatki kluczowe animacji (`MotionEventProcess()`) generujac np. efekty uderzen lub dzwieki. Na koncu petli obliczana jest pozycja przestrzenna i macierze (`INSTANCEBASE_Transform()`).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
Klasa `CActorInstance` jest wolana przede wszystkim przez:
- `CInstanceBase` (ktora wrapuje aktora dla interfejsu uzytkownika i pythona, dodajac paski HP, nicki z TextTail, system gildii, logike zaleznosci od sieci i statystyk postaci). `CInstanceBase` zarzadza obiektem `CActorInstance`.
- `CPythonCharacterManager` - zarzadzanie globalna pula aktorow.
- Obiekty typu "Target" i sortowniki, takie jak np. `TargetRankSorter` (TargetRankSorter.h) uzywane przy wyborze celu (Tab-targeting), sprawdzajace widocznosc i odleglosc aktora.

**Zaleznosci wyjsciowe (Outbound):**
Modul wola i zalezy od szeregu podsystemow w silniku:
- **Silnik animacji i grafiki:** Granny 3D (obsluga modeli, kosci, np. `CGraphicThingInstance`, w pliku zahaczane przez `RaceData`, `RaceMotionData`). EterLib i silnik DirectX 9.
- **Efekty specjalne:** System Attaching (np. bron, efekty miecza - `CWeaponTrace`), dzwiek i czasteczki.
- **Fizyka i Kolizje:** `CPhysicsObject`, `CDynamicSphereInstanceVector`, detekcja kolizji przez `CheckCollisionDetection`.
- **Otoczenie:** `SpeedTreeWrapper` (drzewa), `AreaTerrain`, `IBackground`.
- **Zarzadzanie danymi zewnetrznymi:** `CRaceData`, `CRaceMotionData`, `CItemData`.
- **Zarzadzanie stanem (Events):** Interfejs `IEventHandler`, z uzyciem wzorca Obserwator do powiadamiania o zmianach stanow, atakach i uszkodzeniach.

**Drzewo dyrektyw `#include` (kluczowe naglowki):**
Z pliku `ActorInstance.h`:
- `FlyTarget.h`
- `RaceData.h`, `RaceMotionData.h`
- `PhysicsObject.h`
- `ActorInstanceInterface.h`
- `Interface.h`
- `SpeedTreeLib/SpeedTreeForest.h`
Z pliku `ActorInstance.cpp`:
- `AreaTerrain.h`
- `SpeedTreeLib/SpeedTreeForestDirectX.h`, `SpeedTreeLib/SpeedTreeWrapper.h`

**Model pamieciowy:**
Glowny cykl wykorzystuje czyste wskazniki C (np. `CItemData*`, `CWeaponTrace*`, wskazy na `CRaceData*`, czyste referencje `CActorInstance&`), w znacznej mierze zarzadzane recznie (np. wywolywanie jawnego Destroy) oraz smart pointery, zaleznie od modulu zewnetrznego (np. `SpeedTreeWrapperPtr`). Zastosowanie kolejek `std::deque` (np. dla FlyTarget, ruchu). Mapy w pamieci podrecznej (np. `THittedInstanceMap`). Modul cechuje ryzyko wyciekow i access violations w razie bledu zwalniania pamieci z racji braku szerokiego uzycia RAII dla zasobow niszczonych recznie.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

#### Klasy i Struktury

| Nazwa symbolu | Rola w systemie | Wlasciciel / Dodatkowe info |
|---|---|---|
| `CActorInstance` | Glowna klasa, silnik aktora w grze. | Watek glowny (Main Thread). Rozmiar obiektu znaczacy z uwagi na liczbe wektorow, deque, map. |
| `CActorInstance::IEventHandler` | Interfejs obslugi eventow ze stanu (OnMove, OnAttack itp). | Implementowany nadrzednie, np. przez `CInstanceBase`. |
| `CActorInstance::SState` | Trzyma informacje o stanie (pozycja, rotacja). | Przekazywany przez referencje do handlerow. |
| `IMobProto` | Singleton do odpytywania typu rasy. | Singleton gry. |
| `TReservingMotionNode` | Wpis kolejki dla systemu kolejkowania animacji. | |
| `SCurrentMotionNode` | Reprezentuje obecnie odtwarzana animacje. | |
| `TMotionEventInstance` | Instancja klatki kluczowej eventu animacji. | |
| `TCollisionPointInstance` | Sfera kolizyjna doczepiona do kosci. | |
| `SSplashArea` | Informacje o atakach obszarowych i splash. | Posiada wektor uderzonych aktorow `THittedInstanceMap`. |
| `THittingData` | Informacja o uderzeniu (typ, klucz, index eventu). | |
| `TAttachingEffect` | Informacja o przypietym efekcie (np. lsnienie, skrzydla). | |

#### Enumery (Enums)
- `CActorInstance::EType` - (TYPE_ENEMY, TYPE_NPC, TYPE_STONE, TYPE_WARP, TYPE_DOOR, TYPE_BUILDING, TYPE_PC, TYPE_POLY, TYPE_HORSE, TYPE_GOTO, TYPE_OBJECT). Bardzo wazne przy identyfikacji w FFI lub C-API.
- `CActorInstance::ERenderMode` - (RENDER_MODE_NORMAL, RENDER_MODE_BLEND, RENDER_MODE_ADD, RENDER_MODE_MODULATE). Tryby renderowania.
- `CActorInstance::EMotionPushType` - (MOTION_TYPE_NONE, MOTION_TYPE_ONCE, MOTION_TYPE_LOOP). Kontrola petli animacji.
- `CActorInstance::EAttachEffect` - (EFFECT_LIFE_NORMAL, EFFECT_LIFE_INFINITE, EFFECT_LIFE_WITH_MOTION).

#### Metody Publiczne (Kluczowe API dla Agentow)

| Sygnatura Metody | Argumenty | Wartosc Zwracana | Funkcja / Warunki |
|---|---|---|---|
| `void INSTANCEBASE_Transform()` | Brak | `void` | Przeprowadza Update silnika fizyki, bounding sphere i drzew kolizji. Powinien byc zwalniany tylko z glownego watku. |
| `void SetEventHandler(IEventHandler* pkEventHandler)` | Pointer na handler | `void` | Ustawia most zwrotny z `CInstanceBase`. Musi byc wazny pointer. |
| `bool SetRace(DWORD eRace)` | Identyfikator rasy | `bool` | Laduje dane rasy (RaceData) i przygotowuje szkielet/kosci, incjalizuje aktora do uzycia. |
| `UINT GetActorType() const` | Brak | `UINT` | Zwraca wewnetrzny typ z `EType`. Uzyteczne dla AI przy ewaluacji otoczenia. |
| `void AttachWeapon(DWORD, DWORD, DWORD)` | Indexy ekwipunku | `void` | Dolozenie broni do modelu. Uzywa Granny bone index. |
| `bool PushOnceMotion(DWORD dwMotion, ...)` | Klucz animacji i fBlendTime | `bool` | Wpycha nowa animacje do kolejki. Zwraca false jesli nie udalo sie (np brak danych ruchu). |
| `void LookAt(float fDirRot)` / `(CActorInstance*)` | Rotacja (float) / Pointer | `void` | Ustawia rotacje i target wizualny postaci. |
| `BOOL AttackingProcess(CActorInstance& rVictim)`| Referencja na cel | `BOOL` | Obliczenia logiki kolizji przy ataku pomiedzy dwiema instancjami, zglasza hit jesli sukces. |

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

**Pakiety Sieciowe (Network Packets):**
`CActorInstance` nie posiada w sobie kodu parserow sieciowych. Jest on ostatecznym konsumentem zdarzen rozkodowanych przez `CPythonNetworkStream` i wywolanych poprzez `CPythonCharacterManager`. Bezposrednio implementowane skutki widac z obslugi pakietow z grupy GC (Game->Client) oraz CG (Client->Game):
- **GC_CHARACTER_ADD** / **GC_CHARACTER_UPDATE**: Tlumaczone na inicjalizacje i aktualizacje pozycji poprzez `NEW_SetSrcPixelPosition`, `NEW_SetDstPixelPosition`.
- **GC_CHARACTER_MOVE**: Tlumaczone na obsluge ruchu po sieci z uzyciem `Move()`, `SetAdvancingRotation()`.
- **GC_DAMAGE_INFO** / **GC_MOTION**: Wolanie interfejsow `__OnHit()` i dodawanie akcji obrazen do kolejki `m_MotionDeque`.
- **CG_ATTACK**: Generowane z klienckiego systemu `NormalAttack()` lub `ComboAttack()`, a nastepnie delegowane wyzej by zglosic trafienia (z `HitDataMap`).

**Python C-API (`PyMethodDef`):**
Ten kod jest czystym C++ i dziala zgodnie z zasada "ZERO-PYTHON" dla kodu rdzeniowego (Core/GameLib), nie eksportuje metod samodzielnie za pomoca makr `PyMethodDef`. Funkcje eksportowane sa poprzez moduly (np. `PythonPlayerModule.cpp`, `PythonCharacterModule.cpp`, `PythonNonPlayerModule.cpp`), ktore posiadaja referencje do `CPythonCharacterManager`, a ten z kolei zarzadza CInstanceBase. Ostatecznie takie komendy jak `chr.MoveTo(...)` z Pythona ewaluuja do zawolania `CActorInstance::Move()`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Zero-Conflict:** Ten kod nalezy do silnika nadrzednego sprzed refaktoringow i musi byc chroniony. Kod nowy wykorzystuje architekture komponentowa, jednak ten obszar dalej polega na monolitycznej budowie. Nie modyfikujemy tego pliku.
- **Zasady wielowatkowosci:** Kod uzywa archaicznej struktury opartej na watku glownym renderowania `Main Game Thread`. Wszystkie transformacje, aktualizacje z `Granny 3D`, i operacje na drzewach kolizji *MUSZA* byc wykonywane synchronicznie na jednym watku. Proby uzycia z watkow sieciowych (np. pobieranie wielkosci Bounding Sphere po asynchronicznym komunikacie) prowadza do wyscigu (Race Conditions) i natychmiastowych Crashy.
- **Potencjalne punkty awarii (Crash Points):** 
  - *Dangling Pointers przy FlyTargets*: W systemie celowania (`m_kFlyTarget`, `m_kQue_kFlyTarget`) mozna przechowywac wskazniki do `CActorInstance`, ktore zostaly usuniete przed updatem sfery kolizyjnej pocisku (przepelnienia/zwolnienia pamieci).
  - *Nulle w Event Handler*: Wywolania `m_pkEventHandler->__On*()` bez uprzedniego sprawdzenia `m_pkEventHandler` to rzadki ale niebezpieczny crash w procesach odlaczania. Zawsze uzywac metody `__GetEventHandlerRef()`, mimo ze nie zapobiega wprost przed pustymi referencjami jesli pointer zostal zniszczony.
- **Zarzadzanie zasobami (RAII):** Kod mocno polega na bezposrednim zarzadzaniu manualnym pamiecia (`new`/`delete`, pule zewnetrzne, `Clear()`). Jesli dodawane sa np. mapy, lub nowe instancje do `TAttachingEffectList`, niszczyciel (Destruktor/Destroy) musi to bezwzglednie wyczyscic, aby uniknac pamieciozernych wyciekow zasobow na kazdym wczytanym modelu z serwera.
- **Optymalizacja odleglosci (Distance Checks):** Zgodnie ze wczesniejsza wiedza o silniku z C++23, petle obliczajace odleglosci do innych aktorow np w systemie splash (`SSplashArea`) lub kolizjach unikaja operacji potegowych (`sqrt`) polegajac na kwadratach dystansow `distanceSq` jesli to mozliwe. Obliczenia rotacji w kierunku postaci musza korzystac ze specyficznej dla gry funkcji `std::atan2(dx, -dy)`.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
Z uwagi na zasade Zero-Conflict, tworzenie nowej fukcjonalnosci dla C++23:
1. Nie dopisuj deklaracji bezposrednio w `CActorInstance.h` jezeli to nowy sub-system.
2. Stworz obiekty EventBus w `src/Client/Core/`.
3. Wywoluj sub-system ze swiezo napisanego Handlera, uzywajac interfejsu (lub jesli wymagane, dodaj funkcje jako Adapter do naglowka w `GameLib`).
4. Jezeli zmiana wymaga uzycia wektora ruchu, wyciagnij go uzywajac `GetMovementVectorRef()` bez modyfikacji wnetrza Update loopa. 

**Jak debugowac i logowac:**
Z uzyciem wewnetrznego loggera `TraceError()` badz `SysErr()`.
Do benchmarkingu `CActorInstance::OnUpdate` posiada przygotowany przelacznik makra `#ifdef __PERFORMANCE_CHECKER__`, co pozwala podpiac system metryk by znalezc klatki lagujace render. Bierz pod uwage ze moduly logowania nie sa tutaj standardowym `std::format`.

**Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
1. By uruchomic Headless doctest uzyj mocka dla stanow DirectX 9, jako ze plik ma includy polegajace na `StdAfx.h`.
2. Przygotuj izolowany enviroment kompilacji `g++ -std=c++23 ... -DTEST_MODE_DISABLE_STDAFX=1` ze zmockowanymi wewnetrznie strukturami `IDirect3DDevice9`.
3. Unikaj symulowania calej postaci, poniewaz wywolanie konstruktora bedzie domagac sie prawidlowo polaczonego interfejsu rasy i fizyki, przetestuj zamiast tego logike biznesowa w C-style lub na minimalnych, mockowanych klasach EterBase.

