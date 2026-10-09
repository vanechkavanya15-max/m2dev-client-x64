---
task_id: "atlas_c08_12_guild_interface"
cluster: "UI"
module_name: "CPythonGuild - Zarzadzanie Gildia i Pobieranie Symboli"
target_files:
- src/UserInterface/PythonGuild.cpp
- src/UserInterface/GuildMarkDownloader.cpp
- src/UserInterface/GuildMarkUploader.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c08_12_guild_interface.md"
architecture_layer: "Interfejs Uzytkownika, Okna i System Tekstu"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- Jaka jest dokladna funkcja tego modulu w architekturze klienta.
Modul ten zarzadza lokalnym stanem gildii po stronie klienta oraz obsluguje pobieranie i wysylanie znakow/symboli (ikon) gildii miedzy klientem a serwerem gry. Klasa `CPythonGuild` pelni role repozytorium danych gildii, zapewniajac interfejs do odczytywania (i modyfikowania z pakietow sieciowych) informacji takich jak liczba czlonkow, prawa, poziom, doswiadczenie i komentarze na tablicy. Klasy `CGuildMarkDownloader` oraz `CGuildMarkUploader` obsluguja specjalne, wydzielone polaczenia TCP sluzyce wylacznie do asynchronicznego przesylania plikow graficznych z ikonami, zeby nie blokowac glownego watku gry i nie przeciazac glownego socketu sieciowego.

- W jakim momencie petli gry (OnUpdate / OnRender / Network Tick) ten kod jest wywolywany.
`CPythonGuild` jest aktualizowany pasywnie w wyniku odbierania pakietow sieciowych przez `PhaseGameGuildBridge` oraz odpytywany asynchronicznie z Pythona poprzez warstwe UI. Downloader i Uploader maja wlasna metode `Process()`, ktora musi byc recznie wywolywana w glownej petli aktualizacji aplikacji (network tick) w celu obslugi wysylania i odbierania pakietow.

- Pelny opis przeplywu danych (Control Flow & Data Flow) krok po kroku.
1. Klient laczy sie z serwerem i wchodzi w `PhaseGame`.
2. Przychodzi pakiet `TPacketGCGuildInfo`, odbierany przez mostek `PhaseGameGuildBridge`.
3. Mostek uaktualnia stan gildii w singletonie `CPythonGuild` uzywajac metod modyfikujacych (np. `SetGuildEXP`, `RegisterMember`).
4. Warstwa UI napisana w Pythonie wywoluje metody C-API (np. `guild.GetGuildMemberCount()`), by odczytac te zmienione dane i wyrenderowac okno gildii.
5. Pobieranie ikonek jest realizowane przy uzyciu `CGuildMarkDownloader`. Laczy sie on w dedykowanym watku (lub trybie asynchronicznym) przez wywolanie `ConnectToRecvSymbol`.
6. Proces uwierzytelniania obejmuje pakiety logowania, `GC::PHASE` (zmiana fazy serwera uwierzytelniajacego), wyslanie danych do serwera znakow i ostatecznie sciaganie danych blokami (pakiety Mark Block).
7. Wysylanie dziala analogicznie w klasie `CGuildMarkUploader`, uzywa stb_image aby zaladowac w C++ ikone 16x12, a nastepnie przesyla komende `CGMarkUpload`.

- Cykl zycia obiektow (Lifecycle: alokacja, inicjalizacja, reset, dealokacja).
Wszystkie 3 glowne klasy (`CPythonGuild`, `CGuildMarkDownloader`, `CGuildMarkUploader`) to singletony (dziedzicza po `CSingleton`). Zostaja utworzone podczas inicjalizacji aplikacji. `CPythonGuild` zeruje swoj stan w metodzie `Destroy()` wywolywanej przy wylogowywaniu. Upload/Download otwieraja obiekty i rezerwuja bufory tylko na czas transferu, zmieniajac stany (`STATE_OFFLINE`, `STATE_LOGIN`, `STATE_COMPLETE`) i nastepnie zamykaja gniazda.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):**
  - **CPythonGuild:** Interfejs uzytkownika Pythona wywolujacy rejestrowane metody `PyMethodDef`. Elementy sieciowe: pakiety z mostka `PhaseGameGuildBridge`.
  - **Downloader/Uploader:** Klasa aplikacji lub modul sieciowy decydujacy o rozpoczeciu transferu ikon (np. `CPythonNetworkStream::__DownloadSymbol`).

- **Zaleznosci wyjsciowe (Outbound):**
  - **Zarzadzanie C++:** `EterLib::CNetworkStream` (obsluga gniazd TCP), `CGuildMarkManager` (przechowywanie, alokacja tekstur ikon i plikow, w oparciu o stb_image).
  - **Siec:** Protokol sieciowy, dekodery i obiekty `Packet.h` (i `Protocol/Protocol.h`).

- **Drzewo dyrektyw `#include`:**
  - `PythonGuild.h`: `Packet.h`
  - `GuildMarkDownloader.h`: `EterLib/NetStream.h`, `MarkManager.h`, `<set>`
  - `GuildMarkUploader.h`: `EterLib/NetStream.h`, `MarkImage.h`
  - *Ryzyka cyklicznych zaleznosci*: Minimalne, architektura jest plaska. Wykorzystanie mostkow C++23 izoluje dekodery (np. `Client::Network::GuildPacketCodec`).

- **Model pamieciowy:**
  - W `CPythonGuild` pamiec zorganizowana jest glownie za pomoca standardowych kontenerow C++: `std::vector` (czlonkowie, komentarze), `std::map` (dane o uprawnieniach, nazwy wrogich gildii). Wszedzie przechowuje sie referencje/wartosci struktur, bez surowych wskaznikow oznaczajacych zarzadzanie wlasnoscia.
  - `CGuildMarkDownloader`/`Uploader` w pamieci uzywaja `std::vector<uint8_t>` dla buforow pobieranych symboli. Wskazniki surowe stosowane sa jedynie do komunikacji poprzez stary kod z CNetworkStream (np. bufory na payload pakietu).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

#### Tabele Klas i Struktur
| Nazwa | Rola | Zaleznosc/Watek |
|---|---|---|
| `CPythonGuild` | Singleton. Centralny punkt danych i stanow powiazanych z gildia w cliencie. | Watek Glowny |
| `TGuildInfo` | Struktura 116-bajtowa (w tym nazwa 12 znakow, ID, master PID, ilosc czlonkow) | Watek Glowny |
| `TGuildMemberData` | Przechowuje informacje o konkretnym graczu: PID, nazwa, ranga, flagi. | Watek Glowny |
| `TGuildSkillData` | Przechowuje aktualny poziom skilli i guild points. | Watek Glowny |
| `CGuildMarkDownloader` | Maszyna stanow zarzadzajaca lacznoscia i pobieraniem symboli z mark serwera. Dziedziczy po `CNetworkStream`. | Sieciowy / Glowny |
| `CGuildMarkUploader` | Przesyla nowo wybrana ikone z dysku serwerowi z weryfikacja wielkosci z `stb_image`. | Sieciowy / Glowny |

#### Tabela Metod Publicznych
| Sygnatura | Wartosc zwracana | Warunki / Skutki |
|---|---|---|
| `void CPythonGuild::RegisterMember(TGuildMemberData &)` | void | Modyfikuje vector; dodaje czlonka jesli nie istnial. |
| `void CPythonGuild::RemoveMember(DWORD dwPID)` | void | Usuwa gracza z wewnetrznego vectora uzywajac PID. |
| `void CPythonGuild::StartGuildWar(DWORD dwEnemyGuildID)` | void | Modyfikuje mape ID wrogow (`m_adwEnemyGuildID`). |
| `bool CGuildMarkDownloader::ConnectToRecvSymbol(...)` | bool | Otwiera gniazdo i inicjalizuje stan asynchronicznego pobierania. Zwraca true po sukcesie. |
| `bool CGuildMarkUploader::ConnectToSendSymbol(...)` | bool | Czyta plik lokalny (`stb_image`), laczy sie i wysyla paczke z logowaniem do mark servera. |

#### Pamieciowy Layout Struktur (Memory Layout & Offsets)
Dla `CPythonGuild::TGuildInfo`:
`dwGuildID` (4 bajty),
`szGuildName` (13 bajtow), padding,
`dwMasterPID` (4 bajty),
`dwGuildLevel` (4 bajty),
itd.
Ogolnie nie wystepuja tu dynamiczne obiekty polimorficzne (brak virtual na tych malych strukturach POD), co sprawia, ze ich uklad pamieci C jest latwy do podpiecia (hookingu).

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe (Protokol):**
  - Opcody Mark/Symbol Servera zdefiniowane w `Protocol.h`:
    - `HEADER_CG_MARK_LOGIN`, `HEADER_CG_MARK_UPLOAD`, `HEADER_CG_MARK_IDXLIST`, `HEADER_CG_MARK_CRCLIST`, `HEADER_CG_SYMBOL_UPLOAD`, `HEADER_CG_SYMBOL_CRC`.
    - Pakiety pobierania: `HEADER_GC_MARK_IDXLIST`, `HEADER_GC_MARK_BLOCK`, `HEADER_GC_GUILD_SYMBOL_DATA`.
  - Glowny serwer Gry (opcody Gildii w `GuildSub::GC` / `TPacketGCGuild`):
    - Wiele sub-opcodow np. `GuildSub::GC::LOGIN`, `GuildSub::GC::LOGOUT`, `GuildSub::GC::INFO`, `GuildSub::GC::WAR`.
    - Wszystkie one sa dekodowane bezpiecznie przez `Client::Network::GuildPacketCodec` i mostkowane przez `PhaseGameGuildBridge`.

- **Metody Pythona (`PyMethodDef` w initguild):**
  - Funkcje C-API takie jak `guildGetGuildID`, `guildGetGuildName`, `guildGetGuildMemberCount`, `guildIsGuildEnable`.
  - Metody mapowane sa jako tablica w formacie `{"NazwaMetodyPythona", nazwaFunkcjiCPP, METH_VARARGS}`.
  - Wywoluja bezposrednio metody obiektu `CPythonGuild::Instance()`. Posiadaja odpowiednie testy argumentow `PyTuple_GetInteger` / `PyTuple_GetString`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Wszystkie odczyty z tablic i map gildii (`std::vector` i `std::map` wewnatrz `CPythonGuild`) sa zoptymalizowane pod watek glowny. Kod NIE uzywa semaforow (`std::mutex`). Dlatego wywolywanie modyfikacji z innego watku (jak watek odbierajacy pakiety gry bez zsynchronizowania) spowoduje korupcje wektorow na operacjach typu `push_back`. Pakietownie i de-pakietowanie w `PhaseGameGuildBridge` MUSI byc na glownym watku, albo uzywac `std::lock_guard` (co aktualnie nie jest robione, co znaczy, ze odbieramy pakiety na glownym petli update).
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Funkcje `PyTuple_GetInteger` nie sa owiniete w weryfikacje obowiazkowe jezeli indeks jest nie poprawny. Zwrot `Py_BuildException()` musi obsluzyc pythonowy try/except.
  - `CGuildMarkUploader::__Load`: polega na zewnetrznym `stb_image`. Szerokosc grafiki mierzona w C-kodzie twardo uzywa limitow. Jesli plik ma uszkodzony format graficzny i stb_image zaalokuje `null`, musimy sprawdzac by uploader nie uderzyl w segmentation fault w `memcpy`.
- **Zarzadzanie zasobami (RAII):** Singleton `CPythonGuild` zyje na stercie i jest zarzadzany globalnie. Downloader uzywa surowych alokacji `new/delete` buforow blokow w starych pakietach. Nalezy trzymac sie nowego standardu `std::span` z C++20 dla operacji sieciowych na symbolach.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zdefiniuj sub-pakiet w `Protocol/Protocol.h`.
  2. Dekoduj pakiet uzywajac C++23 zero-conflict codec np w `src/Client/Network/GuildPacketCodec.cpp`.
  3. Dodaj funkcje rutujaca w `PhaseGameGuildBridge::HandleGuildSub_XXX`.
  4. Dodaj stan wewnetrzny/klase w `CPythonGuild.h` i odpowiednia metode aktualizujaca.
  5. Jesli dane uzywa UI, dopisz wrapper API `PyObject*` i zarejestruj go w tablicy `s_methods` w `PythonGuild.cpp`.
- **Jak debugowac i logowac:** Do przesledzenia ruchu pakietow loguj pakiety w `GuildPacketCodec` za pomoca `EterBase::ModernLogger`. Pobieranie ikon (GuildMark) mozna sledzic, logujac stany w `CGuildMarkDownloader::__StateProcess`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Wykorzystaj `tests/test_c26_rhi_interface.cpp` jak szablon by mockowac pakiety w formacie tablic bajtow. Utworz sztuczne pakiety wejsciowe dla `DecodeGuildHeader`, sprawdzajac wynikowa strukture `std::expected`. W przypadku Singletonu `CPythonGuild`, utworz go w tescie, zapchaj sztucznymi memberami i uzyj asercji GTest do validacji czy zliczenie XP / Sredniego Levla graczy jest poprawne (`GetGuildMemberLevelAverage()`).
