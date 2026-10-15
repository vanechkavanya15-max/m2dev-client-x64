---
task_id: "atlas_c07_02_granny_instance"
cluster: "MOD"
module_name: "CGrannyModelInstance - Deformacja Szkieletowa (Skinning)"
target_files:
- src/EterGrnLib/ModelInstance.cpp
- src/EterGrnLib/ModelInstance.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c07_02_granny_instance.md"
architecture_layer: "Modele 3D, Szkielety Granny, Drzewa i Efekty"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport Audytu Modulu: CGrannyModelInstance

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Modul `CGrannyModelInstance` odpowiada za instancjonowanie i renderowanie pojedynczych modeli 3D w swiecie gry bazujacych na bibliotece Granny 3D. Przechowuje on stan konkretnego bytu 3D (np. postaci, potwora, broni), w tym jego animacje (ruchy), stan szkieletu i deformacje wierzcholkow na podstawie aktualnej klatki animacji (Skinning). 

Podczas petli gry, ten kod jest aktywnie wykorzystywany zarowno w fazie aktualizacji `OnUpdate` (obliczanie uplywu czasu, zmian macierzy i koordynat), jak i fazie renderowania `OnRender` (przesylanie znieksztalconych siatek do modulu renderujacego DirectX). 

Przeplyw Danych (Data Flow) i Control Flow:
1. Inicjalizacja: tworzony przez obiekty w grze (np. managera postaci), model statyczny przypisany do instancji (odwolanie do `CGrannyModel`).
2. Update Czasowy: zmiana `m_fLocalTime` (czas trwania animacji) i zaktualizowanie szkieletu (Bone Evaluation) przez biblioteke Granny (`UpdateSkeleton`).
3. Transformacje: macierze w swiecie, koordynaty mesh'y obliczane sa i zapisywane z uzyciem tzw. CPU Double-Buffering dla optymalizacji i wielowatkowosci (np. `m_meshMatrices`, bufor `m_activeTransformBuffer`).
4. Rendering: podzial procesow dla elementow wymagajacych jednej tekstury (`RenderWithOneTexture`) i dwoch tekstur (`RenderWithTwoTexture`), az po finalne nakladanie efektow.

Cykl zycia:
Klasa wykorzystuje customowy alokator z pulem `CDynamicPool<CGrannyModelInstance>` w celu zapobiegania defragmentacji i spowolnieniom zwiazanym ze zwalnianiem oraz przydzielaniem duzej ilosci malych i szybkich obiektow. Obiekty nastepnie przechodza przez metody inicjalizacyjne i destrukcyjne posrednie jak `__Initialize()`, po rezygnacji - `Clear()`. Urzadzenia DirectX, jak Dynamic Vertex Buffer, przydzielane sa za pomoca `CreateDeviceObjects()`.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

- **Zaleznosci wejsciowe (Inbound):**
Klasa obslugiwana glownie z modulu rysowania i reprezentacji postaci w przestrzeni, m.in. `CPythonCharacterManager`, klasy aktorow w `CInstanceBase`, elementy GUI opierajace sie o podglad modeli (UI). 
- **Zaleznosci wyjsciowe (Outbound):**
  - EterLib (zarzadzanie zasobami graficznymi, w tym kolizje).
  - Granny 3D API (`granny_model_instance`, `granny_local_pose`, `granny_world_pose`).
  - System renderingu Direct3D 9 / D3DXMATH (wektory matematyczne, bufory D3DX).
- **Drzewo dyrektyw `#include`:**
  - `<atomic>` - dla obslugi watkowego licznika buforow.
  - `<cstdint>` - typy stale.
  - `Eterlib/GrpImage.h`
  - `Eterlib/GrpCollisionObject.h` - rozszerzenie (dziedziczy po tym).
  - `Model.h`, `Motion.h`
  - W `ModelInstance.cpp`: `StdAfx.h`, `EterLib/ResourceManager.h`.
- **Model pamieciowy:** 
Wykorzystywane czyste wskazniki C z samodzielnym zarzadzaniem czasem zycia via dynamiczne pule wlasne `CDynamicPool`, wspierane inteligentnym wskaznikiem buforow typu atomowego `std::atomic<uint8_t> m_activeTransformBuffer`. Tablice asynchroniczne i surowe rzutowanie danych modelu.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabela Klas i Struktur
- `CGrannyLocalPose` : Izoluje wywolywanie alokacji alokacji `granny_local_pose` (Stan szkieletu per-instancja), zarzadza nim z pomoca RAII.
- `CGrannyModelInstance` : Glowna klasa, obslugujaca deformacje. Dziedziczy po `CGraphicCollisionObject`. Wielkosc zmienna; zarzadzana przez `ms_kPool`.

### Tabela Metod Publicznych
- `static CGrannyModelInstance* New()`: Pobiera z puli nowy obiekt. Typ zwrotu: `CGrannyModelInstance*`.
- `void SetMaterialImagePointer(const char* c_szImageName, CGraphicImage* pImage)`: Wymusza zmiane grafiki zewnetrznej dla przypisanego mesha. Side Effects: Modyfikuje rejestr w `m_kMtrlPal`.
- `void Update(DWORD dwAniFPS)`: Update logiki instancji szkieletu w petli klatek na sekunde (30-120).
- `void Deform(const D3DXMATRIX * c_pWorldMatrix)`: Przeksztalca strukture 3D z pozycji lokalnych do swiata, bazujac na `c_pWorldMatrix`.
- `const float * GetBoneMatrixPointer(int iBone) const`: Pobranie stanu kosci dla celow np. podczepiania obiketu pod reke/skrzydlo. 
- `const D3DXMATRIX * GetActiveMeshMatrices() const`: Zwraca bezpieczny pointer bufora Double-Buffering `m_meshMatrices`.
- `void SetParentModelInstance(const CGrannyModelInstance* c_pParentModelInstance, int iBone)`: Montowanie pod inna instancje (Mounts/Pets/Weapons).

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
- `m_pModel` (wskaznik, ofset ok. +0x... zaleznie od `CGraphicCollisionObject`)
- `m_pgrnModelInstance` (wskaznik dla instancji powiazanej z API Granny)
- `m_meshMatrices` (CPU Double-Buffering, [2]x wskaznik)
- `m_activeTransformBuffer` (atomic zmienna uint8_t, przelacznik klatki buffora dla `m_meshMatrices`).
- `m_kLocalDeformableVertexBuffer` / `m_pkSharedDeformableVertexBuffer`: Vertex buffory deformowalne uzyte w GPU.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
Klasa `CGrannyModelInstance` operuje czysto w warstwie klient-side (Graphics/View) w architekturze MVC gry Metin2. Nie bezposrednio dotyka protokolow sieciowych (CG/GC) ani struktur Pythona. Jest owijana przez `CInstanceBase` (ktora to mapuje sie z VID z serwera i wylapuje eventy). API Pythona w module `chr` uderza na ten komponent posrednio przelaczajac pakiety logiki w ruchy modelu.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Obliczenia fizyki deformacji (skinning) uzywaja `m_meshMatrices` w dwoch buforach, co oznacza, ze aktualizacja struktur Granny moze zachodzic asynchronicznie od glownego watku D3D renderujacego wierzcholki (styk `m_activeTransformBuffer`). Jest to krytyczne przy modyfikacji tego modulu, by nie zepsuc `std::memory_order_acquire`.
- **Potencjalne punkty awarii (Crash Points):** Nalezy uwazac na przelaczanie/usuwanie macierzy, gdy silnik D3D9 proboje rysowac - co wiaze sie z ryzykiem NullPointerExceptions na buforze. Pamieci w `CGrannyModelInstance` korzystaja mocno z pointerow (np. `m_pModel`, `m_ppkSkeletonInst`), stad wycieki badz podwojne zwolnienia z `CDynamicPool`.
- **Zarzadzanie zasobami (RAII):** Zasoby z DirectX (np. Dynamic Vertex Buffer) musza byc sprzatane przez `DestroyDeviceObjects()`. Zwolnienia struktur lokalnych szkieletu sa w RAII `CGrannyLocalPose`.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** 
1. Dodaj pole do `CGrannyModelInstance.h` na dole definicji klas (najlepiej blisko pol testowych na dole).
2. Zainicjalizuj pole na `NULL` lub jego ekwiwalent w `CGrannyModelInstance::__Initialize`.
3. Napisz kod posredniczacy na podstawie istniejacego i wyeksportuj jego proxy metody w naglowku.
4. Sprawdz podwojny bufor pod katem mozliwego naruszenia watkow D3D9 vs CPU logiki.
- **Jak debugowac i logowac:** Nalezy dodawac asercje i logowania przy uzyciu `EterBase::ModernLogger` (nie dodawac tracen(), bo codebase wymaga modern_logger z C++23). Breakpoint na `Deform()` przy wyliczaniu transformacji punktow narazonych na artefakty wyswietlania. 
- **Jak testowac bez interfejsu graficznego (Headless):** Skup sie na tworzeniu pustych makr obiektowych oraz na mockach dla logiki biblioteki `Granny` np. `GrannyFreeLocalPose` badz obejsc za pomoca specjalnych dyrektyw typu `#ifndef TEST_MOCK_D3D9`.
