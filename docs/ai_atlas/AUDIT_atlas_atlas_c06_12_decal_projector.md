---
task_id: "atlas_c06_12_decal_projector"
cluster: "RND"
module_name: "CDecal - Rzutnik Naklejek Terenowych i Sladow Krwi"
target_files:
- src/EterLib/Decal.cpp
- src/EterLib/Decal.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c06_12_decal_projector.md"
architecture_layer: "Renderowanie, Direct3D 9, Shading i Culling"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Modul `CDecal` jest niskopoziomowym podsystemem renderingu sluzacym do nakladania dwuwymiarowych tekstur na nierowna geometrie 3D. Biznesowo odpowiada za wyswietlanie takich elementow wizualnych jak cienie pod postaciami (drop shadows), slady uderzen na ziemi, animacje czarow dzialajacych obszarowo czy plamy krwi. Technicznie, modul ten dziala poprzez definiowanie trojwymiarowego obszaru w przestrzeni (tzw. "box" rzutowania) okreslanego srodkiem, normalna, tangensa, szerokoscia, wysokoscia i glebokoscia, a nastepnie "przycina" (clipping) przekazywana geometrie wielokatow wejsciowych do tych plaszczyzn (z uzyciem `ClipMesh` i `ClipPolygon`), tworzac dynamiczny bufor werteksow w ksztalcie wachlarza trojkatow (Triangle Fan).

Proces powstawania i rysowania:
1. Alokacja i inicjalizacja (`Clear`, domyslny konstruktor).
2. Wywolanie (najczesciej w klasach dziedziczacych np. `CTerrainDecal::Make`) funkcji generujacej bryle przyciecia (frustum/box 6 plaszczyzn - Left, Right, Bottom, Top, Front, Back).
3. Dodawanie wielokatow terenu (badz innej geometrii) i ich przycinanie za pomoca `ClipMesh` (ktore korzysta z `ClipPolygon` obcinajacego pojedynczy wielokat przeciwko 6 plaszczyznom po kolei - algorytm Sutherlanda-Hodgmana).
4. Budowanie i przechowywanie gotowych trojkatow (`m_Vertices`, `m_Indices`, oraz wpisy `m_TriangleFanStructVector`). Modul wykorzystuje klasyczne tablice (max 256 werteksow).
5. Rysowanie (`Render`) wywolywane z petli renderowania glownego watku (z wykorzystaniem bezposrednio menedzera stanow `STATEMANAGER` EterLib i wywolan `DrawIndexedPrimitiveUP` z `D3DPT_TRIANGLEFAN`). Dealokacja / resetowanie odbywa sie przez `Clear()`.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

- **Zaleznosci wejsciowe (Inbound):** Modul ten jest zazwyczaj wolany przez systemy wyzszego rzedu obslugujace teren i efekty, glownie przez klase dziedziczaca `CTerrainDecal` (`src/GameLib/TerrainDecal.h` i `.cpp`), ktora rozszerza go o specyfike terenu i integruje z `CMapOutdoor`. System cieni i sladow na ziemi inicjuje zapytania do `CDecal`.
- **Zaleznosci wyjsciowe (Outbound):** `CDecal` wymaga DirectX 9 SDK (`D3DXVECTOR3`, `D3DXPLANE`, `D3DCOLOR`, funkcje matematyczne D3DX np. `D3DXVec3Cross`, `D3DXVec3Dot`, `D3DXPlaneDotCoord`), `EterLib/GrpBase.h` dla podstaw grafiki oraz `EterLib/StateManager.h` (singleton `STATEMANAGER` jako jedyny punkt mutujacy stan DirectX podczas renderowania).
- **Drzewo dyrektyw `#include`:** `stdafx.h`, `Decal.h`, `StateManager.h` w `.cpp`. Ryzyko zaleznosci cyklicznych jest tutaj niemal zerowe, poniewaz klasa opiera sie wylacznie na matematyce D3DX oraz singletonie stanow D3D.
- **Model pamieciowy:** W klasie glowne dane trzymane sa na stosie / bezposrednio w ciele klasy. Zdefiniowana statycznie tablica `TPDTVertex m_Vertices[MAX_DECAL_VERTICES]` i tablica `WORD m_Indices[MAX_DECAL_VERTICES]`. Przestrzen ta nie wymaga zewnetrznego uzycia inteligentnych wskaznikow, nie alokuje sterty poza ewentualnym powiekszeniem wewnetrznego vectora `std::vector<TTRIANGLEFANSTRUCT> m_TriangleFanStructVector`.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur**
- `CDecal`: (Klasa podstawowa). Rola: Algorytm clippingu rzutnika geometrii, zarzadzanie buforem lokalnych trojkatow. Wlasciciel watku: Glowny watek renderujacy.
- `CDecal::TTRIANGLEFANSTRUCT`: Struktura wewnetrzna (typedef). Rola: Grupuje definicje dla pojedynczego wachlarza trojkatow w obrebie buffora indexow/vertexow w celu wywolania `DrawIndexedPrimitiveUP` w jednym passie. Kluczowe pola: `m_wMinIndex`, `m_dwVertexCount`, `m_dwPrimitiveCount`, `m_dwVBOffset`.

**Tabela Metod Publicznych (`CDecal`)**
- `void Clear();` - Resetuje liczniki i czysci plaszczyzny rzutnika oraz wektor struktur rysowania. Pre-conditions: Brak. Skutki: Przestaje wyswietlac stara zawartosc.
- `virtual void Make(D3DXVECTOR3 v3Center, D3DXVECTOR3 v3Normal, D3DXVECTOR3 v3Tangent, float fWidth, float fHeight, float fDepth) = 0;` - Metoda czysto wirtualna; definiuje orientacje i wymiary "boxa" dla ciec.
- `virtual void Render();` - Rysuje naklejke uzywajac stanu DirectX zdefiniowanego poza naklejka. Zapisuje identycznosc dla macierzy swiata, ustawia FVF (`D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1`) i rysuje petle `STATEMANAGER.DrawIndexedPrimitiveUP`. Zalezy w calosci od aktualnie ustawionego tekstury, alphy.

**Pamieciowy Layout Struktur (Memory Layout & Offsets)**
Kluczowe stale: `MAX_DECAL_VERTICES = 256`. 
Pola pamieci klasy: 
- Wektory centralne i normalne `m_v3Center`, `m_v3Normal`
- 6 Plaszczyzn clippingowych (D3DXPLANE): `m_v4LeftPlane`, `m_v4RightPlane`, `m_v4BottomPlane`, `m_v4TopPlane`, `m_v4FrontPlane`, `m_v4BackPlane`.
- Statyczne tablice preallokowane: `m_Vertices` (rozmiar `256 * sizeof(TPDTVertex)`), `m_Indices` (`256 * sizeof(WORD)`).
- Bufor wektora opisow renderowania: `m_TriangleFanStructVector`.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

Modul `CDecal` jest elementem bezposredniego rurkowania renderingu i nie posiada:
- Powiazan wprost ze skryptami Pythona (metody nie sa bindowane przez `PyMethodDef`).
- Protokolow Sieciowych (brak parsowania GC/CG opcodes bezposrednio w modulu). 
Zarza sie nim w calosci z poziomu engine'u, kiedy `InstanceBase` albo efekt terenu wymusi pojawienie sie cienia, czy krwi.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Zasady wielowatkowosci:** Poniewaz `CDecal::Render()` korzysta wprost ze stanowego bufora DirectX (`STATEMANAGER.DrawIndexedPrimitiveUP`), metoda ta **musi** byc wywolywana wylacznie w glownym watku renderujacym (Render Thread). 
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** Przepelnienie tablicy werteksow. Algorytm w `CDecal::AddPolygon` ma proste zabezpieczenie (`if (m_dwVertexCount + dwAddCount >= MAX_DECAL_VERTICES) return false;`). Jesli obszar terenu pod naklejka ma nadzwyczaj drobny podzial (wiecej niz 256 powstalych po clippingu werteksow), dalsza czesc geometrii naklejki zostanie zignorowana i po prostu niewyswietlona - obcinajac brzydko ksztalt naklejki, ale zapobiegajac memory corruption / out of bounds exception.
- **Zarzadzanie zasobami (RAII):** Kod bardzo prymitywny pod wzgledem sterty - brak wskzanikow do alokowanej pamieci VRAM w `CDecal`. Rendering oparty na "User Pointers" (`UP`) jest z zalozenia wolny jesli wykonany w gigantycznej skali, bo geometria z pamieci systemowej rzucana jest co klatke przez szyne do karty.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** Aby zwiekszyc rozdzielczosc geometrii (wsparcie gladszego terenu), zmien stala `MAX_DECAL_VERTICES` z 256 na wyzsza liczbe (np. 1024), upewniajac sie w `Decal.cpp` w obiekcie petli clippingu (`D3DXVECTOR3 v3TempVertex[9];`) czy statyczne tablice pomocnicze nie wymagaja rowniez powiekszenia jesli wejsciowe wielokaty maja wieksza ilsoc wierzcholkow niz 3. Aktualnie clipowany jest po jednym trojkacie (`ClipPolygon(3...)`), stad array of 9 is generally safe from math (3 vertices clip by 6 planes worst case yields some small n-gon).
- **Jak debugowac i logowac:** Przechwytujac rzutowanie rzutnika podepnij sie pod zmienna predykatowa w `ClipMesh`: patrz jaka wartosc ma mianownik epsilona z `D3DXVec3Dot(&m_v3Normal, &v3Cross)`. Blad obliczen w dot product'ie moze skutkowac naglym znikaniem rzutowania. Dodaj EterBase::ModernLogger.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Wymaga mocka na `STATEMANAGER`. Prawidlowy unit test powinien ustawic macierze boxa przez `Make` w stubb'ie (poniewaz jest to czysto wirtualna), dac tablice wejsciowa kwadratowego obszaru plaskiego terenu i sprawdzic liczbe wynikowych `m_dwVertexCount` po `ClipMesh` -- czy odpowiada oczekiwaniom nakladania sie z rzutnia frustum. Czysta matematyka. Nie nalezy wolac `Render` w izolacji bez atrapy `StateManager`u.
