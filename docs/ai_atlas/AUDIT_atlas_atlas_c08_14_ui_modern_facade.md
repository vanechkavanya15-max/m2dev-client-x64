---
task_id: "atlas_c08_14_ui_modern_facade"
cluster: "UI"
module_name: "Nowoczesna Fasada UI dla Agentow i Headless Botow"
target_files:
- src/Client/UI/UIManager.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c08_14_ui_modern_facade.md"
architecture_layer: "Interfejs Uzytkownika, Okna i System Tekstu"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Ten modul stanowi centralny punkt zarzadzania hierarchia okien (WindowHierarchy) oraz wejsciem uzytkownika w nowej architekturze UI klienta gry Metin2. Zamiast bezposredniego symulowania klikniec mysza, pozwala agentom AI i botom headless na programatyczny dostep do drzewa okien, weryfikacje stanow okien modalnych, sprawdzanie widocznosci oraz manipulowanie obiektami UI.
Kod ten jest zazwyczaj wywolywany w glownej petli klienta (OnUpdate dla logiki, OnRender dla rysowania) oraz reaguje na zdarzenia wejsciowe przetwarzane w warstwie platformy.
Przeplyw danych: Event wejsciowy trafia do `WindowManager::HandleMouseEvent`, ktory za pomoca funkcji `HitTest` (dostepnej w `Window`) weryfikuje Z-Order oraz stos okien modalnych (Modal Stack), a nastepnie dystrybuuje zdarzenie wzdluz drzewa rodzic-dziecko az do obslugi (handled) lub anulowania operacji drag-and-drop.
Cykl zycia: Obiekty `Window` sa zarzadzane w pamieci za pomoca `std::shared_ptr`. `WindowManager` przechowuje glowny wezel (`m_root`), a kazde okno pamieta swoje dzieci (`m_children`). Dealokacja nastepuje automatycznie, gdy okno zostaje usuniete z drzewa, a liczba referencji spadnie do zera.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound):** Glowna petla gry zasilajaca zdarzenia (np. z wejscia systemowego Windows/DirectInput), agenci i boty headless, ktore bezposrednio wstrzykuja akcje (programistyczne wywolania).
- **Zaleznosci wyjsciowe (Outbound):** Aktualnie modul jest wysoce niezalezny, uzywa narzedzi ze standardowej biblioteki C++ i operuje na czystej logice pozycjonowania 2D (Point, Rect).
- **Drzewo dyrektyw `#include`:** Wewnatrz implementacji dolaczane sa `<vector>`, `<memory>`, `<string>`, `<optional>`, `<functional>`, `<algorithm>`. Brak naruszen warstwy lub zaleznosci od d3d9 czy Pythona (Zasada ZERO-DIRECTX & ZERO-PYTHON).
- **Model pamieciowy:** Hybrydowy. Wlasnosc wezlow-dzieci jest reprezentowana przez `std::shared_ptr<Window>`. Nawigacja w gore w drzewie do rodzica odbywa sie przez surowe wskazniki (`Window*`), aby uniknac cyklicznych zaleznosci i wyciekow. Stos okien modalnych (`m_modalStack`) oraz referencje drag-drop (`m_dragSource`) rowniez uzywaja bezpiecznych, surowych wskaznikow niewlascicielskich.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur:**
- `Client::UI::Point`: Prosta struktura 2D (x, y), wielkosc 8 bajtow, watek glowny.
- `Client::UI::Rect`: Obszar 2D (x, y, width, height) z metoda `Contains`, 16 bajtow, watek glowny.
- `Client::UI::MouseEventType`: Enum klasowy okreslajacy rodzaj zdarzenia (MouseMove, OnClick, ButtonDown, itp.).
- `Client::UI::DragDropPayload`: Struktura z `std::string dataType` i `int dataId`, do przenoszenia metadanych np. ID przedmiotu przy drag & drop.
- `Client::UI::Window`: Wezel drzewa okien, zarzadza hierarchia, pozycjonowaniem i widocznoscia.
- `Client::UI::WindowManager`: Singleton-like menedzer przechowujacy `m_root`, koordynujacy routing zdarzen wejsciowych oraz system okien modalnych.

**Tabela Metod Publicznych:**
- `Window::HitTest(const Point& globalPos) -> Window*`: Zwraca okno (najwyzej w Z-Order), ktore zawiera podany punkt.
- `Window::GetGlobalRect() const -> Rect`: Oblicza bezwzgledna pozycje okna poprzez rekursywne dodawanie offsetow z wezlow nadrzednych.
- `Window::BringToFront()`: Przesuwa to okno na sam koniec listy dzieci u swojego rodzica (wierzch Z-Order).
- `WindowManager::HandleMouseEvent(const MouseEvent& event) -> bool`: Glowny punkt wejsciowy dla wstrzykiwania zdarzen przez boty. Uwzglednia okna modalne.
- `WindowManager::SetModalWindow(Window* window)`: Blokuje input dla wszystkich elementow nienalezacych do hierarchii podanego okna.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
- Obiekty C++ uzywaja RTTI i vtable (poniewaz `Window` posiada metody wirtualne). `m_parent` w `Window` znajduje sie zaraz za struktura stringa z nazwa.
- `m_modalStack` w `WindowManager` to wektor wskaznikow; wektor mozna odnalezc pod koniec struktury obiektu managera i iterowac, by okreslic, co aktualnie blokuje dzialanie UI.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Pakiety Sieciowe:** Brak bezposredniego bindowania z pakietami GC/CG na tym poziomie abstrakcji. UI dziala pasywnie i reaguje na EventBus lub kontrolery.
- **Metody Pythona (`PyMethodDef`):** Modul stanowi C++ fasade. Z punktu widzenia architektonicznego, fasada ta zostala celowo oczyszczona z Pythona, umozliwiajac wirtualnej maszynie C++ bezposrednia kontrole UI bez narzutu interpretera.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Wszystkie operacje na obiektach `Window` musza odbywac sie w glownym watku, z uwagi na modyfikacje zaleznosci miedzy oknami (`m_children`, `m_parent`) bazujace na niewspolbieznych kontenerach STL.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** Brak poprawnego wyczyszczenia okna modalnego w `ClearModalWindow` pozostawi zwisajacy wskaznik w `m_modalStack`. Surowe wskazniki typu obserwator uzyte w `m_dragSource` moglyby ulec uniewaznieniu, gdyby okno zostalo usuniete w trakcie dragu.
- **Zarzadzanie zasobami (RAII):** Silne uzycie `std::shared_ptr` w hierarchii rozwiazuje wiekszosc problemow alokacyjnych. Nalezy pamietac o rozbijaniu zaleznosci cyklicznych - dlatego `m_parent` jest `Window*`.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Odszukaj pliki w `src/Client/UI/` (szczegolnie te odpowiedzialne za hierarchie okien).
  2. Zaimplementuj nowa metode operacyjna w obiekcie bazowym okna lub w menedzerze.
  3. Pamietaj o aktualizacji logiki okien modalnych i testowania zdarzen drag-and-drop, jesli wplywa to na input.
- **Jak debugowac i logowac:** Przechwytuj obiekty wejsciowe i ich polozenie (np. `event.position`) oraz monitoruj stos `m_modalStack` pod katem nieprawidlowo dzialajacego skupienia (focus traps).
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Utworz wyizolowana instancje menedzera okien i drzewo wezlow w systemie testowym. Wstrzykuj sztuczne zdarzenia uzywajac narzedzia testowego i badaj wartosc logiczna sprawdzajac wywolania powrotne bez renderowania w DirectX.
