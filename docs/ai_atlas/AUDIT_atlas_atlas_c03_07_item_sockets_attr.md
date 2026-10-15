---
task_id: "atlas_c03_07_item_sockets_attr"
cluster: "ITM"
module_name: "System Gniazd (Socketow) i Bonusow Przedmiotow"
target_files:
- src/GameLib/ItemDataRegistry.h
- src/Client/Gameplay/ItemBonusCalculator.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c03_07_item_sockets_attr.md"
architecture_layer: "Ekwipunek, Przedmioty, Handel i Gospodarka"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul ten odpowiada za fundamentalna warstwe danych o przedmiotach oraz dynamiczne kalkulacje ich wlasciwosci w grze Metin2. Zamiast operowac na przestarzalych GUI, logika ta zostala w pelni zmodernizowana. 
W pliku `ItemDataRegistry.h` zdefiniowano `ItemDataRegistry` - centralne statyczne zrodlo wiedzy (Single Source of Truth) o podstawowych cechach wszystkich przedmiotow. Pozwala na o(1) lub szybkie mapowanie VNUM (identyfikatorow) na parametry bazy, z limitami 3 socketow (gniazd na kamienie/przetopy), do 3 na bonusy aplikowane i 6 wartosci bazowych. 
W pliku `ItemBonusCalculator.h` rezyduje `ItemBonusCalculator`, ktory kalkuluje calkowity wplyw zalozonego ekwipunku gracza, dzialajac na abstrakcji `EquippedItemInfo` (do 7 bonusow z ulepszen, 4 sockety - choc w bazie limit to zwykle 3, system jest rozszerzalny). 
Cykl zycia `ItemDataRegistry` zazwyczaj przebiega podczas poczatkowego ladowania (OnLoad/Init) parsowania paczek `.pck` VFS. Dane sa alokowane jednorazowo w `std::unordered_map` dla pojedynczych VNUM oraz `std::vector` dla zakresow. `ItemBonusCalculator` prawdopodobnie wystepuje per instancja postaci lub jako bezstanowy serwis uzywany podzas OnUpdate/OnNetworkTick, gdy klient aktualizuje statystyki (np. po zmianie przedmiotu w ekwipunku albo zalozeniu kamienia). Kod uzywa architektonicznego paradygmatu wstrzykiwania zaleznosci przy uzyciu `std::function` dla logiki customowych kamieni w gniazdach (`SocketBonusResolver`).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** `ItemBonusCalculator` wywolywany jest przez logike ekwipunku (`InventoryDomain.h`, np. funkcja wewnetrzna przeliczajaca state) lub przy aktualizacji statystyk bohatera po odebraniu zmian. `ItemDataRegistry` bedzie uzywane wszedzie tam, gdzie gra prosi o dane bazowe VNUM przedmiotu, wlacznie z renderowaniem GUI, upuszczaniem na ziemie i handlem.
- **Zaleznosci wyjsciowe (Outbound):** Kod opiera sie na typach standardowych z biblioteki standardowej (STL) - `std::unordered_map`, `std::vector`, `std::array`, `std::optional`, `std::function`, `std::string_view`. Brak twardych referencji do DirectX, Python C-API czy Grannego 3D na poziomie samych klas domenowych, co czyni to swietnym, izolowanym subsystemem. Zalezny od `InventoryDomain.h` dla `ItemAttribute`.
- **Drzewo dyrektyw `#include`:** W `ItemDataRegistry.h`: `<cstdint>`, `<string_view>`, `<unordered_map>`, `<vector>`, `<optional>`, `<array>`, `<algorithm>`. W `ItemBonusCalculator.h`: `<cstdint>`, `<vector>`, `<unordered_map>`, `<array>`, `<functional>`, `"InventoryDomain.h"`. Ryzyko cykli jest bliskie zeru, gdyz naglowki uzywaja wbudowanych bibliotek i tylko prostej powiazanej przestrzeni nazw domenowej.
- **Model pamieciowy:** Nowoczesny C++: `std::vector`, `std::array` (kontenery przylegle w pamieci ulatwiajace zaleznosci cache), `std::unordered_map`. Posiada RAII. Unika nagich wskaznikow za wyjatkiem zwrotu const wskaznika w `GetItemData` (bezpieczne poniewaz registry zarzadza zyciem danych). Ciagi znakow w binarnej tablicy `char` rzutowane na `std::string_view`.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabele Klas i Struktur:**
  - `ItemLimit`: Reprezentuje pojedynczy limit uzycia (np. lv, plec). Pola: `uint8_t type`, `int32_t value`. 5 bajtow + padding (zalezne od kompilatora).
  - `ItemApply`: Wbudowany bazowy bonus na itemie. Pola: `uint8_t type`, `int32_t value`.
  - `ItemDataEntry`: Duzy POD reprezentujacy pelen zestaw cech z bazy np. item_proto. Wlasciciel pamieci z registry. Cechy kluczowe: 64 znaki na imiona, limity gniazd `sockets` (max 3), `values`, `applies`.
  - `ItemDataRegistry`: Singleton lub obiekt cyklu zycia klienta do rejestracji / zapytan o itemy (`O(1)` na mape, plus skan vectora na zakresowe VNUM).
  - `EquippedItemInfo`: Struktura agregujaca wyposazenie do kalkulacji - `vnum` (4b), `attributes` (7 elementow tablicy `ItemAttribute`), `sockets` (4 elementy tablicy gniazd np. kamieni dusz). Przeznaczona na stos w czasie wyliczania statystyk.
  - `ItemBonusCalculator`: Bezstanowy serwis/komponent z mozliwoscia przechowywania resolverow wstrzykiwanych za pomoca `std::function`. Posiada rowniez mape dodatkowych, niestandardowych rejestracji.

- **Tabele Metod Publicznych:**
  - `ItemDataRegistry::RegisterItem(const ItemDataEntry&)`: Dodaje item. Brak bezposredniego zwracania bledu.
  - `ItemDataRegistry::GetItemData(uint32_t) const`: Zwraca const wskaznik z zywotnoscia rowna instancji rejestru, lub nullptr przy braku przedmiotu.
  - `ItemDataRegistry::Clear()` i `Size()`: Kontrola pojemnosci mapy.
  - `ItemBonusCalculator::SetSocketBonusResolver(SocketBonusResolver)`: Wstrzykuje lambde/funkcje rozwiazujaca wnetrze VNUM na konkretne wlasciwosci (bonusy z kamieni/przetopow). 
  - `ItemBonusCalculator::RegisterSocketBonus(uint32_t, uint8_t, int16_t)`: Dodaje hardkodowany bonus dla podanego VNUM.
  - `ItemBonusCalculator::CalculateTotalBonus(uint8_t, const std::vector<EquippedItemInfo>&) const`: Sumuje wszystkie atrybuty i wplywy ze wszystkich socketow z calej zalozonej tabeli `EquippedItemInfo`.
  - `ItemBonusCalculator::AggregateAllBonuses(const std::vector<EquippedItemInfo>&) const`: Generuje slownik zbiorczy ze wszystkim naliczonymi bonusami.
  - `ItemBonusCalculator::GetSocketBonuses(uint32_t) const`: Pomocniczo rzutuje konkretny kamien z gniazda na liste bonusow korzystajac z wbudowanej mapy lub resolvera.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Kody w plikach zrolowych sa wyabstrahowane i same te pliki NIE posiadaja bezposredniego kodu protokolu (jak recv() czy send()). Wartosci generowane z kalkulatora prawdopodobnie sa po to, by sprawdzac czy zgadzaja sie z wartosciami przesylanymi pakietem `GC::CHARACTER_UPDATE` z serwera, badz stanowia element lokalnego podgladu u gracza (predykcja po stronie klienta). ItemDataRegistry laduje dane dostarczone przez patche, czesto nie wymaga synchronizacji pakietowej.
- **Metody Pythona:** Ten podsystem zostal celowo oderwany od starej implementacji `CPythonItem` w Python C-API, poniewaz to czysty C++. Nie zawiera na zewnatrz METH_VARARGS czy `PyMethodDef`. Dopiero wrapper wyzej moglby wstrzykiwac C-API na podstawie odpowiedzi tych metod (np. `playerGetStatus` na podstawie zsumowanych atrybutow).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Funkcje w `ItemBonusCalculator` sa generalnie `const` w metodach wykonujacych zapytania, co sprzyja uzytkowaniu miedzywatkowym pod warunkiem, ze rejestracje (Register) sa wykonywane sekwencyjnie w watku inicjalizacji. `ItemDataRegistry` tez nie ma wbudowanego muetxa (np. std::shared_mutex) zatem po wczytaniu powinno stac sie immutable w watkach zapytujacych, aby zapobiec Data Race (wspolbieznego wywolywania `RegisterItem` i `GetItemData`).
- **Niezgodnosci limitow struktur:** `ItemDataEntry` przewiduje 3 sloty gniazd (sockets), a struktura `EquippedItemInfo` w `ItemBonusCalculator` dopuszcza 4 gniazda (czesty bug lub rezerwa po modernizacji na dodatkowy socket bizuterii np. alchemia/energia). 
- **Brak walidacji wskaznikow NULL:** `ItemDataRegistry::GetItemData` zwraca wskaznik `const ItemDataEntry*`, ktory wyladuje z wartoscia null w razie niespelnienia warunkow. AI musi pamietac o weryfikacji przez instrukcje `if (!ptr) [[unlikely]]`.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Dodawanie nowej logiki (Step-by-step):** Jesli musisz dodac nowy system na 4 gniazda i wspierac to na itemach, zmien `ITEM_SOCKET_MAX_NUM` w `ItemDataRegistry.h` z 3 na 4. Dzieki architekturze z `std::array`, cala mapa struktury uaktualni offsety. Aby zmienic interpretacje wygasania (expiration) w kamieniu (np. w przetopie), nalezy zdefiniowac wlasna lambde i wstrzyknac w `SetSocketBonusResolver(SocketBonusResolver)` podczas etapu bootowania modulu Inventory klienta. Nie ruszac starych plikow bez sprawdzenia ZERO-CONFLICT rule.
- **Logowanie i Debugowanie:** Wszelkie bledy przy przypinaniu kamieni dodawaj stosujac `EterBase::ModernLogger::Error("Socket VNUM {} failed resolving", socketVnum);` - korzystajac z formaterow `{}` wg zasady loggowania std::format zamiast starych funkcji TraceError().
- **Unit Test Harness:** Z uwagi na to, ze modul opiera sie na standardowej bibliotece i jest bezstanowy/nie uzywa D3D, mozesz uzyc zwyklego GTest bez potrzeby `TEST_MOCK_D3D9`. Wystarczy dolaczyc oba naglowki, wygenerowac sobie mockowy `std::vector<EquippedItemInfo>` na stosie i odpalic `CalculateTotalBonus`. Idealne do Test-Driven Development (TDD).

