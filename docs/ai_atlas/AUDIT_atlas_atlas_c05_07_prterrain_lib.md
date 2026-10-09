---
task_id: "atlas_c05_07_prterrain_lib"
cluster: "WLD"
module_name: "PRTerrainLib - Niskopoziomowy Silnik Teksturowania Terenu"
target_files:
- src/PRTerrainLib/Terrain.cpp
- src/PRTerrainLib/Terrain.h
- src/PRTerrainLib/TextureSet.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c05_07_prterrain_lib.md"
architecture_layer: "Swiat Gry, Teren, Kolizje i Nawigacja"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport AI: PRTerrainLib - Niskopoziomowy Silnik Teksturowania Terenu

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
**Funkcja modulu:** 
Modul `PRTerrainLib` (obejmujacy `CTerrainImpl` oraz `CTextureSet`) implementuje niskopoziomowe struktury danych i zarzadzanie pamiecia dla geometrii terenu, teksturowania, atrybutow kafelkow (fizyki) oraz efektow wodnych i cieni w grze Metin2. Przechowuje statyczne wymiary map (np. kafelki terenu, rozdzielczosci UV) oraz pozwala na wczytywanie map kolizji, wodnych, oraz map wysokosci bezposrednio z Wirtualnego Systemu Plikow (PackLib).

**Moment wywolania w petli gry:**
- **Inicjalizacja i Ladowanie:** Funkcje wejscia takie jak `LoadHeightMap`, `LoadAttrMap`, `RAW_LoadTileMap`, czy `CTextureSet::Load` sa wywolywane glownie podczas procesu ladowania swiata przez `CMapOutdoor` oraz `CAreaTerrain` w momencie wchodzenia na mape lub teleporacji.
- **Odpytywanie w trakcie symulacji (OnUpdate):** Odczyt wlasciwosci terenu jest przeprowadzany bardzo czesto przez klienta (np. przy poruszaniu uzytkownika, kalkulacji wysokosci z `GetHeightMapValue` lub odczytywaniu barier przy pomocy `ATTRIBUTE_BLOCK`).
- **Renderowanie (OnRender):** W trakcie rysowania odpytywany jest np. kolor cienia `GetShadowMapColor` w celu cieniowania postaci i geometrii na mapie z tekstury cieni.

**Przeplyw i cykl zycia danych:**
1. Alokacja zasobow nastepuje po utowrzeniu instancji `CTerrainImpl`.
2. Podsystemy wyzszych rzedow wolaja metody ladujace (np. `LoadAttrMap`).
3. Modul pobiera zasob przy uzyciu `CPackManager`, analizuje naglowek (struktury spakowane `SAttrMapHeader`, `SWaterMapHeader`), zabezpiecza rozmiary i sprawdza "Magic Numbers", po czym kopiuje dane do wewnetrznych tablic bajtowych (`m_abyAttrMap`, `m_awRawHeightMap` itd.).
4. `CTextureSet` parsuje pliki tekstowe (`.txt`) listujace tekstury, skale i ofsety i instancjonuje je poprzez `CResourceManager`.
5. Oczyszczenie w `Clear()`, wlacza `Release()` dla tekstur alpha i wola `Destroy()` na obrazach bazowych w `CTextureSet`.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
- Moduly wyzszego rzedu w systemie swiata: `CMapOutdoor`, `CAreaTerrain`, `CPythonBackground` (do odpytywania o atrybuty fizyczne terenow).
- Fizyka/Kolizje ruchu: `CInstanceBase`, `CPythonMiniMap` uzywaja stalych przestrzennych terenu do poprawnego lokalizowania jednostek oraz sprawdzania legalnosci ruchu czy atakowania uzytkownikow.

**Zaleznosci wyjsciowe (Outbound):**
- **VFS / Zasoby:** `PackLib::CPackManager` do otwierania zrodel bisekto/binarnych i odczytywania map bitych; `CResourceManager` do ladowania tekstur (GrpImageInstance).
- **DirectX 9 API:** API zwraca wprost surowe wskazniki `LPDIRECT3DTEXTURE9`. Wskazniki te uzywane sa do stanow renderowania terenu.

**Drzewo dyrektyw `#include`:**
- Typy terenow: `"TextureSet.h"`, `"TerrainType.h"`
- W `TerrainType.h`: `"EterLib/GrpVertexBuffer.h"`, `"EterLib/GrpIndexBuffer.h"` (zaleznosci wiazace graficzne zasoby geometrii na sztywno).
- W zrodlach `.cpp`: `"Stdafx.h"`, `"PackLib/PackManager.h"`, `<math.h>`

**Model pamieciowy:**
Jest to standard C++98/03 oparty glownie na bezposrednich, pre-alokowanych statycznie tablicach buforowych (`m_awRawHeightMap`, `m_abyAttrMap`), surowych wskaznikach interfejsow COM dla DirectX (`LPDIRECT3DTEXTURE9`), oraz stalych rozmiarach buforow wyrazonych w globalnych wyliczeniach `enum`. System jest ciezki ze wzgledu na rozrzut objetosci statycznych zasobow.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabele Klas i Struktur:**
| Nazwa (Klasa/Struct) | Rola i przeznaczenie | Rozmiar / Pola | Wlasciciel Watku |
| --- | --- | --- | --- |
| `CTerrainImpl` | Glowny model pamieciowy i implementacja terenu. Trzyma surowe wartosci w mapach statycznych | Ogromny rozmiar pamieciowy przez tablice (np. `m_awRawHeightMap` 131x131 * 2B, + atrybuty, + woda) | Brak restrykcji bezposrednio, ale wymaga uzycia w Main Thread (D3D). |
| `CTextureSet` | Zarzadca list tekstur i materialow nakladanych jako Splaty terenowe. | Standardowy kontener | Main Thread (wykorzystanie CResourceManager) |
| `TTerrainTexture` | Struktura dla pojedynczego materialu. | `stFilename`, `LPDIRECT3DTEXTURE9`, `UScale/VScale`, `UOffset/VOffset`, `bSplat`, transformacje macierzy UV. | Wewnetrzna obsluga tekstur. |

**Tabela Metod Publicznych (`CTerrainImpl`):**
| Sygnatura | Wartosc Zwracana | Funkcja i Skutki uboczne |
| --- | --- | --- |
| `SetTextureSet(CTextureSet*)` | `void` | Ustawia globalny statyczny zewnetrzny wskaznik `ms_pTextureSet`. Nie usuwa istniejacego, jesli NULL wpisuje pusty tekst. |
| `LoadAttrMap(const char*)` | `bool` | Wczytuje z pliku `.atr` do `m_abyAttrMap`, mapuje na `ATTRMAP_XSIZE x ATTRMAP_YSIZE`. Bezposrednio modyfikuje surowy bufor i weryfikuje naglowki. |
| `LoadWaterMap(const char*)` | `bool` | Wczytuje `.wtr`, modyfikuje `m_abyWaterMap` oraz tablice dlugosci wod `m_lWaterHeight`. Zwraca falsz, gdy naglowek uszkodzony. |
| `GetShadowMapColor(float, float)` | `DWORD` | Dokonuje konwersji ze wspolrzednych map na pozycje na surowej mapie cienia `m_awShadowMap` i tworzy maske cienia dla renderingu jako wartosc RGBA `DWORD`. |
| `GetHeightMapValue(short sx, short sy)` | `WORD` | Pobiera bezposrednio wpis ze skompresowanej tablicy (1D -> 2D) z pomoca wytycznych stalych rozdzielczosci wysokosci terenu. Brak walidacji wielkosci `sx/sy`. |

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
Atrybuty wykorzystuja operacje bitowe.
Flagi Atrybutow zdefiniowane w `enum`:
- `ATTRIBUTE_BLOCK = (1 << 0)` (1)
- `ATTRIBUTE_WATER = (1 << 1)` (2)
- `ATTRIBUTE_BANPK = (1 << 2)` (4) - dla stref anty-PVP w miastach.
Struktury naglowkow (w `LoadAttrMap` oraz `LoadWaterMap`) sa wylacznie dla FFI wyrownane przy uzyciu pragma pack(1) `struct SAttrMapHeader { WORD magic; WORD width; WORD height; }`.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Brak Modulow Pythona C-API bezposrednio w tej bibliotece**.
- Powiazania sa wykonywane na najwyzszym poziomie w modulach `CPythonBackground` i `CPythonMiniMap`. Atrybuty zdefiniowane tutaj `ATTRIBUTE_BLOCK` narzucaja poprawnosc pakietow serwera, zapobiegajac generacji pakietow `Walk` czy `Fly` na obszary z zablokowanym ruchem lub poza limitami mapy. Brak tutaj kodu sieciowego `PyMethodDef`.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady Wielowatkowosci:** Funkcje renderowania terenu i operacje ladowania (szczegolnie tworzenie tekstur alpha, tekstur zasobow) musza odbywac sie z jedynego glownego watku wspoldzielonego z DirectX API, poniewaz wywolywany jest `Release()` z `LPDIRECT3DTEXTURE9`. Ponadto `ms_pTextureSet` dziala jako nie synchronizowany zaden sposobem singleton statyczny dla wszystkich istniejacych blokow.
- **Pulapki Buforowe:** Dostep do tablic terenu (np. w funkcji `GetHeightMapValue`) nie korzysta z bound checkingu. Programista czy agent uzywajacy tych bindow musi upewnic sie, ze wprowadzane koordynaty (`sx, sy`) nie wykraczaja poza granice wielkosci narzuconych `TerrainType.h`.
- **Typowe Punkty Awarii:** Brak integralnosci w paczkach gry (.epk/.eix). Modul co prawda zglosi blad jesli "Magic Number" z pliku .wtr / .atr nie odpowiada np. `2634` i funkcja zwroci `false`, co pociaga ladowanie terenu za pomoca pustych/fail-safe danych i map (wtedy teren z reguly ma zerowa wysokosc/kolizje). 
- **Zarzadzanie zasobami (RAII):** Kod mocno polega na bezposrednich zrzutach C-array pamieciowych i wyczyszczeniach interfejsow DirectX bez wlasciwych standardow nowoczesnego RAII. 

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
**Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
Jesli potrzebujesz dodac nowa mape do kafelka (np. PBR Roughness):
1. Zadeklaruj wielkosc mapy w `CTerrainImpl` w sekcji `enum` (np. `ROUGHNESSMAP_XSIZE`).
2. Dodaj statyczny array `BYTE m_abyRoughnessMap[...]` lub analogiczny pointer.
3. Dodaj nowa metode ladowania `LoadRoughnessMap` - zastosuj parsowanie tak jak dla `LoadAttrMap` zabezpieczajac paczke naglowkiem np. `SRoughMapHeader`.
4. Wykorzystaj ten nowy bufor w wyzszym module (np. pobieranie do Buffora Graficznego podczas przesylania na GPU).

**Jak debugowac i logowac:**
Logowanie odbywa sie tu standardowym macro z `TraceError` lub `Tracef` – uwazaj, by w petlach (`GetShadowMapColor`) unikac tych wywolan, gdyz moga zapchac I/O rejestru logowania.
W punkcie Breakpoint na pobraniu `m_lWaterHeight` analizowac mozna indeksy tablic dla narzuconej ilosci zbiornikow w ukladzie koordynat.

**Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
Aby przetestowac parsing danych, mozna w izolacji (poza gra) napisac mocka na `PackManager::Instance().GetFile`, tak, aby podawal wektor wlasnych recznie preparowanych bajtow symulujacych plik `.atr` ze wlasciwymi polami magic. Unika sie wtedy pelnego C++ RHI (Render Hardware Interface) by zweryfikowac, ze `m_abyAttrMap` posiada wygenerowane z bitow atrybuty. Wystarczy pominac wlaczanie obslugi plikow tekstur, albowiem moga powodowac wysypy jesli nie mockowany DX9 jest aktywny.
