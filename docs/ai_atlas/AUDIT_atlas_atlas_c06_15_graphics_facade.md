---
task_id: "atlas_c06_15_graphics_facade"
cluster: "RND"
module_name: "Fasada Renderera dla Klienta Beztrybowego i Testow"
target_files:
- src/Client/Graphics/GraphicsEngine.h
- src/EterLib/Render/BlendStateScope.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c06_15_graphics_facade.md"
architecture_layer: "Renderowanie, Direct3D 9, Shading i Culling"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Modul mial na celu dostarczenie fasady pozwalajacej na uzycie silnika graficznego bez interfejsu (headless), umozliwiajac testy i beztrybowe dzialanie klienta, odpinajac warstwe renderujaca od Direct3D 9. 
- **Zbadany stan faktyczny:** Plik `src/Client/Graphics/GraphicsEngine.h` aktualnie nie istnieje w repozytorium (zostal prawdopodobnie usuniety lub nie zostal jeszcze zaimplementowany). Z kolei plik `src/EterLib/Render/BlendStateScope.h` zawiera implementacje pomocnicza - straznika zakresu (RAII scope guard) dblajacego o automatyczne zapisywanie, nadpisywanie oraz przywracanie stanow blendowania (alpha blending) w potoku Direct3D 9.
- **Miejsce wywolania (Petla gry):** Obiekty `BlendStateScope` sa tworzone tymczasowo na stosie wewnatrz glownej petli renderowania (OnRender) w watku Direct3D, krotko przed wyslaniem polecen rysowania (draw calls) modeli czy efektow wymagajacych przezroczystosci.
- **Przeplyw danych i cykl zycia:** Alokacja odbywa sie lokalnie (na stosie) podajac predefiniowany tryb (`BlendMode`) lub dokladne parametry (np. `srcBlend`, `destBlend`). Konstruktor wywoluje singleton `CStateManager::Instance()` i zabezpiecza obecny stan operacji `D3DRS_ALPHABLENDENABLE`, `D3DRS_SRCBLEND`, `D3DRS_DESTBLEND` i `D3DRS_BLENDOP`, by nastepnie je zaktualizowac. Podczas niszczenia (destruktor) stany te sa automatycznie przywracane do wartosci poczatkowych (LIFO), zapobiegajac wyciekom stanow renderowania.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Kod rysujacy interfejs uzytkownika (UI), efekty czasteczkowe, modele z przezroczystoscia, ktory wola bezposrednio API `BlendStateScope` do izolacji stanu renderowania. Brak istniejacego pliku `GraphicsEngine.h` uniemozliwia wskazanie jego zaleznosci.
- **Zaleznosci wyjsciowe (Outbound):** `BlendStateScope` wola `CStateManager::Instance()` (pochodzacy z `../StateManager.h`), zeby manipulowac stanami API Direct3D 9.
- **Drzewo dyrektyw `#include`:** 
  - `<d3d9.h>`
  - `"RenderStateTypes.h"` (dostarcza m.in. potencjalnie enum BlendMode lub pokrewne)
  - `"../StateManager.h"` (do zarzadzania maszyna stanow)
- **Model pamieciowy:** Wzorce RAII w oparciu o alokacje na stosie; brak wskaznikow w strazniku; wszystkie wewnetrzne obiekty to zmienne lokalne lub referencje do Singletonu (C++ `auto& sm = CStateManager::Instance()`).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
- `EterLib::Render::BlendStateScope`: Klasa RAII; wlasciciel watku: glowny watek D3D (Direct3D thread). Wielkosc w bajtach: 1 bajt (brak pol klasy, korzysta z wywolan systemowych).

**Tabela Metod Publicznych (`BlendStateScope`):**
- `explicit BlendStateScope(BlendMode mode)`: Konstruktor ze zdefiniowanym typem blendowania (Opaque, AlphaBlend, Additive, Multiply). Skutki uboczne: Zmienia wewnetrzne render states w D3D.
- `BlendStateScope(bool blendEnable, D3DBLEND srcBlend, D3DBLEND destBlend, D3DBLENDOP blendOp = D3DBLENDOP_ADD)`: Konstruktor z recznie definiowanymi wartosciami. Skutki uboczne: Jak wyzej.
- `~BlendStateScope()`: Destruktor, przywraca zapisane (w cache menedzera stanu D3D) parametry D3D.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
Brak pol czlonkowskich, obiekty `BlendStateScope` nie maja dedykowanego layoutu danych na uzytek hookowania. Stan D3D jest globalnie przechowywany w CStateManager. Brak danych dla braku pliku `GraphicsEngine.h`.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Modul renderowania `BlendStateScope` nie korzysta z pakietow sieciowych; grafika po stronie klienta jest tu niezalezna od sieci.
- **Metody Pythona (`PyMethodDef`):** Klasa i fasada nie eksportuja bezposrednich punktow wejscia do Pythona. Operacje renderujace i obsluga RAII pozostaja na niskim poziomie C++.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Inicjalizacja oraz zwolnienie straznika blendowania MUSI nastapic w glownym watku renderowania (D3D thread). Jakiekolwiek wykorzystanie w innych watkach spowoduje `D3DERR_INVALIDCALL` i awarie (hard crash).
- **Potencjalne punkty awarii:** Niezbalansowane strazniki na stosie. Zwracanie wyjatkow (choc w C++ EterLib unika tego), nie mialoby znaczenia dzieki semantyce RAII straznika, zachowujacego wlasciwy zrzut stanow (Cache). Brak pliku `GraphicsEngine.h` to potencjalny bloker dla budowania glownej fasady.
- **Zarzadzanie zasobami (RAII):** Kod w stu procentach polega na wywolywaniu destruktora przez kompilator poza zasiegiem zadeklarowanego zagniezdzenia. Nie uzywac `new BlendStateScope` - uzywac wylacznie bezposredniego tworzenia na stosie!

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji:**
  1. Analizujac nowa operacje blendowania, dodaj nowy typ wyliczeniowy do enum `BlendMode` (prawdopodobnie w zaleznym kodzie np. w D3D).
  2. Zmodyfikuj metode `ApplyMode` wewnatrz `BlendStateScope.h`, by wspierala nowa opcje za pomoca odpowiednich stalych DirectX 9 (np. `D3DBLEND_SRCCOLOR`).
- **Jak debugowac i logowac:** Do debugowania wywolan nienormalnych operacji mozna zalozyc breakpoint na funkcjach wywolywanych w `CStateManager::Instance().SaveRenderState`. Obiekty nie posiadaja metody drukujacej (ToString).
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** W przypadku braku pliku `GraphicsEngine.h` nalezy najpierw udokumentowac i odtworzyc brakujaca fasade renderowania (czyli mockowanie Device Direct3D w `CStateManager`). Podczas testowania jednostkowego, nalezy uzywac podrobionych naglowkow dla `<d3d9.h>` w katalogu `mock_include`.
