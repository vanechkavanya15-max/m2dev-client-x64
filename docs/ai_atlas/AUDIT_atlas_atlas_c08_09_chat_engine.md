---
task_id: "atlas_c08_09_chat_engine"
cluster: "UI"
module_name: "CPythonChat - Bufor Wiadomosci i Filtracja Czatu"
target_files:
- src/UserInterface/PythonChat.cpp
- src/UserInterface/PythonChat.h
- src/UserInterface/InsultChecker.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c08_09_chat_engine.md"
architecture_layer: "Interfejs Uzytkownika, Okna i System Tekstu"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Dokladna funkcja:** `CPythonChat` (wzorzec Singleton) odpowiada za logike wizualizacji, buforowanie i zarzadzanie tekstami na czacie glownym, logach systemowych, w ukladach okienkowych (tzw. `ChatSet`) oraz w szeptach (zakladki prywatnych wiadomosci `CWhisper`). Posiada mechanizmy formatowania, opozniania wiadomosci (`TWaitChatList`), filtrowania graczy (`m_IgnoreCharacterSet`) oraz obsluguje rendering dwukierunkowy (LTR/RTL). Z kolei `CInsultChecker` zapewnia szybkie, in-memory filtrowanie i podmienianie na znaki gwiazdek ('*') wulgaryzmow wysylanych w pakietach przed ich przeprocesowaniem do czatu, wykorzystujac slownik ladowany z sieci.
- **Punkt wywolania (Game Loop):** `CPythonChat` jest bezposrednio wpiety w interfejs Pythona (wywolywany ze skryptow uiscript) i `CPythonApplication`. Renderowanie okienek chat (`Render`) i szeptow (`RenderWhisper`) jest uruchamiane z modulu Pythona na event OnRender, a buforowane wiadomosci szeptow odswiezaja opoznienia na `Update`.
- **Przeplyw danych (Data & Control Flow):** 
  1. Wiadomosci przychodza z sieci, gdzie dekoduje je `CPythonNetworkStream`.
  2. Modul sieciowy sprawdza z `CInsultChecker` obecnosc wulgaryzmow.
  3. Jesli to wiadomosc ogolna lub systemowa, jest dodawana przez Pythona lub z modulu sieci przez `CPythonChat::Instance().AppendChat(...)` lub bezposrednio w mapie w `AppendChatWithDelay`.
  4. Nowy wpis trafia do kolejki globalnej `m_ChatLineDeque` ograniczanej do `CHAT_LINE_MAX_NUM` (300).
  5. Dla kazdego "zestawu okna czatu" (`TChatSet`), system ustala w zaleznosci od map maski (`m_iMode`) i stanu okienka (`BOARD_STATE_VIEW`, `EDIT`, `LOG`), co i jak powinno zostac przeniesione na liste wyswietlana `m_ShowingChatLineList`.
  6. W fazie renderingu klasa bazuje na iteracji widocznych linijek z listy `m_ShowingChatLineList` i odpala rysowanie przez `CGraphicTextInstance`.
- **Cykl zycia obiektow:**
  - `SChatLine` oraz `CWhisper::TChatLine` i glowne instancje `CWhisper` sa alokowane uzywajac wlasnych dynamicznych pul (`CDynamicPool`).
  - W momencie nadejscia nowej wiadomosci alokowana jest obwodka tekstu (`Instance.SetValue` itp.).
  - Jesli osiagnieto limit (300) to najstarszy `SChatLine` zostaje trwale zniszczony i oddany do puli.
  - Szepty `CWhisper` istnieja w slowniku `m_WhisperMap`, poki uzytkownik nie usunie zakladki (`ClearWhisper`). Zamkniecie klienta czysci pule.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** 
  - `CPythonNetworkStream` - Odbiera pakiety z serwera, uzupelnia uklad `CInsultChecker` lub dorzuca powiadomienia na ekran.
  - Skrypty Pythona (przez API `PythonChatModule.cpp` m.in. `chat.AppendChat()`, `chat.Update()`, `chat.Render()`) do zarzadzania instancja w systemie GUI (np. `uiChat.py`).
- **Zaleznosci wyjsciowe (Outbound):** 
  - `EterLib`: Narzedzia timerow, zewnetrzny `CGraphicTextInstance` do bindowania fontow, pozycjonowania i renderowania kolorow, `CDynamicPool`.
  - `EterBase`: Timer, klasa bazowa `CSingleton`.
  - PyBridge (via `PythonChatModule.cpp`) - do eksponowania i parsowania argumentow do Pythona.
- **Drzewo dyrektyw `#include`:** 
  - `Packet.h`, `AbstractChat.h` (interfejs chat bazowy), `PythonApplication.h`, `PythonCharacterManager.h`, `EterBase/Timer.h`, `<utf8.h>`. (Ryzyko: brak wiekszych cykli poniewaz modul opiera sie glownie na zaleznosciach z EterLib i abstrakcjach w abstrakcjach, np. `IAbstractApplication`).
- **Model pamieciowy:** Dominuja dedykowane pule obiektow (`CDynamicPool`) uzywane we wlasnym zakresie (`static SChatLine* New()`, `static void Delete()`). Slowniki i wektory uzywaja `std::` map/setow przetrzymujacych bezposrednie wskazniki C bez uzycia std::unique_ptr (to pulowe, nie-RAII wlasnosciowe obiekty).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
| Nazwa (Klasa/Struktura) | Rola i przeznaczenie | Rozmiar bajt. | Watki / Wlasciciel |
|---|---|---|---|
| `CPythonChat` | Singleton glownego huba czatu (okienka ogolnego, systemow, i zestawow UI). | (Singleton) | Glowny watek D3D / UI |
| `TChatSet` | Pojedynczy set (jak okno UI, gdzie kazde `dwID` tworzy wlasne bufory co jest aktualnie widoczne dla uzytkownika - po filtrach `m_iMode`). | ~ | Wew. CPythonChat |
| `CPythonChat::SChatLine` | Pojedyncza linijka na czacie globalnym. Buforuje do 3 kolorow i obiekt tekstu (`CGraphicTextInstance`). | ~ | Pula dynamiczna |
| `CWhisper` | Bufor i widok dla czatu PM / Szeptow. | ~ | Pula, CPythonChat |
| `CWhisper::TChatLine` | Linijka w oknie prywatnej wiadomosci. | ~ | Pula dynamiczna |
| `CInsultChecker` | Singleton obslugujacy slownik wulgaryzmow podlegajacy cenzurze. | ~ | Glowny watek (bez mt) |

**Tabela Metod Publicznych (`CPythonChat` / `CInsultChecker`):**
| Sygnatura | Wartosc zwr. | Pre-cond / Uwagi (Skutki uboczne) |
|---|---|---|
| `int CreateChatSet(DWORD dwID)` | `int` | Tworzy nowe okno w pamieci (lub nadpisuje) o ID (`dwID`), by moc filtrowac zakladki. |
| `void AppendChat(int iType, const char* c_szChat)` | `void` | Zalezy od fontow i UI. Odklada alokacje tekstu. Limit 300 instancji w deque. Wspiera format `Name : Message` i poprawny reorder RTL. |
| `void IgnoreCharacter(const char* c_szName)` | `void` | Dodaje nick do `m_IgnoreCharacterSet`. Zawsze pamieta graczy (do restartu klienta). |
| `void FilterInsult(char* szLine, UINT uLineLen)` | `void` | `CInsultChecker`: Moduluje pamiec bufora in-place podmieniajac wulgaryzm na `*`. |
| `void ClearWhisper(const char* c_szName)` | `void` | Dealokuje mape `CWhisper` usuwajac wirtualnie "zakladke". |

**Pamieciowy Layout Struktur (Offsety pod Boty):**
- W obrebie `CPythonChat`: Glowne kontenery `m_ChatLineDeque`, `m_ShowingChatLineList`, `m_ChatSetMap`, `m_WhisperMap`, `m_IgnoreCharacterSet`.
- Boty lub in-memory scannery szukajace konwersacji na szepcie moga odnalezc offset do `m_WhisperMap`, iterujac mape wg kluczy (nazw uzytkownikow) i dekodowac tekst zawarty w `TChatLineDeque` dla danego `CWhisper`.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe (via `PythonNetworkStream`):** Pakiety np. logowania lub konfiguracji dodaja insulty w fazie ladowania sieci: (`CPythonNetworkStream::PhaseLoading::SetInsultList()`) korzystajac bezposrednio z `CInsultChecker::AppendInsult`. Inne pakiety typu PM czy normalny czat podlegaja sprawdzaniu w `PythonNetworkStreamPhaseGame.cpp`.
- **API Python (Eksporty z `PythonChatModule.cpp`):**
  - Wszystkie glowne metody szeptu, filtru zakladek i renderu maja mapping:
  - `chat.Update(iID)` -> `chatUpdate` -> `CPythonChat::Instance().Update(iID)`
  - `chat.Render(iID)` -> `chatRender` -> `CPythonChat::Instance().Render(iID)`
  - `chat.AppendChat(iType, szChat)` -> `chatAppendChat`
  - `chat.CreateWhisper(szName)`, `chat.AppendWhisper(iType, szName, szChat)`, `chat.ClearWhisper(szName)`.
  - State board constants: `chat.BOARD_STATE_VIEW`, `chat.BOARD_STATE_EDIT`, `chat.BOARD_STATE_LOG`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zarzadzanie zasobami (Pule alokacji vs pamiec D3D):**
  - Teksty alokuja pod soba bezposrednie tekstury `CGraphicTextInstance`. Dlatego usuniecie czatu z `CPythonChat` *musi* uzywac `SChatLine::Delete(...)` i wolac `Instance.Destroy()`, inaczej poleci masywny wyciek tekstur VRAM w systemie renderowania i ukladach czcionek.
- **Multithreading:** Operacje UI czatu (AppendChat itp.) musza odbywac sie z glownego watku, zwlaszcza odswiezanie pozycji (`CGraphicTextInstance::Update`). Uklad nie posada muteksow na pulach alokacyjnych `CDynamicPool`.
- **Cenzura (InsultChecker):**
  - Funkcje cenzora (`FilterInsult`) modifikuja argument IN/OUT w `char* szLine` podmieniajac w miejscu `memset`em na gwiazdki `*`. Operuje to zatem na buforze podanym ze skryptu sieci, a bajty zachowuja te same dlugosci - brak ryzyka przelewu z offsetami, ale ryzykowna struktura stringa gdy podamy r-value.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Rozszerzanie typow chatu / nowe maski zakladek:** Nowe maski/typy wystarczy dodac do `enum EWhisperType` badz stale Pythona np. `CHAT_TYPE_NOTICE`. Pamietaj podniesc (jesli dotyczy) kolorowanie tablicy `CHAT_TYPE_MAX_NUM` oraz mapy filtrow czatu (bo kazdy nowy enum zwieksza wektor `m_iMode`).
- **Unit Testing Headless:** Modul czatu jest powiazany z instancja Singletonu UI. Ciezko to testowac zupelnie bez inijcalizacji `CTextManager` / `EterLib`. Aby testowac logike "buforowania", `CGraphicTextInstance` mozna zastapic no-op Mockiem lub dziedziczac i ucinajac bezposrednio kod wywolujacy Direct3D9.
- **Debug / Logi:** Logika czatu wycisza bledy (gdy przekraczamy `CHAT_LINE_MAX_NUM`, starocie sa puszczane). Mozesz ustawic breakpoint w `CPythonChat::AppendChat`, by sledzic wchodzacy tekst bez formatowania badz zweryfikowac, co z wrzuconym offsetem wygenerowal InsultChecker.
