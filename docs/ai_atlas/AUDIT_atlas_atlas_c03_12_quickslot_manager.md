---
task_id: "atlas_c03_12_quickslot_manager"
cluster: "ITM"
module_name: "CQuickslotManager - Pasek Szybkiego Dostepu"
target_files:
- src/UserInterface/QuickslotManager.cpp
- src/UserInterface/QuickslotManager.h
- src/Client/Gameplay/QuickslotDomain.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c03_12_quickslot_manager.md"
architecture_layer: "Ekwipunek, Przedmioty, Handel i Gospodarka"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport AI: CQuickslotManager - Pasek Szybkiego Dostepu

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Modul `QuickslotManager` pelni role zarzadcy interfejsu paska szybkiego dostepu w grze (tzw. Quickslot). Pozwala on graczowi przypisac przedmioty z ekwipunku (np. mikstury lecznicze), umiejetnosci oraz emocje do dostepnych na ekranie pol (zwykle klawisze 1-4, F1-F4). 

Architektonicznie modul zostal podzielony w duchu Zero-Conflict na dwie spojne warstwy:
1. **`Client::Gameplay::QuickslotDomain`** - bezstanowa (wzgl. UI i sieci) i pozbawiona bezposrednich referencji do Pythona / DirectX domena logiki. Odpowiada za przechowywanie 36 slotow (maksymalna pojemnosc) w `std::array`, zarzadzanie biezaca strona (od 0 do 3, cztery strony z 8 slotami = 32 sloty dostepne pod skrotami) i sprawdzanie inwariantow logicznych oraz arytmetycznych. Wykorzystuje C++23 `std::expected`.
2. **`QuickslotManager`** - warstwa adaptacyjna (glue layer), ktora utrzymuje instancje powyzszej domeny, implementuje interfejs oczekiwany przez dawny system `CPythonPlayer` (legacy C-API) i tlumaczy interakcje interfejsu na odpowiednie pakiety wysylane przez singleton `CPythonNetworkStream::Instance()`.

**Przeplyw Danych (Data Flow):**
Akcja UI (np. przeciagniecie ikony, wcisniecie przycisku uzycia) -> Wywolanie metody `Request...` na `QuickslotManager` -> Utworzenie konkretnego pakietu (np. `SendQuickSlotAddPacket`) i wyslanie do serwera -> Po potwierdzeniu z serwera, wywolywane sa metody ustawiajace stan (np. `AddQuickSlot`), ktore bezposrednio aktualizuja strukture danych w domenie `QuickslotDomain` oraz w synchronizowanej tablicy `m_quickSlots`.

**Cykl zycia:** Obiekt domeny alokowany jest staly czas wraz z samym managerem. Czyszczenie przed ponownym uzyciem realizowane jest poprzez `Clear()`, wypelniajace sloty zerami (brak typu, brak pozycji).

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

- **Zaleznosci wejsciowe (Inbound):**
  - Wywolywany przez kod obslugi klawiszy (UI) oraz skrypty interfejsu z Pythona poprzez Python-C API z `PythonPlayer` lub dedykowane mostki skryptowe.
  - Wywolywany przez Handlery sieciowe przy odbieraniu informacji GC (Game->Client) o modyfikacji quickslotu z innych systemow.
  - Eventy: UI korzysta z handlerow zwrotnych (np. `m_skillClickHandler`, `m_pageChangeHandler`).

- **Zaleznosci wyjsciowe (Outbound):**
  - Modul komunikacji sieciowej: `CPythonNetworkStream` (via `PythonNetworkStream.h`) uzywany do emisji zadan takich jak dodanie/usuniecie/przesuniecie czy uzycie (itemu ze slotu).
  - Obiekty zdarzen przekazywane za posrednictwem `std::function`.

- **Drzewo dyrektyw `#include`:**
  - **Domenowe:** `<cstdint>`, `<array>`, `<span>`, `<optional>`, `<expected>`, `../../EterBase/Result.h`, `../Core/StrongTypes.h` (wlasne typy bezpieczne, brak odniesien cyklicznych).
  - **Menedzera:** `GameType.h`, `Packet.h`, `Client/Gameplay/QuickslotDomain.h`, `PythonNetworkStream.h` (bezposrednie odwolanie), `StdAfx.h`, `<algorithm>`, `<cstring>`, `<functional>`.

- **Model pamieciowy:**
  - Wszelkie struktury `m_slots` i `m_quickSlots` dzialaja jako prealokowane `std::array` bez zadnej alokacji na stercie w petli. Delegaty to dynamiczne func object (`std::function`). Calosc scisle okreslona, bez wyciekow pamieciowych (stos/skladowe instancji menedzera).

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabele Klas i Struktur
| Symbol | Rola | Rozmiar / Pola | Watek |
|--------|------|----------------|-------|
| `Client::Gameplay::QuickslotItem` | Reprezentacja pojedynczego slotu. | 2 bajty (`uint8_t type`, `uint8_t position`). | Glowny / Logika |
| `Client::Gameplay::QuickslotDomain` | Czysta logika biznesowa Quickslotow. | Zawiera `std::array<QuickslotItem, 36>` i `int32_t m_pageIndex`. | Glowny |
| `QuickslotManager` | Glue layer, zglasza pakiety i obsluguje zderzenia z UI. | Posiada `m_domain`, kopie `m_quickSlots`, delegaty eventow. | Glowny |

### Metody Publiczne (wybrane)

#### QuickslotDomain
- `void SetPage(int32_t pageIndex)` - bezpiecznie kalkuluje (modulo/offset) wazna strone.
- `SlotIndex LocalToGlobalIndex(SlotIndex localSlotIndex) const` - przelicza slot 0-7 na 0-35 odpowiedniej strony.
- `std::expected<QuickslotItem, EterBase::InventoryError> GetSlot(SlotIndex globalSlotIndex)` - pobranie z weryfikacja.

#### QuickslotManager
- `void RequestUseLocalQuickSlot(DWORD dwLocalSlotIndex)` - rozroznia `SLOT_TYPE_INVENTORY`, `SLOT_TYPE_SKILL`, `SLOT_TYPE_EMOTION` i stosuje odpowiednia akcje sieciowa lub event.
- `TQuickSlot & RefGlobalQuickSlot(int SlotIndex)` - UWAGA: zwraca encje referencyjna legacy. Przy blednym indexie zwraca statyczny falszywy obiekt `s_kQuickSlot` by ominac crash.

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
Dla systemow botow / hookingowych:
- `QuickslotDomain`: offset 0 -> `std::array` wielkosci 72 bajty. Nastepnie padding i `m_pageIndex` (int32_t).
- `QuickslotManager`: offset 0 -> obiekt `QuickslotDomain`. Potem legacy `std::array<TQuickSlot, 36>`, po czym obiekty `std::function`.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

- **Pakiety Sieciowe wysylane (CG):**
  - Akcje uzytkownika do Quickslotow sa enkapsulowane za pomoca istniejacego pakietu obslugi. W `CPythonNetworkStream` zawarto odpowiedniki: `SendQuickSlotAddPacket`, `SendQuickSlotDelPacket`, `SendQuickSlotMovePacket`.
  - Dla przedmiotu INVENTORY: uzywany bezposrednio `SendItemUsePacket` z odpowiednim slotem. W przypadku skilli i emocji uruchamiany jest delegat.
  - Odpowiedniki po stronie serwera to prawdopodobnie komendy uzycia lub wymiany slotow (HEADER_CG_QUICKSLOT_ADD, itp.).

- **Metody Pythona (`PyMethodDef`):**
  - Funkcjonalnosc jest udostepniana przez powloke w zewnetrznym pliku bindowania (np. `PythonPlayerModule.cpp` lub podobnym). Kod menedzera bezposrednio nie zajmuje sie udostepnianiem sam z siebie, ale obsluguje bezposrednio polecenia UI z API za pomoca np. `GetLocalQuickSlotData()`.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Potencjalne punkty awarii (Crash Points):** Metoda `RefGlobalQuickSlot` stosuje obejscie blednego indeksu jako cicha obsluge ("silent fail") poprzez statyczna zmienna `s_kQuickSlot`. Nalezy uwazac by tej statycznej wartosci lokalnej nie uzywac gdzies dalej poza zasiegiem widocznosci, ani na zaufanie stanu. Do nowego kodu koniecznie uzywac `GetDomain().GetSlot(...)` z `std::expected`.
- **Wielowatkowosc:** Modul obecnie dziala plynnie na jednowatkowym systemie renderu/gry - operacje sieciowe (zapisy asynchroniczne i synchroniczne pakiety UI) moga wymagac lockowania przy ewentualnym przechodzeniu na pelen asynchron, tu brakuje mutexow na tablicach `m_quickSlots`.
- **Zasady architektoniczne (Zero-Conflict):** Domena `QuickslotDomain` celowo w obrebie nowej subarchitektury posiada wlasna definicje `QuickslotItem`, podczas gdy manager w celach kompatybilnosci uzywa starszego `TQuickSlot`. Synchronizowanie ich dwoch (w np. `AddQuickSlot`) stanowi niewielki overhead ale pozwala odcinac zaleznosci od naglowkow interfejsu (legacy). Nie uzywaj stalych magicznych, zawsze uzywaj `QUICKSLOT_MAX_NUM`.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. *Jesli zmieniasz logike:* Zmodyfikuj `src/Client/Gameplay/QuickslotDomain.h/cpp` (np. obsluga dodatkowych regul ukladania itemow). Dodaj test do w pelni mockowalnych Unit Testow.
  2. *Jesli zmieniasz sposob polaczenia z UI:* Dodaj nowy delegat do `QuickslotManager` korzystajac z interfejsu `<functional>`.
  3. Zawsze korzystaj z typow domenowych `SlotIndex` z `Core::StrongTypes` aby nie pomylic indeksu globalnego z lokalnym. Zobacz `LocalToGlobalIndex`.

- **Jak debugowac i logowac:**
  - W razie braku reakcji w slocie, wprowadz breakpoint lub log w funkcji wewnatrz `RequestUseLocalQuickSlot`. Przewaznie problemem bedzie nieuruchamianie wlasciwego `m_skillClickHandler`.

- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  - Sama `QuickslotDomain` mozna podpiac pod `doctest` od razu ze wzgledu na zaleznosc wylacznie do standardu i EterBase/Core typow.
  - Testowanie calego menedzera mozna osiagnac piszac atrape dziedziczaca lub implementujaca pseudo `CPythonNetworkStream`, poniewaz modul bezposrednio odwouje sie do singletona `Instance()`. Tego kroku raczej nalezy unikac na korzysc testowania wlasnie samej instancji `QuickslotDomain`.
