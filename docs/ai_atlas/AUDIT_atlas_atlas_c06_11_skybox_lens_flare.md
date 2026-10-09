---
task_id: "atlas_c06_11_skybox_lens_flare"
cluster: "RND"
module_name: "CSkyBox i Efekty Atmosferyczne Nieba"
target_files:
- src/EterLib/SkyBox.cpp
- src/EterLib/SkyBox.h
- src/EterLib/LensFlare.cpp
- src/EterLib/LensFlare.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c06_11_skybox_lens_flare.md"
architecture_layer: "Renderowanie, Direct3D 9, Shading i Culling"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Modul odpowiedzialny za zarzadzanie i renderowanie kopuly nieba (SkyBox) oraz efektu blasku slonca (Lens Flare). `CSkyBox` obsluguje wielokatne sciany nieba (z teksturami lub kolorami gradientowymi), animowane chmury (scrollowane tekstury). `CLensFlare` symuluje fizyczny efekt rozproszenia swiatla w obiektywie kamery, obliczajac widocznosc slonca na podstawie kierunku swiatla i rysujac glowne "slonce" (Main Flare) oraz wtorne refleksy (CFlare).
- **Punkt wywolania (Game Loop):** Metody `Update()` sa wywolywane w glownym OnUpdate (odswiezanie pozycji chmur, rotacji nieba wzgledem kamery, oraz widocznosci Lens Flare). Renderowanie nastepuje po narysowaniu nieprzezroczystych modeli w fazie OnRender (Render, RenderCloud, DrawBeforeFlare, DrawFlare).
- **Przeplyw danych (Data Flow):** 
  - W `CSkyBox`: Parametry (kolory, skale, tekstury chmur) sa ustawiane z zewnatrz (np. CEnvironment, parser atlasu). Metoda `Refresh` generuje werteksy dla 6 scian oraz warstwy chmur (tworzac CSkyObjectQuad). `Render` aplikuje stany DirectX i rysuje uzywajac DrawPrimitiveUP lub rysuje prymitywy z pomoca managera.
  - W `CLensFlare`: Otrzymuje kierunek swiatla (Compute). Projekcja w przestrzeni 2D pozwala na ustalenie, czy slonce jest na ekranie. Lens Flare skaluje jasnosc w zaleznosci od centrum ekranu i widocznosci, a nastepnie rysuje elementy flary wzdluz wektora slonce-srodek ekranu.
- **Cykl zycia (Lifecycle):** Inicjalizowane raz na zaladowanie mapy lub warunkow srodowiska (np. ladowane przez skrypty). Alokuja tekstury (`CGraphicImageInstance`) poprzez EterLib/ResourceManager. Destruktor zwalnia instancje tekstur. W CLensFlare uzywana jest wektor `CFlare::SFlarePiece` z zaalokowanymi recznie obrazkami.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):**
  - Wywolywane przez: Klasy srodowiska (`CMapOutdoor`, `CEnvironment`, `CPythonBackground`), ktore aktualizuja widocznosc, przelaczaja warunki pogodowe i renderuja horyzont.
- **Zaleznosci wyjsciowe (Outbound):**
  - **DirectX 9 / Statemanager:** Modul korzysta mocno z globalnego `STATEMANAGER` (`STATEMANAGER.SaveRenderState`, `STATEMANAGER.DrawPrimitiveUP`).
  - **EterLib:** `CGraphicImageInstance`, `CResourceManager`, `CCameraManager`, `CTimer`, `CColorTransitionHelper`.
- **Drzewo dyrektyw `#include`:** Dolacza "GrpBase.h", "GrpScreen.h", "GrpImageInstance.h", "ColorTransitionHelper.h", "Camera.h", "StateManager.h", "ResourceManager.h". Nie ma tu narazenia na bezposrednie Python C-API (czysty kod renderujacy C++).
- **Model pamieciowy:** W C++ `CLensFlare` korzysta ze std::vector do wewnetrznego trackingu `SFlarePiece` alokowanych przez operator `new`. Tekstury sa zarzadzane w `CSkyBox` przez inteligentny wrapper na wskaznik (instancja GraphicImageInstance) z wlasnym systemem refcount w ResourceManagerze. Istnieje mapa `TGraphicImageInstanceMap`.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
- `CSkyBox` (dziedziczy po `CSkyObject` po `CScreen`) - Wlasciciel: Main Thread (Render). Rozmiar: Sredni (wiele macierzy, kolekcji). Role: zarzadzanie bryla nieba i gradientami pory dnia.
- `CSkyObjectQuad` - Role: Reprezentuje sciane lub jej czesc z 4 werteksami uzytkownika i buforem indeksow. Zawiera narzedzia do tranzycji koloru.
- `CLensFlare` (dziedziczy po `CScreen`) - Wlasciciel: Main Thread. Role: Obliczenia swiatla i rendering "bliku" na ekranie.
- `CFlare` - Role: Renderuje zestaw pod-refleksow ("duszkow" flary).

**Tabela Metod Publicznych:**
- `CSkyBox::Refresh()` - Brak argumentow, generuje podzial scian, pre-condition: parametry gradientu ustawione.
- `CSkyBox::SetSkyColor(const TVectorGradientColor & c_rColorVector, ...)` - Inicjuje tranzycje koloru dla calej bryly. Skutki: ustawienie wartosci w helperach.
- `CSkyBox::Update() / Render() / RenderCloud()` - Update przesuwa koordynaty tekstur chmur (scroll), Render pcha rysowanie.
- `CLensFlare::Compute(const D3DXVECTOR3 & c_rv3LightDirection)` - Skutki: wyliczenie pozycji na ekranie, kierunku, testu backface dla slonca. Zmienia wewnetrzne parametry m_fBeforeBright/m_fAfterBright.
- `CLensFlare::DrawBeforeFlare()` - Rysuje "Main Flare" w trybie ADD (SrcAlpha/InvSrcAlpha) z wylaczonym Depth.
- `CLensFlare::DrawFlare()` - Rysuje wtorne refleksy (CFlare::Draw).

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
W `CSkyBox` wazne obiekty:
`m_FaceCloud` - zarzadza wielokatem dla chmur. `m_Faces[6]` - 6 scian pudla.
W `CLensFlare`:
`float m_afFlarePos[2]` - wspolrzedne flary (0.0 - 1.0).
`CFlare m_cFlare` - podklasa dla renderowania kaskady obrazkow flarowych (flare2.dds, flare1.dds itd.).

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
Brak bezposrednich powiazan z siecia (pakiety GC/CG). 
Dane o porze dnia, kolorze nieba czy naslonecznieniu sa wysylane przez serwer (Event, Time) lub konfigurowane skryptowo w PyBackground, ale ten modul przyjmuje je juz w postaci typow EterLib (TColor, D3DXVECTOR3). To samo dotyczy sciezki ladowania zasobow z VFS.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Cale renderowanie w `SkyBox` i `LensFlare` MUSI byc wywolywane z glownego watku, ktory "trzyma" kontekst D3D. Update moze byc wywolane asynchronicznie, o ile nie uzywa StateManagera. Zmiany stanow D3D (np. w Render) odbywaja sie natychmiastowo.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Null checki podczas ladowania: Funkcja `SetFaceTexture` i `SetCloudTexture` korzysta z `GenerateTexture`. Brak zasobu moze wywolac assert lub zwrocic NULL, co doprowadzi do crasha, jesli nie zabezpieczono mapy.
  - Zle ustawienie macierzy rzutowania (Projection) moze znieksztalcic Flare. Nalezy korzystac z poprawnych funkcji `SaveTransform` i `RestoreTransform` `STATEMANAGER`a.
- **Zarzadzanie zasobami (RAII):** Zasoby zwalniane prawidlowo (`DeleteTexture` w SkyBox), `CFlare` musi iterowac std::vector i dealokowac `SFlarePiece` z operatorem `delete` – uwaga, w pliku brak definicji destruktora w CFlare iterujacego po wektorze w celu usuniecia pamieci. Sugeruje to *potencjalny niewielki wyciek pamieci* (`m_vFlares.push_back(pPiece)` bez sprzatania w `~CFlare()`). Wymaga podgladu w realnym patchu / C++11 unique_ptr. (W kodzie brakuje recznego delete z `m_vFlares` w destruktorze `~CFlare()`!).

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  Aby dodac nowa sciane nieba (np. wielokat do sferycznego modelu skydome):
  1. Zamiast tablicy statycznej `m_Faces[6]` w CSkyBox, uzyj std::vector<TSkyObjectFace>.
  2. W `Refresh()` wygeneruj siatke sfery zamiast 6 scianek quad-pudla.
  3. Zmodyfikuj algorytm `SetSkyColor` by aplikowal gradient na kule pionowo.
- **Jak debugowac i logowac:**
  Podczas analizy blasku flary, sledz zmienna `m_bFlareVisible` w `Compute()`. Wypisanie fDotProduct ulatwia sprawdzenie bledu Field-of-View (FOV). `CLensFlare` zostal dostosowany do Direct3D przez zastapienie archaicznego OpenGL, ale zachowano komentarze takie jak `// glDisable(GL_DEPTH_TEST);`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  W EterLib mozesz skompilowac moduly podpiete pod atrapy `STATEMANAGER`. Poniewaz moduly te wywoluja glownie metody `SaveRenderState` czy `DrawPrimitiveUP`, uzyj frameworku (np. doctest z mockami D3D), zeby sprawdzic cykl `Update()` dla ruchu chmur (`m_fCloudPositionU`) polegajacy na czasie (`CTimer::Instance().GetCurrentMillisecond()`).
