---
task_id: "atlas_c03_01_item_data_core"
cluster: "ITM"
module_name: "CItemData - Definicje Przedmiotow i Prototypy (item_proto)"
target_files:
- src/GameLib/ItemData.cpp
- src/GameLib/ItemData.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c03_01_item_data_core.md"
architecture_layer: "Ekwipunek, Przedmioty, Handel i Gospodarka"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** `CItemData` to podstawowa klasa hermetyzujaca statyczne dane o przedmiotach pochodzace z bazy `item_proto` na potrzeby klienta. Przechowuje informacje o wlasciwosciach (typ, podtyp, wymagania, bonusy, ceny, flagi), a takze referencje do zasobow graficznych (modele w reku, modele wyrzucone na ziemie, ikony) ze srodowiska `EterLib::ResourceManager`. Dziala jako rdzen informacji o danym Vnum przedmiotu.
- **Moment wywolania:** Modul samoistnie jest pasywna struktura danych. Jest uzywany w nastepujacych momentach:
  - Faza inicjalizacji i logowania gry: ladowanie i cachowanie calej struktury `item_proto` do wektorow przez `CItemManager`.
  - Faza Renderu: Pobieranie tekstur ikon (`GetIconImage`) do rysowania siatki ekwipunku w interfejsie uzytkownika, a takze decydowanie o renderze lsnienia broni (`GetSpecularPowerf`).
  - Faza logiki/eventow: Przypisywanie i podlaczanie modeli broni z Granny 3D do instancji gracza (`CActorInstance`), ewaluacja zalozenia sprzetu (wymogi statystyk i flag).
- **Przeplyw danych (Data Flow):** Surowe definicje bazy binarnej kopiowane sa z pamieci przez `CItemManager` przy uzyciu bezposredniego narzutu pamieci (`memcpy(&m_ItemTable, pItemTable, sizeof(TItemTable));`) w metodzie `SetItemTableData`. Na zapytanie o konkretne wlasnosci (np. przez `PythonItem` lub UI) wywolywane sa hermetyczne gettery o zlozonosci czasowej O(1). Pobieranie zasobow z zewnatrz (modele i ikony) opiera sie na tzw. leniwym inicjowaniu (lazy loading) badz jawnym `__LoadFiles()`.
- **Cykl zycia (Lifecycle):** Obiekty `CItemData` sa alokowane uzywajac dedykowanej wlasnej puly `CDynamicPool<CItemData> ms_kPool` (bez uzycia systemowego `new` w glownej petli). Metoda klasowa `CItemData::New()` alokuje swiezego handlera, `Clear()` resetuje wszystkie zmienne z powrotem do zera, a zniszczenie i zwrot obsluguje metoda statyczna `Delete()`. To gwarantuje niski stopien defragmentacji sterty na modely liczace tysiace wystapien.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** 
  - Menedzery: `CItemManager` zarzadza cala kolekcja.
  - Skrypty i interfejsy Pythona: `CPythonItem` (obsluga tooltipow i dzwieku typu `__GetUseSoundType`), `CPythonPlayer` (sprawdzanie posiadanego sprzetu), `PythonItemModule` (eksport stalych dla interfejsu uzytkownika).
  - Silniki wyrenderowania i postaci: `CActorInstance` / `ActorInstanceAttach` (laczenie slotow ekwipunku postaci i reki z modelami), `CInstanceBase` (efekty refinementu i swiecenia z broni/zbroi).
  - Obliczenia poboczne klienta: `FishingHandler`, `LootPriorityRankCalculator`, walidatory sieci i inwentarza.
- **Zaleznosci wyjsciowe (Outbound):** 
  - `EterLib` -> `ResourceManager` w celu wciagania binarnego `CGraphicThing` (pliki .gr2) i `CGraphicSubImage` (ikony przedmiotow .sub/.tga). 
  - Logowanie za pomoca wbudowanych preprocesorowych macro-dyrektyw `TraceError()`.
  - Klasa pozostaje izolowana w GameLib, brak bezposrednich zaleznosci od WinSocka (brak bezposredniego ruchu) lub DirectX (poza oddelegowaniem modeli).
- **Drzewo dyrektyw `#include`:** 
  - `StdAfx.h` - Naglowek prekompilowany wymuszony architektura - tworzy grube powiazanie (zagrozenie przy refaktoryzacji iz moze ciagnac za soba np. Windows.h).
  - `EterLib/ResourceManager.h` w .cpp oraz `EterLib/GrpSubImage.h`, `EterGrnLib/Thing.h`, `GameType.h` w .h. 
- **Model pamieciowy:** W module wszedzie uzywa sie "golych" wskaznikow (`CGraphicThing*`, `CGraphicSubImage*`) typowych dla starszych standardow (w C++98/03/11 bez wykorzystania `std::unique_ptr` z racji wlasnosci spoczywajacej w Menedzerach i Puli). Referencje wewnetrzne `m_pModelThing` sa typu "slabego" (poziom powiazania jest pasywny, zwalnianiem obiektow wlasciwych po wylaczeniu zajmuje sie EterLib - w module ItemData zasoby nie dostaja instrukcji `delete`).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur:**
  - `CItemData`: Pula z danymi (200-300 bajtow lacznie z wektorami) zawierajaca wskazniki leniwych modelow 3D i obiektow 2D, bufor `TItemTable`, operujaca jako interfejs publiczny do danych item_proto na kliencie.
  - `CItemData::TItemTable`: Hermetycznie skompresowana (przez preprocesor `#pragma pack(1)`) struktura, jej pamiec dokladnie odpowiada definicji C struct dla serwera / bazy metina. Posiada m.in. `szName[65]`, limity, wartosci gniazd. Umozliwia bezposrednie mapowanie offsetami binarnego pliku bazy wprost na pamiec.
  - `CItemData::TItemLimit`: Tablica wewnetrzna limitow (poziom i atrybuty wymagane). Struktura ta posiada `BYTE bType` oraz `long lValue` co zajmuje rowne 5 bajtow w pamieci uzywajac pack(1).
  - `CItemData::TItemApply`: Atrybuty i bonusy jakie item dostarcza standardowo (max HP, odpornosci, itp.). Identyczna struktura z `bType` (5 bajtow pod pack 1).

- **Kluczowe Enums (Typy Oznaczania i Flagi):**
  - `EItemType`: Od `ITEM_TYPE_WEAPON` przez `ITEM_TYPE_ARMOR`, do `ITEM_TYPE_FISH` i modyfikacji dodanych dla szarf/petow/kostiumow. Okreslaja na najwyzszym poziomie z jakim rodzajem kodu pracujemy.
  - `EWeaponSubTypes` (SWORD, DAGGER, BOW) / `EArmorSubTypes` / `EUseSubTypes` itd.: Opis podtypow poszczegolnych asortymentow majacych kluczowe znaczenie przy uzyciu i animacjach (np. `WEAPON_BOW` wlacza odrebny silnik fizyczny strzal i toru uderzenia).
  - `EItemFlag` / `EItemAntiFlag` / `EWearPositions`: Parametry logiczne uniemozliwiajace upuszczanie, handel lub okreslajace gdzie dokladnie w inwentarzu (jak i ciele awatara) ukladany jest element.

- **Tabela Metod Publicznych:**
  - `SetDefaultItemData(const char*, const char*)` / `SetItemTableData(TItemTable*)`: Ustawia instancje (wymaga zainicjalizowanego bufora wejsciowego TItemTable). Brak zwracanej wartosci.
  - `const char* GetName()`, `BYTE GetType()`, `const char* GetUseTypeString()`, `DWORD GetWeaponType()`: Metody zlozonosci O(1) interpretujace zmienne bez modyfikacji wlasnych stanow.
  - `BOOL GetLimit(BYTE byIndex, TItemLimit * pItemLimit) const`: Oczekuje z gory `byIndex < ITEM_LIMIT_MAX_NUM`. Zwraca skopiowana na podany wskaznik odpowiednia instancje, uzywa narzedzia `assert()` przeciw bledom indeksu zewnetrznego (programisty).
  - `UINT GetRefine() const`: Interfejs ujawniajacy stopien ulepszenia opierajacy sie (obecnie w tym branchu) wylacznie na operacji matematycznej: `GetIndex() % 10` - w metinie to znaczy ze 279 odnosi sie do "broni 270 + 9".
  - `CGraphicThing* GetModelThing()` / `GetIconImage()`: Inicjalizuja lub zwracaja cached obiekt. Warunek the leniwe wczytanie zadzialalo i obiekt istnial u Menedzera Wirtualnych Plikow gry. Skutek uboczny: przywolanie pliku do pamieci z dysku, jesli wywolywane z `GetIconImage()`.

- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  - Ustalony blok w `TItemTable` dla agentow hookujacych (przy pragma pack(1)): 
    - `[0x00]` `DWORD dwVnum`
    - `[0x08]` `char szName[65]`
    - `[0x49]` `char szLocaleName[65]`
    - `[0x8A]` `BYTE bType`, `BYTE bSubType`...
  - To pozwala zewnetrznym modyfikacjom takim jak Arthion zczytujac bajty w runtime zrzucic obiekty na logike Rust'a.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Modul `ItemData` samoistnie NIE zajmuje sie wysylka pakietow, jednak jest absolutnie integralny w tworzeniu pakietow sieciowych o typie Handlu (Shop, Exchange), uzyciu Itemu czy Sieciowych handlerow. W kodach typu `FishingHandler` nastepuje porownanie bazujace na typie podanym w paczce ze sprawdzeniem prawidlowosci poprzez `CItemManager::GetItemDataPointer(...)`. Jesli po uzyciu, klient zauwazy niezgodnosc `EItemAntiFlag` a probuje wrzucic to w oknie Trade'u, pakiet z oknami w ogole nie zostanie sformulowany przez interfejsy wyzej polozone w grafie wywolan.
- **Metody Pythona (`PyMethodDef`):** Enumeratory stanow sa bezposrednio zrzucane na wirtualna maszyne w pliku `UserInterface/PythonItemModule.cpp`. Zdefiniowano tam miedzy innymi wpisy typu `PyModule_AddIntConstant(poModule, "APPLY_RESIST_EARTH", CItemData::APPLY_RESIST_EARTH);`. Cale API modulowe Pythona jest wiec w oparciu o sztywny rejestr stalych. Interfejs z samym instansem obiektu odbywa sie m.in. poprzez funkcje CPythonItem::GetInstance().

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Obiekt to Singleton-like Pool z alokatorem jednowatkowym. Metin2 na poziomie zasobow i klienta glownego renderu oraz ladowania dziala przewaznie w Single-Thread. Przestrzega sie braku lockowania na muteksach. Przejscie modulu w architekturze wielowatkowej bez oblozenia `ms_kPool` oraz metody `SetItemTableData` lockguardami skonczy sie natychmiastowym UB i crashem.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - `GetLimit(BYTE byIndex)` i reszta uzywa bezwzglednie asercji `assert(byIndex < ...)` w wersji debug w C++. Jesli serwer lub plik `item_proto` wysle niewalidyzowane gniazda gora-dol (OutOfBounds), klient odrzuci te informacje badz rzuci wyjatkiem przy wczesniejszej kompilacji deweloperskiej.
  - Brak bezpieczenstwa referencji - zewnetrzny kod uzywa `CItemManager::GetItemDataPointer` i dostaje surowy wskaznik z powrotem. Brak sprzawdzenia `if (ptr == nullptr)` na zwrocie doprowadzi do twardego craszu w systemach np. rysowania tooltipa ekwipunku, jesli wystepuje defekt w spakowaniu item_proto klienta i vnum u gracza figuruje a u klienta gry fizycznie nie istnieje.
- **Zarzadzanie zasobami (RAII):** Kod jest relatywnie lekki. Wlasne zarzadzanie instancjami lezy po stronie `CDynamicPool`. Istnieje ryzyko, ze niezwalniane zasoby obrazow po ich asynchronicznym badz leniwym zaladowaniu nie zwolnia sie z pamieci jesli serwer mialby tendencje do spamowania nowymi encjami uzytkownika, a nie obsluzy tego `CResourceManager`.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Otworz i zadeklaruj nowa wartosc enum np. w `EUseSubTypes` (plik `ItemData.h`). Zachowaj ostroznosc podczas naruszania esencji rozmiarow `TItemTable`. Najlepiej trzymac parametry w dotychczasowych limitach albowiem modyfikacja rozmiaru `TItemTable` zrywa binarne zaleznosci odczytow `item_proto` na dysku lokalnym oraz kompatybilnosc parsowania. 
  2. Jesli enum sluzy pod system dla Pythona, przejdz do interfejsu (na przyklad `PythonItemModule.cpp`) by dodac rejestr przez `PyModule_AddIntConstant(...)`.
  3. Zaaktualizuj zachowania interfejsowe i eventowe obslugujac swoja nowa zmienna (np. nowe modyfikatory dzwiekowe w `CPythonItem::__GetUseSoundType`).
- **Jak debugowac i logowac:** Klasyczne macro `TraceError("%s...", )` dostepne jest do monitoringu. Sprawdzenie stanu logiki nie wymaga break-pointa w renderowaniu, jednak weryfikacja leniwego przypisania `m_pIconImage` podczas otwarcia plecaka to klasyczny przyklad miejsca stopu na upewnienie sie ze path ikonografiki prawidlowo wchodzi w funkcje dociagajaca `EterLib`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Wymaga to zmockowania zaleznosci od `EterLib::ResourceManager` oraz wyrzucenia `#include "StdAfx.h"` na rzecz odrebnego macro preprocesora. Inicjalizacja do pamieci odbywa sie recznie (`CItemData::New();`), wstrzykuje sie preparowana strukture binarnej reprezentacji i uderza w poszczegolne gettery. Nie ma problematyki polaczen sieciowych czy sprzetu DirectX poniewaz wyciaganie wlasciwosci wewnatrz logiki to w 95% operacje operujace na RAM (CPU bound).
