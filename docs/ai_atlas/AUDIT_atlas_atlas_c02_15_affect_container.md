---
task_id: "atlas_c02_15_affect_container"
cluster: "ACT"
module_name: "Kontener Afektow i Stanow Negatywnych/Pozytywnych"
target_files:
- src/UserInterface/AffectFlagContainer.cpp
- src/UserInterface/AffectFlagContainer.h
- src/UserInterface/InstanceControllers/InstanceAffectImpl.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c02_15_affect_container.md"
architecture_layer: "Maszyna Stanow Postaci i Aktorzy"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport AI Atlas: Kontener Afektow i Stanow Negatywnych/Pozytywnych

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Modul `CAffectFlagContainer` pelni role niskopoziomowego kontenera bitowego (bitset) uzywanego do przechowywania stanow i afektow nakladanych na aktorow w grze Metin2 (np. otrucie, spowolnienie, aury, niewidzialnosc). Kontener ma staly rozmiar 64 bitow (8 bajtow), co pozwala na przechowywanie do 64 roznych flag afektow dla kazdej instancji postaci.

W architekturze klienta modul ten nie posiada wlasnej aktywnej logiki petli gry (nie ma metod `OnUpdate` czy `OnRender`). Jest to pasywna struktura danych (Value Object), operujaca wylacznie poprzez bezposrednie wywolania metod z zewnatrz (setter, getter, copy).
Dane sa kopiowane bezposrednio ze strumienia sieciowego w postaci binarnej przy uzyciu metody `CopyData` oraz wymieniane z pakietami synchronizacji.

Cykl zycia `CAffectFlagContainer`:
- **Alokacja i Inicjalizacja**: Najczesciej tworzony na stercie jako czesc wiekszego obiektu (np. aktora lub pancerza) lub bezposrednio jako wartosc na stosie, z konstruktorem wywolujacym `Clear()`, ktory zeruje wszystkie bity.
- **Aktualizacja**: Metody `Set`, `IsSet` uzywane do manipulacji, zas `CopyData` naklada bity bezposrednio z bajtow pakietow (np. sieciowych z serwera). Posiada takze helper konwersji bitow na liczby `ConvertToPosition`.
- **Dealokacja**: Bez specjalnej dealokacji (czysta pamiec statyczna obiektu, brak alokacji dynamicznych sterty wewnatrz kontenera).

*Uwaga historyczna: Zgodnie z inwentaryzacja, plik `src/UserInterface/InstanceControllers/InstanceAffectImpl.cpp` wymieniony w liscie zadania NIE ISTNIEJE fizycznie w repozytorium. Analiza opiera sie na dostarczonym naglowku i zrodle klasy glownej.*

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound):** 
  - Glownie wywolywany przez kod obslugujacy pakiety z serwera odbierajace flagi statusow afektow, a takze przez logike postaci, instancje aktorow wewnatrz silnika postaci oraz narzedzia UI.
- **Zaleznosci wyjsciowe (Outbound):** 
  - Loger klienta w postaci funkcji `TraceError`, wywolywany przy probie dostepu do bita spoza dozwolonego zakresu (> 64). Brak powiazan z DirectX czy silnikiem renderowania w samym module kontenera. 
- **Drzewo dyrektyw `#include`:** 
  - W pliku naglowkowym (h): `<cstdint>` (dla stalych typow jak `uint32_t`).
  - W pliku implementacyjnym (cpp): `"StdAfx.h"`, `"AffectFlagContainer.h"`.
- **Model pamieciowy:** Brak alokacji pamieci dynamicznej. Czyste modyfikowanie tablicy na stosie / bezposrednio w klasie (`Element m_aElement[BYTE_SIZE]`). Nie korzysta ze smart pointerow. Zoptymalizowany dla maksymalnej wydajnosci i malej zlozonosci instrukcji.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabela Klas i Struktur
| Nazwa | Rola | Wielkosc (bajty) | Wlasciciel watku |
|-------|------|------------------|------------------|
| `CAffectFlagContainer` | Przechowywanie bitowych stanow afektow | 8 bajtow (tablica `m_aElement`) | Any Thread (Brak mutexow - uzywac tylko na main thread lub w izolacji) |

### Tabela Metod Publicznych
| Sygnatura | Wartosc Zwracana | Warunki Wstepne | Skutki Uboczne |
|-----------|------------------|-----------------|----------------|
| `CAffectFlagContainer()` | `void` | Brak | Zeruje wszystkie bity. |
| `~CAffectFlagContainer()` | `void` | Brak | Brak operacji. |
| `void Clear()` | `void` | Brak | Uzywa `memset`, czysci wszystkie 8 bajtow do `0`. |
| `void CopyInstance(const CAffectFlagContainer&)` | `void` | Podany argument wlasciwie wyalokowany | Kopiuje 8 bajtow za pomoca `memcpy`. |
| `void Set(uint32_t uPos, bool isSet)` | `void` | `uPos < 64` (kontrolowane) | Zmienia pojedynczy bit, zglasza `TraceError` gdy IndexOutOfBounds. |
| `bool IsSet(uint32_t uPos) const` | `bool` | `uPos < 64` (kontrolowane) | Zwraca wartosc logiczna bita. Zglasza `TraceError` i `false` gdy out of bounds. |
| `void CopyData(uint32_t uPos, uint32_t uByteSize, const void* c_pvData)` | `void` | `c_pvData` musi wskazywac na pamiec z przynajmniej `uByteSize` bajtow. | Wykonuje shift bitowy zapisujac podane bajty w tablice kontenera od podanej pozycji `uPos`. |
| `void ConvertToPosition(unsigned* uRetX, unsigned* uRetY) const` | `void` | Wskazniki nie moga byc `nullptr` | Rzutuje `m_aElement` na inty, wklejajac bitowe flagi pod `uRetX` (pierwsze 4 bajty) i `uRetY` (kolejne 4 bajty). |

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
- `CAffectFlagContainer` (Calkowity rozmiar: 8 bajtow):
  - Offset `0x00`: `Element m_aElement[8]` (gdzie `Element` = `unsigned char`). 

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Pakiety Sieciowe:** Modul bezposrednio parsuje czesci bitowe pakietow uzywajac metody `CopyData` oraz rzutuje pakiet do dwoch unsigned intow przez `ConvertToPosition`. Rozmiar kontenera `BYTE_SIZE` odpowiada typowym mniejszym bitsetom afektow serwera do 64 roznych typow (np. bitset afektow przesylany w `TPacketGCCharacterAdd` jako 2 `DWORD`s: `dwAffectFlag[2]`).
- **Metody Pythona (`PyMethodDef`):** Modul jest wylacznie klasy C++ po stronie bazowej. Eksportowanie poszczegolnych stanow do Pythona w warstwie UI gry jest obslugiwane przez zewnetrzne moduly opakowujace.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Kontener NIE jest thread-safe. Brak obiektow wlasnych typu `std::mutex`. Instancje musza byc obslugiwane na watku w ktorym zostaly zainicjowane lub obiekty wywolujace modyfikacje musza stosowac wlasna synchronizacje. Zazwyczaj bedzie to glowny watek gry lub watek obslugi zdarzen sieciowych.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - **`CopyData` pointer danger:** `c_pvData` musi miec zagwarantowana poprawna wielkosc, metoda nie waliduje pamieci wejsciowej. Moze spowodowac Out-Of-Bounds (OOB) memory read.
  - **`ConvertToPosition` crash point:** Metoda zaklada, ze wskazniki `uRetX` i `uRetY` sa uzyteczne, wyluskanie `*uRetX` na `nullptr` spowoduje segfault. Metoda zaklada rowniez iz klasa posiada `BYTE_SIZE >= 8` (2x 32-bit unsigned uinty to 8 bajtow). Zmiana `BIT_SIZE` na ponizej 64 zlamie rzutowanie przez wskaznik w `ConvertToPosition`.
- **Zarzadzanie zasobami (RAII):** Nie alokuje na stercie dynamicznej zadnych zasobow, jest w 100% bezpieczna jako zmienna na stosie i jako kompozycja innych klas.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zwiekszenie ilosci dostepnych stanow (jesli pow. 64) wymaga modyfikacji `BIT_SIZE` w `AffectFlagContainer.h`.
  2. Zmiana `BIT_SIZE` MUSI zaktualizowac cialo `ConvertToPosition`, gdyz rzutowanie `pos[0]` i `pos[1]` nie zadziala dla mniejszego ani nie przesle wiekszego `BIT_SIZE` niz 64 bity. 
  3. Najlepszym rozwiazaniem przedluzenia jest zrezygnowanie z `ConvertToPosition` i wysylanie calego kontenera paczka binarna `memcpy` na gniazdach.
- **Jak debugowac i logowac:**
  - Standardowy log "TraceError" zadziala przy wyjsciu za zakres. Nasluchuj na bledy `CAffectFlagContainer::Set` w konsoli klienta deweloperskiego.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  - Proste unit testy `std::assert` podlaczajace bitowe flipery, tworzac zmienna typu `CAffectFlagContainer container;`, iterujac 0-63 uzywajac `Set`, i nasluchujac zwrotow `IsSet`. Mozna testowac na dummy harnessie bez D3D9.
