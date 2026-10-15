---
task_id: "atlas_c01_07_phase_game_actor"
cluster: "NET"
module_name: "Obsluga Pakietow Aktorow i Spawnu (PhaseGame Actor)"
target_files:
- src/UserInterface/PythonNetworkStreamPhaseGameActor.cpp
- src/UserInterface/NetworkActorManager.cpp
- src/UserInterface/NetworkActorManager.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c01_07_phase_game_actor.md"
architecture_layer: "Silnik Sieciowy, Maszyna Faz i Protokol"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul odpowiada za przyjmowanie pakietow z serwera dotyczacych innych aktorow w swiecie gry i tlumaczeniu tych pakietow na odpowiednie instrukcje dla silnika gry.
Stanowi swego rodzaju menedzera cyklu zycia - to on odbiera pakiet dodawania aktora, aktualizacji danych postaci czy usuwania oraz ruchu postaci.
W glownej mierze uzywa on logiki `CPythonNetworkStream`, poniewaz te klasy implementuja bezposrednie przetwarzanie pakietow po stronie klienta - tlumaczac odbierane przez siec obiekty w swiecie gry.
Cykl zycia bytu to odpowiednie `RecvCharacterAppendPacket` z powiazaniem `AppendActor` z `CNetworkActorManager`, ruch pakietem i odtworzeniem go na ekranie.  Aktory dodawane przez ten modul moga pozniej zostac usuniete za pomoca `RecvCharacterDeletePacket` badz zaktualizowane poprzez `RecvCharacterUpdatePacket` (oraz nowa wersje `RecvCharacterUpdatePacketNew`). Odtworzone byty podlegaja rowniez rygorom buforowania zaleznosci renderowania i instancji wizualnych poprzez klase `CNetworkActorManager`. 
Wartoscia dodana dla dzialania klienta jest implementacja systemu widocznosci - serwer przysyla dane na temat aktorow z wiekszego promienia niz sa widoczne, a system tlumaczy uzycie silnika do wyrenderowania instancji tylko tych ktore powinny byc rysowane.  

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Menedzer sieci po stronie klienta (`CPythonNetworkStream`) wolajacy metody przyjmujace z buforow obslugujacych protokol TCP po stronie klienta. Gralna logika uzywa `CNetworkActorManager::Update()` np. przy zmianie kamery badz w glownym `GameLoop` po zakonczeniu odbioru pakietow ze strumienia.
- **Zaleznosci wyjsciowe (Outbound):** Uzywa bezposrednio menedzera postaci Pythona `CPythonCharacterManager` ktory posredniczy do wywolan nizszego rzedu - jak Granny 3D badz sam interfejs klienta. Dodatkowo uzywa `IAbstractPlayer` z interfejsem odpytujacym parametry aktualnego gracza (np `IsMainCharacterIndex`, `SetMainCharacterIndex`).
- **Drzewo dyrektyw `#include`:** W pliku glowy `NetworkActorManager.h` mozemy zaobserwowac `InstanceBase.h` ktora w sobie kryje logike menedzerow podklas gry. Rozwiazania oparte o biblioteke standardowa `map`, `string`.
- **Model pamieciowy:** Brak smart pointerow. Kod polega na wskaznikach bezposrednich z menedzera zarzadzania instancjami. Klasa `CNetworkActorManager` korzysta tez z mechanizmu `map` do trzymania relacji pomiedzy unikalnym VID (`m_kNetActorDict[dwVID]`) a struktura pakietowa.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur:**
  - `SNetworkActorData` - 136+ bajtow. Przechowuje zdenormalizowane dane z poszczegolnych aktorow.  Pole VID, predkosci ataku, statystyki itp.
  - `SNetworkMoveActorData` - Struktura posredniczaca, przechowujaca parametry ruchu konkretnego VID przed przeslaniem logiki dalej.
  - `SNetworkUpdateActorData` - Struktura aktualizacji dla aktora na poszczegolnym VID. Uzywana podczas update bytu w swiecie gry.
  - `CNetworkActorManager` - Wlasciciel slownika sieciowego VID-to-Data. Posiada kluczowy wektor `m_kNetActorDict` przechowujacy stan swiata ze strony serwera.
- **Tabela Metod Publicznych `CNetworkActorManager`:**
  - `AppendActor(const SNetworkActorData&)`:  Zwraca void.  Dodaje badz nadpisuje powiazana strukture VID i prosi system postaci o pokazanie. Side-effect: Kasuje mount i podmienia widocznosc instancji.
  - `RemoveActor(DWORD dwVID)`: Usuwa aktora z listy. Side-effect: Wywolanie delete z CharacterManager.
  - `UpdateActor(const SNetworkUpdateActorData&)`: Odswieza flagi ataku, zycia badz modelu dla instancji.
  - `MoveActor(...)`: Dodaje wektor przesuniecia do danej struktury `SNetworkActorData` by moc interpolowac polozenie docelowe.
- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  - Brak dedykowanych offsetow hackow bez glebokiej analizy pamieci, VID postaci stanowi klucz do mapy poszukiwan `m_kNetActorDict` pod adresem offsetowym obiektu.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:**
  -  `TPacketGCCharacterAdd` - struktura przypisana do `RecvCharacterAppendPacket`.
  -  `TPacketGCCharacterUpdate` / `TPacketGCCharacterUpdate2` - do flag / opcji aktualizacji.
  -  `TPacketGCCharacterDelete` - pakiet w uzyciu na DeletePacket.
  -  `TPacketGCMove` - wylapywany przez `RecvCharacterMovePacket`.
  -  `TPacketGCSyncPosition` - pakiety z pozycja X/Y w celach debugu.
  -  `TPacketGCOwnership` - do zdefiniowania wlasciciela (np dusz).
- **Metody Pythona (`PyMethodDef`):** Modul silnie sieciowy nie implementuje funkcji eksponowanych z pakietu `METH_VARARGS`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Wszystkie struktury i obiekty sa iterowane i aktualizowane sekwencyjnie podczas wywolania OnUpdate oraz petli nasluchujacej w watku glownym uzytkownika - kod nie zabezpiecza w uzyciu mutexow iteracji.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** `RemoveActor` sprawdza istnienie klucza ale iteratory bedace invalid wywoluja trace errory. Rowniez uzycie `IsMainActorVID` opiera sie na bezpiecznym zainicjowaniu MainPlayer'a na wejsciu. `CPythonCharacterManager` potrafi zwrocic null z proby alokacji, co trzeba dodatkowo sprawdzac.
- **Zarzadzanie zasobami (RAII):** Kod stary, alokacje instancji robione w menedzerze Pythona sa podlegle wlasnym listom i alokatorom. Nie ma tu zadnych std::unique_ptr ani std::shared_ptr, dlatego usuwac trzeba przez proxy w postac wywolania DeleteInstance z CharacterManager. 

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** W pierwszej kolejnosci musisz przedefiniowac pakiet GC wysylany z serwera, poszerzyc istniejace `SNetworkActorData` badz `SNetworkUpdateActorData` o pozadane dane (np. flagi stunna) by wpasc z nimi na parser. Nastepnie dobudowac logike w `UpdateActor` badz `AppendActor` by wywolac logike uzytkowa np w instancji `pNewInstance->SetStun(...)`.
- **Jak debugowac i logowac:** Logika `CPythonNetworkStream` dostarcza Tracen oraz Tracef do wyswietlania bledow badz zachowania strumienia gniazd, nalezy zachowac ten sam mechanizm (uwaga na ModernLogger jesli projekt uzywa nowych mechanizmow w tym obszarze, nalezy do nich zmienic wywolania).
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** W uzyciu harnessow do testowania uzywac `CPythonCharacterManager` Mocka by zrzucic powiazania sieciowe z mechanizmow dzialajacych w domenie renderowania (DirectX/Granny). 
