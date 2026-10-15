---
task_id: "atlas_c05_03_area_terrain_mesh"
cluster: "WLD"
module_name: "CAreaTerrain - Siatka Wysokosciowa i Tekstury Podloza"
target_files:
- src/GameLib/AreaTerrain.cpp
- src/GameLib/AreaTerrain.h
- src/GameLib/Area.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c05_03_area_terrain_mesh.md"
architecture_layer: "Swiat Gry, Teren, Kolizje i Nawigacja"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport Audytu: CAreaTerrain i CArea - Siatka Wysokosciowa, Tekstury Podloza i Obiekty Mapy

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Moduly `CAreaTerrain` (CTerrain) i `CArea` w bibliotece `GameLib` stanowia fundament renderowania i zarzadzania swiatem 3D w kliencie gry. 
Odpowiadaja za wczytywanie terenu, siatek wysokosciowych, tekstur, modeli drzew (SpeedTree), efektow dzwiekowych (Ambience) i graficznych, obiektow statycznych oraz budowli na poszczegolnych obszarach mapy.

### CAreaTerrain (CTerrain)
Odpowiada za reprezentacje siatki terenu w 3D, wysokosci (HeightMap), wektorow normalnych (NormalMap), atrybutow kolizji i bezpiecznych stref (AttrMap) oraz obszarow wodnych.
- **Funkcja:** Wczytuje i buforuje z dysku (pliki RAW i .msh/.mdat) uksztaltowanie terenu, z uwzglednieniem alpha-blendingu dla przejsc miedzy warstwami roznych rodzajow podloza (trawa, skala, snieg, piasek) tzw. "splatting".
- **Petla gry:** Posiada metody renderujace wywolywane z klasy wlasciciela (CMapOutdoor) w fazie `OnRender` (renderowanie patch'y terenu, minimapy i wody).
- **Control & Data Flow:** CMapOutdoor deleguje ladowanie konkretnych kwadratow mapy (sektorow) do `CTerrain`. Klasa mapuje wysokosci pikseli (0-255) z pliku raw uzywajac wlasciwego mnoznika (`m_fHeightScale`) i zapisuje obliczone wierzcholki 3D w pamieci karty graficznej uzywajac D3D9 Vertex Bufferow.

### CArea
Zarzadza wszystkim, co na tym terenie "stoi" lub sie "dzieje" w sensie statycznym lub stacjonarnym.
- **Funkcja:** Odpowiada za obiekty na mapie (budynki, drzewa ze SpeedTree, DungeonBlocki), zrodla dzwieku przestrzennego (Ambience) oraz stacjonarne efekty czasteczkowe.
- **Petla gry:** Metody `Update()` i `Render()` sa wywolywane per-klatka z CMapOutdoor. Update deformuje modele animowane i odswieza efekty dzwiekowo/wizualne na bazie pozycji bohatera. Render sortuje i wyswietla wlasciwe instancje modeli do backbuffera D3D9.
- **Zarzadzanie pamiecia:** Uzywa pre-alokowanych `CDynamicPool` dla kazdego typu objektu co minimalizuje koszty realokacji podczas przechodzenia przez granice sektorow (seamless map loading). Alokacja nastepuje po wejsciu gracza do sektora, czyszczenie i zwalnianie `Clear_DestroyObjectInstance()` ma miejsce przy opuszczaniu/rozlaczaniu.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

### Zaleznosci wejsciowe (Inbound):
- **Wywolania przez:** Glownie instancje `CMapOutdoor`, obsluge watku ladowania mapy w tle (`AreaLoaderThread`), oraz zewnetrzne narzedzia klienta przez `CMapManager`.
- **Systemy gry:** Pathfinding/Nawigacja i wykrywanie kolizji gracza wykorzystuja funkcje pobierajace `Height` z CTerrain oraz atrybuty obszarow (np. czy teren to woda albo blokada).

### Zaleznosci wyjsciowe (Outbound):
- **DirectX 9 (D3D9):** Obfite wykorzystanie interfejsow `IDirect3DTexture9`, staniel renderowania (np. AlphaBlendEnable), buforow wierzcholkow per patch uzywajacych formatow `A8R8G8B8` itp.
- **Zasoby, EterLib i Systemy Trzecie:** `CResourceManager`, `SpeedTreeForestDirectX` (renderowanie lasow/drzew), `CEffectManager` (efekty graficzne typu ogniska statyczne), `SoundEngine` (obsluga dzwiekow `Ambience`).
- **Granny 3D:** Renderowanie i kolizje obiektow zdefiniowanych jako Building przez strukture `CGraphicThingInstance`.

### Drzewo dyrektyw `#include`:
- Silne zaleznosci od `PRTerrainLib/Terrain.h` dla interfejsow ladowania map, oraz pliki EterLib: `EterLib/ResourceManager.h`, `EterLib/StateManager.h` (zarzadzanie pamiecia i stany D3D).
- Wlaczenie `PackLib/PackManager.h` wskazuje na ladowanie zawartosci wprost ze spakowanych archiwow gry (np. `shadowmap.raw`).

### Model pamieciowy:
- `CDynamicPool` dla kluczowych instancji: `ms_kPool` w CTerrain oraz obiektow w CArea, tj. `TObjectInstance`, `CAttributeInstance`. Zwieksza bezpieczenstwo i redukuje fragmentacje sterty.
- Nagminne wykorzystanie czystych wskaznikow C np. `CGraphicThingInstance*` i `CEffectInstance*`, brak nowoczesnych `std::shared_ptr` co obniza odpornosc na wycieki przy zlej implementacji dealokacji. Nalezy bezwzglednie pamietac o recznych wywolaniach `.Release()` lub powiazanych w `Clear`.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabela Klas i Struktur

| Nazwa | Rola | Wlasciciel watku | Uwagi |
| --- | --- | --- | --- |
| `CTerrain` | Implementuje bazowy wezel obszaru terenu 3D z siatka, splatami, woda i kolizjami wysokosciowymi. | Watki: Glowny i Ladowania Tla | Dziedziczy po `CTerrainImpl` i `CGraphicBase`. |
| `CArea` | Pojemnik na wczytane instancje obiektow stojacych na terenie (Modele, Efekty, Ambience, Drzewa). | Glowny D3D | Rozmiar obszaru zazwyczaj odpowiada sektorowi 256x256 metrow (25600x25600 jednostek enginu). |
| `CArea::TObjectData` | Definicja "Struktury Zrodlowej" odczytywanej z plikow `AreaData.txt` (Pozycja XYZ, Rotacja, Yaw, CRC). | Glowny / Ladowania Tla | Statyczny opis (Blueprint). |
| `CArea::TObjectInstance` | Reprezentacja wygenerowanego obiektu na scenie, uzywa konkretnego `dwType` (Drzewo, Budynek, Efekt). | Glowny D3D | Dynamiczny model pamieci (runtime). |
| `CArea::TAmbienceInstance`| Odtwarzacz dzwiekow pozycyjnych 3D z wlasciwoscia odleglosci `dwRange`. | Glowny | Uzywa fmod / Miles / SoundEngine do zapetlen (Loop, Step, Once). |

### Tabela Metod Publicznych

| Metoda (`CTerrain` i `CArea`) | Sygnatura | Zwraca | Skutki uboczne / Dzialanie |
| --- | --- | --- | --- |
| `CTerrain::GetHeight` | `float GetHeight(int x, int y)` | Wysokosc (Z) jako `float` | Szybko interpoluje wysokosc z siatki bazujac na swiatowych koordynatach X,Y. Czesy watek glowny. |
| `CTerrain::isAttrOn` | `bool isAttrOn(WORD wCoordX, WORD wCoordY, BYTE byAttrFlag)` | `true`/`false` | Odczytuje mapy Attr, np. sprawdzajac kolizje stref non-pvp i barier. |
| `CTerrain::RAW_GenerateSplat` | `void RAW_GenerateSplat(bool bBGLoading = false)` | `void` | Buduje alfa-tekstury w D3D9 na bazie tablic `m_abyTileMap`, zastepujac poprzednie wydania starych pamieci. Zabezpieczenie przed przepelnieniem na VRAM. |
| `CArea::Load` | `bool Load(const char * c_szPathName)` | Sukces (bool) | Wczytuje obiekty i Ambience. |
| `CArea::Render` | `void Render()` | `void` | Renderuje modele statyczne na mapie z odrzucaniem nie-widocznych i zlewaniem (blending/opaque). |

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
W `CArea::TObjectInstance`:
- `DWORD dwType;` - okresla podklase pointera. Offset +0x0.
- `CAttributeInstance * pAttributeInstance;` - offset +0x4/0x8 (zalezne od arch).
- `CSpeedTreeForest::SpeedTreeWrapperPtr pTree;` (zalezy od rozmiaru pamieci kompilatora), i `CGraphicThingInstance * pThingInstance`. Zle przypisanie pamieci zniszczy stabilnosc klienta! Uzywac narzedzia Type dla rzutowania.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

W tym module brakuje eksportowania do Pythona per-se ani wlasnych obslug pakietow. Te klasy to czysta warstwa silnika, zarzadzajaca danymi na dysku lub otrzymywanymi juz od procesora (z reguly z posrednika `CPythonBackground`).
- Skrypty Python (`background.py` itp.) uzywaja modulu `background` w `PythonBackgroundModule.cpp`, ktory nakazuje `CMapOutdoor` odswiezyc terrain/area, wiec API to dziala tylko poprzez proxy.
- Opcody pakietow: Same tereny nie sa przesylane siecia z pakietow GC (z wyjatkiem np. pakietow o zmianie srodowiska np. dzwon nocny/dzien, zmiany pory roku `HEADER_GC_ENVIRONMENT` badz zmiany terenu snieznego, choc samo zarzadzanie fizyka pozostaje lokalne w GameLib).

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

### Zasady wielowatkowosci:
- Proces ladowania RAW (np. `RAW_LoadTileMap`) moze wykonac sie w watku tla (BGLoading), by gracze nie uswiadczyli "zacinania sie" mapy. Ale operacje wykorzystujace metody takie jak `ms_lpd3dDevice->CreateTexture` (widziane w `CTerrain::AddTexture32` i `AllocateMarkedSplats`) wylacznie w systemach bez sprzetowych ograniczen mogly byc wywolywane zewnetrznie. Upewnij sie ze modyfikacje D3D z Tla maja wlaczony parametr `D3DCREATE_MULTITHREADED`, w przeciwnym razie nastapi awaria (Device Lost lub CRASH).

### Potencjalne punkty awarii (Crash Points & Edge Cases):
1.  **Wyciek tekstur alpha:** `CTerrain::RAW_GenerateSplat` wielokrotnie release'uje tekstury. Niedoskonale obsluzenie `ulRef > 0` na zablokowanych interfejsach D3D spowoduje wyciek VRAM'u.
2.  **Splatting na granicy Patchow:** Bedy bufora pamieci mogly sie pojawiac w iteracjach granicznych. W `CTerrain::RAW_CountTiles()` operatory uzywaja `std::max` i `std::min` w celu bezpieczenstwa - usuniecie ich zniszczy program przy granicy ladowanego sektora.
3.  **Ambience Volume:** Brak sprawdzenia granic na wektorach dzwieku. Upewnij sie, ze wektor dzwiekow nie jest pusty (`AmbienceData.AmbienceSoundVector.empty()`) przed jego referencjonowaniem `[0]`.

### Zarzadzanie zasobami (RAII):
Brak RAII dla wskaznikow COM, takich jak `m_lpAlphaTexture` czy `m_lpMarkedTexture`. Wymagane jet reczne zwalnianie. Jesli agent wprowadza modyfikacje, powinnien brac to pod uwage badz z refaktorowac kod do inteligentnych COM pointerow np. `Microsoft::WRL::ComPtr`. Obecnie polega to recznym `->Release()`. Pule `CDynamicPool` pomagaja unikac wyciekow dla wewnetrznych elementow C++, pod warunkiem ze funkcja `.Clear()` na instancji dziala poprawnie.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

### Instrukcja dodawania nowej funkcji (Step-by-step extension guide):
Jesli np. musisz wprowadzic nowy atrybut do terenu w `AttrMap` lub nowy typ interaktywnego obiektu na `CArea`:
1.  Zmodyfikuj enum typow property w `PRTerrainLib` / `Property.h` i odpowiednio `GameLib/Area.cpp` (`__SetObjectInstance`).
2.  Zaprogramuj parsowanie nowych parametrow w `__Load_LoadObject`.
3.  Rozszerz `TObjectInstance`, aby uwzglednic nowy typ podzespolu, i nie zapomnij zmodyfikowac `.Clear()`, `__Clear_DestroyObjectInstance` i w konstruktorze.
4.  W zadnym wypadku nie usun starego mechanizmu CRC (na jego podstawie pobierana jest pelna konfiguracja z `CPropertyManager`). Wszelkie formaty plikow `.txt` uzywaja parsowania wiazanek i szukaja stringow w mapie.

### Jak debugowac i logowac:
- Klasy opieraja sie na makrach `Tracef` oraz `TraceError`. Aby weryfikowac, uzywaj plikow `syserr.txt` (ktore zapisza `TraceError`) z uzytecznymi stringami, takimi jak: `TraceError("CArea::SetBuilding: There is no data: %s", Data.strFileName.c_str());`
- Jesli `Render` nie wczytuje Twojego objektu, wlacz breakpoint w sekcji kompilujacej `m_kRenderedGrapphicThingInstanceVector` - prawdopodobnie funkcja `.isShow()` lub widocznosc frustum ukryla go na stalej odleglosci `ViewRadius`.

### Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):
- W przypadku pisania testow dla `GetHeight` i `WE_GetHeightMapValue`, nalezy calkowicie wylaczyc `StdAfx.h` (uzywajac parametru srodowiskowego `#ifndef TEST_MODE_DISABLE_STDAFX`).
- Utworz recznie obiekt `CTerrain::New()`, wykreuj pusta macierz uzywajac std::vector dla wlasnych testow `m_awRawHeightMap` - poniewaz klasa odnosi sie do plikow przez `CPackManager`, do testow izlowanych musisz zaslepic system pakowania podmieniajac go fasadowym modelem ktory zwroci pre-definiowany bitset do funkcji wczytujacej mape uzywajac interfejsu ladowania C++.
