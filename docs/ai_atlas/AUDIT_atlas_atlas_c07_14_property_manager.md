---
task_id: "atlas_c07_14_property_manager"
cluster: "MOD"
module_name: "CPropertyManager - Baza Wlasciwosci Budynkow i Dekoracji"
target_files:
- src/GameLib/Property.cpp
- src/GameLib/PropertyManager.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c07_14_property_manager.md"
architecture_layer: "Modele 3D, Szkielety Granny, Drzewa i Efekty"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Modul zarzadza parowaniem oraz przechowywaniem wlasciwosci obiektow takich jak budynki, drzewa oraz efekty, zapisanych w archiwach lub plikach gry jako .prb, .prd, .prt. Wlasciwosci obiektow w grze reprezentowane sa na podstawie sumy kontrolnej CRC nazwy pliku. Klasa CPropertyManager to glowny zarzadca odpowiedzialny za ladowanie, zapisywanie i pobieranie wlasciwosci (CProperty), zas samo CProperty odpowiada za przetwarzanie pojedynczego bloku wlasciwosci.
- **Moment wywolania w petli gry:** Glowny menedzer (CPropertyManager) inicjalizowany jest raz, zwykle na etapie ladowania gry lub silnika zasobow (etap Initialize). Rejestracja wlasciwosci nastepuje dynamicznie przy starcie poprzez wczytanie wlasciwosci ze spakowanego pliku w formacie (ZPack), lub recznie.
- **Przeplyw danych (Data Flow & Control Flow):**  
  1. CPropertyManager zostaje zainicjalizowany przez funkcje `Initialize()`, z podaniem pliku archiwum zasobow (z reguly spakowanego pliku konfiguracyjnego wlasciwosci).
  2. Inicjalizacja archiwum (`CPack`) i pobranie jego slownika indeksow.
  3. Rejestracja poszczegolnych plikow wlasciwosci. Jezeli dany element slownika ma sciezke 'property/reserve', zostaje wczytany przez `LoadReservedCRC()`. Wszystkie pozostale pliki zostaja przekazane do funkcji `Register()`.
  4. Funkcja `Register()` wykorzystuje `CPackManager::Instance().GetFile()` do pobrania bufora danych. Bufor jest potem parsowany w klasie `CProperty` w funkcji `ReadFromMemory()`. Format wlasciwosci zawiera poczatkowy naglowek (np. 'YPRT') a potem typowe wlasciwosci klucz-wartosc. Wartosci przechowywane sa w klasie CProperty w specjalnym slowniku w postaci tokenow (CTokenVectorMap).
  5. Plik jest mapowany pod kluczem, ktorym jest jego suma kontrolna (CRC).
  6. Przyszli klienci podsystemu korzystaja z `CPropertyManager::Get(DWORD dwCRC, CProperty ** ppProperty)` lub Get ze sciezka aby pobrac skompletowane CProperty.
- **Cykl zycia (Lifecycle):**
  Obiekty CProperty sa alokowane za pomoca slowa kluczowego `new` bezposrednio w `CPropertyManager::Register` i zarzadzane (posiadane) przez mapy z CPropertyManager (`m_PropertyByCRCMap`). Dealokacja calego drzewa wlasciwosci nastepuje w momencie zniszczenia `CPropertyManager` wywolujac `Clear()`, ktory korzysta z `stl_wipe_second` usuwajac wylaczone elementy.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Menedzer ten zazwyczaj wolywany jest z WorldEditor'a i modulu Mapy w procesie budowania oraz rysowania swiata w grze, ladujacego dekoracje oraz budynki wedlug ich unikalnych ID, m.in. z pakietow i ladowania modulu PackManager.
- **Zaleznosci wyjsciowe (Outbound):** 
  - `PackLib/PackManager.h` oraz klasy powiazane `CPack`, `TPackFile` do ladowania archiwow ze spakowanymi zasobami.
  - Kod pomocniczy z silnika np. `CSingleton<CPropertyManager>`, narzedzie z bazowego Etera (`EterBase/TempFile.h`).
  - Ladowanie tekstowe z `CMemoryTextFileLoader`.
  - Kod bezpieczenstwa / obslugi CRC (`GetCRC32`).
- **Drzewo dyrektyw `#include`:** 
  `Property.h`: `<string>`
  `Property.cpp`: `"StdAfx.h"`, `<string.h>`, `"EterBase/TempFile.h"`, `"PropertyManager.h"`, `"Property.h"`
  `PropertyManager.h`: `"PackLib/PackManager.h"`
  `PropertyManager.cpp`: `"StdAfx.h"`, `"PackLib/PackManager.h"`, `"PropertyManager.h"`, `"Property.h"`
- **Model pamieciowy:** Do alokacji obiektow stosowane sa czyste wskazniki (`CProperty *`). Menedzer paczek sam w sobie alokowany jest z poziomu `std::shared_ptr<CPack> m_pack;`. Czyste wskazniki zawarte w pamieci CPropertyManager zwalniane sa explicit.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabele Klas i Struktur:**
- `CProperty`: Klasa przechowywujaca wlasciwosci w formie mapy nazwa/wartosc dla danego obiektu na planszy gry (budynek/drzewo/dekoracja). Wlasciciel watku glownego. Rozmiar zalezny jest od m_stTokenMap (mapy wektorow kluczy std::string).
- `CPropertyManager`: Glowny singleton zarzadzajacy pobieraniem i ladowaniem zasobow budynkow (Property). Klasa przechowuje mapy CRC -> CProperty. Singleton glowny na gre.

**Tabela Metod Publicznych (CProperty):**
- `CProperty(const char * c_pszFileName)`: Konstruktor wczytujacy plik na poziomie konfiguracyjnym (bez ladowania z pamieci).
- `void Clear()`: Czysci wektor kluczy.
- `bool ReadFromMemory(const void * c_pvData, int iLen, const char * c_pszFileName)`: Laduje z bufora pamieci plik naglowka ('YPRT') oraz mapuje kolejne tokeny klucz-wartosc. Return true na sukces.
- `const char * GetFileName()`: Getter.
- `bool GetVector(const char * c_pszKey, CTokenVector & rTokenVector)`: Pobiera z mapy tokenow wektor tokenow za pomoca klucza (np. wektor koordynatow dla `Position`). Zwraca `false` jesli klucza brak.
- `bool GetString(const char * c_pszKey, const char ** c_ppString)`: Pobiera string wedlug pierwszego pola tokenu w wektorze.
- `void PutVector(const char * c_pszKey, const CTokenVector & c_rTokenVector)`: Ustawia / Modyfikuje wektor tokenow w mapie.
- `void PutString(const char * c_pszKey, const char * c_pszString)`: Tworzy i podstawia wpis jako single-string value z przypisanym wektorem 1-elementowym.
- `DWORD GetSize()`: Zwraca liczbe wpisow klucz-wartosc.
- `DWORD GetCRC()`: Getter CRC obiektu dla mapowania w Managerze.

**Tabela Metod Publicznych (CPropertyManager):**
- `bool Initialize(const char * c_pszPackFileName)`: Ladowanie zasobow glownych (CPropertyManager inicjalizacja bazy CRC). Inicjalizuje struktury VFS (CPack).
- `bool LoadReservedCRC(const char * c_pszFileName)`: Laduje tzw plik rezerwacji (`property/reserve`) sluzacy do unikania kolizji z wbudowanymi CRC dla zdefiniowanych stringow.
- `void ReserveCRC(DWORD dwCRC)`: Dodanie pojedynczego CRC do m_ReservedCRCSet.
- `DWORD GetUniqueCRC(const char * c_szSeed)`: Wylosowanie / Wygenerowanie wolnego i nieuzywanego numeru CRC wedlug c_szSeed, rozwiazujace ewentualne kolizje algorytmem brute-force. Zwraca bezpieczne unikalne CRC.
- `bool Register(const char * c_pszFileName, CProperty ** ppProperty = NULL)`: Rejestruje plik do CProperty i alokuje jego obiekt.
- `bool Get(DWORD dwCRC, CProperty ** ppProperty)`: Getter referencyjny na poszukiwany element wg jego sumy kontrolnej.
- `bool Get(const char * c_pszFileName, CProperty ** ppProperty)`: Getter via Register().
- `void Clear()`: Niszczy wszystkie alokowane za pomoca czystych wzkaznikow `CProperty*` z mapy `m_PropertyByCRCMap`.

**Pamieciowy Layout Struktur:**
- `CProperty::m_stTokenMap`: Typu `CTokenVectorMap` (mapa stringow -> wektor stringow) bedace uzywana do slownikowania parametrow.
- `CPropertyManager::m_PropertyByCRCMap`: Klasyczna `std::map<DWORD, CProperty*>`.
- `CPropertyManager::m_ReservedCRCSet`: `std::set<DWORD>` dla chronionych indeksow CRC.
- `CPropertyManager::m_pack`: `std::shared_ptr<CPack>`.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
Modul narazie nie posiada wyeksportowanych funkcji publicznych do API w C-Pythonie. Modul tez nie przesyla wprost informacji przez bezposrednie polaczenia z serwerem. Caly modul odnosi sie do map ladowania elementow estetycznych lokalnych (Client-Side). Serwer przesyla identyfikatory (CRC dekoracji) z mapy, ktore to za pomoca menedzera sa przetwarzane po stronie silnika klienckiego.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Wszystko zostaje operowane w glownym watku ladowania aplikacji. `CPropertyManager::Initialize` wywolywany jest przed wejsciem do glownej petli Direct3D. Z tego wzgledu, w kodzie nie wystepuja locki pamieciowe a uzywanie map oraz setow w petli roboczej nie grozi desynchronizacja (jesli edytory / gra jest poprawnie zamknieta).
- **Zarzadzanie zasobami (RAII) & Potencjalne Wycieki Pamieci:** Modul uzywa klasycznych alokacji przez `new CProperty` podczas `Register()`. W CPropertyManager istnieje mechanizm ktory czysci mape rezydualna (`stl_wipe_second(m_PropertyByCRCMap)` w `Clear()`). Z tego wzgledu niszczenie CPropertyManager'a jest warunkiem bezwzglednym dla poprawnego dealokowania uzytej pamieci dla CProperty. 
- **Zastosowania unikania kolizji hash-ow (Gotcha):** Funkcja `GetUniqueCRC` zwraca pierwsze wolne i bezpieczne hash-CRC z wykorzystaniem prostej rekursji na szukaniu (do seedu dodawane sa po kolei losowe liczby pomiedzy 0, a 9 w loopie nieskonczonym az trafimy w brak kolizji). Ze wzgledu na brak ograniczenia (while(1)) bardzo duze nagromadzenie kolizji i zasobow (lub nieskonczone zapetlanie rand w tym samym secie przez bug siewu RNG) spowoduje twarde zawieszenie algorytmu na wieki.
- **CProperty::GetVector:** Wywolywanie funkcji i przypisywanie elementu bezposrednio moze tworzyc problem zawieszania pamieci map/vector na starszych wdrozeniach. Dlatego autorzy recznie wpisali rozwiazanie push_back iterujac po obiekcie do nowego CTokenVector'a jako workaround dla WorldEditor'a. Nie modyfikowac tego fragmentu.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** W razie proby rozszerzenia parsowania typow .prb / .prd dla nowych wlasciwosci w grze, plik parsuje juz wszystko co w nim jest. Funkcje w klasie uzywajace zasobow (jak CInstance) to glowni uzytkownicy uzywajacy `GetVector` na CProperty. Modul tu opisany uzywa elastycznego CTokenVector'a, ktory jest zdatny na wieksze modyfikacje.
- **Jak testowac i debugowac:** Mozliwe jest odpiecie modulu do srodowiska Headless, w tym celu nalezy zastapic `CPackManager::Instance().GetFile` zaslepka lokalna pobierajaca bufor testowego tekstu do CProperty (wykonujac `ReadFromMemory`). Modul z latwoscia da sie zamknac w biblioteki statyczne, testowac go mozna generujac proste stringi wejsciowe.
- **Lokalne wazne logowania:** Do sledzenia rejestrowania elementow uzywana jest flaga `Tracef` (np. przypisywanie drugiego property nad pierwszym bedzie ostrzegalne). W przypadku bledu dekodowania naglowku funkcji 'YPRT' logowany zrzucany jest poprzez TraceError.
