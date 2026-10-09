---
task_id: "atlas_c07_07_speedtree_wrapper"
cluster: "MOD"
module_name: "CSpeedTreeWrapper - Renderowanie Lisci, Koron i Galezi"
target_files:
- src/SpeedTreeLib/SpeedTreeWrapper.cpp
- src/GameLib/SpeedTreeInstancedBridge.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c07_07_speedtree_wrapper.md"
architecture_layer: "Modele 3D, Szkielety Granny, Drzewa i Efekty"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

## Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul \`CSpeedTreeWrapper\` wraz z mostkiem \`SpeedTreeInstancedBridge\` odpowiada za calosciowe zarzadzanie i renderowanie elementow roslinnosci pochodzacych z oprogramowania SpeedTree (drzewa, liscie, korony, galezie). Stanowi warstwe posredniczaca miedzy silnikiem gry Metin2, zewnetrzna biblioteka SpeedTreeRT oraz niskopoziomowym renderowaniem Direct3D 9, w tym systemem instancjonowania sprzetowego (Hardware Instancing). 
Kod ten jest uruchamiany glownie w petli renderujacej (OnRender) silnika graficznego. \`CSpeedTreeWrapper\` obsluguje tradycyjne wyrysowanie, w tym efekt powiewania na wietrze na GPU lub CPU, LOD (Level of Detail), bilboardy dla drzew oddalonych oraz przygotowanie buforow wierzcholkow i indeksow na podstawie geometrii pozyskanej z biblioteki SpeedTreeRT. 
\`SpeedTreeInstancedBridge\` sluzy do optymalizacji: grupuje wyrysowanie drzew wykorzystujac instancjonowanie sprzetowe, mapujac zewnetrzne obiekty do struktury instancji Direct3D (HardwareMeshInstancer) zawierajacej pozycje, kat obrotu, skale, kolor (tint) oraz faze wiatru.
Cykl zycia obiektu \`CSpeedTreeWrapper\` obejmuje wczytanie pliku \`.spt\` (LoadTree), utworzenie buforow D3D, aktualizacje co klatke (OnRender) uwzgledniajaca transformacje (culling) oraz zwolnienie buforow podczas destrukcji (CleanUpMemory). Z kolei obiekt \`SpeedTreeInstancedBridge\` zyje w warstwie terenu, przyjmujac pozycje drzew i jednorazowo oprozniajac je do instancera w biezacej klatce (FlushTrees, Clear).

## Dokladna Mapa Zaleznosci (Exact Dependency Map):
**Zaleznosci wejsciowe (Inbound):** 
Wywolywany glownie przez \`SpeedTreeForestDirectX\` oraz systemy kafelkow mapy, w szczegolnosci przez obiekt \`CGraphicObjectInstance\` / \`CArea\` (drzewa rozmieszczone na mapie).
**Zaleznosci wyjsciowe (Outbound):** 
\`SpeedTreeRT\` (zewnetrzna biblioteka IDV), \`DirectX 9\` (D3DDevice, bufory VBO, IBO, shadery), \`HardwareMeshInstancer\` w \`Client::Graphics\`, \`EterLib/EterBase\` (Timer, Camera, StateManager, Memory, CGraphicObjectInstance).
**Drzewo dyrektyw #include:**
\`SpeedTreeConfig.h\`, \`SpeedTreeMaterial.h\`, \`SpeedTreeRT.h\`, \`d3d9.h\`, \`d3d9types.h\`, \`d3dx9.h\`, \`vector\`, \`memory\`, \`cstdint\`, \`EterLib/GrpObjectInstance.h\`, \`EterLib/GrpImageInstance.h\`, \`EterBase/MathSIMD.h\` (dla pliku h); dodatkowo w pliku cpp: \`StdAfx.h\`, \`EterBase/Debug.h\`, \`EterBase/Timer.h\`, \`EterLib/ResourceManager.h\`, \`EterLib/Camera.h\`, \`EterLib/StateManager.h\`, \`SpeedTreeForestDirectX.h\`, \`VertexShaders.h\`. W \`SpeedTreeInstancedBridge.cpp\`: \`cmath\`, \`Client/Graphics/HardwareMeshInstancer.h\`. Potencjalne ryzyko zaleznosci cyklicznych pomiedzy wrapperem a managerem lasu (\`SpeedTreeForestDirectX\`).
**Model pamieciowy:** 
W \`CSpeedTreeWrapper\` szeroko uzywane sa standardowe wskazniki C, jednak wprowadzono \`std::shared_ptr<CSpeedTreeWrapper>\` dla zarzadzania wspoldzielonymi obiektami, zwlaszcza instancjami drzew klonowanych (m_pInstanceOf i m_vInstances). \`SpeedTreeInstancedBridge\` alokuje dane dynamiczne w wektorze wpisow i wysyla gotowy zestaw struktur typu by-value do instancera bez alokacji stertowych na instancje.

## Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
1. \`CSpeedTreeWrapper\` - dziedziczy z \`CGraphicObjectInstance\` oraz \`std::enable_shared_from_this\`. Obiekt (rozmiar okolo kilkuset bajtow z racji wielu buforow VBO, IBO, materialow). Wlasciciel: Glowny watek renderujacy Direct3D.
2. \`SpeedTreeInstancedBridge\` - zarzadca pamieci i mostek konwertujacy parametry na \`InstanceData\`.
3. \`TreeInstanceEntry\` (z \`SpeedTreeInstancedBridge.h\`) - struktura POD: \`treeTypeId\`, \`posX\`, \`posY\`, \`posZ\`, \`scale\`, \`rotationYaw\`, \`tintColor\`, \`windPhase\`, \`lodIndex\`. Rozmiar 32 bajty.
4. \`TreeRotationAxis\` (enum z \`SpeedTreeInstancedBridge.h\`) - Z_Up (0) dla Metin2, Y_Up (1) dla standardowego D3D.
5. \`TreeMeshMapping\` (struktura) - \`meshId\`, \`materialId\`. Rozmiar 8 bajtow.

**Tabela Metod Publicznych (CSpeedTreeWrapper):**
- \`LoadTree(const char*, const BYTE*, unsigned int, UINT, float, float)\` - wczytuje drzewo, zwraca bool. Efekt uboczny: alokuje obiekty CSpeedTreeRT, resetuje bufory D3D.
- \`OnRender()\` - glowna metoda wyrysowujaca, nie zwraca wartosci, uzywa StateManager i wywoluje IDirect3DDevice9::DrawPrimitive.
- \`RenderBranches()\`, \`RenderFronds()\`, \`RenderLeaves()\`, \`RenderBillboards()\` - rysowanie poszczegolnych geometrii. 
- \`SetupBranchForTreeType()\`, \`SetupFrondForTreeType()\`, \`SetupLeafForTreeType()\` - ustawiajashadery, stany renderowania, materialy.
- \`MakeInstance()\` - tworzy sklonowany obiekt oparty na \`std::shared_ptr\`.

**Tabela Metod Publicznych (SpeedTreeInstancedBridge):**
- \`RegisterTree(uint32_t, float, float, float, float, float, uint32_t, float, uint32_t)\` - dodaje wpis do vectora \`m_trees\`.
- \`FlushTrees(Client::Graphics::HardwareMeshInstancer&)\` - przekazuje i czysci kolejke, wysylajac instancje do GPU.
- \`BuildInstanceData(const TreeInstanceEntry&, TreeRotationAxis)\` - statyczna metoda mapujaca pozycje i obrot na \`InstanceData\` w konwencji Row-Major 4x4.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
\`TreeInstanceEntry\`: [0-3] treeTypeId, [4-15] posX/Y/Z, [16-19] scale, [20-23] rotationYaw, [24-27] tintColor, [28-31] windPhase, [32-35] lodIndex.
Zwracac uwage na offset wskaznikow D3D w \`CSpeedTreeWrapper\`: \`m_pSpeedTree\`, \`m_pBranchVertexBuffer\`, \`m_pLeafVertexBuffer\`. Ulatwi to ewentualny hooking przy podmianie silnika graficznego.

## Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
Moduly te stanowia warstwe wylacznie graficzna i kliencka. Nie wchodza w bezposrednia interakcje z pakietami sieciowymi serwera. Parametry drzew i roslinnosci sa ladowane ze statycznych plikow mapy w kliencie. Nie maja rowniez bezposrednich wrapperow (C-API/PyMethodDef) do Pythona. Jedyny punkt styku z Pythonem odbywa sie przez moduly ladujace tlo i mapy, ktore wolaja w C++ funkcje obszaru (CArea).

## Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Funkcje modyfikujace bufory Direct3D oraz rysujace (\`OnRender\`, \`SetupBuffers\`) musza byc wolane wylacznie z glownego watku D3D. Praca ze \`SpeedTreeInstancedBridge\` w teorii moglaby zostac zrownoleglona na CPU podczas zbierania widocznych drzew do rysowania (Culling Worker Thread), ale ostateczne \`FlushTrees\` powinno zachodzic przed generowaniem paczek instancji, co wiaze sie z wyrysowaniem instancera na glownym watku.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** Brak zainicjalizowanych shaderow (jesli instancja z singletonu SpeedTreeForestDirectX nie zadbala o wywolanie SetVertexShaders). Wskazniki w \`m_pGeometryCache\` po resecie drzewa moga byc invalidowane; konieczne ostrozne obslugiwanie \`SAFE_RELEASE\` i \`SAFE_DELETE\`. Pulapka przy modyfikowaniu lisci z uzyciem CPU (\`m_pLeavesUpdatedByCpu\`) - latwo o dostep poza zakresem, uzyto dynamicznego buforu Lock z \`D3DLOCK_DISCARD\`. Zle skalkulowana macierz 4x4 w instancerze (zmiana konwencji \`Z_Up\` na \`Y_Up\`) moze zdeformowac cale drzewo.
- **Zarzadzanie zasobami (RAII):** Kod starego SpeedTree uzywa niefortunnej, recznej pamieci (\`SAFE_RELEASE\` dla interfejsow COM D3D, \`SAFE_DELETE\` dla wskaznikow C). Ograniczono wycieki dla instancji dzieki \`SpeedTreeWrapperPtr\` (\`std::shared_ptr\`), ale czesc wewnetrznych tablic jest recznie deletowana w \`CleanUpMemory\`.

## Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** W przypadku ulepszenia shadow mappingu dla drzew zrob update w \`OnRenderToShadowMap\` / \`OnRenderShadow\`. Uzyj zasobow w \`SpeedTreeInstancedBridge\`, zeby dodac mapowanie bufora do nowej wersji C++23. Dodaj brakujace parametry (np. \`ambientColor\`) do \`TreeInstanceEntry\` w pliku .h i przerob \`BuildInstanceData\`.
- **Jak debugowac i logowac:** Do modulu mozna dodac \`EterBase::ModernLogger::Error\`. Podczas badania niewidzialnych drzew, sprawdz czy wywolano SetLodLevel oraz jaka byla odpowiedz \`TestFrustumCulling()\`. Sprawdz zawartosc wewnetrznego \`std::vector<TreeInstanceEntry> m_trees\`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** \`SpeedTreeInstancedBridge\` mozna w pelni przetestowac narzedziami unit-test na Linuxie. Aby to zrobic, stworz mocki struktur wektorow z \`HardwareMeshInstancer\` w doctest i zbadaj czy macierze swiata produkowane przez \`BuildInstanceData\` sa identyczne z obliczonymi algebraicznie. Modulu \`CSpeedTreeWrapper\` unikaj w srodowisku bezposrednim (mockowanie interfejsow COM z DirectX jest zbyt czasochlonne), testuj to na poziomie wyzwalaczy w CGraphicObjectInstance.

