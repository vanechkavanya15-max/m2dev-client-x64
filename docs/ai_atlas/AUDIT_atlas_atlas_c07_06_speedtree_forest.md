---
task_id: "atlas_c07_06_speedtree_forest"
cluster: "MOD"
module_name: "CSpeedTreeForest - Ekosystem Lasu i Instancji Drzew"
target_files:
- src/SpeedTreeLib/SpeedTreeForest.cpp
- src/SpeedTreeLib/SpeedTreeForest.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c07_06_speedtree_forest.md"
architecture_layer: "Modele 3D, Szkielety Granny, Drzewa i Efekty"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport Audytu AI: CSpeedTreeForest

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Modul `CSpeedTreeForest` pelni role scentralizowanego zarzadcy (ekosystemu) dla wszystkich drzew generowanych za pomoca technologii SpeedTreeRT w swiecie gry. Jego glownym celem biznesowym jest optymalizacja pamieciowa i wydajnosciowa poprzez wspoldzielenie zasobow. Zamiast ladowac model drzewa z dysku dla kazdego z tysiecy drzew na mapie, modul ten wczytuje glowny "wzorzec" drzewa (Main Tree) tylko raz i generuje na jego podstawie lekkie instancje rozrzucone po mapie, ktore wspoldziela geometrie, ale posiadaja wlasne transformacje (pozycje).

W architekturze klienta, klasa ta dziala jako klasa bazowa, z ktorej dziedziczy wlasciwa implementacja renderujaca `CSpeedTreeForestDirectX`.

**Cykl zycia i przeplyw danych (Control & Data Flow):**
1. **Ladowanie:** System mapy (np. `CMapOutdoor`) napotyka definicje drzew. Wywoluje `CreateInstance`, co w sposob transparentny pobiera z dysku wzorzec drzewa (jesli jeszcze go nie zaladowano, poprzez `GetMainTree` i `PackManager`) i dodaje go do mapy `m_pMainTreeMap`. Nastepnie generowana jest instancja (SpeedTreeWrapperPtr).
2. **Aktualizacja:** W glownej petli gry (OnUpdate), najczesciej z menedzera mapy (`MapOutdoorUpdate.cpp`), wywolywana jest metoda `UpdateSystem`, ktora przekazuje uplyniety czas (Delta Time). Nastepuje tu aktualizacja macierzy wiatru (Wind Matrices), wyliczana jest analitycznie rotacja galezi.
3. **Renderowanie (Delegowane):** `CSpeedTreeForest` udostepnia narzedzia do przechowywania globalnych parametrow srodowiska (swiatlo, mgla, wektory srodowiska, sily wiatru), ktore nastepnie w klasie pochodnej sluza do wysylania tych danych na karte graficzna przed faza wlasciwego rysowania.
4. **Dealokacja:** Instancje sa usuwane przez `DeleteInstance`, gdy obiekt znika z radaru/obszaru (np. sektor mapy jest zwalniany). Glowna mapa (Main Trees) uwalniana jest metoda `Clear()`.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
- Modul ten jest wywolywany glownie przez warstwy mapy i otoczenia (Engine/GameLib). Naleza do nich m.in.: `CArea`, `CMapOutdoor`, `CMapManager` oraz okazjonalnie w obiektach podmiotow: `CActorInstance`.

**Zaleznosci wyjsciowe (Outbound):**
- **SpeedTreeRT:** Modul opiera sie bezposrednio na bibliotece zewnetrznej IDV SpeedTree (naglowek `SpeedTreeRT.h`), obslugujacej bazowa implementacje wiatru i geometrii drzew.
- **VFS / Zabezpieczenia (EterBase & PackLib):** Pobieranie unikalnych sum kontrolnych CRC32 (dla cache) z `EterBase/CRC32.h` oraz system wirtualnego systemu plikow (VFS) za pomoca `PackLib/PackManager.h`.
- **Biblioteka standardowa:** Wykorzystanie w pelni zmodernizowanych wskaznikow w C++: `std::shared_ptr`, kontenery `std::map` (dla mapowania CRC na instrukcje), `std::vector`.

**Drzewo dyrektyw `#include` i Ryzyka:**
- Plik `.h`: `<SpeedTreeRT.h>`, `"SpeedTreeWrapper.h"`, `<vector>`, `<map>`, `"EterBase/CRC32.h"`. 
- Plik `.cpp`: `"StdAfx.h"`, `"EterBase/Filename.h"`, `"PackLib/PackManager.h"`, `"SpeedTreeForest.h"`, `"SpeedTreeConfig.h"`, `<cfloat>`.
- Brak cyklicznych zaleznosci; klasa jest dobrze odizolowana jako posrednik miedzy SpeedTree a klientem. Wystepuje tu jednak dziedziczenie do implementacji zaleznej od API sprzetowego (Polimorfizm przez `UploadWindMatrix` / `Render`).

**Model pamieciowy:**
- Kod ten uzywa powszechnie bezpiecznych, wspoldzielonych inteligentnych wskaznikow (Smart Pointers). Aliasing to `using SpeedTreeWrapperPtr = std::shared_ptr<CSpeedTreeWrapper>;`. Glowny system przechowuje slownik (`std::map<DWORD, SpeedTreeWrapperPtr>`) powiazany z glownymi modelami wczytanymi do pamieci RAM (Main Trees).

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur:**
| Nazwa symbolu | Rola / Opis | Wlasciciel Watku |
|---|---|---|
| `CSpeedTreeForest` | Glowny rejestr i zarzadca drzew, ekosystem srodowiskowy. Klasa bazowa (posiada czysto wirtualne metody np. Render). | Watek Glowny (Render/Logic) |

**Tabela Zdefiniowanych Makr / Wektorow Bitowych (Bitfield Masks):**
| Nazwa | Wartosc | Opis |
|---|---|---|
| `Forest_RenderBranches` | `(1 << 0)` | Wskazuje, by renderowac galezie. |
| `Forest_RenderLeaves` | `(1 << 1)` | Wskazuje, by renderowac liscie. |
| `Forest_RenderFronds` | `(1 << 2)` | Wskazuje, by renderowac drobne paprocie/krzaki/elementy podloza zwiazane z drzewem. |
| `Forest_RenderBillboards` | `(1 << 3)` | Wskazuje, by renderowac uproszczone wersje 2D (billboardy) drzew dla oddalonych obszarow (LOD). |
| `Forest_RenderToShadow` | `(1 << 5)` | Zleca wygenerowanie maski do mapowania cieni z drzew. |

**Tabela Metod Publicznych (`CSpeedTreeForest`):**
| Sygnatura | Zwraca | Warunki wstepne i Skutki uboczne |
|---|---|---|
| `Clear()` | `void` | Usuwa wszystkie wczytane modele z mapy oraz ich instancje, zwalnia pamiec zarzadzana przez `std::shared_ptr`. |
| `GetMainTree(DWORD dwCRC, SpeedTreeWrapperPtr& ppMainTree, const char* c_pszFileName)` | `BOOL` | Probuje wczytac lub pobrac z cache wzorzec drzewa bazujac na CRC, pobiera go asynchronicznie via `PackManager` (jesli konieczne). |
| `CreateInstance(float x, float y, float z, DWORD dwTreeCRC, const char* c_szTreeName)` | `SpeedTreeWrapperPtr` | Tworzy i zwraca nowa instancje z ustawiona pozycja. Zwieksza licznik referencji drzewa wzorcowego. Dodaje do niego instancje. |
| `DeleteInstance(SpeedTreeWrapperPtr pTree)` | `void` | Bezpiecznie deleguje usuniecie instancji do odpowiedniego drzewa wzorcowego. |
| `UpdateSystem(float fCurrentTime)` | `void` | Aktualizuje stan wewnetrznego timera (deltas) na podstawie globalnego zegara oraz wywoluje przebudowe macierzy wiatru. |
| `SetLight(...)` / `SetFog(...)` | `void` | Ustawia globalne parametry srodowiskowe ekosystemu lesnego, kopiuje do wewnetrznych tablic `float`. |
| `SetWindStrength(float fStrength)` | `void` | Zmienia w globalnym rejestrze sile wiatru i propaguje zaktualizowana wartosc do kazdej aktywnej instrukcji glownego drzewa (ktore iteruje po swoich klonach). |
| `SetupWindMatrices(float fTimeInSecs)` | `void` | Skomplikowany kod trygonometryczny budujacy tablice 16-elementowych macierzy obrotu aplikowanych pozniej w shaderach z uzyciem makr `WRAPPER_USE_GPU_WIND` lub na CPU za pomoca biblioteki ST. |

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
Klasa przechowuje mapy i tablice plywajace. Kluczowe pola `float` posiadaja nastepujace offsety/rozmiary:
- `m_afLighting[12]` - kierunek(4), ambient(4), diffuse(4).
- `m_afFog[4]` - z_near, z_far, z_linear_scale, padding/reserved.
- `m_afForestExtents[6]` - minX, minY, minZ, maxX, maxY, maxZ. Sluzy potencjalnie do grubszej analizy widocznosci calego lasu.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
Modul implementacyjny **nie posiada bezposrednich powiazan z protokolem sieciowym ani Pythonem**.
Drzewa sa statycznym elementem generowanym w kliencie za sprawa czytania binarnego pliku z dysku (przez menedzera mapy). Pakiety z serwera nie wywoluja drzewek; co najwyzej posrednio np. poprzez zmiany statusu na mapie. Python uzywa modulu tla (`background`), by operowac otoczeniem. Brak jest tu API eksponowanego jako `PyMethodDef`.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

**Zasady wielowatkowosci:**
Modul dziala w sposob sekwencyjny i zalezy od Glownego Watku Logic/Render klienta. Tablica `m_pMainTreeMap` ani dostepy poprzez `std::shared_ptr` nie sa synchronizowane (brak muteksow). Jakakolwiek modyfikacja zasobow lasu przez inny watek spowodowalaby naruszenie pamieci (Data Race).

**Zarzadzanie zasobami (RAII) & Inteligentne Wskazniki:**
Obecnosc C++ standardu 11+ jest tu bardzo pozytywna. Uzycie `std::shared_ptr<CSpeedTreeWrapper>` zapobiega wyciekom pamieci instancji, jesli sa poprawnie obslugiwane po stronie modulow wykorzystujacych (np. CArea usuwa swoja liste starych instancji przy wyladowaniu strefy). Nalezy unikac rzutowania z powrotem na "raw pointers", chyba ze jest to konieczne ze wzgledu na API legacy (ktorych tu na szczescie nie widac duzo w tej warstwie). Zaleznosci rekurencyjne miedzy wzorcem, a instancja (instancja moze miec wskaznik na wzorzec by wrocic na delete) sa obslugiwane przez klase Wrapper, nalezy uwazac by nie tworzyc `shared_ptr` cykli referencyjnych.

**Potencjalne punkty awarii (Crash Points & Edge Cases):**
- **Plik nie istnieje:** Metoda `GetMainTree` sprawdza via `PackManager` obecnosc pliku. Zwraca FALSE, wiec nadrzedna `CreateInstance` zwroci NULL. Istnieje obowiazek obslugujacego po stronie Engine by na NULL nie operowac.
- **Reset srodowiska (np. Alt+Tab / Lost Device):** Klasa jako implementacja bazowa nie trzyma wskaznikow COM Direct3D9. To robi klasa dziedziczaca (`CSpeedTreeForestDirectX`), natomiast stany w `m_afLighting`, `m_afFog`, `m_fWindStrength` moga byc potencjalnie przydatne po odtworzeniu kontekstu urzadzenia.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
Chcac zaimplementowac na przyklad dynamiczna reakcje drzewa (SpeedTree) na czary postaci np. wiatrowe aury, trzeba:
1. Odnalezc `m_fWindStrength` w `SpeedTreeForest`.
2. Dodac modyfikator lub offset "Local Wind Strength" do interfejsu (w argumentach UpdateSystem, lub przez np. EventBus/RHI), ktory nadpisze lub zasymuluje wieksza wartosc sily wiatru w promieniu `X` jednostek (uzywajac hash-gridu by pozycjonowac modyfikator wokolo `x, y, z` instancji czaru, choc tu jest system globalnego wiatru - nalezaloby przeniesc to nizej).
3. Dodanie obslugi bezposrednio do `SetupWindMatrices`, jesli to modyfikacja logiki falowania.
4. Modyfikacji samej geometrii renderingu czy shaderow nalezy poszukiwac w `CSpeedTreeForestDirectX` oraz odpowiednich plikach shaderow zewnetrznych. Pamietaj o zasadzie ZERO-CONFLICT dla renderera DX - kod refaktoryzacyjny dla AI w 2026 unika pisania po systemach RHI, skupiajac sie na logice domenowej.

**Jak debugowac i logowac:**
- Najlepszym punktem do postawienia breakpointu przy ladowaniu drzew (by zweryfikowac, skad biora sie dane modele na nowej mapie) jest w `CSpeedTreeForest::CreateInstance()`.
- Wektor macierzy wiatru (Wind matrices) jest obliczany wewnatrz tablic numerycznych `afMatrix`.

**Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
`CSpeedTreeForest` jest prawie cakowicie bezstanowe wobec Direct3D (oprocz czysto wirtualnych funkcji i globalnego ST). Mozna stworzyc klase mocka:
```cpp
class MockSpeedTreeForest : public CSpeedTreeForest {
public:
    virtual void UploadWindMatrix(unsigned int, const float*) const override {}
    virtual void Render(unsigned long) override {}
};
```
Pozwala to na w pelni odizolowane testowanie alokacji instancji z `PackManagerem` oraz obliczania matrycy z uzyciem Doctest w C++23 dla modulow logicznych srodowiska.
