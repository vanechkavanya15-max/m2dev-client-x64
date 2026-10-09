---
task_id: "atlas_c02_10_actor_instance_collision"
cluster: "ACT"
module_name: "CActorInstance - Kolizje Bytow i Raycast Kursora"
target_files:
- src/GameLib/ActorInstanceCollisionDetection.cpp
- src/UserInterface/InstanceControllers/InstanceCollisionImpl.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c02_10_actor_instance_collision.md"
architecture_layer: "Maszyna Stanow Postaci i Aktorzy"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

**Funkcja w architekturze klienta:**
Modul zderzen bytow i pickingu (ActorInstanceCollision) stanowi pomost miedzy silnikiem fizyki/kolizji (EterLib/EterGrnLib) a wizualna reprezentacja postaci (CActorInstance / CInstanceBase) w grze. Odpowiada on glownie za testowanie zderzen pomiedzy aktorami na mapie (atak, ruch) oraz umozliwia systemom interfejsu i kamery dokladne wyliczanie przeciec z trojwymiarowymi sferami defensywnymi/atakujacymi (picking promieniem - Raycast). Zderzenia moga dotyczec dwoch glownych grup stref: sfer "ciala" (BodyPointInstanceList) - do weryfikacji kolizji przestrzennych miedzy aktorami w celu np. blokowania ruchu oraz sfer "obronnych/atakujacych" (DefendingPointInstanceList) - do testowania obrazen, atakow broni/magii oraz precyzyjnego ucelowania (targetingu) kursorem myszy. Modul integruje sie bezposrednio z mechanika omijania obiektow (AvoidObject / IsBlockObject). Zauwazono jednak w kodzie brak docelowego pliku `src/UserInterface/InstanceControllers/InstanceCollisionImpl.cpp` (kod ten historycznie, badz docelowo znajdowal sie w plikach takich jak `InstanceBase.cpp` oraz powiazanych), wiec raport dotyczy istniejacych mechanik w `ActorInstanceCollisionDetection.cpp` i polaczonych metod pickingu m.in w `InstanceBase` i `ActorInstance`.

**Miejsce wywolania:**
Logika sprawdzania zderzen i pickingu jest wywolywana asynchronicznie (w ramach glownego watku D3D9 i symulacji), zazwyczaj w fazie `OnUpdate()` (dla przemieszczen) oraz czesto przed sama faza renderingu `OnRender()` (szczegolnie do podswietlania obiektow i oznaczania kursora - system CPythonCharacterManager iteruje by sprawdzic przeciecia bounding boxow i sfer z uzyciem promieni np. w systemach `CharacterPicker`). Wykrywanie ataku (np. podczas ataku normalnego badz skilla) wystepuje przy odswiezaniu stanow petli animacji (podczas aktualizacji klatki animacyjnej Granny).

**Przeplyw Danych (Data Flow & Control Flow):**
1. System interfejsu (lub skrypty w Pythonie) odpytuja o zderzenia poprzez metody takie jak `IntersectDefendingSphere` czy `IntersectBoundingBox` przekazujac globalnie utrzymywany promiec `ms_Ray` (kierunek, polozenie).
2. Nastepuje iteracja po listach obwiedni kolizyjnych przypisanych do kosci w danym modelu postaci (struktury `CDynamicSphereInstanceVector`).
3. Wykorzystywana jest matematyka wektorowa (DX9, wektory odleglosci, iloczyny skalarne - `D3DXVec3Dot`, metoda rownan kwadratowych dla sfery) by zbadac czy promiec kamery przebija konkretna obwiednie postaci - co decyduje np. o zaznaczeniu celu.
4. Gdy zachodzi weryfikacja kolizji ruchomej (`TestActorCollision`, `TestPhysicsBlendingCollision`), sfery aktualnej instancji poddawane sa testom z uzyciem interpolacji po czasie miedzy wektorami `v3LastPosition` a `v3Position` obu badanych instancji w podzialce 50 krokow czasowych (`nSubCheckCount`), badajac m.in czy zderzenie wystapilo i czy obiekty aktualnie sie zblizaja.

**Cykl Zycia Obiektow (Lifecycle):**
Sfery (jako zagniezdzone instancje `CDynamicSphereInstance`) inicjalizowane sa m.in na etapie ladowania modelu (w `UpdatePointInstance`). Sfery sa mapowane na kosci (bone matrix) przez API Granny (`GrannyGetWorldPose4x4`), alokowane jako wektory w pule pamieci danego `CActorInstance` i niszczone wylacznie, gdy zniszczeniu ulega sama instancja bohatera (dealokacja wektora obwiedni przy usuwaniu `CGraphicThingInstance` lub `CActorInstance`).

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
- **Klasy i Moduly:** 
  - `CInstanceBase`, `CPythonCharacterManager`, `CharacterPicker` - te byty z interfejsu wywoluja metody przeciagania `IntersectDefendingSphere()`, `IntersectBoundingBox()` oraz m.in `TestPhysicsBlendingCollision()` podczas pickingu mysza i aktualizacji klatek.
  - Skrypty Pythona poprzez warstwe interfejsu gry moga uderzac do API powodujac testy np. dla eventow zderzeniowych (atak combo, celowanie tarczy).
  - Klasy ruchowe jak np. `CActorInstance` (sam w sobie) z metod `__SplashAttackProcess` oraz wlasciwych petli atakow broni.
- **Wydarzenia:** 
  - Mouse Click/Hover - raycast generowany przez kursor na plaszczyznie widoku.
  - Pakiety odswiezania ataku (wymuszenie weryfikacji w OnUpdate() na podstawie `MotionEvent`).

**Zaleznosci wyjsciowe (Outbound):**
- **DirectX 9 API:** D3DXVec3LengthSq, D3DXVec3Dot, D3DXVec3Lerp, operacje na promieniach (`CRay`), D3DXVECTOR3.
- **EterLib/EterGrnLib:** Systemy takie jak kolizje statyczne/dynamiczne (np. `DetectCollisionDynamicSphereVSDynamicSphere`), API `CDynamicSphereInstance`, API Granny (pobieranie macierzy kosci `GetCompositeBoneMatrixPointer`, alokacje sterty).
- Wlasne byty wspierajace jak `CMapOutdoor` do sprawdzania pickingu po terenie i wyliczania promienia (`GetPickingPointWithRayOnlyTerrain`).

**Drzewo dyrektyw `#include` i Ryzyka:**
- `<StdAfx.h>`
- `"EterLib/GrpMath.h"`
- `"ActorInstance.h"`
- (Oraz ewentualnie system logowania jezeli modyfikowane)
Ryzykiem w pliku kolizji i raycastingu jest naduzycie cyklicznych naglowkow w razie bezposredniej zaleznosci od `CPythonCharacterManager` - kod kolizji utrzymany jako domenowy w `GameLib/EterGrnLib` zapobiega silnemu wiazaniu, jednak przenikanie funkcji np. do modulu UserInterface wymaga abstrakcji.

**Model pamieciowy:**
Glownie obiekty w czystych wskaznikach (C-pointers). Wezly sfer zarzadzane w `std::vector` jako obiekty wartosciowe i struktury list (np. `std::list<TCollisionPointInstance>`). Raycasting uzywa globalnego/statycznego promienia `ms_Ray` lub instancji na stosie jako ref.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur:**
- `CActorInstance` - Glowna klasa menedzera postaci w przestrzeni. Przechowuje logike `IntersectDefendingSphere` i testy cial. Wlasciciel watku D3D.
- `TCollisionPointInstance` - Struktura wezla. Obejmuje wskaznik do stalych danych o zderzeniach z danych rasy postaci, flagi (czy dolaczone), indeks modelu, indeks kosci i wektor sfer (`CDynamicSphereInstanceVector`).
- `CDynamicSphereInstance` - Przechowuje promiec (fRadius), pozycje z ostatniej klatki (`v3LastPosition`) i pozycje aktualna (`v3Position`), w celu symulowania interpolacji drogi sfery pomiedzy dwoma klatkami.
- `CDynamicSphereInstanceVector` - `std::vector<CDynamicSphereInstance>`.
- `TCollisionPointInstanceList` - `std::list<TCollisionPointInstance>`.

**Tabela Metod Publicznych i Krytycznych:**
- `bool CActorInstance::TestPhysicsBlendingCollision(CActorInstance & rVictim)`:
  - Argumenty: Referencja do innej instancji aktora. Zwraca: `bool`. 
  - Wykonuje: Test zderzenia w trybie interpolacji miedzy klatkami, probkujac 50 razy odcinek delta aby uniknac bledu przelatywania pocisku/aktorow ("tunneling effect").
- `bool CActorInstance::TestActorCollision(CActorInstance & rVictim)`:
  - Argumenty: `CActorInstance & rVictim`. Zwraca: `bool`.
  - Wykonuje: Szybki test miedzy dwoma listami instancji sferycznych bez zaawansowanego blendingu, z zabezpieczeniem dlugosci promienia max 800 jednostek.
- `bool CActorInstance::IntersectDefendingSphere()`:
  - Zwraca: `bool` - Prawda jesli globalny promien (`ms_Ray`) przebija jakakolwiek obronna/ataku sfere (raycast pick).
  - Skutki: Umozliwia zakotwiczenie kursora myszy do ataku badz podswietlenie celu.
- `BOOL CActorInstance::__TestObjectCollision(const CGraphicObjectInstance * c_pObjectInstance)`:
  - Argumenty: Wskaznik na graficzny obiekt tla / environment.
  - Wykonuje: Weryfikacja zderzenia w srodowisku (budynki, sciany, omijanie - block movement).
- `void CActorInstance::UpdatePointInstance(TCollisionPointInstance * pPointInstance)`:
  - Argumenty: Wskaznik do sfery w celu odswiezenia klatki transformacji kosci modelu Granny 3D na plaszczyzne D3D i na poszczegolne strefy uderzen.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
`CDynamicSphereInstance` jest kluczowa struktura do narzedzi. Jest ciasno pakowana z trzema polami D3DXVECTOR3 (po 12 bajtow) plus promiec float (4 bajty), zaleznie od wyrownania. W sferze FFI wazne sa wczytywania offsetow w kolejkach Granny Bone Matrix. FFI powinno iterowac bezposrednio `std::list` uzywajac interfejsow getterow `CActorInstance`, by uniknac naruszenia offsetu kompilatora.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

**Pakiety Sieciowe:**
- Same pakiety sieciowe nie steruja bezposrednio promieniami zderzeniowymi klienta - pakiety np. `HEADER_GC_CHARACTER_POSITION` tylko zmieniaja transformacje macierzy swiata aktora, co uaktualnia sfery zderzeniowe pod radarem Granny poprzez system update (odswiezenie `m_worldMatrix`). Ewentualne ataki odpalane pakietem generuja update sfer. 
- Moga istniec pakiety odpowiedzialne za sync ataku wymuszajac na logice iteracje sfer (np. synchronizacje combo).

**Metody Pythona (PyMethodDef):**
Ten bezposredni kod dziala w C++, jednak wynik raycastu jest transportowany do pythona via metody `playerGetTargetVID` badz metody zaznaczania obiektu mysza:
- np. `playerSetTarget(vid)` jako rekreacja wyniku.
- np. metody w modulu background do pickingu podloza (czyli modulu Raycast `backgroundGetPickingPoint` - choc ten uzywa raycastu po terenie `CMapOutdoor`). Do raycastu aktora sluzy instancja zarzadcy postaci.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

**Zasady wielowatkowosci:**
Modul jest glownie **jednowatkowy** w swoim przebiegu modyfikacji i testu zderzenia (dziala z reguly na glownym watku symulacji `MainThread` lub logicznym). Proba ingerowania z watku pobierania danych sieciowych by zablokowac zderzenie spowoduje Data Race ze statusem animacji w Granny 3D.
  
**Potencjalne punkty awarii (Crash Points & Edge Cases):**
- **Nullowe kosci (Missing Bones):** Wywolanie zlej macierzy z kosci badz `GetBoneMatrixPointer` na brakujacym wezle skutkuje crashem. System broni sie flagami ale czeste sa wycieki badz desync dla niezgodnych modeli gr2 (np. brak dummy bone do attachmentu stref ataku).
- **Tunneling (Przebijanie scian):** Jesli FPS jest bardzo niski (ogromny deltaTime), postac moze przeskoczyc sferami inna postac badz budynek miedzy testami. Uzywana jest czesciowo funkcja `TestPhysicsBlendingCollision` majaca probkowanie (50 tickow w przod) by zwalczyc ten problem dla uderzen z reki/miecza, jednak z optymalizacja `fDistanceSq > 800.0f*800.0f` przed sprawdzeniem by oszczedzic CPU. W przypadku wiekszych predkosci movementu niz wymiary sfery jest to narazone na tunneling.
  
**Zarzadzanie zasobami (RAII):**
Dynamiczne listy jak `m_DefendingPointInstanceList` alokuja wielokrotne narzuty wezlow na stercie w petli logiki stalej - sa odnawiane i iterowane (cache-unfriendly std::list, co jest znanym watelgardlem Metin2). Zaleca sie ewentualne przepisywanie list na plaskie wektory i unikanie usun-stworz w srodku walki.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
Przykladowo, implementacja nowej sfery oslaniajacej (tarczy odbijajacej magie) lub boxu AABB na koncu promienia:
1. Skopiuj/przepisz logike sprawdzania promienia do nowej funkcji w `CActorInstance` (podobnie jak `IntersectDefendingSphere`).
2. Zdefiniuj nowa metode w naglowku i wstrzyknij ja do petli interfejsu klienta pickingu (np. `CPythonCharacterManager`).
3. Jezeli dodajesz nowy typ ksztaltu np. Box w raycastingu, zmodyfikuj EterLib (`CollisionData.h` + zrodlo) wdrazajac detekcje promien-AABB (`IntersectBoundingBox`), gdyz aktualnie kod czesto sprowadza pudla do OBB box z api modelu lub ray-sphere z matematyki.

**Jak debugowac i logowac:**
Z uwagi na hot-path (wywolywane kilkadziesiat tysiecy razy na klatke przy setkach potworow), unikaj wstawiania funkcji `Tracef` czy `Log` w glebi logiki interpolacji. Zrob to na brzegu metody w razie trafienia z flagami `static bool test_trigger`. Zobacz klatke macierzy np. w `UpdatePointInstance`, aby wykluczyc zerowe macierze - mozna to renderowac (wywolanie opcji debug UI render collision box - w `CPythonCharacterManager::RenderCollision`).

**Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
Mockuj `CDynamicSphereInstance` poprzez test w GoogleTest / Doctest. Przygotuj prosta instalacje `D3DXVECTOR3` na pozycje stala + zasymuluj wczytanie ray`a z `ms_Ray` recznie (np. poprzez uzycie instancji statycznej `CRay`). Sprawdz czy twoje rownanie `b*b - c >= 0` prawidlowo znajduje srodki zderzen przy zadanym float fRadius dla raycastingu. Nie zapomnij zbudowac atrapy macierzy kostnych Granny, jezeli potrzebujesz testowac test aktualizacji klatkowej.
