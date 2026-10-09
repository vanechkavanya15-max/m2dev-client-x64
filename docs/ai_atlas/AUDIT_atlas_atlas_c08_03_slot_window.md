---
task_id: "atlas_c08_03_slot_window"
cluster: "UI"
module_name: "CSlotWindow - Okno Obslugi Slotow Przedmiotow"
target_files:
- src/EterPythonLib/PythonSlotWindow.cpp
- src/EterPythonLib/PythonSlotWindow.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c08_03_slot_window.md"
architecture_layer: "Interfejs Uzytkownika, Okna i System Tekstu"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport z Audytu: CSlotWindow

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
- **Funkcja modulu**: Modul `CSlotWindow` jest fundamentalnym elementem interfejsu uzytkownika (UI) w kliencie gry Metin2. Odpowiada za obsluge logiki oraz wizualizacji siatek (gridow) i pojedynczych slotow przeznaczonych na przedmioty. Jest uzywany w oknach ekwipunku, pasku skrotow (quickbar), magazynie, oknach handlu oraz umiejetnosci. Obsluguje wyswietlanie ikon, skalowanie modeli do wlasciwego rozmiaru w gridzie (np. 1x1, 1x2, 1x3, 2x2), nakladanie filtrow wizualnych (cooldown, zablokowanie) oraz interakcje mysza (Klikniecie, Select, Drag & Drop).
- **Miejsce w petli gry**: Metody modulu (przede wszystkim `OnUpdate` oraz `OnRender`) sa wywolywane z glownej petli gry w watku interfejsu uzytkownika i renderowania klatki. Oznacza to, ze odpowiadaja m.in. za plynna animacje odliczania czasu (cooldown overlay) i reakcje na polozenie kursora, aktualizowane co Frame.
- **Przeplyw danych**: Skrypty UI napisane w jezyku Python alokuja obiekt okna i uzywaja API C++ do tworzenia kolejnych komorek (`AppendSlot`). W trakcie interakcji z graczem, silnik detekcji kolizji klienta (na bazie wspolrzednych X/Y kursora) przechwytuje zdarzenia (np. klikniecie, over), wewnatrz C++ dopasowuje zdarzenie do danego indeksu slota (z listy `TSlot`) i emituje wywolanie zwrotne (callback) do zdefiniowanego w Pythonie handlera za pomoca systemu Stringow Internowanych (np. wywolujac `OnSelectItemSlot`). Ostateczna logika rozgrywki i generowanie pakietow w odpowiedzi na klikniecie zalezy wiec bezposrednio od modulu warstwy Python.
- **Cykl zycia obiektow**: Obiekt inicjalizowany jest w fazie parsowania okien poprzez konstruktor i wywolanie ukrytego `__Initialize()`. Przypisanie zasobow przebiega recznie w wywolaniach konfiguracyjnych z Pythona. Dealokacja (wywolywana recznie w `Destroy()` oraz w dekonstruktorze) czysci zawartosc kontenera STL (`m_SlotList`), niszczy dedykowane efekty, timery i kontrolki potomne uzywajac wewnetrznego `CWindowManager`.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound)**: Modul wywolywany jest przede wszystkim przez narzedzia renderujace warstwy EterPythonLib (CWindowManager) zarzadzajace hierarchia okien oraz warstwe skryptowa Pythona ktora modyfikuje stan slotow podczas runtime'u.
- **Zaleznosci wyjsciowe (Outbound)**:
  - *DirectX 9 / Graphic*: Uzywa `CPythonGraphic::Instance()` by wykonywac operacje renderowania prymitywow prostokatnych dla cieni / cooldownow (RenderBox2d, RenderBar2d, RenderCoolTimeBox) oraz zarzadza `CGraphicImageInstance`, `CAniImageBox`.
  - *Engine UI System*: Bazuje na `CWindow`, operuje okienkami pomocniczymi jak `CSlotButton`, odwoluje sie do submodulow `CWindowManager`.
  - *GameLib*: Istnieje bezposrednie powiazanie z logika gry w GameLib (co lamie nieco warstwy izolacji) do ulatwienia sprawdzania poziomow umiejetnosci przez `CPythonPlayer` i `CPythonSkill` (uzywane glownie przy logice transferu odnowienia z `TransferSlotCoolTime`).
- **Drzewo dyrektyw `#include`**: Wypisane zrodla includuja: `StdAfx.h`, `EterBase/CRC32.h`, `EterBase/Filename.h`, `PythonWindow.h`, `PythonSlotWindow.h`, `PythonInternedStrings.h`, `UserInterface/PythonSkill.h`, `UserInterface/PythonPlayer.h`. Znaczace zaleznosci krzyzowe do Core wystepuja w powiazaniu UI z Game (Player/Skill).
- **Model pamieciowy**: Kod uzywa archaicznego modelu opartego na "nagich" wskaznikach (raw pointers) C++ (np. `CImageBox*`, `CGraphicImageInstance*`, `CCoverButton*`) zamknietych w ramach wezlow STL (jak `std::list<TSlot>`, `std::map`). Wymaga to manualnego `delete` w metodach niszczacych (Destroy), brak jest stosowania RAII czy inteligentnych wskaznikow (std::unique_ptr / std::shared_ptr).

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)
- **Tabela Klas i Struktur**:
  - `UI::CSlotWindow`: Klasa bazowa (dziedziczy po `CWindow`). Implementuje mechanizm obslugi grupy slotow oraz logiki interfejsu. Watek: Glowny Render Loop.
  - `UI::CSlotWindow::TSlot` (alias `SSlot`): Struktura trzymajaca dane pojedynczego slotu. Przechowuje id (dwSlotNumber), wspolrzedne lokalne (ixPosition, iyPosition), rozmiary (ixCellSize, iyCellSize), stany blokady (dwState), parametry czasu odnowienia (fCoolTime, fStartCoolTime) oraz zbior recznie zarzadzanych wskaznikow do powiazanych obiektow UI (np. pCoverButton, pNumberLine, pFinishCoolTimeEffect). Wielkosc: ~100-120 bajtow (zaleznie od wariantu kompilatora i paddingu z uwaga na pointery 32bit / 64bit). Watek: Glowny.
  - `UI::CSlotWindow::SStoreCoolDown`: Prosta struktura przechowujaca zapamietane czasy odnowienia (fCoolTime, fElapsedTime, bActive).
  - `UI::CSlotWindow::CSlotButton`: Zagniezdza klase `CButton`, wewnetrzna obsluga przycisku przypietego stricte pod jeden przypisany mu dwSlotNumber.
  - Enumerable: `ESlotStyle`, `ESlotState`.
- **Tabela Metod Publicznych (`CSlotWindow`)**:
  - `void AppendSlot(DWORD dwIndex, int ixPosition, int iyPosition, int ixCellSize, int iyCellSize)`: Definiuje nowy element w liscie `m_SlotList`. 
  - `void SetSlot(DWORD dwIndex, DWORD dwVirtualNumber, BYTE byWidth, BYTE byHeight, CGraphicImage * pImage, D3DXCOLOR& diffuseColor)`: Konfiguruje wizualna reprezentacje polozonego elementu na danym logicznym slocie.
  - `void SetSlotCount(DWORD dwIndex, DWORD dwCount)` / `SetSlotCountNew(...)`: Renderuje mala cyfre prawego-dolnego naroznika slotu (ilosc przedmiotow).
  - `void SetSlotCoolTime(DWORD dwIndex, float fCoolTime, float fElapsedTime = 0.0f)`: Rozpoczyna wewnetrzny zegar odnawiania i rendering filtra ciemniejszego koloru w ukladzie "wskazowkowym".
  - `BOOL GetSlotPointer(DWORD dwIndex, TSlot ** ppSlot)`: Wyciaga do wewnetrznego uzytku referencje do kontenera przechowujacego dany slot, zwraca boolean o powodzeniu zlokalizowania slota (zlozonosc obliczeniowa wyszukiwania jest O(N) po `m_SlotList`). Zwracany podwojny pointer sluzy jako out-param by zmodyfikowac wewnetrzny wskaznik.
- **Pamieciowy Layout Struktur (Memory Layout & Offsets)**:
  Wazne przy podpinaniu narzedzi automatycznych/botow. Struktura TSlot ulozona jest z DWORD'ami na poczatku. Layout zaklada (offsety szacunkowe):
  0x0: `dwState`, 0x4: `dwSlotNumber`, 0x8: `dwCenterSlotNumber`, 0xC: `dwItemIndex`, 0x10: `isItem` (BOOL). Zewnetrzne systemy FFI powinny bazowac na metodzie uzywania pakietow Python, zamiast na re-parsowaniu offsetow tej listy w pamieci by pominac zjawiska pading'ow struktury (sizeof i offsetof moga byc zmienne).

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Pakiety Sieciowe**: Brak bezposredniego polaczenia, warstwa interfejsu (wizualna). Konsekwencje akcji wykonanych na oknie generuja w Pythonie pakiety typu `HEADER_CG_ITEM_MOVE`, `HEADER_CG_ITEM_DROP`, `HEADER_CG_ITEM_USE` itp., jednak sam CSlotWindow ich nie obsluguje.
- **Metody Pythona (`PyMethodDef`)**: API wewnetrznie przesyla informacje o zdarzeniach w modulu uzywajac dynamicznie podczepianych wywolan, realizowanych poleceniem: `UI::PythonInternedStrings::Call`. Modul zaklada nasluchiwanie przez obiekt Pythona ponizszych funkcji:
  - `OnSelectEmptySlot(int iSlotNumber)`
  - `OnSelectItemSlot(int iSlotNumber)`
  - `OnUnselectEmptySlot(int iSlotNumber)`
  - `OnUnselectItemSlot(int iSlotNumber)`
  - `OnUseSlot(unsigned long dwSlotNumber)`
  - `OnOverInItem(unsigned long dwSlotNumber)`
  - `OnOverOutItem()`
  - `OnPressedSlotButton(unsigned long dwSlotNumber)`

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci**: Jak wszelkie instancje pochodne od `CWindow` opierajace sie na `CPythonGraphic`, tak i to API, jest CALKOWICIE nieodporne na obsluge z innych watkow. Operacje i inicjalizacja MUSZA zostac przeprowadzone wewnatrz glownego watku gry, zapobiega to crash'om Direct3D oraz Race Condition na wewnetrznych elementach STL (listy, mapy) ktore nie sa objete Mutexami (`std::mutex`).
- **Potencjalne punkty awarii (Crash Points & Edge Cases)**:
  - Naruszenia Cyklu Zycia (Dangling Pointers): Elementy potomne jak przyciski sa niszczone w trakcie `Destroy()` i referencje usuniete ze strony `CWindowManager`, ale jesli zewnetrzny modul Pythona nadal przetrzymuje referencje lub odwoluje sie opoznionym call'em, klient zwroci blad `0xC0000005` z powodu pointer dereference do usunietej instancji slotu (np. `m_pToggleSlotImage`).
  - Liniowy czas wyszukiwania slotow: Przeciazanie CSlotWindow jako tablicy z dziesiatkami tysiecy slotow poskutkuje "lagiem" iteracyjnym z uwagi na implementacje przez `std::list`. Przeliterowywanie w `OnRender` list do O(N) potrafi dlawic proces renderowania uzytku okna typu duzy magazyn.
- **Zarzadzanie zasobami (RAII)**: Brak stosowania konwencji RAII powoduje zwiekszone szanse na Memory Leak'i (Wycieki pamieci). Podczas alokowania obiektow z silnika (jak okienka, obrazy `CGraphicImageInstance`) nalezy wkladac ekstremalny wysilek na weryfikacje zwolnienia w funkcji Destroy / Destruktorze, szczegolnie dla rekonfiguracji layoutu na zywym organizmie.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide)**:
  1. Dodaj odpowiednia wlasciwosc do struktury `TSlot` badz samej klasy w pliku naglowkowym `PythonSlotWindow.h`.
  2. Jesli to dodatkowy element GUI (np. nakladka w postaci ikonki), alokuj wskaznik w konstruktorze `CSlotWindow::__Initialize` na NULL.
  3. Zaimplementuj kod obslugi tworzenia oraz poprawnego niszczenia elementu (za pomoca `delete` badz dedykowanego zwalniacza) w funkcji `CSlotWindow::Destroy`.
  4. Modyfikuj zachowanie w funkcji `CSlotWindow::OnRender()` gdzie petla dokonuje przejscia (iteracji) po obiektach graficznych do renderingu na klatke.
  5. By wystawic opcje dla UI skryptu zewnetrznego (Pythona), dodaj obsluge metody C-API (w wrapperze `PythonWindowManagerModule.cpp` ktory zarzadza UI) wykonujaca rzutowanie pointera i wywolujaca nowa funkcje.
- **Jak debugowac i logowac**: Kod ten nie wykorzystuje zbytnio wbudowanych narzedzi takich jak `LogBox` czy `TraceError` w waznych sciazkach logiki. W celu implementacji debugowania, poleca sie wstrzykniecie w kodzie funkcji `OnMouseLeftButtonDown` wywolan `Tracef()` rzucajacych w konsole `syserr.txt` detale o trafieniu kolizji kursora myszy (`lx`, `ly`).
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness)**: Ze wzgledu na bardzo ciasne powiazanie i monolitycznosc systemu GUI wzgledem `DirectX` oraz `Python`, modul CSlotWindow jest trudno testowalny poza glownym kontekstem. Agenty maja dostep do koncepcji flagi kompilacji `#define TEST_ENV`. Jesli chce sie ztestowac strukture modulu z poziomu C++ Doctest Framework: stworz zmockowany obiekt `CPythonGraphic`, `CWindowManager` minimalnymi srodkami, oraz odepnij powiazanie z `CPythonPlayer` posrednictwem polimorfizmu (w aktualnej implementacji brak jednak odpowiednich interfejsow). Wskazane testowanie "Black-Box" przez mockowane instancje eventow i symulacje wywolan PyAPI na powiazanych elementach.
