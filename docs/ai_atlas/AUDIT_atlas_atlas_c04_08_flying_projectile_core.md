---
task_id: "atlas_c04_08_flying_projectile_core"
cluster: "CBT"
module_name: "CFlyingInstance - Silnik Pociskow Balistycznych i Strzal"
target_files:
- src/GameLib/FlyingInstance.cpp
- src/GameLib/FlyingInstance.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c04_08_flying_projectile_core.md"
architecture_layer: "System Walki, Umiejetnosci i Fizyka Pociskow"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** `CFlyingInstance` jest odpowiedzialny za symulacje i renderowanie latajacych pociskow (np. strzal, pociskow magicznych) w przestrzeni 3D gry. Obsluguje krzywe balistyczne, grawitacje, przyspieszenie, naprowadzanie na cel (homing) oraz detekcje uderzen w cel (obiekty) lub tlo.
- **Moment wywolania:** Kod jest wywolywany glownie podczas glownej petli gry w fazie aktualizacji (`CFlyingManager::Update` i `CFlyingManager::Render`), gdzie aktualizowana jest pozycja i predkosc dla kazdej instancji pocisku. Petla wywoluje `Update()` dla symulacji fizycznej (ruch, kolizje) i `Render()` dla wyswietlania efektow.
- **Przeplyw danych (Data Flow):** 
  - Utworzenie z danych definicyjnych: Otrzymuje wlasciwosci z plikow skryptowych lotu poprzez `CFlyingData` (poczatkowa predkosc, grawitacja, rotacja, naprowadzanie).
  - Aktualizacja: Oblicza wektory predkosci na podstawie przyspieszenia z czasem (`CTimer::Instance().GetElapsedSecond()`). Detekcja zderzenia oblicza kwadrat odleglosci (`square_distance_between_linesegment_and_point`).
  - Rozbicie (Explosion): Jesli dojdzie do trafienia, wysylane jest powiadomienie do `IFlyEventHandler`, nastepuje redukcja wartosci z przebiciem (`PierceCount`) lub zniszczenie pocisku (`__Explode`, `__Bomb`), a cel otrzymuje obrazenia za posrednictwem interfejsu `IFlyTargetableObject`.
- **Cykl zycia (Lifecycle):** Instancje `CFlyingInstance` sa zarzadzane przez dynamiczna pule (Memory Pool) `CDynamicPool<CFlyingInstance> ms_kPool`. Alokacja pamieci realizowana jest poprzez `CFlyingInstance::New()`, a zwolnienie przez `CFlyingInstance::Delete()`. Metoda `Destroy()` jest wywolywana w celu resetu stanow klasy, bez koniecznosci uwalniania z pamieci. Menadzer obiektow (`CFlyingManager`) steruje tym cyklem podczas dzialania.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** 
  - Otrzymuje wywolania od managera obiektow (`CFlyingManager`), ktory gromadzi instancje lotu.
  - Otrzymuje informacje o celu od `CFlyTarget`.
  - Powiazanie z `CActorInstance` jako wlascicielem (`m_pOwner`) uzywajac interfejsu `IActorInstance`.
  - Inicjatorzy, np. `CInstanceBase` oraz `PythonPlayerEventHandler` (posrednio przez handlery lotu).
- **Zaleznosci wyjsciowe (Outbound):** 
  - **Fizyka i matematyka:** Funkcje D3DX dla macierzy, kwaternionow i wektorow (`D3DXVECTOR3`, `D3DXQUATERNION`), operacje na punktach (`square_distance_between_linesegment_and_point`), moduly matematyczne (`GrpMath.h`).
  - **Silnik FX / Renderowanie:** System efektow czasteczkowych (`CEffectManager::Instance()`), efekty ogona pociskow przylaczane bezposrednio `CFlyTrace` i wlasne systemy renderingu obiektow `CGraphicObjectInstance`.
  - **Obliczanie kolizji:** Obiekty menadzera culling (`CCullingManager`), sprawdzanie uderzen w potwory/tlo (`FCheckBackgroundDuringFlying`, `FCheckAnotherMonsterDuringFlying`). Zalezy rowniez od managera mapy w celu pobierania rzednych terenu (`CFlyingManager::Instance().GetMapManagerPtr()->GetTerrainHeight`).
  - **Zdarzenia zwrotne (Callbacks):** Wywolywanie funkcji interfejsu `IFlyEventHandler` przy trafieniach, poza dystansem itp. Zglaszanie trafien wroga poprzez `IFlyTargetableObject::OnShootDamage`.
- **Drzewo dyrektyw `#include`:** `Stdafx.h`, `EterLib/GrpMath.h`, `EffectLib/EffectManager.h`, `MapManager.h`, `FlyingData.h`, `FlyTrace.h`, `FlyingObjectManager.h`, `FlyTarget.h`, `FlyHandler.h`.
- **Ryzyko zaleznosci (Cyclic Deps):** Istnieja delikatne zaleznosci krzyzowe z klasami pochodnymi / managerem (np. wywolanie singletonu `CFlyingManager::Instance()` z wewnatrz), co utrudnia odizolowane testowanie samej klasy `CFlyingInstance`.
- **Model pamieciowy:** Dominuja czyste wskazniki (`CFlyingData *`, `IFlyEventHandler *`, `IActorInstance *`). Pola te sa powiazane jako referencje ("pozyczone") z zewnetrznymi komponentami nie bedacymi ich wlascicielami. Dynamiczna pula zarzadza instancjami samej klasy.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
| Nazwa Symbolu | Rola | Wielkosc/Zarzadzanie |
|---|---|---|
| `CFlyingInstance` | Silnik lotu pojedynczego pocisku; symuluje ruch, zderzenia z otoczeniem. | Watek: D3D/Glowny, Memory Pool: `CDynamicPool`. |
| `CFlyingInstance::TAttachEffectInstance` | Przechowuje informacje o dolaczonym efekcie (np. ogonku pocisku, iskry). | Dynamiczny kontener `std::vector` (okolo 12+ bytes struktura). |
| `FCheckBackgroundDuringFlying` | Funktor dla `CCullingManager` sluzacy do detekcji zderzen z otoczeniem (drzewa, woda itp). | Zmienne lokalne petli. |
| `FCheckAnotherMonsterDuringFlying` | Funktor do sprawdzenia zderzen z obiektami pobocznymi (np. inne potwory) poza glownym celem. | Zmienne lokalne petli. |

**Tabela Metod Publicznych:**
| Metoda | Sygnatura C++ | Opis i Efekty Uboczne |
|---|---|---|
| `Create` | `void Create(CFlyingData*, const D3DXVECTOR3&, const CFlyTarget&, bool)` | Inicjuje nowy obiekt lecacy uzywajac definicji (`CFlyingData`), ustala wektory startowe. |
| `Update` | `bool Update()` | Oblicza nastepny krok fizyki lotu, predkosc zalezy od FPS i timera. Wyzwala eksplozje w zderzeniu, zwraca false jesli lot sie zakonczyl. |
| `Render` | `void Render()` | Wywoluje rendering podpietych instancji ogona/efektow wizualnych. |
| `AdjustDirectionForHoming`| `void AdjustDirectionForHoming(const D3DXVECTOR3 & v3TargetPosition)` | Korekta kierunku do modulu homing (naprowadzanie rakiet). Oblicza kat z uzyciem maxAngle miedzy obrotem celowania. |
| `SetEventHandler` | `void SetEventHandler(IFlyEventHandler * pHandler)` | Ustawia odbiorce zdarzen dla tego konkretnego uderzenia pocisku. |

**Pamieciowy Layout Struktur:**
| Pola (Member Variables) | Znaczenie w FFI |
|---|---|
| `m_v3Position` (`D3DXVECTOR3`) | Biezaca globalna pozycja obiektu w swiecie gry. Bardzo latwo podpiac narzedzia FFI na to pole do manipulacji pociskiem. |
| `m_v3Velocity` (`D3DXVECTOR3`) | Wektor poruszania sie w tym tyku. Mozna z tego obliczac pozycje przyszla dla bota unikajacego strzal. |
| `m_FlyTarget` (`CFlyTarget`) | Hermetyzowany znacznik z informacja dokad ma doleciec. |
| `m_bAlive` (`bool`) | Flaga okreslajaca waznosc pocisku w danym Update frame. |
| `m_HittedObjectSet` (`std::set<IActorInstance *>`) | Przechowuje obiekty, z ktorymi nastapila juz interakcja (np. funkcja Pierce / wielokrotne uderzenia dzialaja poprawnie i unikaja dubli). |

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Modul ten dziala CALKOWICIE lokalnie w kliencie. Nie obsluguje bezposrednio opcodow pakietow. Pakiet uderzenia lub zakonczenia ruchu obsluguje logika instancji aktorow (`ActorInstance`), z ktorej strzaly/animacje uzywaja `CFlyingManager`.
- **Metody Pythona (`PyMethodDef`):** Brak bezposredniego C-API dla Pythona. Skrypty Python (`Resource.cpp`) bezposrednio dotykaja glownego singletona puli, w celu wyczyszczenia na starcie poprzez `CFlyingInstance::DestroySystem()`. System dziala cicho pod maska jako fragment logiki renderowania.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Funkcje `Update()` i `Render()` musza byc wolane w glownym watku, z uwagi na wykorzystywanie stalych wspoldzielonych globalnych, D3D rendering state (`CEffectManager`), singletonow timera oraz zewnetrznych bibliotek `CCullingManager`. Brak mutexow.
- **Zarzadzanie zasobami (RAII):** Brak nowoczesnych smart pointers! Zamiast tego zaimplementowana jest metoda recznego wylapywania zwolnien poprzez `Delete` z `CDynamicPool`. Obiekty z `CDynamicPool` nigdy nie sa kasowane z pamieci operacyjnej do konca aplikacji (jedynie resetowane jest ich miejsce i przelaczane na liste "dostepnych"). 
- **Potencjalne punkty awarii (Gotchas):** 
  - Ryzyko **Use-After-Free**: Jesli obiekty `IActorInstance` na ktore pocisk zostal zapiety (`m_pOwner` badz przez zestaw `m_HittedObjectSet`) zostana skasowane przez GameServer / Pakiety sieciowe zanim pocisk uderzy, proby operowania na nich poprzez funktor lub sprawdzania `TestCollisionWithDynamicSphere` moga poskutkowac Segmentation Fault. Podobny problem jest z pointerem na `IFlyEventHandler`.
  - Null Pointer Exception: Nalezy uwazac na sprawdzanie map managera `CFlyingManager::Instance().GetMapManagerPtr()`, ktory jest rzutowany warunkowo.
  - Wyjatki i matematyka: Detekcja kolizji wykorzystuje dzielenie przez wektory dlugosci blisko 0. System stara sie uchronic (`if (D3DXVec3LengthSq(&vv1) < 0.001f) return;`).

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Extending module):** 
  1. Zaplanuj nowe parametry dla pocisku (np. pocisk penetrujacy pancerz).
  2. Zmodyfikuj najpierw struktury danych przetrzymujace pre-definiowane wlasciwosci: `CFlyingData` (odczyt `.msm` w `FlyingData.cpp`). 
  3. Skrypt musi czytac nowa flage, nastepnie rozszerz `CFlyingInstance::__SetDataPointer` aby te flage podpiac do klasy lotu.
  4. Dodaj obsluge kolizji i zmodyfikowanej paraboli w `CFlyingInstance::Update()`.
  5. Dla eventow uszkodzen poszerz `IFlyEventHandler` i zaimplementuj w implementacji odbierajacej (zazwyczaj w klasie postaci - aktorze).
- **Jak debugowac i logowac:** Do przesledzenia pozycji mozna uzywac `Tracenf` z wartosciami wektorowymi. Wartym uwagi momentem do pulapki na bugi sa metody `__Explode` oraz modyfikacje `m_fRemainRange`.
- **Jak testowac (Headless Unit Testing):**
  - Zamockuj globalne instancje `CTimer`, `CEffectManager`, `CCullingManager` za pomoca wlasnych klas singletonowych zastapionych dla trybu testowego (flaga `-DTEST_MODE_DISABLE_STDAFX`).
  - Stworz atrapowy interfejs `IFlyTargetableObject` by z symulowac postac atakowanego na stale przypisanego pancerza/pozycji. 
  - Przekaz do zmockowanego celu obiekt, ustaw `Update()` i wymuszaj przyrost sekund recznie, zliczajac liczbe frame-ow dopoki `IsAlive()` nie wroci na false (zakonczony test trafienia).
