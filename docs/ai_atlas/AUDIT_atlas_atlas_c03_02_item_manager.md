---
task_id: "atlas_c03_02_item_manager"
cluster: "ITM"
module_name: "CItemManager - Menedzer Zasobow Przedmiotow"
target_files:
- src/GameLib/ItemManager.cpp
- src/GameLib/ItemManager.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c03_02_item_manager.md"
architecture_layer: "Ekwipunek, Przedmioty, Handel i Gospodarka"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport AI: CItemManager - Menedzer Zasobow Przedmiotow

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Dokladna funkcja modulu:** `CItemManager` zarzadza baza danych przedmiotow w kliencie Metin2. Sluzy jako globalna pamiec podreczna (cache) dla obiektow `CItemData`, przechowujaca definicje przedmiotow (statystyki, obostrzenia) z ladowanych tablic binarnych `item_proto`, asocjacje z modelami 3D i ikonami (z `item_list.txt`) oraz teksty opisow (z `itemdesc.txt`). Udostepnia szybkie metody do pobierania wlasciwosci przedmiotu na podstawie jego VNUM (identyfikatora wirtualnego przedmiotu).
- **Miejsce w petli gry:** Ladowanie zasobow zachodzi jednorazowo podczas fazy inicjalizacji aplikacji (Phase Loading / logowanie do gry). Pobieranie informacji (`GetItemDataPointer` / `SelectItemData`) nastepuje asynchronicznie i na zadanie np. podczas otrzymywania pakietow sieciowych (Network Tick), tworzenia tooltipow (OnUpdate), renderowania ikon w inwentarzu i rzucania itemow na ziemie (OnRender).
- **Przeplyw danych (Data & Control Flow):** 
  1. `CItemManager` laduje dane z wirtualnego systemu plikow VFS (`LoadItemTable` dekompresuje plik binarne poprzez biblioteke LZO).
  2. Przypisuje domyslne sciezki modeli `.gr2` i ikon `.tga` uzywajac parsera pliku tesktowego (`LoadItemList`).
  3. Wzbogaca struktury przedmiotow o dynamiczne opisy i podsumowania za pomoca `LoadItemDesc`.
  4. Nastepnie przechowuje zainicjalizowane struktury w mapie `m_ItemMap` (klucz to VNUM).
  5. Inne moduly wywoluja `GetItemDataPointer` lub stary interfejs ze stanem `SelectItemData`, aby uzyskac wlasciwosci w danym momencie.
- **Cykl zycia obiektow:** Obiekty `CItemData` sa alokowane i inicjowane podczas fazy ladowania (lub przez wywolanie funkcji `MakeItemData`) za pomoca zoptymalizowanej puli wlasnej `CDynamicPool<CItemData> ms_kPool`. Dealokacja wszystkich instancji w mapie odbywa sie dopiero podczas zamkniecia systemu w funkcji `Destroy()` poprzez zwolnienie ich z powrotem do puli (`CItemData::Delete`). Zapewnia to stabilny pointer retention w trakcie gry.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Do tego modulu zapytania skladaja praktycznie wszystkie interfejsy operujace na logice przedmiotow: `CPythonItemModule` (C-API dla Pythona), `CPythonPlayerModule` (C-API ekwipunku), podsystem sieciowy handlerow `Network/Handlers/` (np. walidacja, paczki `GC::ITEM_SET`, drop z ziemi, wymiana), pakiety handlu, sklepika, wedkarstwa (`CharDispatcher_Fishing`, `ItemTooltip_SocketFormatter`).
- **Zaleznosci wyjsciowe (Outbound):** Zaleznosci wyjsciowe to modul wirtualnych plikow i zarzadzania scieszkami (VFS): `PackLib/PackManager` oraz `EterLib/ResourceManager`, kompresja pamieci zewnetrznej `EterBase/lzo` (funkcje deszyfrujace), jak i klasa docelowa modelu danych `ItemData` oraz parser tekstowy z pamieci `CMemoryTextFileLoader`.
- **Drzewo dyrektyw `#include`:** 
  `#include "StdAfx.h"`, `#include "PackLib/PackManager.h"`, `#include "EterLib/ResourceManager.h"`, `#include "EterBase/lzo.h"`, `#include "ItemManager.h"`, `#include "ItemData.h"`. (Czyste dyrektywy bez widocznego ryzyka cyklicznego).
- **Model pamieciowy:** W klasie uzyto wylacznie wskaznikow w stylu jezyka C. Glowna relacja przechowywana jest w `std::map<DWORD, CItemData*> m_ItemMap`. Osobny wektor `std::vector<CItemData*> m_vec_ItemRange` przyspiesza przeszukiwanie przedmiotow z zakresu VNUM (np. ksiegi ulepszen dzialajace dla wielu VNUM'ow - np. od vnum + vnumRange).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabele Klas i Struktur:**
  - `CItemManager`: Rozszerzenie generycznego wzorca `CSingleton<CItemManager>`. Menedzer i magazyn definicji modeli przedmiotow w pamieci podrecznej. Brak synchronizacji blokad (owner thread: watek glowny aplikacji).
  - `CItemData`: Definiuje pelny graf wlasciwosci wirtualnego przedmiotu, od modeli po statystyki i sockety.
  - `CItemData::TItemTable`: Odzwierciedla wiersz definicji z pliku `item_proto`. Rozmiar ustala dyrektywa `#pragma pack` (1 bajt alignment). W zaleznosci od limitow zadeklarowanych w makrach (np. APPLY_MAX_NUM), zajmuje blisko 150-180 bajtow pamieci w jednym bloku struktury.

- **Tabele Metod Publicznych:**
  - `BOOL CItemManager::GetItemDataPointer(DWORD dwItemID, CItemData ** ppItemData)`: Najwazniejsza i bezpieczna metoda przeszukujaca slownik przedmiotow wedlug ID (VNUM). Wynik przypisuje do podanego podwojnego wskaznika. Zwraca TRUE jezeli sukces, FALSE z bezpiecznym ominieciem jezeli brakuje definicji i loguje `Tracef(" FIND ERROR [%d]\n")`.
  - `BOOL CItemManager::SelectItemData(DWORD dwIndex)`: Wybiera konkretny VNUM przedmiotu, zastepujac wewnetrzny stan buforu menedzera (`m_pSelectedItemData`).
  - `CItemData * CItemManager::GetSelectedItemDataPointer()`: Zwraca przetrzymywany wyzej obiekt definicji, bazujacy na stanie.
  - `bool CItemManager::LoadItemTable(const char* c_szFileName)`: Odczyt z LZO zdekodowanej tablicy przedmiotow i polaczenie VNUM z nazwa obrazka `icon/item/%05d.tga`. Funkcja modyfikujaca strukture slownika z licznikiem `dwElements`.
  - `bool CItemManager::LoadItemList(const char* c_szFileName)`: Interpretuje rozdzielana tabulacjami zawartosc `.txt` definiujaca model wyposazalny np. zbroi, sciezke VNUM i ikone.

- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  W klasie `CItemData::TItemTable` krytyczne zaleznosci: bajtowy klucz unikalnego rekordu to `dwVnum` (offset: 0), zas kolejne wazne elementy po nazwach to `bType`, `dwFlags` (np. ITEM_FLAG_COUNTABLE), oraz tablice struktur nakladajacych bonusy: `aLimits` i `aApplies`.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Walidacja definicji nastepuje po stronie pakietow w module `CPythonNetworkStream`. Identyfikatory VNUM ladowane z pakietow opartych o ekwipunek, takie jak `TPacketGCItemSet` (`GC::ITEM_SET`), drop `TPacketGCItemGroundAdd`, badz transakcje w handlu i bezpiecznym magazynie (`TPacketGCExchange`, `GC::SAFEBOX_SET`).
- **Most do Pythona (`PyMethodDef`):** 
  Ten podsystem wspoldziala na scislym zlaczu mostu Pythona za sprawa `src/UserInterface/PythonItemModule.cpp`. Zdefiniowano tu most eksponujacy metody dla interfejsu (GUI) - modul nazwany "item" z predefiniowana tabela `s_methods[]`. Funkcje z plikow interfejsu UI (`uiInventory.py` oraz `uiToolTip.py`) wywoluja moduly C-API jako np.: `item.SelectItem()`, `item.GetItemName()`, `item.GetIconImageFileName()`, `item.GetValue()`. Metoda dziala opierajac sie na buforze stanow `SelectItemData()`. Dodatkowo, eksportowane sa dziesiatki globalnych pol enum (flagi przedmiotu, pozycje ubioru, typy i subtype bonusow `APPLY_STR`).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Kod modulu operuje wylacznie w oparciu o pamiec watku glownego i UI. Brakuje rozwiazan chroniacych na poziomie operacji lockowania, wiec mutacja tablic z poziomu innych watkow moze spowodowac Race Condition i Data Race przy `std::map`. Zabezpieczeniem przed tym jest ladowanie do pamieci przed wejsciem do logiki wirtualnego swiata mapy i brak modyfikacji TItemMap po etapie inicjalizacji okna GUI PhaseLoading.
- **Typowe pulapki (Crash Points & Gotchas):**
  - **Przekazywanie identyfikatora pustego (0):** Zapytanie `GetItemDataPointer` podajace 0 jako argument poprawnie przechwytuje problem `if (0 == dwItemID) return FALSE;` zapobiegajac operacjom zerowym.
  - **Poleganie na wewnetrznym pointerze globalnym:** Opcja stateful `SelectItemData()` przypisuje skompilowany rezultat zapytania z argumentem VNUM w buforze podrecznym singletonu `m_pSelectedItemData`. To zachowanie moze sprowadzac bledy, gdy dwie rozedrgane funckje lub callbacki nadpisza bufor - wzorzec naraza na ukryty problem pointer validation w zaleznosci od momentu cyklu programu. Poleca sie bezwarunkowo korzystac z `GetItemDataPointer(vnum, &ptr)` - uniezalezniajac stan z globalnego scope.
- **Zarzadzanie zasobami (RAII):** Deserializacja narzuca tworzenie duzych wektorow na the Heap, nastepnie jednak zostaje to usuniete w obiekcie VFS poprzez dealokacje (brak widocznego wycieku dzieki puli alokacyjnej). Z pamiecia nalezy uwazac podczas operowania elementami puli (nie wolno uzywac bezposrednio slow kluczowych standardowych C++, np. de-alokacja odbywa sie przez `CItemData::Delete()` nalezac do pule z `CDynamicPool`, a nie poprzez standardowy wariant `delete pItem`).

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja rozszerzania funkcjonalnosci (Step-by-step):**
  1. Jesli wprowadzasz nowy atrybut i kolumne bazy danych itemow, musisz zaktualizowac tool kompilacji - plik `src/DumpProto/dump_proto.cpp`.
  2. Nastepnie dodaj wlasciwe pole upewniajac sie o wielkosci i alignment struktury `TItemTable` w pliku naglowkowym `src/GameLib/ItemData.h`. Pamietaj o #pragma pack by zachowac scislosc z pakietem binarnym wyeksportowanym do LZO i nie zlamac wielkosci w tablicy struktur.
  3. Skompiluj nowa tablice klienta nowym toolem `DumpProto`. Powieksz rozmiary docelowych buforow wyswietlania. 
  4. Wyeksportuj metode umozliwiajaca wejscie i odczyt nowo dopisanego atrybutu do `src/UserInterface/PythonItemModule.cpp` jako modul C-API (np. nowa opcja PyCFunction METH_FASTCALL) oraz wpisz ja do interfejsu (ui.py, uat, uishop, tooltips).
- **Jak debugowac i logowac:** Przechwytywanie zablokowanych wyszukiwan przy dodaniu nowego item_proto sprawdzaj uzywajac sciezki syserr (wynik makra w C++: `Tracef(" CItemManager::SelectItemData - FIND ERROR [%d]\n")`). Obserwuj wskaznik zwrocony poprzez debug breakpoint po wyjsciu funkcji ladowania wezlow modelarskich LZO. Sprawdz format VNUM. W przypadku ulepszaczy przedzialowych VNUM-Range (magiczne zwoje, metiny dzialajace na duzo opcji) - wektor `m_vec_ItemRange` odpowiada za wylapywanie wyjatkow i odrzucan range'y.
- **Testowanie i Headless:** 
  W wypadku refaktoringu `CItemManager` nie musisz w pelni inicjowac Direct3D, albowiem klasa moze dzialac po za grafika (nie alokuje shaderow, zarzadza jedynie definicjami map w pamieci jako model). Do przetestowania narzedziami doctest stworz symulator-mock interfejsu VFS `CResourceManager`, umozliwiajac lokalne mapowanie rekordu na sztywno, zapobiegajac bledom zwiazanym z szukaniem sciezki LZO podczas kompilacji `test_main.cpp`. Uzywaj operacji bazujacych na wstawianiu `CItemData` i odpytywania funkcji za pomoca `GetItemDataPointer` w asercjach narzedzia doctest. Uzywaj logiki stringowej testowanej w `CMemoryTextFileLoader`.
