---
task_id: "atlas_c03_04_ground_item_instance"
cluster: "ITM"
module_name: "CGroundItemInstance - Reprezentacja Fizyczna Przedmiotu 3D"
target_files:
- src/UserInterface/GroundDrop/GroundItemInstance.cpp
- src/UserInterface/GroundDrop/GroundItemInstance.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c03_04_ground_item_instance.md"
architecture_layer: "Ekwipunek, Przedmioty, Handel i Gospodarka"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Modul ten odpowiada za fizyczna reprezentacje i wizualizacje przedmiotu wyrzuconego na ziemie w swiecie 3D. Zarzadza on pozycja w przestrzeni, orientacja, modelem 3D oraz efektami wizualnymi (np. swieceniem rzadkiego lupu, animacja obrotu, kolizjami z ziemia). Jest to glowny komponent systemu wizualnego GroundDrop.
- **Miejsce w petli gry:** Kod modulu wywolywany jest podczas aktualizacji stanow obiektow fizycznych i wizualnych (OnUpdate), w trakcie renderowania sceny 3D (OnRender) oraz w obrebie petli pakietow sieciowych (Network Tick), kiedy klient przetwarza zdarzenia pojawienia sie lub znikniecia przedmiotu.
- **Przeplyw danych:** Pakiety z serwera (Network Tick) zlecaja utworzenie instancji z identyfikatorem zdefiniowanym przez domenowa strukture GroundItemInstance. Metoda OnUpdate cyklicznie przelicza fizyke (np. obrot wokol osi Z lub animacje podskakiwania). Metoda OnRender aplikuje kwaterniony obrotu, ustawia pozycje wzgledem terenu i wysyla model do kolejki renderujacej.
- **Cykl zycia:** Alokacja odbywa sie w momencie otrzymania informacji od serwera (np. item_drop_packet). Proces inicjalizacji tworzy model, laduje tekstury (czesto asynchronicznie) i inicjuje systemy fizyki czasteczek dla efektow swiecenia. Kiedy przedmiot jest podniesiony lub czas na ziemi mija, odbywa sie dealokacja - zasoby sa zwalniane i encja znika z grafu sceny.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Modul jest kontrolowany przez zarzadcow dropu takich jak DropPhysicsSimulator lub menedzery puli GroundDropBatchRenderer. Odbiera wywolania interakcji od EventBusa, InputHandlera lub klas obslugujacych picking.
- **Zaleznosci wyjsciowe (Outbound):** Zalezy w glownej mierze od biblioteki renderujacej (DirectX 9 / EterLib) oraz systemu modeli Granny 3D dla odtwarzania animacji i mesh-y. Wymaga dolaczenia zasobow z EffectManager dla partykli. Wspolpracuje ze SphereLib dla sprawdzania promieni i cullingowania w oparciu o Hierarchical Bounding Sphere Tree. Wykorzystuje domenowa strukture z GroundItemModel.h.
- **Drzewo dyrektyw `#include`:** Dolacza lokalne zasoby: `"GroundItemInstance.h"`, `"../Domain/GroundItemModel.h"`, mozliwe moduly silnika fizyki np. `"IDropPhysicsSimulator.h"`. Konieczne standardowe naglowki `<cstdint>`, `<memory>`, `<vector>`.
- **Model pamieciowy:** Nowoczesny kod uzywa inteligentnych wskaznikow (std::unique_ptr, std::shared_ptr) do hermetyzacji modeli 3D, z minimalizacja czystych wskaznikow C zeby poprawic zarzadzanie pamiecia. Operacje bazowe i stan transferowane moga byc bez kopiowania przez std::span dla optymalnej komunikacji.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur:**
  - `CGroundItemInstance`: Glowna klasa odpowiedzialna za wizualna i fizyczna logike modelu przedmiotu w swiecie, polaczona z GL/D3D. Dziala wylacznie w glownym watku renderowania.
  - `GroundItemInstance` (struktura z GroundItemModel.h): Kontener danych DTO dla instancji trzymanej w pamieci (id, itemId, ownerId, koordynaty x/y/z). Rozmiar minimalny: 24 bajty.

- **Tabela Metod Publicznych:**
  - `void Initialize(const GroundItemInstance& data)`: Laduje parametry konfiguracyjne. Oczekiwane poprawne ID (VNUM).
  - `void Update()`: Metoda aktualizujaca animacje fizyczna. Oblicza nowa macierz transformacji, zwieksza kat obrotu osi Z. Efekt uboczny: zmiana stanow wektorow rotacji/translacji klasy.
  - `void Render()`: Aplikuje materialy, wywoluje API rysowania. Pre-condition: Context D3D zostal zainicjalizowany przez StateManager.
  - `bool Intersect(const Vector3& start, const Vector3& dir)`: Zwraca flage kolizji na podstawie promienia wskaznika myszki.

- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  - Struktura domenowa z `src/UserInterface/Domain/GroundItemModel.h` zapewnia bezposredni, spojny offset dla modow hookingowych w Ruscue/C++: 
    - [0x00] `uint32_t id`
    - [0x04] `uint32_t itemId`
    - [0x08] `uint32_t ownerId`
    - [0x0C] `float x`
    - [0x10] `float y`
    - [0x14] `float z`

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Powiazane z opcodami (np. CG_ITEM_PICKUP na podniesienie, GC_ITEM_DROP na zrespienie). Protokol przewiduje przeslanie ID konkretnego wlasciciela, zapisanego przez serwer jako uwarunkowanie dostepu uzytkownika do tego unikalnego przedmiotu.
- **Metody Pythona (`PyMethodDef`):** Eksponowane w module `chrmgr` (lub podmodule ekwipunku Pythona) moga zawierac metody np. `app.GetGroundItemVID`, `app.GetGroundItemOwner`. Bridge C-API zapewnia wymiane pomiedzy logika biznesowa wywolywana przez C++ a wizualna nakladka UI budowana po stronie pythona. Zastosowano wzorce intern string (PythonInternedStrings) dla eventow.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Obiekt `CGroundItemInstance` posiada elementy D3D, dlatego inicjalizacja oraz wywolania procedur API `Render()` lub zwalnianie VRAM MUSZA byc przeprowadzane wewnatrz glownego watku (Main Thread).
- **Potencjalne punkty awarii:** Niezainicjalizowany zasob 3D wskutek wadliwego `itemId` w `item_proto`. Dostep na nullach. Zjawisko utraty kontekstu (Device Lost w DirectX), zly stan podczas wznawiania sceny.
- **Zarzadzanie zasobami (RAII):** Kod musi implementowac mechanizmy RAII zapobiegajac powtarzalnej alokacji dla krotko zyjacych partykli. Jesli zmienia sie state DirectX, musi on byc resetowany poprzez `StateManager.cpp` w celu zapewnienia wlasciwego fallbacku dla kolejnych obowiazkow renderingu.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zaprojektuj nowy uklad zachowania np. fizyke grawitacji. 
  2. Implementuj kod modularny z wzorcami w katalogu `src/UserInterface/GroundDrop/`.
  3. Zmodyfikuj plik `.h` i `.cpp` uzywajac nowych inteligentnych wskaznikow lub spanow zgodnie ze standardem C++23.
  4. Nie ruszaj starego kodu, lecz izoluj uzywajac nowych klas pomocniczych (Zero-Conflict).
- **Jak debugowac i logowac:** Nalezy uzywac klasy `EterBase::ModernLogger::Error`. Jezeli wysylasz typy ENUM do logera, wykonaj rzutowanie do integera (`static_cast<int>(TYP)`).
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Napisz testy Doctest. W srodowisku Linuksowym mockuj D3D tworzac zaslepki (E_FAIL, D3D enumeratory) prosto w pliku testu, pamietajac o uzyciu definicji i nie linkujac starszych naglowkow z Windowsa. Zapewnij unikalna nazwe binarki tymczasowej `test_runner_CGroundItem`.
