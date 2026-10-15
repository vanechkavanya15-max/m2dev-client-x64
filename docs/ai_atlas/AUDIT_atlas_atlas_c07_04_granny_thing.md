---
task_id: "atlas_c07_04_granny_thing"
cluster: "MOD"
module_name: "CGraphicThing - Kontener Zasobow Modeli 3D"
target_files:
- src/EterGrnLib/Thing.cpp
- src/EterGrnLib/ThingInstance.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c07_04_granny_thing.md"
architecture_layer: "Modele 3D, Szkielety Granny, Drzewa i Efekty"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Modul zarzadza plikami modeli .gr2 (Granny 3D). Klasa `CGraphicThing` pelni role wspoldzielonego kontenera w pamieci, w ktorym trzymane sa obiekty Granny3D odczytane z dysku/pamieci. `CGraphicThingInstance` stanowi instancje tych modeli umieszczana na scenie. Obejmuje zarzadzanie LOD (Level of Detail), animacjami (motion) oraz deformacjami.
- **Punkt wywolania w petli gry:** 
  - `OnUpdate()`: Aktualizuje przeliczenia czasu, kontroler LOD i pozycje na podstawie przypisanego `CCamera`.
  - `OnDeform()`: Przelicza deformacje siatki z uzyciem macierzy kosci na dany moment czasu.
  - `OnRender()`, `OnBlendRender()`, `OnRenderShadow()`: Renderuje ostateczny obraz wykorzystujac DirectX poprzez warstwe narzedziowa.
- **Przeplyw danych:** Z pliku modelu .gr2 nastepuje odczyt struktur statycznych (`granny_file`, `granny_file_info`) do pamieci RAM (w tym wierzcholki i siatka w `LoadModels` oraz animacje w `LoadMotions`). Nastepnie dla kazdego z nich w czasie rzeczywistym system LOD oraz przeliczenia ruchu aktualizuja macierze transformacji i ostatecznie rzutuja na obiekty D3D9.
- **Cykl zycia obiektow:** 
  - `CGraphicThing` jest inicjalizowane podczas odczytu danych przez Managera zasobow (`CResource`), ktory dziala na referencjach. Reset/dealokacja zwalnia bufory (np. `GrannyFreeFileSection`). 
  - Obiekty instancji (`CGraphicThingInstance`) alokowane sa w wydzielonej puli `CDynamicPool<CGraphicThingInstance> ms_kPool`.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Modul powolywany jest do zycia za posrednictwem narzedzi ladowania gier lub instancjowania klas swiata takich jak systemy map, bytow czy FX. Korzystaja z tego eventy zmiany LOD, zmiany renderowania oraz system detekcji kolizji.
- **Zaleznosci wyjsciowe (Outbound):** 
  - Biblioteka Granny 3D (do obslugi siatek i animacji).
  - DirectX 9 (dla renderowania wierzcholkow i zarzadzania cieniem).
  - `EterLib`: Klasy podrzedne jak `CGraphicObjectInstance`, `CResource`, `CDynamicPool`, `CGrannyLODController`.
- **Drzewo dyrektyw `#include`:** Dolaczone `#include "StdAfx.h"`, `#include "Eterbase/Debug.h"`, `"Thing.h"`, `"ThingInstance.h"`, `"Model.h"`, `"Motion.h"`, `"Eterbase/Stl.h"`, `"Eterlib/GrpObjectInstance.h"`, `"Eterlib/GrpShadowTexture.h"`, `"LODController.h"`.
- **Model pamieciowy:** Do alokacji instancji uzywany jest systemowy `CDynamicPool`. Zaleznosci pomiedzy plikami/instancjami zarzadzane sa glownie przez inteligentne wskazniki wlasnego autorstwa `CRef` (np. `CGraphicThing::TRef`), oraz surowe wskazniki dla starszego interfejsu Granny 3D (`granny_file`, `granny_file_info`).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

- **Tabela Klas i Struktur:**
  - `CGraphicThing`: Model statyczny w pamieci, rozszerza `CResource`. Zawiera bufory plikow.
  - `CGraphicThingInstance`: Instancja modelu 3D na mapie. Wlasciciel watku D3D9/Logiki, alokowana z Pool. Obiekt renderowalny rozszerzajacy `CGraphicObjectInstance`.
  - `SModelThingSet`: Wewnetrzna struktura trzymajaca liste wskaznikow `CGraphicThing::TRef` nalezacych do konkretnego elementu LOD.

- **Tabela Metod Publicznych (`CGraphicThing`):**
  - `OnLoad(int, const void*) -> bool`: Laduje plik `.gr2` do pamieci za pomoca Granny.
  - `GetModelPointer(int) -> CGrannyModel*`: Pobiera wyluskany model z zainicjalizowanego kontenera.
  
- **Tabela Metod Publicznych (`CGraphicThingInstance`):**
  - `UpdateLODLevel() -> void`: Oblicza odleglosc od `CCamera` i wplywa na obiekty kontrolujace poziom szczegolowosci. Skutek uboczny: zmiana statutu renderowania komponentow.
  - `SetMotion(DWORD, float, int, float) -> bool`: Wymusza przelaczenie na inna akcje (np. z biegu na atak). Powoduje interpolacje na kontrolerach LOD.
  - `AttachModelInstance(int, const char*, int) -> void`: Doczepia np. bron do lapy uzytkownika korzystajac ze stalej kosci `c_szBoneName`.
  
- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  - W klasie `CGraphicThingInstance` mnostwo wektorow: `std::vector<CGrannyLODController*> m_LODControllerVector`, `m_modelThingSetVector`, offset zalezy od kompilatora i wyrownania (padding), ale znajduja sie na koncu obok stanow numerycznych jak `m_fLocalTime`, `m_v3Center`.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- Nie ma tutaj bezposrednich pakietow sieciowych (CG/GC). 
- Modul ten jest warstwa renderingu nizszego poziomu i dziala transparentnie z perspektywy sieci. Siec dziala wyzej na aktorach, a te instruuja warstwe graficzna.
- Brak bezposredniego wiazania Python C-API w analizowanych plikach, API graficzne dla Pythona znajduje sie zazwyczaj w warstwie wyzszej (EterPythonLib).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Kod uruchamiany jest glownie w watku wyrysowywania logiki klienckiej. Kontenery na czas deformacji sa podatne na zmiany z zewnatrz. Konieczne jest aby nikt z inego watku nie kasowal obiektow `CGraphicThing` w trakcie dzialania `OnDeform()` oraz `OnUpdate()`.
- **Potencjalne punkty awarii:** 
  - Przekroczenie wielkosci tablic wskaznikow jezeli Granny3D wyrzuci ujemne numery instancji, czesciowo niwelowane przez `CheckModelInstanceIndex` sprawdzajace rozmiar w kontenerach (`m_LODControllerVector.size()`).
  - Przeskakiwanie watku w timerach miedzy `timeGetTime()` a wlasnym zegarem serwera.
  - Pamietaj, aby zawartosc modelu Granny zwalniac z wywolaniem specjalnego API Granny, np. `GrannyFreeFileSection`.
- **Zarzadzanie zasobami (RAII):** System CRef implementuje reczne podbicia licznikow. Nowe instancje nalezy koniecznie powolywac uzywajac `CGraphicThingInstance::New()` i usuwac za pomoca `CGraphicThingInstance::Delete()` dla optymalnej pracy Cache (Zero allocations per frame).

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji:**
  1. Jezeli dotyczy wnetrza modelu lub wczytywania, zmodyfikuj `CGraphicThing::LoadModels()`.
  2. Dodaj wsparcie w logice instancji `CGraphicThingInstance`.
  3. Zarejestruj na szynie eventow obiekty powiazane z `m_bUpdated`.
- **Jak debugowac i logowac:** Do dyspozycji masz preprocesor `__PERFORMANCE_CHECKER__`, generuje on `perf_thing_onupdate.txt` podczas wolnego updatu na dysk C:/ (bardzo przydatne do diagnozy lagow przy 100+ obiektach!).
- **Jak testowac:** Mozesz stworzyc atrapowy `CCamera` oraz sfalszowac mockiem `granny_file` bez akceleracji DirectX w celu weryfikacji dzialania C++ przeksztalcen wektorow AABB `GetBoundingAABB`.
