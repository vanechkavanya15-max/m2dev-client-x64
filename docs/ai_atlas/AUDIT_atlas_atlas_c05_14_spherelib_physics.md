---
task_id: "atlas_c05_14_spherelib_physics"
cluster: "WLD"
module_name: "SphereLib - Hierarchiczne Drzewo Sfer Zderzeniowych i Frustum"
target_files:
- src/SphereLib/sphere.cpp
- src/SphereLib/spherepack.cpp
- src/SphereLib/frustum.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c05_14_spherelib_physics.md"
architecture_layer: "Swiat Gry, Teren, Kolizje i Nawigacja"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

**Funkcja modulu:** `SphereLib` jest podsystemem matematyczno-fizycznym (autorstwa Johna W. Ratcliffa), odpowiedzialnym za reprezentacje sfer kolizyjnych (Bounding Spheres), zarzadzanie pakietowaniem (hierarchiczne drzewa sfer - Bounding Sphere Trees / Sphere Trees), dynamiczne balansowanie tych drzew oraz testowanie kolizji/widocznosci z ostroslupem widzenia (Frustum Culling), prmieniami (Ray Tracing) oraz odleglosciami (Range Testing). W kliencie Metin2 sluzy bezposrednio menedzerowi culling'u (`CCullingManager` w `EterLib`) do odrzucania niewidocznych obiektow (CGraphicObjectInstance) i optymalizacji renderowania 3D.

**Wywolywanie:** Drzewo sfer (SphereTree/SpherePack) jest aktualizowane ("integrate" / "recompute") podczas petli gry poprzez `CCullingManager::Update()`. Przetwarzanie i sprawdzanie widocznosci odbywa sie glownie w fazie renderowania (`CCullingManager::Process()`), ktory wywoluje `m_Factory->FrustumTest(...)`. 

**Przeplyw danych (Data & Control Flow):**
1. Silnik gry (Game Client) dodaje nowy obiekt graficzny wywolujac `CCullingManager::Register`. Pobierana jest z niego wstepna Bounding Sphere (srodek i promien obiektu).
2. `SpherePackFactory` przydziela instancje sfery uzywajac wlasnej puli pamieci (template `Pool<SpherePack>`), klasyfikujac ja i umieszczajac na liscie do integracji.
3. Obiekty trafiaja na kolejki FIFO (`mIntegrate`, `mRecompute`). Metoda `Process()` uaktualnia drzewa hierarchiczne korzeni.
4. Przed rysowaniem, `CCullingManager::Process()` buduje nowy ostroslup (Frustum) i wola wyliczenia uzywajac callbackow (`SpherePackCallback`).
5. Drzewo jest rekursywnie przeszukiwane pod katem zawierania sie sfer we Frustum (`ViewState`: `VS_OUTSIDE`, `VS_INSIDE`, `VS_PARTIAL`).
6. Kiedy zauwazona jest zmiana stanu zderzenia, nastepuje wywolanie `VisibilityCallback`, ktory na podstawie polaczonego `mUserData` (`CGraphicObjectInstance*`) wlacza (Show) lub wylacza (Hide) widocznosc i odciaza GPU.

**Cykl zycia obiektow (Lifecycle):**
- **Alokacja:** Obiekty naleza globalnie do glownej fabryki, alokowane za pomoca z gory zdefiniowanych, hermetycznych pul (`Pool`). Elementy pakietowania nie wywoluja alokatora C++ (`new`) podczas pracy gry.
- **Inicjalizacja:** CullingManager zaklada fabryke podajac twarde limity: ilosc max obiektow (np. 10000), promien root (6400), promien leaf (1600).
- **Reset/Reintegracja:** Sfery moga byc przepinane, modyfikowane i umieszczane do reobliczen na liscie recompute (FIFO buffers).
- **Dealokacja:** Niszczenie zwalnia pamiec hurtowo usunienciem fabryki i pul `Pool`, bez ryzyka wyciekow poszczegolnych wezlow.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
- `EterLib/CullingManager.cpp` oraz `.h`: Zarzadza i hermetyzuje `SpherePackFactory` oraz definiuje zaleznosci miedzy systemem gry a hierarchia culling'u.
- `EterLib/GrpObjectInstance`: Sluzy jak dane bezposredniego wezla graficznego obslugiwanego na callbackach (Hide()/Show()). Posiada id / wskaznik typu `CullingHandle`.
- Inne testy sferyczne, jak `RangeTest` i `PointTest2d`, zlecane przez mapy lub interface (UI/Teren).

**Zaleznosci wyjsciowe (Outbound):**
- System DirectX 9 (`D3DXVECTOR3`, `D3DXMATRIX`, `D3DXPLANE`) uzywany do definicji plaszczyzn kamery, a `Vector3d` bezposrednio jest jego podklasa.
- Standardowa biblioteka wektorow i math C++.

**Drzewo dyrektyw `#include`:**
- `sphere.h`: Wymaga `"vector.h"`.
- `frustum.h`: Wymaga `"vector.h"`.
- `spherepack.h`: Wymaga `<assert.h>`, `"vector.h"`, `"pool.h"`, `"sphere.h"`, `"frustum.h"`.
- Brak zaleznosci cyklicznych; EterLib includuje SphereLib, lecz SphereLib nie wie nic (statycznie) o kodzie gry oprocz uzytkowania wskaznikow polimorficznych / pustych danych (`mUserData`).

**Model pamieciowy:**
- Czyste wskazniki w obrebie list jednokierunkowych lub obukierunkowych zawartych w polach struktury i ciaglej alokacji Pool. Bardzo wysoka optymalizacja Cache (Data Locality). Callback oparty jest na VTable i posylowany przez pointer instancji.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur:**
- `Vector3d` (dziedziczy `D3DXVECTOR3`): Glowny typ do zapisu wymiarow 3D (X, Y, Z).
- `Frustum`: Definiuje ostroslup (6 plaszczyzn w `m_plane[6]`). Posiada zmienna wspierajaca wczesne sprawdzanie sfer `m_bUsingSphere`.
- `Sphere`: Baza reprezentujaca mCenter, mRadius, mRadius2 (promien do kwadratu do szybkich operacji).
- `SpherePackCallback`: Interfejs sluzacy integracji klienta. (`VisibilityCallback`, `RayTraceCallback`, `RangeTestCallback`, `PointTest2dCallback`).
- `SpherePack` (dziedziczy po `Sphere`): Reprezentuje konkretny wezel na drzewie. Przechowuje hierarchie i dane dla powiazania: `mNext`, `mPrevious`, `mParent`, `mNextSibling`, `mPrevSibling`, `mChildren`, uzywa `mUserData` do przechowania CGraphicObjectInstance.
- `SpherePackFifo`: Standardowy stos oparty na Ring Buffer dla kolejek sfer.
- `SpherePackFactory`: Glowny kontroler operujacy na `Pool<SpherePack>`, umozliwiajacy przeszukania i testy, steruje korzeniami sfer (drzewem root i leaf).
- `Pool<Type>` (`pool.h`): Uniwersalny Pool Allocator.

**Tabela Metod Publicznych:**
- `Sphere::Compute(const SphereInterface &source)` -> Implementuje algorytm Bounding Sphere (Jack Ritter z Graphics Gems).
- `Frustum::BuildViewFrustum(D3DXMATRIX & mat)` -> Wylicza i normalizuje plaszczyzny Frustum na podstawie View-Proj Matrix D3D.
- `Frustum::ViewVolumeTest(const Vector3d &c_v3Center,const float c_fRadius)` -> Analizuje widocznosc prosta metoda odleglosci wektorowych od center z promieniami do kazdej z 6 plaszczyzn (zwraca kluczowy stan enum `ViewState`: INSIDE/PARTIAL/OUTSIDE).
- `SpherePackFactory::Process()` -> Sprawdza ilosc oczekujacych zmian alokacji/realokacji sfer dla zachowania prawidlowego i szybkiego zrownowazonego wezla rodzica i korzenia (Integrate & Recompute w drzewie).
- `SpherePackFactory::FrustumTest(...)` -> Punkt kontrolny renderingu. Odpala test drzew wezlow (zarowno od korzeni sfer nadrzednych, badajac cala zawartosc, na lisciach koncowych konczac) by okreslic status wywolania callbacka widocznosci do CullingManagera.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
- Klasa `SpherePack` wymaga sporego rezerwy bajtow: oprocz pozycji wektora i zmniennych promienia `Sphere`, trzyma okolo 6 wskaznikow do innych sfer (dzieci, rodzice, sasiadujace sfery) plus wlasny stan flag. 
- Tablica pod `mSpheres` wewnatrz glownej fabryki to skondensowany blok, idealny cel FFI czy bota rysujacego radar (wystarczy rzutowanie `mUserData` pod adres wskazujacy `CGraphicObjectInstance`).

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

- **Pakiety Sieciowe:** Modul operuje calkowicie w obrebie klienta jako silnik pomocniczy przestrzeni i renderingu. Nie posiada bezposrednich pakietow sieciowych (CG/GC). 
- **Python C-API:** Brak bezposredniego narazenia struktury na skrypty Pythona. Dziala calkowicie od strony wezlow C++, niewidzialny dla Pythona, za wyjatkiem ewentualnych funkcji silnika odrzucania obiektow w renderingu zewnetrznym (UI nie zarzadza cullingiem na powaznie). Brak dyrektyw `PyMethodDef`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Zasady wielowatkowosci:** Modul nie uzywa zadnych blokad (Mutex/Spinlock). Metody puli `Pool`, oraz caly mechanizm `Process` w fabryce, dzialaja poprawnie wylacznie, gdy wywolywane glownie przez Glowny Watek (Direct3D Rendering Thread). Implementacja watkow asynchronicznych podczas updatow pozycji obiektow calkowicie zepsuje `Pool<SpherePack>` pointer chains.
- **Limit Puli Sfer (Buffer Size Overflow):** Domyslnie konstruktor tworzy ustalone maksumum obiektow (np 10000). Jesli ilosc wywolan AddSphere (dodanych do swiata wrogow, modeli) przekroczy to, `Pool` i `SpherePackFifo` zawiedzie. Wymaga to bacznego kontrolowania liczebnosci obiektow lub wdrozenia realokacji uzytkowej. 
- **Gotchas Callbackow:** Klasy nasluchujace (np. `CCullingManager::VisibilityCallback`) otrzymuja event wylacznie w momencie ZMIANY stanu `ViewState` (z np. `VS_OUTSIDE` na `VS_INSIDE`), nie dostaja go co klatke dla tego samego stanu. AI piszace podsystem nie moze zakladac powtarzalnego tykania wlasciwosci widocznosci na callbacku.
- **Wyciek (Memory Leaks) a UserData:** System nie jest wlascicielem zycia samego `CGraphicObjectInstance` ukrytego pod `mUserData`. Zniszczenie instancji graficznej musi manualnie wyrejestrowac (Unregister) sfere, by nie trzymala wiszacego (dangling) pointera i nie wykonala callbacka po zniszczeniu RAM.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zmodyfikuj typ stanow, by zwrocil szukany zakres lub rodzaj zderzenia (np. dodanie `VS_OVERLAP`).
  2. Dostosuj plik `spherepack.h` o nowe flargi konfiguracyjne bitowe w enumeratorze `SpherePackFlag` by przechwac dodatkowy stan bez alokacji bitu wiecej struktury `SpherePack`.
  3. Zaimplementuj rozwiniecia w interfejsie `SpherePackCallback` (w `spherepack.h`) dodajac nowa wirtualna metode sluchacza jesli to nowa cecha. Zaimplementuj cialo sluchacza w EterLib (`CCullingManager.cpp`).
  4. W `Frustum::ViewVolumeTest` dopisz swoja matme do nowo rozszerzonego detektora widocznosci.

- **Jak debugowac i logowac:**
  - Pliki te maja specjalna kompilacyjna flage `#ifdef SPHERELIB_STRICT`. Wlaczenie jej wrzuci logiki typu `puts("CCullingManager::VisibilityCallback")` umozliwiajace weryfikacje spamujacego drzewa zderzen (odradzane przy duzej ilosci wezlow).
  - W razie wycieku/crasha badaj offset struktury Pool: `mCurrent`, `mHead`, oraz `mFree`. Zwykle usuniety obiekt w srodku dzialania usunie polaczenie z wezlem `mPrevious`/`mNext`.

- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  - Modul nie ma zadnych twardych wymogow na powolany wczesniej Windows czy IDirect3DDevice9 (mimo includow naglowkowych od D3D).
  - Stworz prosty CTest: Zainicjalizuj `SpherePackFactory`, dodaj mock class `DummyCullCallback : public SpherePackCallback`, recznie wgraj matryce jedynkowa dla okna widocznosci do klasy `Frustum` i zlec `AddSphere_`, po czym wywolaj `Process` - upewnij sie ze zglasza test na wlasciwym obiekcie z wlasciwymi wektorami.
