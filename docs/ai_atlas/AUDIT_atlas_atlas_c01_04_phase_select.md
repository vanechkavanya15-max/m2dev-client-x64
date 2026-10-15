---
task_id: "atlas_c01_04_phase_select"
cluster: "NET"
module_name: "Faza Wyboru Postaci i Empire"
target_files:
- src/UserInterface/PythonNetworkStreamPhaseSelect.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c01_04_phase_select.md"
architecture_layer: "Silnik Sieciowy, Maszyna Faz i Protokol"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Plik `PythonNetworkStreamPhaseSelect.cpp` realizuje w warstwie sieciowej logike fazy "Select", ktora obejmuje miedzy innymi operacje zakladania postaci, usuwania postaci, wyboru krolestwa (empire), wyboru samej postaci do wejscia do gry oraz zmiany nazwy postaci.

Faza ta aktywowana jest za pomoca `CPythonNetworkStream::SetSelectPhase()`. W zaleznosci od parametru `__DirectEnterMode_IsSet()`, gra moze przejsc bezposrednio do fazy ladowania (`SetLoadingPhase`) albo, za posrednictwem skryptow Python (C-API), aktywowac warstwe UI dla wyboru postaci (`SetSelectCharacterPhase`) lub wyboru imperium (`SetSelectEmpirePhase`).

Mechanizm faz w `CPythonNetworkStream` opiera sie na polimorficznych callbackach. Petla zglasza `SelectPhase` jako glowny proces (`m_phaseProcessFunc`), ktory co kazdy "network tick" (klatka gry) wywoluje wewnetrzna metode dispatchujaca - `DispatchPacket(m_selectHandlers)`. Umozliwia to przetwarzanie kolejki pakietow sieciowych przychodzacych w ramach tego konkretnego stanu.

Cykl zycia:
1. Przejscie do fazy Select.
2. Wysylanie zadan akcji przez interfejs (np. `SendSelectCharacterPacket`).
3. Odbieranie pakietow sieciowych poprzez tablice procedur z mapy handlerskiej `m_selectHandlers`.
4. Raportowanie powrotne do logiki w UI z wykorzystaniem C-API Pythona (wywolania np. na obiekcie okna fazy z uzyciem metody z czesci `m_apoPhaseWnd`).
5. Przejscie do fazy nastepnej i opuszczenie obecnej po zainicjowaniu przez inny podsystem.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

- **Zaleznosci wejsciowe (Inbound):** 
  Modul ten jest wywolywany po pomyslnym procesie logowania (przejscie z `LoginPhase`) za posrednictwem managera logowania i maszyny stanow. Interfejs zdefiniowany w Pythonie (UI wyboru postaci) wykorzystuje wiazania C-API zeby aktywowac wysylanie pakietow, takich jak `SendSelectEmpirePacket`, `SendCreateCharacterPacket` itd.
  
- **Zaleznosci wyjsciowe (Outbound):** 
  - EterLib / Siec: `Send()` i `Recv()` z bazowych klas `CNetworkStream`, mechanizmy dyspozycji (tzw. handler map `m_selectHandlers`).
  - Python / UI: `PyCallClassMemberFunc` wykorzystywany do zglaszania stanow (sukces/porazka) z powrotem do warstwy prezentacji (zmienne klasy to np. m_poHandler, m_apoPhaseWnd).
  - Narzedzia: Makra debugowania i logowania, np. `Tracen` oraz `TraceError`.
  
- **Drzewo dyrektyw `#include`:** 
  Plik bazuje na wczesniej skompilowanym srodowisku naglowkow (Precompiled Header):
  - `#include "StdAfx.h"`: Zawiera referencje zalezne od srodowiska Windows i EterLib, biblioteki Pythona itp.
  - `#include "PythonNetworkStream.h"`: Definiuje klase `CPythonNetworkStream`, state machine faz sieciowych i metody wysylajace/odbierajace.
  - `#include "Packet.h"`: Definiuje struktury protokolu komunikacyjnego miedzy serwerem gry a klientem, takie jak `TPacketCGSelectCharacter`, opcody typu `CG::CHARACTER_SELECT`.
  
- **Model pamieciowy:** 
  Obiekty klas zarzadzane sa na stosie (structy pakietow sa tworzone lokalnie na stosie dla celow kompilacji pakietu `sizeof`). Nastepnie przesylane jako surowe wskazniki z rozmiarem za pomoca `Send(sizeof(T), &packet)`. Bufor prostych metadanych gracza znajduje sie w tablicy stalego rozmiaru, np. `m_akSimplePlayerInfo` w glownej klasie.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Metod Publicznych i Prywatnych `CPythonNetworkStream` zdefiniowanych w tym pliku:**

1. `void SetSelectPhase()`
   - Zwraca: `void`
   - Pre-conditions: Przejscie wewnatrz grafu stanow (maszyny faz).
   - Side-effects: Aktywuje callbacki przypisane do fazy `Select` dla `m_phaseProcessFunc`. Inicjuje okno UI (Python) w zaleznosci od wyboru.

2. `void SelectPhase()`
   - Zwraca: `void`
   - Pre-conditions: Obecna faza sieci to Select.
   - Side-effects: Wywoluje wewnetrzne `DispatchPacket(m_selectHandlers)`, ktore procesuje kolejke sieciowa.

3. `bool SendSelectEmpirePacket(DWORD dwEmpireID)`
   - Zwraca: Sukces lub porazka wysylania
   - Typy argumentow: `DWORD dwEmpireID` (identyfikator krolestwa).

4. `bool SendSelectCharacterPacket(BYTE Index)`
   - Zwraca: Status operacji `Send`.
   - Typy argumentow: `BYTE Index` (Index postaci wybrany do wejscia w gre, zwykle 0-3 zaleznie od liczby slotow).

5. `bool SendDestroyCharacterPacket(BYTE index, const char * szPrivateCode)`
   - Zwraca: boolean
   - Typy argumentow: `BYTE index` (ID postaci) oraz podany kod pin z warstwy UI - `szPrivateCode`.

6. `bool SendCreateCharacterPacket(BYTE index, const char *name, BYTE job, BYTE shape, BYTE byCON, BYTE byINT, BYTE bySTR, BYTE byDEX)`
   - Typy argumentow: Wszystkie parametry wymagane przez backend serwerowy (statsy i kosmetyki w fazie kreacji) zrzutowane na bajty.
   
7. `bool SendChangeNamePacket(BYTE index, const char *name)`
   - Typy argumentow: Indeks na liscie oraz nowa nazwa gracza.

8. Handler Odbiorcze (np. bool `__RecvPlayerCreateSuccessPacket()`, `__RecvPlayerCreateFailurePacket()`, `__RecvPlayerDestroySuccessPacket()`, `__RecvPlayerDestroyFailurePacket()`, `__RecvChangeName()`)
   - Pre-conditions: Pakiety sieciowe wywolujace operacje z protokolu GC zdefiniowane sa w mapie dispatcherze.
   - Side-effects: Zmieniaja stany tablic w przestrzeni pamiaci modulu `m_akSimplePlayerInfo`, uzywaja `PyCallClassMemberFunc` by uaktualnic interfejs Pythonowski.


### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

- **Pakiety Sieciowe (Network Packets)**:
  - `CG::EMPIRE` (`TPacketCGEmpire`) - Wybor nacji.
  - `CG::CHARACTER_SELECT` (`TPacketCGSelectCharacter`) - Wejscie wybrana postacia do gry.
  - `CG::CHARACTER_DELETE` (`TPacketCGDestroyCharacter`) - Zadanie usuniecia z kodem pin.
  - `CG::CHARACTER_CREATE` (`TPacketCGCreateCharacter`) - Wykorzystuje wiele parametrow (ksztalt, stats).
  - `CG::CHANGE_NAME` (`TPacketCGChangeName`)
  - Otrzymywane odpowiedzi z serwera (GC), ktore obsluguje w kodzie ten obszar pliku: 
    - `TPacketGCPlayerCreateSuccess`, `TPacketGCCreateFailure`, `TPacketGCDestroyCharacterSuccess`, `TPacketGCBlank` dla destroy failure, `TPacketGCChangeName`.
    
  Waznym elementem protokolu z perspektywy stabilnosci (i zgloszonym w regule pamieci) jest sprawdzenie stalej slotu, przy ladowaniu odpowiedzi logowania uzywa m.in stalej `PLAYER_PER_ACCOUNT4`.

- **Metody Pythona (Python Callback Bridge)**:
  Wykonywane na klasie proxy Python za pomoca `PyCallClassMemberFunc`. Uzywane wywolania:
  - na handlerze UI (`m_poHandler`): `SetLoadingPhase`, `SetSelectCharacterPhase`, `SetSelectEmpirePhase`.
  - na fazach szczegolowych (`m_apoPhaseWnd[PHASE_WINDOW_CREATE]` i `PHASE_WINDOW_SELECT`): `OnCreateSuccess`, `OnCreateFailure`, `OnDeleteSuccess`, `OnDeleteFailure`, `OnChangeName`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Zasady wielowatkowosci:** Cale wysylanie (`Send`) i procesowanie w fazie odbywa sie synchronicznie na wiodacym watku w wewnetrznej petli `OnUpdate` interfejsu (ktora potem aktywuje maszyny stanow z glownej petli gry w `Run()`). Klasa ta zazwyczaj rozdziela logicznie ruch sieciowy od samego renderingu przez dyspozycje kolejki (`DispatchPacket`). Brak w kodzie blokad (mutex/lock) z wbudowanej biblioteki standardowej. 
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Bufor na nazwe uzytkownika i kod usuniecia (`szPrivateCode`, `name`) korzysta z niebezpiecznego `strncpy`. Nalezy uwazac aby wartosc `CHARACTER_NAME_MAX_LEN` oraz `PRIVATE_CODE_LENGTH` z protokolu dokladnie odzwierciedlala ta z pliku naglowkowego backendu, inaczej w przypadku FFI nastapi wyciek danych binarnego znaku `\0`.
  - Kod bezpiecznie radzi sobie z przekroczeniem buforu przy obieraniu pakietu (porownuje wielkosc slotu do limitu `PLAYER_PER_ACCOUNT4`). Nalezy utrzymac walidacje np. `bAccountCharacterSlot >= PLAYER_PER_ACCOUNT4`.
- **Zarzadzanie zasobami (RAII):** Brak specyficznych wskaznikow RAII. Obiekty pakietowe alokowane na stosie `TPacketCGEmpire packet` nastepnie wyslane metoda `Send` nie tworza obciazen z perspektywy wycieku pamieci i automatycznie znikaja przy zakonczeniu dzialania bloku funkcji.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zdefiniuj nowa strukture pakietu w `src/UserInterface/Packet.h` dla naglowka CG / GC i nowa stala `enum` protokolu sieciowego.
  2. Zaimplementuj wyslanie metody w `CPythonNetworkStreamPhaseSelect.cpp`, korzystajac z interfejsu np. `bool SendMyCustomPacket(...)`. 
  3. Przejdz do `src/UserInterface/PythonNetworkStream.h` aby dolaczyc jej deklaracje do glownego obiektu klasy.
  4. Dodaj mapowanie (dispatch rule) w funkcji startowej inicjujacej mapy `m_selectHandlers` zazwyczaj na starcie klasy sieci.
  5. Przepnij ja do nowej metody w plikach z logiki Pythona.
  
- **Jak debugowac i logowac:**
  Do zlokalizowania awarii i potwierdzenia poprawnosci parametrow, modul uzywa `Tracen` (log informacyjny) i `TraceError` (logi o bledach krytycznych - np. gdy indeks logowania postaci na ekranie przekracza zalozona wartosc, pojawia sie blad OUT OF RANGE SLOT). Mozna tez postawic breakpoint na poczatku petli `CPythonNetworkStream::SelectPhase()`.
  
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  Jako agent korzystajacy z izolowanego kontenera i zablokowanego dostepu do UI i X11, nie mozemy w pelni odpalic silnika gier pod DX9 na tym Linuxie do celow testow tego protokolu. Metody CPythonNetworkStream mozna zmockowac izolujac klase. Zbudowanie unit testu wyaga zainjectowania mock`a `TPacket...` do wirtualnego ringu/buforu EterLib symulujacego przychodzacy pakiet oraz zastapienia mockiem funkcji systemowych Pythona (takich jak `PyCallClassMemberFunc`), by upewnic sie, ze nie poleca sigsegvy w glownej petli C-API przy analizie naglowkow.
