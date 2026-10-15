---
task_id: "atlas_c07_03_granny_motion"
cluster: "MOD"
module_name: "CGrannyMotion - Animacje i Sciezki Ruchu Kosci"
target_files:
- src/EterGrnLib/Motion.cpp
- src/EterGrnLib/Motion.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c07_03_granny_motion.md"
architecture_layer: "Modele 3D, Szkielety Granny, Drzewa i Efekty"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Klasa `CGrannyMotion` to prosta warstwa abstrakcji (wrapper) na strukture `granny_animation` z silnika Granny 3D. Reprezentuje ona pojedyncza animacje modelu 3D (np. bieg, atak, smierc). Pozwala na powiazanie danych z pliku `.msa` / `.gr2` i umozliwia odczyt czasu trwania animacji oraz sciezek tekstowych (text tracks), ktore sluza m.in. do synchronizacji dzwiekow czy efektow wizualnych (np. uderzenie w ziemie) z klatkami animacji.
- **Wywolanie:** W momencie inicjalizacji obiektow 3D (proces ladowania w `CGraphicThing`). Odpytywana o informacje jest w glownym watku aplikacji podczas fazy `Update()` (np. w `CGrannyModelInstance::SetMotionPointer`), aby ustawic nowa animacje dla modelu, czy tez przy liczeniu interpolacji.
- **Przeplyw danych:** 
  1. `CGraphicThing::LoadMotions()` odczytuje dane animacji (`m_pgrnFileInfo->Animations[m]`).
  2. Przekazuje wskaznik `granny_animation*` poprzez wywolanie `BindGrannyAnimation()` do instancji `CGrannyMotion`.
  3. Klasy takie jak `CGrannyModelInstance` czy `CGrannyLODController` pobieraja animacje via `GetGrannyAnimationPointer()` aby ustawic ja w kontrolerze Granny (np. `GrannyPlayControlledAnimation`).
  4. Moduly dzwieku i efektow (np. eventy) pobieraja eventy z `GetTextTrack()`.
- **Cykl zycia:** Alokacja `CGrannyMotion` zazwyczaj zachodzi w formie tablicy przez `new CGrannyMotion[motionCount]` wewnatrz `CGraphicThing`. Nastepuje `Initialize()`. Potem wiazana jest animacja `BindGrannyAnimation()`. Podczas niszczenia `Destroy()` zeruje wskaznik, ale NIE zwalnia obiektu `granny_animation` - cyklem zycia pamieci zasobow zarzadza sam silnik Granny (alokator w `CGraphicThing` poprzez `granny_file`).

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** 
  - `CGraphicThing` alokuje, trzyma i inicjalizuje obiekty `CGrannyMotion`.
  - `CGrannyModelInstance` oraz `CGrannyLODController` pobieraja i uzywaja `CGrannyMotion` do kontrolowania ruchu.
  - `GameLib/ActorInstanceMotion` i system eventow moga mapowac zdarzenia tekstowe pobrane przez `GetTextTrack`.
- **Zaleznosci wyjsciowe (Outbound):** 
  - API Granny 3D: operuje bezposrednio na strukturach silnika takich jak `granny_animation`, `granny_track_group`, `granny_text_track`.
- **Drzewo dyrektyw `#include`:** 
  - `Motion.h` uzywa przeddeklaracji `struct granny_animation`, nie zalacza calej biblioteki, co zmniejsza czas kompilacji.
  - `Motion.cpp` zalacza `"StdAfx.h"` (co prawdopodobnie podciaga naglowki Granny) oraz `"Motion.h"`.
- **Model pamieciowy:** 
  - Uzywa czystych wskaznikow C (`granny_animation * m_pgrnAni`). Obiekt dziala jak slaba referencja (non-owning pointer). Pamiecia bazowa zarzadza wlasciciel (Granny).

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
| Nazwa | Rola | Wielkosc | Wlasciciel watku |
|-------|------|----------|------------------|
| `CGrannyMotion` | Wrapper na animacje Granny | ok. 8-16 bajtow (vtable + wskaznik) | Watek Glowny (Main Thread) / Watek Ladowania |

**Tabela Metod Publicznych:**
| Metoda | Sygnatura | Wartosc zwracana | Warunki wstepne i skutki uboczne |
|--------|-----------|------------------|----------------------------------|
| Konstruktor | `CGrannyMotion()` | Brak | Ustawia `m_pgrnAni` na `NULL`. |
| Destruktor | `virtual ~CGrannyMotion()` | Brak | Wywoluje `Destroy()`. |
| IsEmpty | `bool IsEmpty()` | `bool` | Brak efektow ubocznych. |
| Destroy | `void Destroy()` | `void` | Resetuje `m_pgrnAni` na `NULL`. Nie zwalnia zasobow Granny. |
| BindGrannyAnimation | `bool BindGrannyAnimation(granny_animation* pgrnAni)` | `bool` | Wymaga by `IsEmpty()` bylo prawda (assert). Ustawia wskaznik animacji. |
| GetGrannyAnimationPointer | `granny_animation * GetGrannyAnimationPointer() const` | `granny_animation *` | Brak efektow. Zwraca czysty wskaznik. |
| GetName | `const char * GetName() const` | `const char *` | Zwraca nazwe wprost z `m_pgrnAni->Name`. Zalozenie: pointer musi byc wazny. |
| GetDuration | `float GetDuration() const` | `float` | Zwraca czas z `m_pgrnAni->Duration`. Zalozenie: pointer wazny. |
| GetTextTrack | `void GetTextTrack(const char * c_szTextTrackName, int * pCount, float * pArray) const` | `void` | Wypelnia tablice podana jako `pArray` uzywajac `pCount` jako iteratora (dodaje). Zalozenie: wielkosc tablicy musi byc odpowiednia. |

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
| Pole | Typ | Offset | Opis |
|------|-----|--------|------|
| vtable | `void**` | 0x00 | Wskaznik do tabeli wirtualnej (z powodu wirtualnego destruktora). |
| `m_pgrnAni` | `granny_animation *` | 0x08 | Wskaznik 64-bitowy na strukture wnetrza Granny 3D. |

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- Modul ten jest czysto klient-side'owy i nie komunikuje sie bezposrednio z siecia ani nie posiada wlasnych pakietow sieciowych. Odtwarzane animacje sa skutkiem pakietow (np. `ActorAddPacket` lub uzycia skilla), jednakze same pakiety operuja na indeksach animacji, nie na tej klasie wprost.
- Nie mapuje bezposrednio API do Pythona przez `PyMethodDef`. Cale sterowanie animacjami ze skryptow przechodzi przez klasy nadrzedne jak `CPythonCharacterManager` i moduly `chr` i `chrmgr`.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Wszystkie dzialania na `CGrannyMotion` powinny byc wykonywane w glownym watku ladowania zasobow lub glownym watku update/render gry ze wzgledu na powiazanie ze wspoldzielonym srodowiskiem Granny.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - **Nullowe wskazniki:** Metody `GetName`, `GetDuration`, `GetTextTrack` nie sprawdzaja, czy `m_pgrnAni` nie jest zepsuty lub czy nie rowna sie `NULL`. Wywolanie ich przed wiazaniem skonczy sie SegFault.
  - **Brak walidacji trackow:** Metoda `GetTextTrack` ma zakomentowany `assert(!"CGrannyMotion::GetTextTrack - TrackCount is not 1");` i zaklada, ze tekstowe tracki beda zdefiniowane w `TrackGroups[0]`. Jesli animacja bedzie zepsuta lub wygenerowana z dziwnego formatu wyzszego rzedu, moze to doprowadzic do Segmentation Fault poprzez probe dostepu do braku grupy trackow.
  - **Przepelnienia buforow:** `pArray` w `GetTextTrack` jest zapisywane z inkrementowanym indeksem `(*pCount)++`. Brak jest parametru maksymalnego rozmiaru `pArray`. To jest niebezpieczny wzorzec i wymaga gwarancji od klasy wywolujacej ze `pArray` jest odpowiednio duzy.
- **Zarzadzanie zasobami (RAII):**
  - Klasa uzywa tzw. weak-referencing (nie jest wlascicielem `m_pgrnAni`). Nigdy nie nalezy dealokowac w niej `m_pgrnAni`. Cykl obslugiwany przez `granny_file`.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** 
  - Aby dodac np. zwracanie konkretnych trackow, nalezy poszerzyc API `CGrannyMotion.h`. Metody powinny najpierw zrobic prewencyjnego if-a sprawdzajacego `if (!m_pgrnAni) return;`.
  - Przydatne moze byc przepisanie `GetTextTrack` aby przyjmowala np. `std::vector<float>&` lub `std::span` z limitem max klatek dla poprawy bezpieczenstwa, ale pociagneloby to zaleznosci w kodzie wolajacym. Nalezy modyfikowac tylko w trybie safe-refactoring.
- **Jak debugowac i logowac:**
  - W razie crashy animacji, postaw breakpoint w `GetTextTrack`. Jesli track grupowy w Granny nie istnieje a model posiada specyficzne sciezki (np. stare .gr2 w nowym silniku), tu poleci wyjatek pamieci.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** 
  - W testach jednostkowych mozna zdefiniowac dummy struct dla `granny_animation` i mocki w przypadku braku pelnego srodowiska SDK Granny, po czym zbindowac dummy animacje poprzez `BindGrannyAnimation` i nastepnie zrobic test na wartosci zwracane przez gettery, bez modyfikacji kodu zrodlowego. Nalezy zamockowac struktury uzywane w `GetTextTrack`, aby symulowaly poprawne lub puste eventy stringowe.
