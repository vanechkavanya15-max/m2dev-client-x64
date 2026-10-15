---
task_id: "atlas_c01_12_phase_game_quest_chat"
cluster: "NET"
module_name: "Obsluga Pakietow Zadan i Czatu (PhaseGame Quest & Chat)"
target_files:
- src/UserInterface/PythonNetworkStreamPhaseGameQuest.cpp
- src/UserInterface/PythonNetworkStreamPhaseGameChat.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c01_12_phase_game_quest_chat.md"
architecture_layer: "Silnik Sieciowy, Maszyna Faz i Protokol"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Moduly te zajmuja sie parsowaniem i dystrybucja informacji otrzymanych z serwera przez watek sieciowy (w fazie GAME) dotyczacych dwoch krytycznych podsystemow gry:
1. **Zadan (Questow):** Kod z `PythonNetworkStreamPhaseGameQuest.cpp` analizuje struktury pakietow zwiazane ze stanem misji (QuestInfo, QuestConfirm) oraz wykonuje powiazane zdarzenia skryptowe. Pakiety aktualizuja stan misji (rozpoczecie, aktualizacja, zakonczenie) w CPythonQuest i powiadamiaja Pythona, aby zaktualizowal interfejs uzytkownika. Obslugiwane jest rowniez parsowanie komend skryptowych na UI.
2. **Czatu (Chat i Whisper):** Kod z `PythonNetworkStreamPhaseGameChat.cpp` zajmuje sie analizowaniem komunikatow od graczy i systemu (Czat Normalny, Party, Guild, Shout, Whisper). Pakiety sa sprawdzane, a dane odpowiednio tlumaczone (np. filtracja wulgaryzmow, wielojezyczne lacza przedmiotow - LocalizeItemLinks, oraz szyfrowanie miedzy krolestwami - ConvertEmpireText) zanim dotra do klasy CPythonChat i tekstu nad glowami (CPythonTextTail). Modul odpowiada rowniez za tworzenie i wysylanie komunikatow z klienta do serwera (SendChat, SendWhisper).

**Cykl zycia i Control Flow:**
Oba pliki dzialaja jako most (Bridge) w warstwie sieciowej, gdzie poszczegolne funkcje statyczne (Handle*) sa wywolywane przez CPythonNetworkStream, gdy w buforze odbiorczym pojawi sie pakiet (GC::QUEST_INFO, GC::QUEST_CONFIRM, GC::CHAT, GC::WHISPER itp). Funkcje odczytuja payload z bufora, a nastepnie dystrybuuja go bezposrednio do Singletonow biznesowych (CPythonQuest, CPythonChat, CPythonCharacterManager) lub do skryptow Python przez `PyCallClassMemberFunc` (np. na obiekcie okna fazy). Cykl zycia pakietu zamyka sie w granicach wywolania (funkcje oczekuja w pelni zdeserializowanych pakietow z wyjatkiem dynamicznych ladunkow odczytywanych przez `pStream->Recv()`).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):**
  - Oczekiwane wywolanie z wewnatrz `CPythonNetworkStream::RecvGamePhase` z wlasciwym identyfikatorem opcodu z serwera.
- **Zaleznosci wyjsciowe (Outbound):**
  - `CPythonNetworkStream` (pStream) dla dostepu do stanu, faz, wysylania odpowiedzi oraz operacji pomocniczych (np. parsowanie emotikon).
  - Klasy UI/Logiki `CPythonQuest`, `CPythonChat`, `CPythonTextTail`, `CPythonCharacterManager`, `CInstanceBase`, `CPythonEventManager`.
  - Kodowanie pakietow w C++: `Client::Network::QuestPacketCodec`.
  - Python C-API (wywolywanie metod interfejsu takich jak `RefreshQuest`, `OnRecvWhisper` na oknie PhaseWnd).
- **Drzewo dyrektyw `#include`:**
  - Standardowe `StdAfx.h` lub `Packet.h` dla testow.
  - Zaleznosci domenowe: `PythonNetworkStream.h`, `PythonQuest.h`, `PythonChat.h`, `PythonTextTail.h`, `PythonCharacterManager.h`, `InstanceBase.h`, `PythonEventManager.h`.
  - Zaleznosci systemowe: `<algorithm>`, `<cstring>`, `<cassert>`.
  - UWAGA na ryzyko silnych sprzezen przez Singletony (`CPythonChat::Instance()`, `CPythonQuest::Instance()`).
- **Model pamieciowy:** Kod w wiekszosci opiera sie na czystych wskaznikach (`CPythonNetworkStream*`, `CInstanceBase*`) bez uzycia nowoczesnych inteligentnych wskaznikow (np. RAII). Przekazywanie wiadomosci jako `const char*` czy referencji. Pamiec pod bufory na tekst jest alokowana statycznie i na stosie wewnatrz metod (np. `char buf[1024 + 1];`).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
**Tabela Klas i Struktur:**
- `PhaseGameQuestBridge` - Klasa z metodami statycznymi mostkujacymi logike parsowania questow (watek sieciowy / glowny). 0 bajtow z perspektywy instancji (tylko metody statyczne).
- `PhaseGameChatBridge` - Klasa z metodami statycznymi mostkujacymi logike parsowania oraz tworzenia pakietow czatu. 0 bajtow instancji.
- `TPacketGCQuestInfo` - Pakiet GC z informacja o stanie questa. Zawiera index (uint16_t) i flagi (uint8_t). Rozmiar: 5 bajtow.
- `TPacketGCQuestConfirm` - Pakiet GC uzywany przy zapytaniach potwierdzen od questa. Zawiera msg (64 znaki), timeout i requestPID. Rozmiar 76 bajtow.
- `TPacketGCChat` - Pakiet GC odbierany dla czatu. Zawiera typ (uint8_t), VID aktora (uint32_t) i ID imperium (uint8_t). Wymaga doczytania tekstu z bufora. Rozmiar: 9 bajtow + tekst.
- `TPacketCGChat` - Pakiet CG wysylany. Zawiera typ (uint8_t). Oczekuje po sobie przeslania tablicy char. Rozmiar: 5 bajtow + tekst.
- `TPacketGCWhisper` / `TPacketCGWhisper` - Pakiety szeptu z dynamiczna wielkoscia wiadomosci. Wymagaja na koncu zawartosci ciagu znakow. Zawieraja typ (bType) i nick szepczacego (szNameFrom/szNameTo). Rozmiary stalej czesci to 29 i 28 bajtow w zaleznosci od strony.

**Tabela Metod Publicznych (`PhaseGameQuestBridge`):**
- `static bool HandleQuestInfo(CPythonNetworkStream* pStream, const TPacketGCQuestInfo& pack)` - Warunki: pack zdeserializowany z bufora. Skutki: Opcjonalnie odczytuje dodatkowe wartosci z gniazda pStream zaleznie od flagi QUEST_SEND_*. Aktualizuje CPythonQuest i wywoluje "RefreshQuest" / "BINARY_ClearQuest" z UI.
- `static bool HandleQuestConfirm(CPythonNetworkStream* pStream, const TPacketGCQuestConfirm& pack)` - Warunki: pack zdeserializowany z bufora. Skutki: Wywoluje "BINARY_OnQuestConfirm" na oknie fazy gry.
- `static bool HandleScript(CPythonNetworkStream* pStream, const std::string& script)` - Skutki: Rejestruje skrypt jako set eventowy do CPythonEventManager z iIndexem, nastepnie wykonuje `pStream->OnScriptEventStart(0, iIndex);`.

**Tabela Metod Publicznych (`PhaseGameChatBridge`):**
- `static bool HandleChat(CPythonNetworkStream* pStream)` - Deserializuje naglowek `TPacketGCChat` wraz z tekstem. Wywoluje przeciazenie ponizej.
- `static bool HandleChat(CPythonNetworkStream* pStream, const TPacketGCChat& kChat, const char* pChatBuf, size_t chatBufSize)` - Warunki: Poprawne dane wiadomosci i naglowek. Skutki: Przeprowadza parsowanie emotikonow, cenzurowanie wulgaryzmow, system tlumaczenia krolestw. Przesyla ostateczny ciag do `CPythonChat`, `CPythonTextTail` lub do okna gry `BINARY_SetTipMessage` / `BINARY_SetBigMessage`.
- `static bool HandleWhisper(CPythonNetworkStream* pStream)` - Podobnie jak HandleChat. Odczytuje caly ladunek do pamieci na stosie i wola ponizsze przeciazenie.
- `static bool HandleWhisper(CPythonNetworkStream* pStream, const TPacketGCWhisper& whisperPacket, const char* pChatBuf, size_t chatBufSize)` - Skutki: formatuje string na pods. typu szeptu (System, Error, GM) i przekazuje to do UI przez np. `OnRecvWhisper`.
- `static bool SendChat(CPythonNetworkStream* pStream, const char* c_szChat, BYTE byType)` - Ogranicza dlugosc max 512, formatuje do `TPacketCGChat` i wywoluje pStream->Send.
- `static bool SendWhisper(CPythonNetworkStream* pStream, const char* name, const char* c_szChat)` - Ogranicza dlugosc max 255. Wysyla do strumienia pod szyldem `TPacketCGWhisper`.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe (z `Protocol.h`):**
  - Odbierane: `TPacketGCQuestInfo` (Header: `GC::QUEST_INFO`), `TPacketGCQuestConfirm` (Header: `GC::QUEST_CONFIRM`), `TPacketGCChat` (Header: `GC::CHAT`), `TPacketGCWhisper` (Header: `GC::WHISPER`).
  - Wysylane: `TPacketCGChat` (Header: `CG::CHAT`), `TPacketCGWhisper` (Header: `CG::WHISPER`).
- **Mostkowanie (Python UI & C-API callbacks):**
  - Oczekiwane na klasie okna Pythona (`CPythonNetworkStream::PHASE_WINDOW_GAME`):
    - `BINARY_ClearQuest(int index)`
    - `RefreshQuest()`
    - `BINARY_OnQuestConfirm(string msg, int timeout, int requestPID)`
    - `BINARY_SetTipMessage(string buf)` (Dla CHAT_TYPE_NOTICE)
    - `BINARY_SetBigMessage(string buf)` (Dla CHAT_TYPE_BIG_NOTICE)
    - `OnRecvWhisper(int type, string name, string text)`
    - `OnRecvWhisperSystemMessage(int type, string name, string text)`
    - `OnRecvWhisperError(int type, string name, string text)`

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Wielowatkowosc:** Cale operacje sieciowe (odczyt/zapis socketu z `CPythonNetworkStream`) powinny odbywac sie w watku obslugujacym logike gry. Wynika to z silnego uzywania Singletonow logiki / UI, ktore beda podatne na race conditions jezeli wywolane asynchronicznie w innym watku.
- **Odczyt z gniazda:** W `PhaseGameQuestBridge::HandleQuestInfo`, wystepuje warunkowy odczyt sieciowy (`pStream->Recv`) na bazie bitmask flag (np. `QUEST_SEND_TITLE`, `QUEST_SEND_CLOCK_NAME`). Jezeli pakiet zostanie uszkodzony badz zhackowany w drodze, a Recv failuje, to polaczenie natychmiast wraca z zerem (utrata synchronizacji TCP), a strumien zamyka w konsekwencji gniazdo.
- **Bezpieczenstwo bufforow (Memory Bounds):** W kodzie z czatem dlugosc tekstu czatu uzywa maksa 1024 a szepczacy 512. Sprawdzane bezpiecznie po stronie odbioru uzywajac `assert(uWhisperSize < 512)` w kodzie docelowym oraz instrukcji kopiujacych jak `size_t copyLen = (chatBufSize < 512) ? chatBufSize : 512;`.
- **Pulapki zwiazane z null pointerem:** W przypadku pakietow z id VID instancji czatujacej (Dla uChat.dwVID > 0), brak takiej instancji w CPythonCharacterManager zignoruje wazny proces zglaszania wiadomosci na chacie.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zdefiniuj nowy typ pakietu GC lub CG w pliku `Protocol.h`.
  2. Stworz dedykowany Handler w klasie `PhaseGameQuestBridge` lub `PhaseGameChatBridge` (np. `HandleNowyQuest`).
  3. Zaimplementuj kod obslugi uzywajac istniejacego standardu parsowania lub `QuestPacketCodec` jezeli format jest skomplikowany. Posiadaj ostroznosc w uzyciu `pStream->Recv` - operuj preferowanie na pre-alokowanych buforach jesli to mozliwe.
  4. Skonfiguruj okno Pythona, aby poprawnie odpowiadalo na nowe C-API wywolywane z warstwy `PyCallClassMemberFunc`.
- **Debugowanie i Logowanie:** Z uwagi na to, ze pakiety Quest i Chat moga generowac bardzo duzo ruchu logiki, zaleca sie debugowanie poszczegolnych komend z uzyciem punktow wstrzymania (breakpoints) bezposrednio w punktach odczytu (np. `HandleChat` lub `HandleWhisper`) z pominieciem ciaglego odpytywania funkcji (ping lub update z petli).
- **Testowanie bez interfejsu (Headless / Unit Test Harness):** W zwiazku z brakiem wstrzykiwania zaleznosci interfejsu Pythona, ten podsystem najlepiej bedzie testowany uzywajac Mocka CPythonNetworkStream oraz interfejsow singletonow (CPythonCharacterManager) w dedykowanych testach Doctest, jak `Client::Simulation::MockNetworkPortAdvanced`. Umozliwi to odseparowanie kodu od calego narzutu srodowiska Metin. Warto zmockowac wywolanie C API, jezeli jest uzywane, poprzez wstrzykniecie Mockowych Handlerow okna gry.
