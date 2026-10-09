---
task_id: "atlas_c01_09_phase_game_combat"
cluster: "NET"
module_name: "Obsluga Pakietow Walki, Trafien i Smierci (PhaseGame Combat)"
target_files:
- src/UserInterface/PythonNetworkStreamPhaseGameCombat.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c01_09_phase_game_combat.md"
architecture_layer: "Silnik Sieciowy, Maszyna Faz i Protokol"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- Modul ten dziala jako most (bridge) w warstwie sieciowej, ktory odbiera i przetwarza pakiety walki nadchodzace z serwera podczas fazy gry (PhaseGame).
- Kod jest wywolywany w glownym watku, zazwyczaj w ramce czasowej odbierania pakietow sieciowych, podczas wywolan petli gry (Network Tick).
- Przeplyw danych (Control Flow & Data Flow) zaczyna sie od odebrania surowego pakietu sieciowego (np. `TPacketGCDamageInfo` lub `TPacketGCDead`) przez `CPythonNetworkStream`. Nastepnie funkcja mostu (np. `PhaseGameCombatBridge::HandleDamageInfo`) dekoduje ten pakiet korzystajac z `Network::CombatPacketCodec` do zdeserializowanego formatu. Na podstawie identyfikatora VID z pakietu odnajdywany jest docelowy aktor (`CInstanceBase`) przez `CPythonCharacterManager`. Nastepnie wywolywana jest odpowiednia logika postaci - np. dodanie efektu obrazen (`AddDamageEffect`) lub obsluga smierci (`Die`).
- Cykl zycia obiektow: Obiekty pakietow sa przelotne, dekodowane na stosie/lokalnie. Instancje postaci sa zarzadzane wylacznie przez `CPythonCharacterManager` i ten modul jedynie pobiera do nich nagie wskazniki, nie zarzadzajac ich czasem zycia.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Modul wolany jest przez system odbioru pakietow `CPythonNetworkStream` z warstwy klienta, ktory otrzymuje pakiety z serwera.
- **Zaleznosci wyjsciowe (Outbound):** Kod odwoluje sie do `Network::CombatPacketCodec` do deserializacji, `CPythonCharacterManager` by odnalezc instancje, `CInstanceBase` by wymusic zmiany w stanie (np. efekt obrazen, smierc), a w przypadku smierci glownego bohatera takze do `CPythonPlayer` oraz systemu Pythona za pomoca `PyCallClassMemberFunc` by powiadomic UI (okno gry) wywolaniem `OnGameOver`.
- **Drzewo dyrektyw `#include`:** `StdAfx.h`, `PythonNetworkStreamPhaseGameCombat.h`, `PythonNetworkStream.h`, `PythonCharacterManager.h`, `PythonPlayer.h`, `InstanceBase.h`, `Client/Network/CombatPacketCodec.h`, `<span>`. Brak cyklicznych zaleznosci miedzy systemem walki a naglowkami.
- **Model pamieciowy:** W module wszedzie uzywane sa czyste (nagie) wskazniki C (np. `CPythonNetworkStream*`, `CInstanceBase*`). Pamiec uwalniana po dekodowaniu pakietu automatycznie zarzadzana na stosie (`std::span` sluzacy tylko jako warstwa bezpiecznego dostepu i referencje const ref do pakietow).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur:**
  - `PhaseGameCombatBridge`: 1 bajt (pusta klasa z wylacznie statycznymi metodami), rola: kontener na funkcje mapujace obsluge walki z pakietow do systemow klienta.
- **Tabela Metod Publicznych:**
  - `static bool HandleDamageInfo(CPythonNetworkStream* pStream, const TPacketGCDamageInfo& pack);` - warunek wstepny: poprawny, nie-nullowy `pStream`. Dekoduje pakiet obrazkujacy otrzymane rany (`TPacketGCDamageInfo`) przez `CombatPacketCodec::DecodeDamageInfo` i aplikuje efekt graficzny (`AddDamageEffect`). Zwraca `true` w przypadku powodzenia (badz wygasniecia logiki bez koniecznosci zwrotu `false`), `false` jesli `pStream` jest null lub wystapil blad dekodowania. Skutki uboczne: wizualne efekty, zaktualizowanie modelu ran na aktorze.
  - `static bool HandleDead(CPythonNetworkStream* pStream, const TPacketGCDead& pack);` - warunek wstepny: poprawny, nie-nullowy `pStream`. Dekoduje pakiet smierci (`TPacketGCDead`) i uruchamia smierc aktora (`Die()`). W przypadku glownego bohatera wywoluje `OnGameOver` na window z `CPythonNetworkStream::PHASE_WINDOW_GAME` oraz `NotifyDeadMainCharacter` z `CPythonPlayer::Instance()`.
- **Pamieciowy Layout Struktur (Memory Layout & Offsets):** W samym pliku nie sa definiowane zadne nowe struktury, jedyna uzywana jest pusta klasa `PhaseGameCombatBridge`. Zmiany opieraja sie o istniejacy uklad np. `TPacketGCDead` i `TPacketGCDamageInfo`, ktore dekodowane sa pod postacia zrefaktoryzowanych struktur w ramach kodekow sieciowych.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Kody wiaza sie z uzyciem pakietow Game->Client: `TPacketGCDamageInfo` (prawdopodobnie 0x0304 lub podobny) i `TPacketGCDead` zawierajacymi struktury uzywane w obsludze ran/zadania obrazen w grze.
- **Metody Pythona (`PyMethodDef`):** Ten modul nie udostepnia samodzielnie nowej tabeli metod w standardzie C-API (nie ma funkcji modulu), jednak pelni odwrotny most (C++ wola Pythona), wolajac po smierci glownego gracza zdarzenie Pythonowe uzywajac nazwy: `OnGameOver` na referencji obiektu okna glownej gry pobranego uzywajac `CPythonNetworkStream::PHASE_WINDOW_GAME` (z uzyciem `PyCallClassMemberFunc` i przekazaniem krotki argumentow `()`).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Obiekty pakietow z warstwy UI i mostow sieciowych, szczegolnie odnajdywanie `CInstanceBase`, musza operowac na wylacznosc jednego glownego watku uzywanego do aktualizacji UI (pojedynczy watek gry C++ / UI) badz posiadac bezpieczne wiazania, gdyz operowanie i wywolania Pythonowe z interfejsem C (`PyCallClassMemberFunc`) nie wspieraja uzycia bezposrednio poza glownym watkiem ze wzgledu na zamek GIL.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** Odwolanie na wyluskanie pustego `pStream` (zabezpieczone przez `if (!pStream)`). Bezpieczenstwo w rzutowaniu reinterpret_cast w std::span, gdzie nalezy zachowac prawidlowa dlugosc danych. Ponadto wywolywanie klas Pythona moze zakonczyc sie zgloszeniem wyjatku w zaleznosci od jego istnienia, wymagajac istnienia stabilnego okna (okno PhaseGame zadeklarowane wczesniej).
- **Zarzadzanie zasobami (RAII):** Brak dynamicznych alokacji; referencje stosu i pule na zmiennych lokalnych. Skutecznie zapobiega wyciekom zarzadzajac referencjami instancji poprzez istniejace struktury jak `CPythonCharacterManager::Instance().GetInstancePtr(...)`.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zdefiniuj obsluge nowego pakietu walki jako funkcje statyczna w `PhaseGameCombatBridge` (np. `HandleCombo`).
  2. Dodaj wpis deklaracji funkcji statycznej z uzyciem typow w `PythonNetworkStreamPhaseGameCombat.h`.
  3. Zaimplementuj kod oparty o uzycie kodekow `Network::CombatPacketCodec` wraz ze zweryfikowanym blokiem pamieci `std::span`.
  4. Dodaj do odpowiedniego switcha (np. w pliku glownej obslugi strumienia gry) wezel wywolujacy `PhaseGameCombatBridge::HandleCombo(this, pPakiet)`.
- **Jak debugowac i logowac:** Nalezy uzywac funkcji wbudowanych np. `Tracenf("komunikat");` w obszarze modulu do inspekcji zdarzen w logach klienta. Przydatne punkty zatrzymania (breakpoints) to linie przed odwolaniem sie do `res.value()` w obsludze pakietu, umozliwiajace weryfikacje zdeserializowanej tresci.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** W przypadku unit testow headless mozliwe jest bezposrednie wywolywanie `PhaseGameCombatBridge::HandleDamageInfo` podstawiajac zmockowana wersje klasy `CPythonNetworkStream` a takze pusty `CInstanceBase`, jednak ze wzgledu na wiazania globalne (Singleton), takie jak `CPythonCharacterManager`, test musi najpierw postawic symulowany kontekst menedzera z instancja o zadanym identyfikatorze VID, upewniajac sie by ominac bezposrednie powiazania i logiki D3D (DirectX).
