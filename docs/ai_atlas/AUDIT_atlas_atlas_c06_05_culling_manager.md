---
task_id: "atlas_c06_05_culling_manager"
cluster: "RND"
module_name: "CCullingManager - Globalny Menedzer Widocznosci Sceny"
target_files:
- src/EterLib/CullingManager.cpp
- src/EterLib/CullingManager.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c06_05_culling_manager.md"
architecture_layer: "Renderowanie, Direct3D 9, Shading i Culling"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul `CCullingManager` pelni funkcje globalnego menedzera widocznosci sceny w kliencie gry Metin2. Odpowiada za odrzucanie obiektow poza bryla widzenia (View Frustum Culling), bazujac na sferach otaczajacych (Bounding Spheres) obiektow graficznych, wykorzystujac do tego zewnetrzna biblioteke `SphereLib` (implementujaca strukture akceleracyjna `SpherePack`).
W cyklu petli gry, ten kod jest wywolywany glownie podczas aktualizacji klatki (`Update`/`Process`), przed finalnym wyrenderowaniem grafiki, by szybko zidentyfikowac, ktore obiekty przestrzenne znajduja sie w polu widzenia kamery (Frustum), a ktore z nich powinnismy ukryc.

**Przeplyw danych (Control Flow & Data Flow):**
1. Na etapie dzialania glownej petli gry, CCullingManager aktualizuje uklad macierzy widoku i rzutowania z D3D (`UpdateViewMatrix()`, `UpdateProjMatrix()`) i przebudowuje View Frustum kamery (`BuildViewFrustum()`).
2. Wywolywana jest funkcja `m_Factory->FrustumTest(GetFrustum(), this)`, ktora iteruje przez wewnetrzna strukture drzewiasta `SpherePack` z zewnetrznej biblioteki.
3. Kiedy stan widocznosci (ViewState) obiektu w `SphereLib` staje sie `VS_OUTSIDE` (poza polem widzenia), odpalany jest callback `VisibilityCallback`, ktory wywoluje `Hide()` (ukrycie) na rzutowanym przez `GetUserData()` obiekcie `CGraphicObjectInstance`. W przeciwnym razie wywolywane jest `Show()` (pokazanie obiektu).
4. Klasa umozliwia rowniez zapytania przestrzenne takie jak rzucanie promieni (`FindRay`, `RayTraceCallback`), poszukiwania w zasiegu promienia (sfera - `FindRange`, 2D Point Test - `PointTest2d`). 

**Cykl zycia obiektow:**
Obiekty graficzne rejestruja sie w menedzerze za pomoca metody `Register`, podajac srodek ciezkosci (center) oraz promien (radius) sfery okalajacej. Rejestracja konczy sie uzyskaniem uchwytu `CullingHandle` (`SpherePack*`), ktory pozniej mozna wykorzystac do wyrejestrowania obiektu z menedzera przez metode `Unregister`. Fabryka sfer (`SpherePackFactory`) alokowana jest w konstruktorze `CCullingManager`, i rezerwowana w pamieci na z gory zalozona wielkosc z parametrami: `10000` (maksimum), `6400` (root radius), `1600` (leaf radius) i `400` (extra radius), zwalniana bezposrednio w destruktorze (`delete m_Factory;`).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
**Zaleznosci wejsciowe (Inbound):** 
Inne systemy sceniczne (zarzadca sceny, przestrzenie, postacie i obiekty 3D klienta Metin2 - zazwyczaj `CMapOutdoor`, klasy kontrolerow modeli uzywajace `CGraphicObjectInstance`), ktore rejestruja i wyrejestrowuja instancje geometrii, rzucaja promienie w swiat (picking kursorem uzywajac `ForInRay`).

**Zaleznosci wyjsciowe (Outbound):** 
- `SphereLib` (`SpherePackFactory`, `SpherePack`, struktury: `Frustum`, `Vector3d`, callback `SpherePackCallback`): Biblioteka realizujaca matematyke przestrzenna, culling i przechowywanie wezlow.
- `GrpScreen.h` / `CScreen`: Baza dziedziczenia w `CCullingManager` (uzywana do uzyskania danych rzutowania, macierzy kamery).
- `CSingleton`: Modul zarzadzany jako singleton globalny.
- `CGraphicObjectInstance`: Klasa dla graficznej instancji rzutowana bezposrednio ze `GetUserData()`.
- Metody matematyczne widoku: `UpdateViewMatrix`, `UpdateProjMatrix`, `BuildViewFrustum`, `GetFrustum`.

**Drzewo dyrektyw `#include`:**
- `GrpScreen.h`
- `Eterbase/Singleton.h`
- `SphereLib/spherepack.h`
- W pliku .cpp: `StdAfx.h`, `CullingManager.h`, `GrpObjectInstance.h`
Ryzyka cyklicznych zaleznosci sa zlagodzone przez forward-declaration klasy `CGraphicObjectInstance` w pliku `.h`.

**Model pamieciowy:**
Glowna tablica uchwytow uzywa surowych wskaznikow (`SpherePack*` aliased to `CullingHandle` oraz `CGraphicObjectInstance *`), trzymanych na wektorze `std::vector<CGraphicObjectInstance *> TRangeList`. Brak uzycia inteligentnych wskaznikow ze wzgledow na wydajnosc. Zaleznosc instancji graficznej do struktury w `SphereLib` jest implementowana jako wskaznik w `UserData` polimorficznej reprezentacji w bibliotece cullingowej (nie uzywa shared_ptr, wlasciciel obiektu ma pilnowac wyrejestrowania podczas niszczenia).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
- `RangeTester<T>`: Struktura dziedziczaca z `SpherePackCallback`, pelniaca role tensora do funkcji wyszukiwania (RayTrace/Range), uzywana wraz z delegatem funkcyjnym `T* f`. Alokowana na stosie na potrzeby pojedynczego rzucenia.
- `CCullingManager`: Glowna klasa, Singleton, dziedziczy publicznie po `SpherePackCallback` oraz prywatnie z `CScreen`. Kontroluje alokacje drzewiastej struktury `SpherePackFactory` na stercie wewnatrz glownego watku D3D.

**Tabela Metod Publicznych CCullingManager:**
- `void RayTraceCallback(Vector3d, Vector3d, float, Vector3d, SpherePack*)`: Sprawdza odleglosc wyznaczonego elementu i pakuje do `m_list`. Uzywa skutkow ubocznych.
- `void VisibilityCallback(Frustum, SpherePack*, ViewState)`: Odbiera sygnaly o widocznosci, zrzutuje UserData na `CGraphicObjectInstance*` i wola funkcje wyjscia/wejscia Frustum: `Hide()` lub `Show()`. Warunek: Nie wywolywac poza zdefiniowanym drzewem sceny.
- `void RangeTestCallback(Vector3d, float, SpherePack*, ViewState)`: Buduje wektor obiektow `m_list` znajdujacych sie w zasiegu obszarowym.
- `void Reset()`: Zeruje `SpherePackFactory`.
- `void Process()`: Przebudowuje ViewMatrix i rzutuje `m_Factory->FrustumTest`. Glowna funkcja cullingu podpinana w render loop.
- `void FindRange(...) / FindRay(...) / FindRayDistance(...)`: Rezerwuja `m_list` z uzyciem factory.
- Szablony (Templates): `ForInRange2d`, `ForInRange`, `ForInRay`, `ForInRayDistance`. Tworza instrukcje struktury `RangeTester<T>` na stosie i wstrzykuja w metody factory.
- `CullingHandle Register(CGraphicObjectInstance * ob)`: Wrzuca obiekt na drzewo cullingowe, wyciagajac najpierw `obj->GetBoundingSphere`.
- `void Unregister(CullingHandle h)`: Wykresla CullingHandle z factory.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
`CCullingManager`: Dziedziczy po 3 klasach. Glowna wielkosc sklada sie z rozmiaru baz, i wlasnych member fields:
- `TRangeList m_list;` // (std::vector - size=24)
- `float m_RayFarDistance;` // (size=4)
- `SpherePackFactory * m_Factory;` // (size=8)
Brak V-Table od siebie poza dziedziczonym SpherePackCallback. Metody callback sa `virtual`. FFI Hook na VirtualTable `SpherePackCallback` mozliwe w celu sledzenia pakietow rzutowanych instancji.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
Modul CCullingManager jest niskopoziomowa abstrakcja rzutowania geometrii 3D na View Frustum powiazanym bezposrednio z GPU. 
- Brak zaleznosci od protokolow sieciowych. Pakiety CG i GC nie siegaja tego poziomu (jest wywolywane z warstwy silnika `CGraphicObjectInstance`).
- Python C-API nie dotyka bezposrednio `CCullingManager`, poniewaz zarzadzanie instancjami lezy po stronie `CMapOutdoor`/`CInstanceBase` wywolywanego przez API wyzszego poziomu (np. moduly aplikacji CPythonBackground).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Funkcje modyfikujace kolekcje w `CCullingManager` oraz aktualizujace stany instancji (metody klas renderingu `UpdateViewMatrix()`) oraz wywolanie `Hide()` i `Show()` na instancjach, MUSZA byc uruchamiane z watku glownego silnika (watek D3D). Aktualizacja macierzy lub manipulacja `CGraphicObjectInstance` na zlym watku spowoduje niespojnosci i hard-crashe D3D (D3DERR_INVALIDCALL).
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Rejestrowanie z NULL `CGraphicObjectInstance*` powoduje `assert(obj)` crash w Trybie Debug (lub segfault na wyciaganiu promienia).
  - Wyrejestrowanie obiektu, bez wczesniejszego usuniecia go w `SphereLib` poprzez poprawne zachowanie wlascicielstwa uchwytu `CullingHandle` grozi pojawieniem sie "dangling pointers" i rzutowania go pozniej np. na VisibilityCallback. Obiekt zawsze powienien sie wyrejestrowac przed zniszczeniem (lub podczas wczesnego zwolnienia zasobow z puli).
  - Modyfikowanie `m_list` w trakcie obslugi eventow (`SpherePackCallback`) moze zdestabilizowac liste poniewaz `m_list.clear()` zeruje w wezlach korzenia funkcji np. `FindRay`.
- **Zarzadzanie zasobami (RAII):** Fabryka `m_Factory` jest tworzona w konstruktorze. Klienci musza przechowywac wygenerowany token `CullingHandle` po rejestracji az do samego dealokowania by zapobiec memory leaks starych wezlow geometrii w drzewie rzutowania.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zaplanuj deklaracje np. nowej funkcji przecietych obszarow 3D w `src/EterLib/CullingManager.h`.
  2. Zaaplikuj szablon typu `template <class T> ForInArea(...)` bazujac na `RangeTester<T>`.
  3. Upewnij sie, ze funkcja przekazuje zadanie do dedykowanego callbacka `RangeTester::TwojCallback`.
  4. Nie alokuj obiektow na stercie (uzyj istniejacych stalych na wektorze `m_list`).
- **Jak debugowac i logowac:**
  - Odkomentuj makro `#define COUNT_SHOWING_SPHERE` by analizowac ilosci widocznych elementow w logu (uzywa Tracef, uwaga: generuje ekstremalny spam dla silnika w Trybie RelWithDebInfo). 
  - W klasie wewnetrznej `RangeTester` istnieje wbudowany mechanizm testowania `SPHERELIB_STRICT`, ktory wywoluje funkcje `puts()` dla ulatwienia testow.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  Zmockowac interfejs `CGraphicObjectInstance` udostepniajac atrape metody `GetBoundingSphere()`, `Show()`, `Hide()` w `mock_include/GrpObjectInstance.h` oraz `Frustum` dla struktury kamery. Zaprzac mockowanie do zewnetrznego biblioteki `SpherePackFactory` w srodowisku CTest i iterowac widocznosc sztucznie zadeklarowanych uchwytow (CullingHandle) przy stalych pozycjach kamery we wnetrzu wirtualnej przestrzeni euklidesowej, wyzwalajac recznie zdarzenie `Process()`.
