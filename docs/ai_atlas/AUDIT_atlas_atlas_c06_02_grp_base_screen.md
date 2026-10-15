---
task_id: "atlas_c06_02_grp_base_screen"
cluster: "RND"
module_name: "CGraphicBase i CGraphicScreen - Rzutowanie i Macierze"
target_files:
- src/EterLib/GrpBase.cpp
- src/EterLib/GrpScreen.cpp
- src/EterLib/GrpScreen.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c06_02_grp_base_screen.md"
architecture_layer: "Renderowanie, Direct3D 9, Shading i Culling"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Modul stanowi fundament renderowania w silniku (EterLib) dla warstwy graficznej. `CGraphicBase` to stanowy wrapper wokolo globalnego stanu i urzadzenia Direct3D 9, obslugujacy rzutowania macierzy (View, Projection, World) i manipulacje kamera (ViewMatrix) oraz zrodlem promieni (Ray) do celow "picking". Z kolei `CGraphicScreen` rozszerza go (z klasa bazowa `CGraphicCollisionObject`) dodajac system funkcji pomocniczych do renderowania ksztaltow geometrycznych bezposrednio z pamieci operacyjnej na ekranie, uzywajac wbudowanego mechanizmu dynamicznych buforow wierzcholkow D3D9 i stanow (przechodzac przez klasyczny `DrawIndexedPrimitive` / `DrawPrimitive`).
- **Moment wywolania:** Operacje rzutowania (macierze) i zmiany kamer wolane sa glownie podczas fazy `OnUpdate()` (dostosowanie widoku) oraz na wczesnych etapach `OnRender()`. Bezposrednie rysowanie linii (RenderLine3d) i boxow dla GUI czy debuggingu lub efektow nastepuje pozniej w trakcie wlasciwego renderowania sceny (`Begin()`/`End()`).
- **Przeplyw danych (Data Flow):** Wchodza wspolrzedne wektory (D3DXVECTOR3) od logiki kamery (CCameraManager) -> `CGraphicBase` przelicza i modyfikuje globalne stosy D3DXMatrix -> wywoluje `STATEMANAGER.SetTransform()` -> na tej podstawie D3D9 transformuje obiekty 3D na wspolrzedne okna 2D podczas rysowania. Alternatywnie, modul przyjmuje pozycje kursora myszy 2D, przepuszcza przez macierz inwersyjna (Unproject) rzutujac w swiat 3D generujac wektor (Ray) potrzebny do analizy kolizji, np. do sprawdzenia, na ktory model kliknal gracz (Picking).
- **Cykl zycia (Lifecycle):** Obiekty `ms_lpd3dDevice`, `ms_lpd3dMatStack` oraz macierze sa zdefiniowane jako globalne (static). Bufor ramki (BackBuffer), Z-Buffer czyszczone sa per klatka (`Clear()`). Renderowanie geometryczne buduje ulotne bufory z uzyciem wspoldzielonego vertex buffera, podmienia, odrysowuje i zwalnia zamek bez dlugotrwalego alokowania, wspierajac styl RAII bez wyciekow.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Modul jest centralna brama dla kazdego podsystemu rysujacego i liczenia kolizji. Cale UserInterface (`PythonApplication`, `PythonBackground`, `PythonCharacterManager`), EffectLib i Miles/Sound posilkuja sie pozycjami kamery stamtad. Interfejs z myszy w UI przerzuca x,y tu do `SetCursorPosition`.
- **Zaleznosci wyjsciowe (Outbound):** Zalezy w 100% od EterLib/STATEMANAGER (`CStateManager` kontroluje cache stanow D3D aby nie redundowac wywolan API D3D), `CCameraManager` (do pobierania aktualnego View i zrodla oczu), oraz EterBase (Timer, Logger). W warstwie rzutu wola Direct3D 9 API (oraz biblioteke pomocnicza D3DX).
- **Drzewo dyrektyw `#include`:** Dolacza "GrpBase.h", "Camera.h", "StateManager.h", a z OS: <comdef.h>, <utf8.h>. Istnieje uzycie "SphereLib/frustum.h" dla obliczania Frustum.
- **Model pamieciowy:** Dominuja statyczne alokacje zmiennych globalnych D3D w klasie `CGraphicBase`. Do renderingu biezacego uzywa tablic na stosie (`SPDTVertexRaw vertices[8]`) lub `std::vector` (przemijajacy do obrysow). Nie wykorzystuje smart pointerow z STL. Opiera sie na surowych wskaznikach COM D3D z reczna walidacja oraz singletonie.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
1. `CGraphicBase` - klasa czysto statyczna, rdzen stanu RHI dla Direct3D. Stanowi fasade na device d3d9 i macierze rzutu.
2. `CScreen` - dziedziczy po `CGraphicCollisionObject`. Narzedzie renderowania surowych prymitywow na ekran, buforow statystycznych i zarzadzania cyklem klatki D3D (BeginScene / EndScene / Clear / Present/Show).
3. `CD3DXMeshRenderingOption` - pomocnicza struktura wzorca RAII, w konstruktorze zachowujaca stare stany FVF, Materialow i transform, z ustawieniem nowych dla renderu mesh, a w destruktorze robiaca Restore na StateManager.
4. `SPDTVertexRaw` - struktura D3D FVF o pamieciowym ukladzie `x, y, z` (float), `diffuse` (DWORD z kolorem a,r,g,b), `u, v` (float, float) sluzaca zasileniu potoku D3D9 (FVF_XYZ|FVF_DIFFUSE|FVF_TEX1).

**Tabela Metod Publicznych:**
- `void CGraphicBase::UpdateViewMatrix()`: Brak agr., zmienia globalny D3DTS_VIEW state poprzez STATEMANAGER w oparciu o biezacy stan CCamera.
- `void CScreen::RenderLine3d(float sx, sy, sz, ex, ey, ez)`: Cofa pule wierzcholkow (PDTStream) ladujac 2 vertexy i wysyla DRAW_PRIMITIVE. Side effects: podmienia biezacy Texture0, Texture1 i FVF w rurociagu na puste i zdefiniowane. 
- `bool CScreen::GetCursorPosition(float* px, float* py, float* pz)`: Zwraca `bool`, rzutuje pick ray na plaszczyzne ziemi. Wymaga wywolania `SetCursorPosition` przed.
- `void CScreen::ProjectPosition(float x, y, z, float* px, py, pz)`: Wywoluje wewnetrzne `D3DXVec3Project`, przeprowadzajac translacje World -> View -> Proj (okienkowy).
- `void CScreen::Show(HWND hWnd)`: Wykonuje `Present` swap chainu, by wymienic back-buffer z front-buffer. Side effects: Handle do urzadzenia (LostDevice). 

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
- Klasa `CGraphicBase` glownie trzyma czlonki statyczne na stosie pamieciowym sekcji .data/.bss z adresem stalym.
- `SPDTVertexRaw`: 24 bajtow na vertex:
  - offset 0: float x (4B)
  - offset 4: float y (4B)
  - offset 8: float z (4B)
  - offset 12: DWORD diffuse (4B)
  - offset 16: float u (4B)
  - offset 20: float v (4B)
  
### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- Modul NIE ma zadnych bezposrednich powiazan sieciowych ani opcodow.
- Modul NIE deklaruje `PyMethodDef` bezposrednio. Jest owiniety na warstwie wyzszej (`PythonApplication`) przez wlasne wrappery, wiec jego bezposredni styk to wylacznie inne moduly C++.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Wszystko w tym module MUSI byc wywolywane z glownego watku D3D. Silnik Metin2 to aplikacja na ogol jednowatkowa jesli chodzi o rurociag renderujacy grafiki. Wolanie renderu (np `RenderBox3d`) z asynchronicznego watku ladowania zniszczy kontekst i wykolei D3D Device.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - **Utrata Urzadzenia D3D (Device Lost):** Bardzo wazny przypadek to D3DERR_DEVICELOST przy Alt+Tab. Metoda `CScreen::RestoreDevice()` musi pomyslnie zadzialac, i moze zaraportowac log do pliku tekstowego `TraceError` po niepowodzeniu. Brak walidacji wskaznikow COM w D3D mozna sprowokowac przedwczesnym uzyciem Render(...) przed Begin().
  - **Przekroczenie zasobow buforow wierzcholkow:** Zobacz `SetPDTStream()`. Bufor to `PDT_VERTEX_NUM = 16`. Jesli wyslesz wektor powyzej tej kwoty, metoda cicho zawiedzie (`return false`) z `assert(PDT_VERTEX_NUM>=uVtxCount)`. Nie sluzy do renderowania calych formacji obiektow.
- **Zarzadzanie zasobami (RAII):** Kod unika surowego wycieku na zewnatrz na korzysc dynamicznych strumieni. Do manipulacji stanem `CStateManager` zostawia sie uzycie struktury opartej o RAII `CD3DXMeshRenderingOption`.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** 
  Jesli potrzebujesz np dodac nowy precyzyjny rodzaj debug boxa, stworz w CScreen::RenderTwojBox i uzyj statycznego `SPDTVertexRaw` o liczbie wierzcholkow Mniejszej lub rownej 16. Spakuj go poprzez `SetPDTStream`, ustaw staty (FVF, ColorOP, Textury z `STATEMANAGER`) i wywolaj draw `STATEMANAGER.DrawPrimitive`. Zawsze wylaczaj tekstury `STATEMANAGER.SetTexture(0, NULL)` jesli rysujesz same linie kolorem by uniknac przebic pamieci VRAMu uzywanego obok.
- **Jak debugowac i logowac:**
  - Patrz zmienna globalna `ms_lpd3dDevice`. Jesli rzuca blady, to macierze sie wypinaja. 
  - W razie bledu Reset() uzywa metody konwersji znakow WideToUtf8 i zapisuje wyjatek D3D9 do glownego logu (metoda `TraceError`). 
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  Rzutowanie wektorow `ProjectPosition` / `UnprojectPosition` mozna latwo izolowac budujac mocki z API D3DX. Mozna zasymulowac czysta tablice dla wektorow, zdefiniowac `_D3D9_H_` i zmockowac `ms_matView` oraz `ms_matProj` macierzami Identity w `doctest`.
