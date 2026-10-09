---
task_id: "atlas_c07_15_lod_selector"
cluster: "MOD"
module_name: "Selektor Poziomow Szczegolowosci (Actor LOD Selector)"
target_files:
- src/EterLib/Render/ActorLODSelector.cpp
- src/EterLib/Render/ActorLODSelector.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c07_15_lod_selector.md"
architecture_layer: "Modele 3D, Szkielety Granny, Drzewa i Efekty"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Modul `ActorLODSelector` z przestrzeni nazw `EterLib::Render` odpowiada za selekcje poziomu szczegolowosci (Level of Detail - LOD) dla modeli aktorow. Jego zadaniem jest wyliczanie i decydowanie, ktory wariant modelu (np. z pelna czy uproszczona siatka oraz szkieletem) nalezy wyrenderowac, bazujac na odleglosci obiektu od kamery. Przeklada sie to na znaczna oszczednosc zasobow GPU i CPU w zatloczonych lokacjach.

Kod ten uzywany jest w glownej petli gry w trakcie fazy renderowania (OnRender) lub zarzadzania instancjami. Cykl zycia:
1. Obiekt alokowany jest najczesciej jako obiekt wbudowany (value type) wewnatrz glownej instancji modelu (np. instancji aktora).
2. Przy inicjalizacji lub zmianie opcji graficznych wywolywana jest metoda `SetLODThresholds`, gdzie progi konwertowane sa na wartosci podniesione do kwadratu w celu unikania kosztownych operacji pierwiastkowania (`sqrt`) w petli renderowania.
3. W kazdej klatce przy podjeciu decyzji o renderowaniu wywolywane jest `SelectLOD(float distanceSq)`. Zwracana wartosc to 0 (najwyzsza jakosc, model znajduje sie najblizej), 1 (srednia jakosc) lub 2 (nizsza jakosc, model znajduje sie najdalej).

# Dokladna Mapa Zaleznosci (Exact Dependency Map)

* **Zaleznosci wejsciowe (Inbound):** Menedzerowie instancji lub aktorow wywolujacy metode renderowania i updatowania, prawdopodobnie klasy rzedu `CInstanceBase` lub `CActorInstance`, bazujace na odleglosci od kamery z `CCamera`.
* **Zaleznosci wyjsciowe (Outbound):** Modul jest silnie odizolowany i w pelni samowystarczalny, nie jest uzalezniony od zadnego podsystemu zewnetrznego z racji dzialania jako czysta funkcja matematyczna.
* **Drzewo dyrektyw `#include`:** W pliku naglowkowym wymaga jedynie `<cstdint>`, zas w `.cpp` ewentualnie odwoluje sie do `../StdAfx.h` (przy uzywaniu prekompilowanych naglowkow). Brak jakichkolwiek szans na powstawanie cyklicznych zaleznosci.
* **Model pamieciowy:** Wbudowany jako czesc klasy `value type`. Posiada tylko czyste wlasciwosci w formie zmiennoprzecinkowych float.

# Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabela Klas i Struktur

* **Nazwa:** `EterLib::Render::ActorLODSelector`
* **Rola:** Kalkulator poziomu LOD bazujacy na kwardacie odleglosci do progow widocznosci.
* **Wielkosc w bajtach:** 8 bajtow (zawiera dwa floaty).
* **Wlasciciel watku:** Glowny watek logiki/renderowania.

### Tabela Metod Publicznych

* `void SetLODThresholds(float lod1Dist, float lod2Dist) noexcept`
  * **Typy argumentow:** `float lod1Dist` (Dystans w jednostkach ze swiata dla 1 poziomu), `float lod2Dist` (Dystans w jednostkach ze swiata dla 2 poziomu).
  * **Wartosc zwracana:** `void`
  * **Warunki wstepne i skutki uboczne:** Brak gwarancji stanu poczatkowego, modyfikuje dwa zmienne floatowe (dystanse odniesienia). Prekalkuluje kwadraty odleglosci.
* `[[nodiscard]] uint32_t SelectLOD(float distanceSq) const noexcept`
  * **Typy argumentow:** `float distanceSq` (Kwadrat dystansu do obiektu wyznaczany z `D3DXVec3LengthSq`).
  * **Wartosc zwracana:** `uint32_t` (Identyfikator poziomu szczegolowosci od 0 do 2).
  * **Warunki wstepne i skutki uboczne:** Czysta funkcja "read-only", sprawdza dystans z zapisanymi w pamieci progami (LOD2 -> LOD1 -> LOD0). 

### Pamieciowy Layout Struktur (Memory Layout & Offsets)

* `float m_lod1DistSq` (Offset: 0x0)
* `float m_lod2DistSq` (Offset: 0x4)
Ze wzgledu na lekki format i brak wskaznika `vtable` rozmiar zajmuje rowno 8 bajtow. Ulatwia to czytanie odleglosci i offsetow na biezaco przez oprogramowanie AI/hookingi typu Arthion.

# Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

* **Pakiety Sieciowe:** Modul odpowiada wylacznie za efekty i dzialanie po stronie klienta bez wchodzenia w interakcje po TCP/IP z GameSerwerem. Brak polaczonych pakietow (CG/GC).
* **Metody Pythona:** Nie istnieja bezposrednie wrapper'y PyMethodDef w Python API. Konfiguracje odleglosci dla kazdego z poziomow ulegaja zmianom bazujac na globalnych ustawieniach srodowiskowych `CPythonSystem` lub z wnetrza systemu konfigurowania efektow `EterLib::Render`.

# Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

* **Zasady wielowatkowosci:** Ze wzgledu na brak uzycia `std::mutex` lub `std::atomic`, klasa musi byc modyfikowana (`SetLODThresholds`) przed startem w render loop, a jednoczesnie nie nalezy modyfikowac odleglosci w procesie wywolywania dzialania `SelectLOD`.
* **Potencjalne punkty awarii (Crash Points & Edge Cases):** Wejscie `float distanceSq` nigdy nie powinno miec nieliczbowego formatu powstalego z blednej kalkulacji (NaN). Dodatkowo przekazanie niezakodowanego do kwadratu parametru bedzie prowadzic do ciaglego zwracania LOD 0. Poniewaz instancja jest uzywana na stosie lub wbudowana w wyzej sklasyfikowane obiekty, wyklucza to ryzyko awarii typu Null Dereference.
* **Zarzadzanie zasobami (RAII):** Brak dynamicznych alokacji na stercie i zaleznosci od zewnetrznych bibliotek API. System garbage object nie wystepuje. 

# Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

* **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** W razie koniecznosci wlaczenia kolejnego trybu dla najdalszych obiektow `LOD 3`, dodaj nowe pole pamieciowe pod spodem np. `m_lod3DistSq`, a nastepnie edytuj funkcje wprowadzajaca prog. Modyfikuj cialo funkcji `SelectLOD`, aby umiescic warunek typu `if (distanceSq >= m_lod3DistSq)` przed obecnym ifem odpowiadajacym za LOD 2 (warunki maja charakter malejacy).
* **Jak debugowac i logowac:** Do poprawnego zbadania wystarczy postawic Watch na polach `m_lod1DistSq` i `m_lod2DistSq` (korelowane przez rzut ze struktury CActorInstance), oraz logowanie badajace jakie z wejsciowych wartosci `distanceSq` przekraczaja limity. Uzyj do tego pliku logowania systemowego (syserr).
* **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Ten modul swietnie wspolgra z narzedziami do Headless Testing, np. poprzez GTest. Zwykle zaalokowanie na lokalnym stosie (`ActorLODSelector selector;`), przypisanie ustawien `selector.SetLODThresholds(100.0f, 200.0f);` a nastepnie sprawdzenie wyjsc z `SelectLOD` za pomoca instrukcji `EXPECT_EQ(...)` przy podawaniu odpowiednich wartosci kwadratowych umozliwia pelne i skuteczne unit testy.
