---
task_id: "atlas_c07_01_granny_model_core"
cluster: "MOD"
module_name: "CGrannyModel - Ladowanie Plikow Granny 3D (.gr2)"
target_files:
- src/EterGrnLib/Model.cpp
- src/EterGrnLib/Model.h
- src/EterGrnLib/Mesh.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c07_01_granny_model_core.md"
architecture_layer: "Modele 3D, Szkielety Granny, Drzewa i Efekty"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

**Funkcja modulu:** 
Podsystem `EterGrnLib` (szczegolnie `CGrannyModel` i `CGrannyMesh`) jest rdzeniem graficznym warstwy adaptacyjnej miedzy formatem Granny 3D (.gr2) a silnikiem Metin2 (EterLib / DirectX 9). Klasy te odpowiadaja za wczytywanie modeli 3D, ich wierzcholkow (vertices), indeksow wielokatow (indices) oraz przypisan do kosci (bone bindings i deforms), a takze zrzeszaja powiazane z nimi materialy i tekstury z uzyciem `CGrannyMaterialPalette`.

**Miejsce w petli gry:** 
- **Ladowanie (Resource Loading):** `CreateFromGrannyModelPointer` jest wywolywane glownie podczas asynchronicznego lub synchronicznego ladowania zasobow do pamieci, przy inicjalizacji modeli (np. broni, postaci, potworow) reprezentowanych przez obiekty `.gr2`.
- **Renderowanie (OnRender):** W petli renderowania struktury wewnetrzne (Vertex Buffers, Index Buffers) sa wykorzystywane do przesylania danych geometrii i wywolan rysowania w Direct3D (przez `CGraphicVertexBuffer` i `CGraphicIndexBuffer`).
- **Aktualizacja (OnUpdate):** Kosci sa deformowane na biezaco przez `DeformPNTVertices` wspierane ewentualnie instrukcjami SSE2.

**Przeplyw Danych (Data Flow) i Cykl Zycia (Lifecycle):**
1. **Alokacja:** Obiekt `CGrannyModel` powstaje w pamieci. Przejmuje surowy wskaznik na strukture `granny_model*`.
2. **Inicjalizacja:** `CreateFromGrannyModelPointer` wywoluje kolejno wczytanie siatek (`LoadMeshs`), skopiowanie wierzcholkow z formatu Granny do buforow systemowych/graficznych (`__LoadVertices`), a potem wczytanie indeksow (`LoadIndices`). Oraz tworzone sa fizyczne bufory D3D wywolaniem `CreateDeviceObjects()`.
3. **Deformacje:** Dla kazdej klatki, w ktorej postac ma animacje skieletowa, dane sa przepuszczane przez `DeformPNTVertices`. Uzywana jest optymalizacja SSE2 (funkcja `DeformPWNT3432toGrannyPNGBT33332`) lub standardowe funkcje deformujace SDK Granny.
4. **Dealokacja:** Zwalnianie zasobow nastepuje po zakonczeniu dzialania licznika referencji (`CReferenceObject`). Zwalniane sa tablice wezlow siatek (`m_meshNodes`), same siatki (`m_meshs`) oraz usuwane fizyczne obiekty D3D (`DestroyDeviceObjects`, `Destroy`).

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
- Systemy cache zasobow gry (np. `CResourceManager`, `CGraphicThing`).
- Kontrolery instancji wizualnych (np. `CInstanceBase`, `CGrannyModelInstance`).

**Zaleznosci wyjsciowe (Outbound):**
- `Granny 3D SDK` (funkcje m.in. `GrannyGetMeshVertexCount`, `GrannyCopyMeshVertices`, `GrannyDeformVertices`).
- `EterLib` (silnik renderujacy: klasy `CGraphicVertexBuffer`, `CGraphicIndexBuffer`).
- `DirectX 9` (`LPDIRECT3DVERTEXBUFFER9`, buforowanie `D3DUSAGE_WRITEONLY`, definicje FVF np. `D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1`).
- `Deform.h` (zewnetrzna, prawdopodobnie zoptymalizowana przez SSE2 implementacja transformacji wierzcholkow w C++).

**Drzewo dyrektyw `#include`:**
- `Model.h`: `<Eterlib/GrpVertexBuffer.h>`, `<Eterlib/GrpIndexBuffer.h>`, `"Mesh.h"`
- `Mesh.h`: `"Material.h"`
- `Model.cpp` & `Mesh.cpp`: `"StdAfx.h"`, `"Model.h"`, `"Mesh.h"`, `"Material.h"`, `"Deform.h"`
*(Brak naruszen cyklicznych miedzy Model i Mesh dzieki enkapsulacji klas w poprawnym drzewie zaleznosci).*

**Model Pamieciowy:**
- Klasa bazowa `CGrannyModel` dziedziczy z `CReferenceObject`, narzucajac reczny `Intrusive Reference Counting`. Brak smart-pointerow (np. `std::shared_ptr`).
- Manulane zarzadzanie tablicami dynamicznymi (`new []` / `delete []` m.in. dla `m_meshs`, `m_meshNodes`, `m_triGroupNodes`). Koniecznosc uwaznego uzycia `Initialize()` w tandemie z destruktorem.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabele Klas i Struktur
| Nazwa | Rola | Wlasciciel Watku |
|---|---|---|
| `CGrannyModel` | Glowny kontener na dane bryly modelu Granny3D. Posiada bufory. | Watki: Main D3D Thread, watek ladujacy. |
| `CGrannyMesh` | Podmodul. Enkapsuluje pojedyncza zalezna strukture `granny_mesh`. | Main Thread (wywolania renderingu/SSE2). |
| `TMeshNode` (`CGrannyModel::SMeshNode`) | Node powiazany z lista jednokierunkowa uzywana do sortowania siatek w modelu wg typow (Rigid/Deform). | Zarzadzane wewnetrznie przez Model. |
| `TTriGroupNode` (`CGrannyMesh::STriGroupNode`) | Lista wezlow grup trojkatow dla danej grupy materialu. | Zarzadzane wewnetrznie przez Mesh. |
| `granny_pnt3322_vertex` | Struktura wierzcholka FVF z 2 wektorami UV, na potrzeby specyficznych modeli (np. "Dungeon Block"). | Bezstanowy struct DTO. |

### Tabela Metod Publicznych
| Sygnatura Metody | Argumenty | Wartosc Zwracana | Side-effects / Pre-conds |
|---|---|---|---|
| `bool CGrannyModel::CreateFromGrannyModelPointer(...)` | `granny_model* pgrnModel` | `bool` | Wymaga by model byl wczesniej pusty (`IsEmpty() == true`). Tworzy wezly, laduje indices/vertices. Zwieksza refCount. |
| `void CGrannyModel::DeformPNTVertices(...)` | `void* dstBaseVertices, D3DXMATRIX* boneMatrices, std::vector<granny_mesh_binding*>&` | `void` | Zapisuje wprost do pamieci systemowej bufora docelowego (`dstBaseVertices`). Zalezy od CPU/SSE2. |
| `bool CGrannyModel::CreateDeviceObjects()` | `void` | `bool` | Inicjalizuje obiekty COM Direct3D9 w instancji podleglej `EterLib` wlasciwej dla vertex i index buforow. |
| `bool CGrannyMesh::CreateFromGrannyMeshPointer(...)` | `granny_skeleton* pgrnSkeleton, granny_mesh* pgrnMesh, int vtxBasePos, int idxBasePos, CGrannyMaterialPalette& rkMtrlPal` | `bool` | Wymaga pustej siatki (`IsEmpty()`). Konfiguruje deformery z uzyciem API GrannySDK. Rejestruje materialy do palety. |

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
- `CGrannyModel::SMeshNode`: 
  `[0x0] int iMesh` (4b)
  `[0x4/0x8] const CGrannyMesh* pMesh` (Ptr)
  `[0x8/0x10] SMeshNode* pNextMeshNode` (Ptr)
- Bufory GPU trzymane sa instancjach obiektow klas CGraphicVertexBuffer/IndexBuffer (`m_pntVtxBuf`, `m_idxBuf`). Nie nalezy odwolywac sie bezposrednio do ich wnetrza pomijajac blokady `Lock()`.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

**Pakiety Sieciowe:** 
Modul calkowicie odciety od warstwy sieciowej (Network Agnostic). Wszelkie obiekty korzystajace z tego modulu uzyskuja dane od nadrzednych instancji reagujacych na pakiety spawnu (np. `HEADER_GC_CHARACTER_ADD`).

**Python C-API (`PyMethodDef`):** 
Brak bezposrednich ekspozycji dla Pythona z poziomu tych klas. Interfejs z tymi modulami jest ukryty pod poziomami takimi jak `chr` / `chrmgr` module (np. `chrmgr.RegisterCache`).

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

1. **Wielowatkowosc i API D3D:** Poniewaz modul tworzy `Vertex/Index Buffers` (`CreateDeviceObjects`), te funkcje musza byc wywolywane z watku obslugujacego API D3D (Main Thread). Ladowanie asynchroniczne pamiateka RAMu i wyciaganie wierzcholkow bezposrednio moze dzialac obok, ale kreacja hardware bufferow musi uwazac na obwarowania watkowe D3D.
2. **Tablice C-Style `new [] / delete []`:** Ryzyko przecieku pamieci i null pointer dereference. Nalezy bezwzglednie pamietac by funkcja `Destroy()` zwalniala tablice `m_meshNodes`, `m_meshs`, `m_triGroupNodes`. Uzycie smart-pointerow znaczaco zredukowaloby tu technical debt, lecz kod przestrzega dotychczasowych (starych) norm klienta. Zero-Conflict rule zabrania w tym miejscu zmian bez specjalnego pozwoleinia i migracji.
3. **Optymalizacje SSE2:** Zmienna globalna `extern bool CPU_HAS_SSE2` determinuje na biezaco wywolanie optymalizowanej sciezki montazu kosci. Brak inicjalizacji tej zmiennej gdzie indziej uziemi proces z `Undefined Reference` przy budowaniu - jest ona zapewniona na poziomie rdzenia klienta (`UserInterface`/`EterBase`).
4. **Rozszerzony FVF (PNT2):** W przypadku modeli jak blok lochow ("Dungeon Block"), `CGrannyMesh::SetPNT2Mesh()` przelacza format na strukture `granny_pnt3322_vertex`, dodajac UV1. Wlasciwe maskowanie miedzy `m_dwFvF` i materialami jest kluczowe, inaczej format werteksow ulegnie korupcji podczas renderowania pod D3D.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Instrukcja dodawania nowej funkcji (np. wczytywania kolejnego kanalu UV / Normal Mappingu):**
1. Zdefiniuj w `Mesh.h` zaktualizowana strukture werteksow w oparciu o API Granny, np. z `Tangent` lub `Binormal`.
2. Dodaj odpowiedni identyfikator `granny_data_type_definition` dla nowych typow danych w `Mesh.cpp` (obok `GrannyPNT3322VertexType`).
3. Zaktualizuj switch tworzenia FVF (np. `D3DFVF_XYZ | D3DFVF_NORMAL...`) w `CGrannyModel::LoadMeshs()`, sprawdzajac obecnosc nowych kanalow mapy.
4. Modyfikuj bufory - stworz osobny wariant wywolania `.Create(..., nowyFvF, ...)` z modulu `GrpVertexBuffer`.

**Jak debugowac i logowac:**
Zarzadzanie deformacja ma swoje krawedzie - w razie crashy najwiekszym podejrzanym jest pointer na `m_meshs`, ewentualnie de-sync na lini wybranego `MeshBinding`. Jesli indeks wybiega poza `m_pgrnModel->MeshBindingCount`, program zakonczy sie awaryjnie w `GetMeshPointer()` ze wzgledu na zaimplementowane silne asercje `assert(CheckMeshIndex(iMesh));`.

**Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
Aby przetestowac parsing plikow bez D3D9 i UI:
- Nalezy zmockowac wywolania `CreateDeviceObjects` by zwracaly pomyslnie wartosc bez wolania obwodu D3D9.
- Wyodrebnic surowe wywolanie metody pamieciowej CPU wewnatrz `CreateFromGrannyModelPointer()` bez uruchamiania faktycznego mechanizmu renderingu.
- Skompilowac przy minimalnym otoczeniu zastepujac `#include <d3d9.h>` stubami (zgodnie z regula pustych mockow dla DX). Zmienna `CPU_HAS_SSE2` w zaleznosci od OS/srodowiska testowego powinna byc wyodrebniona i sztucznie wstrzyknieta w pliku testowym jako `bool CPU_HAS_SSE2 = false;`.
