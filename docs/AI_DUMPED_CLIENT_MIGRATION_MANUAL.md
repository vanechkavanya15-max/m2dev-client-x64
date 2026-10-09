# PODRECZNIK MIGRACJI ZRZUTU KLIENTA (DUMPED CLIENT MIGRATION MANUAL) DLA AGENTOW AI

> Dokument jest glownym zrodlem wiedzy (Single Source of Truth) dla wszystkich agentow AI (Antigravity, Jules Cloud Workers, subagenci badawczy oraz inzynierowie) realizujacych zadanie pelnej migracji i dostosowania zasobow ze zrzutu dowolnego klienta gry (np. `E:\Alune-AkademiaDUMP` lub rozpakowanych archiwow Ymir) do zmodernizowanego silnika klienta x64 C++23 / glTF 2.0 (`E:\m2dev-client-src-mainOryginalx64`).

---

## 1. Wprowadzenie i Architektura Docelowa

### 1.1. Cel Migracji
Tradycyjne zrzuty klientow Metin2 opieraja sie na przestarzalych technologiach z lat 2004-2014:
1. Zamknieta, 32-bitowa biblioteka binarna Granny 3D (`.gr2` v2.4/v2.9).
2. Przestarzaly interpreter Python 2.7 (kod ASCII, brak Type Hints, brak ochrony pamieci).
3. Pakiety binarne LZO/Tea (`.eix` / `.epk`) o sztywnych limitach.
4. Binarne bazy prototypow `item_proto` i `mob_proto` ze zmiennym szyfrowaniem FourCC/XTEA.

Nasz zmodernizowany klient (`E:\m2dev-client-src-mainOryginalx64`) dziala w oparciu o:
1. **Natywna architekture x64 C++23** zoptymalizowana pod nowoczesne procesory (AVX2/AVX-512, SIMD culling, Lock-free ring buffery).
2. **Nowoczesny silnik graficzny glTF 2.0 Binary (`.glb`)** w bibliotece `EterModelLib` z pelem CPU skinningiem, obsluga kosci, animacji, wag wierzcholkow oraz zdarzen walki (`metin2_events`).
3. **Transparentny Dual-Loader i Automatyczny Fallback (`.gr2` <-> `.glb`)** w `CPackManager`: silnik gry wczytuje `.glb` nawet jesli gra prosi o `.gr2` ze starych plikow konfiguracyjnych!
4. **Interpreter Python 3.14** z mechanizmem `PyBridgeFastCall` oraz scisla walidacja typow.
5. **Zintegrowany system renderowania Direct3D 9** z dynamicznym menedzerem stanow `CStateManager` i buforowaniem wierzcholkow.

---

## 2. Topologia i Anatomia Zrzutu Klienta Gry (Dumped Client Layout)

Standardowy pelny zrzut klienta gry (np. `E:\Alune-AkademiaDUMP` z ponad 30 000 plikow) zawiera nastepujace glowne obszary zasobow:

```
[Katalog Zrzutu / E:\Alune-AkademiaDUMP]
│
├── monster/ , monster2/ , moby/     ──► Modele, animacje i konfiguracje potworow oraz bossow
├── npc/ , npc2/ , npc_mount/ , pet/ ──► Postacie niezalezne, wierzchowce, zwierzaki
├── pc/ , pc2/                       ──► Modele cial wojownika, assassina, sury, szamana (oraz wilkolaka)
├── weapon/ , atreya_weapons/ ...   ──► Modele broni 3D (miecze, luki, sztylety, dzwony, wachlarze)
├── sash/ , wings/                   ──► Szarfy, skrzydla i akcesoria plecowe (modele szkieletowe)
├── zone/ , tree/ , obiekty/         ──► Obiekty otoczenia 3D (budynki, drzewa, mosty, skaly)
├── terrainmaps/ , metin2_map_*      ──► Mapy gry (wysokosci .raw, woda, tekstury podloza, server_attr)
├── effect/ , special_effects/       ──► Efekty czasteczkowe (.mse) i siatki efektowe (.mde)
├── property/                        ──► Wpisy definicji obiektow mapy (.prb, .prd, .prt)
├── sound/ , bgm/                    ──► Efekty dzwiekowe (.wav) oraz sciezki muzyczne (.mp3)
├── uiscript/ , root/                ──► Skrypty interfejsu (.py) i zarzadzania faza gry
└── locale/                          ──► Tlumaczenia, questy, bazy item_proto i mob_proto
```

### Mapowanie Sciezek Wirtualnego Systemu Plikow (VFS)
Klient gry odwoluje sie do zasobow za pomoca wirtualnych sciezek Ymir:
- `d:/ymir work/monster/alune_ork_mik24/alune_ork_mik24.msm`
- `d:/ymir work/pc/warrior/action/00.gr2`
- `d:/ymir work/ui/game/windows.dds`

Mechanizm normalizacji w `CPackManager::NormalizePath` zamienia ukosniki na `/` i konwertuje sciezke do malych liter. Dzieki temu zasoby moga byc serwowane bezposrednio z dysku lub z archiwow pakietow.

---

## 3. Podsystem 1: Konwersja Modeli i Animacji 3D (.gr2 -> .glb)

### 3.1. Zasada Dzialania Narzedzia `gr2_to_glb.exe`
Lokalizacja narzedzia: `E:\m2dev-client-src-mainOryginalx64\build\tools\gr2_to_glb\Release\gr2_to_glb.exe`

Narzedzie przeprowadza bezstratna translacje struktur Granny do standardu glTF 2.0:
1. **Siatki Deformowalne (Skinned Meshes)**:
   - Odczytuje wierzcholki typu `GrannyPWNT3432VertexType` (Pozycja 3x float, Wagi 4x u8, Indeksy kosci 4x u8, Normalne 3x float, UV 2x float).
   - Generuje atrybuty glTF: `POSITION`, `NORMAL`, `TEXCOORD_0`, `JOINTS_0`, `WEIGHTS_0`.
2. **Siatki Sztywne (Rigid Meshes)**:
   - Odczytuje wierzcholki typu `GrannyPNT332VertexType` (Pozycja 3x float, Normalne 3x float, UV 2x float).
   - Generuje atrybuty glTF: `POSITION`, `NORMAL`, `TEXCOORD_0`.
3. **Indeksy Trojkatow**:
   - Bezstratnie kopiowane przez `GrannyCopyMeshIndices` do bufora `INDICES`.
4. **Hierarchia Szkieletu (Skeletons & Bones)**:
   - Ekstrakcja nazw kosci, indeksow rodzicow (`parentIndex`) oraz macierzy `inverseBindMatrices` (konwersja ukladu wspolrzednych prawoskretnego/lewoskretnego).
5. **Probkowanie Animacji**:
   - Probkowanie krzywych animacji za pomoca natywnego API `GrannyEvaluateCurveAtT` w krokach 30 FPS / 60 FPS.
   - Generowanie kanalow animacji glTF: translacje (LERP), rotacje kwaternionow (SLERP) i skale.
6. **Ekstrakcja Zdarzen Walki (Metin2 Events)**:
   - Odczyt `TextTracks` z Granny i serializacja do JSON `extras.metin2_events`:
     `[{"time": 0.35, "type": "ATTACK"}, {"time": 0.50, "type": "SOUND", "arg": "sound/pve/hit.wav"}]`.
7. **Pobieranie Tekstur**:
   - Pobieranie sciezek rzeczywistych tekstur z `GrannyGetMaterialTextureByType(mat, GrannyDiffuseColorTexture)->FromFileName`.
   - Zapisanie sciezki do `extras.diffuse_texture` oraz `extras.opacity_texture`.

### 3.2. Masowa Konwersja Wsadowa
Lokalizacja skryptu: `E:\m2dev-client-src-mainOryginalx64\tools\batch_convert_gr2_to_glb.py`

#### Uruchomienie konwersji pojedynczego katalogu:
```powershell
python tools/batch_convert_gr2_to_glb.py --dir "E:\Alune-AkademiaDUMP\monster" --threads 16
```

#### Kluczowe flagi skryptu:
- `--dir`: Sciezka do katalogu wejsciowego z plikami `.gr2`.
- `--outdir`: Opcjonalny katalog wyjsciowy (jesli brak, pliki `.glb` tworzone sa bezposrednio obok odpowiadajacych im `.gr2`).
- `--threads`: Liczba watkow roboczych (zalecane: 8 do 16 dla optymalnej przepustowosci I/O).
- `--overwrite`: Wymuszenie ponownej konwersji istniejacych plikow `.glb`.

---

## 4. Podsystem 2: Mechanizm Transparentnego Fallbacku w Silniku Gry

Dzieki modyfikacji w `CPackManager` oraz `CResourceManager`:
1. **Zero zmian w plikach konfiguracyjnych**: Pliki `.msm`, `.msa`, skrypty Pythona moga nadal odwolywac sie do `plik.gr2`.
2. **Kolejnosc wyszukiwania w `CPackManager::GetFileWithPool`**:
   - Krok 1: Sprawdzenie czy istnieje plik pod zadana sciezka (w archiwach `.epk` lub na dysku).
   - Krok 2: Jesli zadana sciezka konczy sie na `.gr2`, a plik nie istnieje, silnik sprawdza czy istnieje `sciezka.glb`.
   - Krok 3: Jesli zadana sciezka konczy sie na `.glb`, a plik nie istnieje, silnik sprawdza czy istnieje `sciezka.gr2`.
3. **Rejestracja Typow w `CResourceManager`**:
   - `gr2`, `glb` oraz `gltf` sa zarejestrowane jako zasoby `CGraphicThing`.
4. **Detekcja w `CGraphicThing::OnLoad`**:
   - Silnik sprawdza pierwsze 4 bajty wczytanego bufora pamieci.
   - Jesli bufor zaczyna sie od `glTF` (magiczne bajty `0x46546C67`) lub `{ "asset"`, model jest natychmiast kierowany do parsera `CGltfModel` i instancji `CGltfModelInstance`.
   - W przeciwnym razie silnik uzywa starego parsera Granny.

---

## 5. Podsystem 3: Konfiguracja Postaci, Ras i Animacji (.msm, .msa, motlist.txt)

Podczas przenoszenia mobow, npc i postaci nalezy zwrocic uwage na spojna triade plikow konfiguracyjnych:

### 5.1. Pliki `.msm` (Model Script Motion)
Definiuja czesci ciala i podpinanie materialow:
```
ScriptType            RaceDataScript
BaseModelSetting
{
    ShapeIndex        0
    Model             "d:/ymir work/monster/alune_ork_mik24/alune_ork_mik24.gr2"
    SourceSkin        "d:/ymir work/monster/alune_ork_mik24/orc_black.dds"
    TargetSkin        "d:/ymir work/monster/alune_ork_mik24/1.dds"
}
```
*Zasada dla agenta AI*: Nie nalezy zmieniac rozszerzenia `.gr2` na sile, poniewaz transparentny fallback w silniku gry automatycznie zaladuje `alune_ork_mik24.glb`!

### 5.2. Pliki `.msa` (Motion Script Animation)
Definiuja parametry czasowe, zasiegi oraz eventy uderzen:
```
ScriptType            MotionData
MotionFileName        "d:/ymir work/monster/alune_ork_mik24/31.gr2"
MotionDuration        1.400000
Accumulation          0.00   -120.00   0.00

Group MotionEventData
{
    MotionEventDataCount     1
    Group Event00
    {
        MotionEventType      1
        StartingTime         0.466667
        AttackingEnable      1
        AttackType           0
        HittingType          1
        SphereDataCount      1
        Group SphereData00
        {
            Radius           80.000000
            Position         0.000000 70.000000 60.000000
        }
    }
}
```
*Zasada dla agenta AI*: Czas trwania `MotionDuration` w pliku `.msa` musi byc zgodny z czasem w animacji `.glb` (narzedzie `gr2_to_glb.exe` gwarantuje dokladnosc mikrosekundowa).

### 5.3. Plik `motlist.txt`
Przypisuje stany silnika gry (np. `WAIT`, `RUN`, `ATTACK`) do konkretnych plikow `.msa`:
```
GENERAL    WAIT       00.msa       100
GENERAL    RUN        03.msa       100
GENERAL    DIE        30.msa       100
GENERAL    DAMAGE     20.msa       100
GENERAL    NORMAL_ATTACK  31.msa   100
```

---

## 6. Podsystem 4: Bazy Danych i Prototypy (item_proto & mob_proto)

W dumpowanym kliencie bazy znajduja sie w:
- `locale/pl/item_proto` (plik binarny)
- `locale/pl/mob_proto` (plik binarny)
- Lub w wersji tekstowej: `item_names.txt`, `item_proto.txt`, `mob_names.txt`, `mob_proto.txt`.

### 6.1. Narzedzie DumpProto
Kod zrodlowy narzedzia znajduje sie w: `src/DumpProto/dump_proto/`.
Narzedzie obsluguje:
- Odczyt i parsowanie plikow CSV `ItemCSVReader.cpp`.
- Generowanie i weryfikacje sum kontrolnych CRC32.
- Serializacje do binarnego bufora `item_proto` i `mob_proto` ze wspolczesnymi polami rozszerzonymi (np. dodatkowe odpornosci, nowe typy przedmiotow, wielowalutowosc).

### 6.2. Integracja z C++23 ItemDataRegistry
W naszym kliencie zaimplementowany jest modul `src/GameLib/ItemDataRegistry.h`:
- Dostarcza natychmiastowy dostep do danych przedmiotu w zlozonosci O(1).
- Zapewnia scisla walidacje typow oraz obsluge socketow i bonusow.

---

## 7. Podsystem 5: Migracja Skryptow Pythona (Python 2.7 -> Python 3.14)

W starych zrzutach skrypty w katalogu `root/` oraz `uiscript/` uzywaja skladni Python 2.7:
1. `print "komunikat"` -> Nalezy zamienic na `print("komunikat")`.
2. Stare wyjatki: `except Exception, e:` -> Nalezy zamienic na `except Exception as e:`.
3. Obsluga tekstu: W Python 2 string byl sekwencja bajtow (`str`). W Python 3.14 mamy wyrazny podzial na `bytes` (dane binarne z sieci i buforow C++) oraz `str` (napisy Unicode / UTF-8).
4. Mostek `PyBridgeFastCall`: Zapewnia wywolywanie funkcji C++ bez posrednictwa narzutu krotek `PyTuple_New`.

---

## 8. Podsystem 6: Mapy, Teren i Obiekty Otoczenia

Podczas przenoszenia map gry (katalogi `metin2_map_*`):
1. **Siatka Wysokosci i Teren**:
   - `height.raw` (16-bitowa surowa siatka wysokosciowa).
   - `tile.raw` (indeksy tekstur podloza).
   - Ladowane bezposrednio przez `CAreaTerrain` oraz `PRTerrainLib`.
2. **Siatka Kolizji i Serwer**:
   - `server_attr` (binarna mapa blokad ruchu i kolizji).
   - Parsowana przez `CMapAttrLoader` i weryfikowana pod katem braku kolizji w `test_c26_terrain_subsystem`.
3. **Obiekty Otoczenia (Property)**:
   - Pliki `.prb` (definicje budynkow i obiektow Granny/glTF).
   - Pliki `.prd` (definicje lochow i wnetrz).
   - Pliki `.prt` (definicje drzew i roslinnosci).

---

## 9. Podzial Zadan dla Agentow AI (Task Sharding & Execution Matrix)

Przy masowej migracji zrzutu o rozmiarze 30 000+ plikow, praca powinna zostac rozdzielona pomiedzy subagentow AI wedlug naturalnych domen (`Domain Batching`):

| Rola Subagenta | Zakres Katalogow | Zadanie Główne | Szacowana Liczba Plikow | Narzedzie |
|---|---|---|---|---|
| **Subagent 1: Monster Specialist** | `monster/`, `monster2/`, `alune_*boss*` | Masowa konwersja potworow i bossow do glTF | ~2 600 | `batch_convert_gr2_to_glb.py` |
| **Subagent 2: NPC & Mount Specialist** | `npc/`, `npc2/`, `npc_mount/`, `pet/` | Masowa konwersja NPC, wierzchowcow i zwierzakow | ~3 200 | `batch_convert_gr2_to_glb.py` |
| **Subagent 3: Player & Armor Specialist** | `pc/`, `pc2/`, `season1/`, `season2/` | Modele cial postaci, pancerze, zbroje i animacje ras | ~4 100 | `batch_convert_gr2_to_glb.py` |
| **Subagent 4: Weapon & Accessories Specialist** | `weapon/`, `sash/`, `wings/`, zestawy broni | Modele broni, szarf i skrzydel, weryfikacja kosci | ~3 500 | `batch_convert_gr2_to_glb.py` |
| **Subagent 5: Environment & Zone Specialist** | `zone/`, `tree/`, `property/` | Obiekty swiata, budynki, roslinnosc | ~10 000 | `batch_convert_gr2_to_glb.py` |
| **Subagent 6: Database & Proto Specialist** | `locale/pl/`, `item_proto`, `mob_proto` | Dekompresja i eksport prototypow do CSV/JSON | - | `DumpProto` |
| **Subagent 7: UI & Python Modernizer** | `root/`, `uiscript/` | Walidacja skladni Python 3.14, naprawa kodowania | ~300 | AST / 2to3 / Ruff |
| **Subagent 8: QA & Simulation Inspector** | `tests/`, `build/` | Uruchomienie CTest, weryfikacja renderowania i logow | - | `ctest -C Release` |

---

## 10. Matryca Rozwiazywania Problemow (Troubleshooting Matrix)

| Objaw / Blad | Prawdopodobna Przyczyna | Rozwiazanie |
|---|---|---|
| **Model jest niewidoczny w grze** | Brak podsiatek lub brak powiazania kosci ze szkieletem | Sprawdz czy plik `.glb` ma atrybut `POSITION` i czy wierzcholki sa deformowane przez `CGltfModelInstance::DeformVertices`. |
| **Tekstura jest biala / czarna** | Sciezka tekstury w pliku `.glb` wskazuje na zly dysk (np. `D:/...`) | Upewnij sie, ze tekstura znajduje sie w katalogu modelu lub zglos brakujaca sciezke do `CPackManager`. |
| **Animacja odtwarza sie zbyt szybko / wolno** | Rozbieznosc czasowa miedzy `.msa` a animacja `.glb` | Zweryfikuj `Duration` w `GltfMotionData` i upewnij sie, ze animacja byla probkowana w 30/60 FPS. |
| **Bron lewituje obok dloni** | Zla nazwa kosci zaczepu | Sprawdz czy nazwa kosci w modelu to `Bip001 R Hand` lub `equip_right`. `CGltfModelInstance::GetBoneMatrixPointer` wyszukuje kosc po scislej nazwie. |
| **Hair Link nie deformuje wlosow** | Fryzura nie zostala powiazana z cialem | Upewnij sie, ze wywolano `pLODController->GetGltfModelInstance()->LinkSkeleton(pBodyInstance)`. |
| **Event ATTACK odpala sie w zlym czasie** | Brak wpisu w `metin2_events` | Sprawdz czy plik `.gr2` posiadal `TextTracks`. Jesli nie, eventy nalezy pobrac z powiazanego pliku `.msa`. |

---

## 11. Podsumowanie dla Agentow AI

Kazdy agent przystepujacy do pracy ze zrzutem klienta powinien dzialac w oparciu o nastepujacy cykl:
1. **Analiza katalogu**: Przeskanuj folder za pomoca `tools/analyze_and_migrate_dump.py`.
2. **Konwersja wsadowa**: Uruchom `tools/batch_convert_gr2_to_glb.py` na odpowiedniej liczbie watkow.
3. **Walidacja w silniku**: Wykonaj testy jednostkowe `ctest -C Release` i upewnij sie, ze wskaznik sukcesu wynosi **100% PASS**.
4. **Zatwierdzenie zmian**: Zrob commit w repozytorium Git z jasnym komunikatem bez polskich znakow diakrytycznych.
