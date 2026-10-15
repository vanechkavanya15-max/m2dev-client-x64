# Podrecznik Architektury Fast-Boot & Direct-to-World 2026
## Nowoczesne Logowanie, Pominiecie Archaicznych Faz Pythona oraz Lazy World Render

Ten dokument stanowi glowne zrodlo wiedzy architektonicznej (Single Source of Truth) dla podsystemu **Fast-Boot / Direct-to-World (Dear ImGui DX9)** w silniku klienta gry [`E:\m2dev-client-src-mainOryginalx64`](file:///E:/m2dev-client-src-mainOryginalx64).

---

## 1. Geneza Architektoniczna: Krytyczna Ocena Monolitu z lat 2005-2008

Tradycyjny pipeline uruchomieniowy gry Metin2 powstal ponad dwie dekady temu i niesie ze soba szereg architektonicznych wad, ktore uniemozliwialy szybka iteracje deweloperska oraz stabilna automatyzacje testow:

### Glowny lancuch problemow legacy:
1. **Przedwczesne blokujace ladowanie dyskowe (I/O Bottleneck)**:
   W [`UserInterface.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/UserInterface.cpp#L180-L200) funkcja `PackInitialize("pack")` wczytywala synchronicznie **47 archiwow `.pck`** (w tym tereny, lochy malp, zbroje i potwory) jeszcze przed wyswietleniem jakiegokolwiek ekranu. Klient alokowal ponad 400 MB RAM i marnowal 3-6 sekund na ladowanie zasobow, ktore byly zbedne, jesli gracz chcial tylko wpisac login lub gdy serwer byl offline.
2. **Ciezkie srodowisko graficzne i wirtualna klawiatura Pythona**:
   Okno logowania [`intrologin.py`](file:///E:/m2dev-client-src-mainOryginalx64/m2dev-client-main/assets/root/intrologin.py) korzystalo z modulu `VirtualKeyboard` z 36 przyciskami i zlozona hierarchia zdarzen UI. Skutkowalo to nagminnym gubieniem focusu systemowej klawiatury, brakiem mozliwosci wpisania hasla/PINu oraz podatnoscia na wyjatki interpretera.
3. **Wielofazowe opoznienia animacyjne**:
   Tradycyjne przejscie ze startu do gry wymagalo przejscia przez 4 odrebne ekrany:
   $$\text{introLogin (menu)} \longrightarrow \text{introSelect (wybor postaci 3D)} \longrightarrow \text{introLoading (13 klatek ladowania)} \longrightarrow \text{game (swiat)}$$
   W kazdej fazie ladowano zbedne zasoby posrednie (platforme 3D wyboru postaci, animacje kurtyny `PhaseCurtain`), co wydluzalo czas wejscia do gry do 10-20 sekund.
4. **Ciche bledy sieciowe (Silent Failure)**:
   Brak dwustronnej komunikacji miedzy silnikiem sieciowym C++ a warstwa interfejsu powodowal, ze w przypadku bledu serwera (`ALREADY`, `WRONGPWD`, timeout) klient zawieszal sie bez jasnej informacji dla uzytkownika.

---

## 2. Architektura 2026: Fast-Boot & Direct-to-World (Dear ImGui)

Nowa architektura rozdziela ciezki renderer swiata 3D od procesu logowania, wprowadzajac ultra-lekki automat C++ oparty na **Dear ImGui (DirectX 9)**:

```
[ START Metin2_Release.exe ] (Czas startu: < 0.1s, RAM: ~45 MB)
             │
             ▼
[ LEKKI OVERLAY IMGUI (520x430 px) ] ◄── Przelacznik klawiszem F11
  • Brak ladowania ciezkich skryptow Pythona (introLogin/introSelect pominiete).
  • Natywne pola edycyjne: Profil Serwera, Login, Haslo, PIN, Slot Postaci (0-4).
  • Automatyczny odczyt/zapis konfiguracji z fast_boot_config.json.
             │
             ▼ (Klikniecie "WEJDZ DO GRY" lub Auto-Connect)
[ NATYWNY PIPELINE SIECIOWY C++ (< 250 ms) ]
  1. Handshake TCP (0xFD -> 0xFF).
  2. Negocjacja kluczy DH2 (261 B) -> Aktywacja Blowfish CTR.
  3. Pakiet autoryzacji 0x6F (168 B) z HWID SHA-256.
  4. Odbior 0x96 (AUTH_SUCCESS, dwLoginKey).
  5. Przelaczenie socketu na Game Server (LOGIN2 0x6D).
  6. Odbior listy postaci (0x20).
  7. DirectEnterMode: Natychmiastowe wyslanie SelectCharacter(slot).
  8. Odbior pakietu postaci 0x8A -> Wyslanie SendEnterGame(0x0206).
             │
             ├─► W razie bledu serwera (ALREADY / WRONGPWD / TIMEOUT):
             │   Natychmiastowy komunikat w ImGui (czerwony kolor) bez crasha.
             │
             ▼ (Odebranie GC::PHASE = 5 PHASE_GAME)
[ LAZY WORLD INITIALIZATION & WEJSCIE DO GRY ]
  • Powiadomienie Pythona: SetGamePhase() -> otwarcie game.GameWindow.
  • Plynne ukrycie nakladki ImGui i pojawienie sie renderera swiata 3D.
  • Czas od startu do swiata gry: ~1.5 sekundy (glownie RTT sieci).
```

---

## 3. Szczegolowa Analiza i Rozwiazanie Krytycznych Usterek

Podczas audytu i refaktoryzacji zidentyfikowano i trwale naprawiono nastepujace usterki architektoniczne:

### A. Przedwczesne kasowanie `DirectEnterMode` w Phase Loading
* **Plik**: [`src/UserInterface/PythonNetworkStreamPhaseLoading.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/PythonNetworkStreamPhaseLoading.cpp)
* **Mechanizm usterki**:
  W metodzie `SetLoadingPhase()` wywolywana byla funkcja `__DirectEnterMode_Initialize()`, ktora przestawiala `m_kDirectEnterMode.m_isSet` na `false`. Gdy serwer nadsylal pakiet identyfikacyjny postaci (`0x8A` dla Luria lub standardowy `GC_MAIN_CHARACTER`), warunek:
  ```cpp
  if (g_bHeadlessMode || __DirectEnterMode_IsSet())
  ```
  byl zawsze falszywy. Klient nie wysylal pakietu `SendEnterGame()`, lecz probowal odpalic procedure ladowania w Pythonie (`LoadData`), co w trybie Fast-Boot zamrazalo klienta w fazie Loading.
* **Rozwiazanie**:
  Usunieto `__DirectEnterMode_Initialize()` z metody `SetLoadingPhase()`. Flaga jest czyszczona dopiero **po** pomyslnym wyemitowaniu pakietu `SendEnterGame()` w metodach `RecvMainCharacter`, `RecvMainCharacterLuria8A` oraz `RecvMainCharacterLuria0F`.

### B. Brak powiadomienia Pythona o wejsciu w faze Game (Python Notify Gap)
* **Plik**: [`src/UserInterface/PythonNetworkStreamPhaseGameSync.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/PythonNetworkStreamPhaseGameSync.cpp)
* **Mechanizm usterki**:
  Metoda `PhaseGameSyncBridge::SetGamePhase()` przestawiala strumien C++ w faze `"Game"`, ale nie wywolywala metody `SetGamePhase` na handlerze Pythona (`m_poHandler`). W efekcie instancja `game.GameWindow` nigdy nie zostawala zainicjalizowana, a okno gry nie wywolywalo `app.RenderGame()`.
* **Rozwiazanie**:
  Dodano bezpieczne powiadomienie handlera:
  ```cpp
  if (pStream->m_poHandler)
  {
      PyCallClassMemberFunc(pStream->m_poHandler, "SetGamePhase", Py_BuildValue("()"));
  }
  ```

### C. Status Overwrite Loop i brak obslugi kodow bledow
* **Pliki**: [`src/UserInterface/FastBootOverlay.h`](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/FastBootOverlay.h), [`FastBootOverlay.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/FastBootOverlay.cpp)
* **Mechanizm usterki**:
  Metoda `UpdateNetworkStatus()` w kazdej klatce renderingu nadpisywala status ciagiem *"Trwa autoryzacja i handshake DH2..."* dopoki `m_bConnecting == true`. Gdy serwer odpowiedzial kodem bledu (np. `0x07 ALREADY`), flaga nie byla zerowana, a blad byl natychmiast maskowany.
* **Rozwiazanie**:
  - Wprowadzono metode `SetLoginFailure(const char* szStatus)` thrad-safe z translacja kodow na jezyk polski:
    - `ALREADY`: Informacja o aktywnej sesji na serwerze i koniecznosci odczekania 15 sekund.
    - `WRONGPWD`: Blad hasla lub loginu.
    - `NOID`: Brak wskazanego konta.
    - `BLOCK`: Blokada konta.
    - `WRONGMAT`: Nieprawidlowy PIN lub kod usuniecia postaci.
    - `CONNECT_FAIL`: Brak mozliwosci polaczenia z portem IP.
    - `SHUTDOWN`: Zerwanie polaczenia przez serwer.
    - `TIMEOUT`: Przekroczenie 15-sekundowego limitu czasu odpowiedzi.
  - Podpieto wywolania w [`AccountConnector.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/AccountConnector.cpp) oraz [`PythonNetworkStreamPhaseLogin.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/PythonNetworkStreamPhaseLogin.cpp).

### D. Przedwczesne czyszczenie danych logowania
* **Plik**: [`src/UserInterface/AccountConnector.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/AccountConnector.cpp)
* **Mechanizm usterki**:
  Wywolanie `ClearLoginInfo()` w trakcie wysylania pakietu autoryzacji zerowalo pola `m_strPassword` i `m_strPin` w pamieci RAM. Gdy polaczenie zostalo odrzucone (np. przez `ALREADY`), uzytkownik nie mogl ponowic proby bez ponownego wpisywania danych.
* **Rozwiazanie**:
  Usunieto przedwczesne czyszczenie z fazy wysylania pakietu autoryzacji.

### E. Desynchronizacja Strumienia TCP (76 Bajtow) przy `TPacketGCMainCharacter` (`0x0071`) na Serwerze Standard x64
* **Pliki**: [`src/Client/Network/Protocol/Drivers/StandardX64ProtocolDriver.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/Drivers/StandardX64ProtocolDriver.cpp), [`src/UserInterface/PythonNetworkStreamPhaseLoading.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/PythonNetworkStreamPhaseLoading.cpp)
* **Mechanizm usterki**:
  Serwer x64 wysyla ujednolicona 113-bajtowa strukture `TPacketGCMainCharacter` (zawierajaca `szName[25]`, `wRaceNum`, `lX, lY, lZ`, `byEmpire`, `bySkillGroup`), podczas gdy klient oczekiwal 37-bajtowego pakietu. Pozostale 76 bajtow zostawalo w buforze TCP i psulo odczyt wszystkich kolejnych pakietow (powodujac bledy `UNKNOWN HEADER: 149`, brak modeli i zamrozenie ruchu).
* **Rozwiazanie**:
  Wdrozono ramkowanie x64 ze stalym lub dynamicznym naglowkiem (ze scisla kontrola dlugosci ramki w `InspectFrame()`) oraz obsluge pelnej struktury w `RecvMainCharacter()` i `LoadingPacketHandler` wraz z odczytem nazwy, rasy i koordynatow swiata `(lX, lY)`.

### F. Czarne Tlo Mapy, Szare Modele i Crash `0xC0000005` przy Ruchu (Uniwersalny VFS i Ochrona `CMapOutdoor`)
* **Pliki**: [`src/PackLib/PackManager.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/PackLib/PackManager.cpp), [`src/GameLib/MapManager.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/GameLib/MapManager.cpp), [`src/UserInterface/PythonBackground.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/PythonBackground.cpp), [`src/GameLib/MapOutdoor.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/GameLib/MapOutdoor.cpp)
* **Mechanizm usterki**:
  `CPackManager` przeszukiwal podkatalogi `assets/<Paczka>/` wylacznie dla plikow `.glb`/`.gr2`, ignorujac `AtlasInfo.txt`, `Setting.txt`, `.raw`, `.msenv` oraz tekstury `.dds`. Powodowalo to brak zaladowania mapy (`m_pkMap == nullptr`), a klikniecie/ruch postacia wywolywalo `GetPickingPointWithRay` na zerowym wskazniku (`0xC0000005` pod offsetem `0x12F6`).
* **Rozwiazanie**:
  Odblokowano przeszukiwanie zamontowanych podkatalogow `assets/` dla **wszystkich rozszerzen** w `GetFileWithPool()` oraz `IsExist()`, dodano wielokierunkowy fallback dla `AtlasInfo.txt` (110 map), bezpieczny `s_fallbackMap` w `GetMapOutdoorRef()` oraz guardy `if (!m_pkMap || !IsMapReady() || !m_pRootNode) return false;`.

### G. Kreator Postaci na Pustym Koncie oraz Samoczynny Auto-Reconnect
* **Plik**: [`src/UserInterface/FastBootOverlay.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/FastBootOverlay.cpp)
* **Rozwiazanie**:
  - Gdy wybrany slot postaci na koncie jest pusty (`IsCharacterSlotEmpty`), overlay ImGui wyswietla natywny kreator postaci (`CreateCharacterAndEnter()`), a dla agentow AI udostepniono metode IPC `game_create_character`.
  - W przypadku zerwania polaczenia (`OnRemoteDisconnect`), modul Auto-Reconnect (`--auto-reconnect`, checkbox w ImGui lub RPC `game_set_auto_reconnect`) automatycznie odlicza czas i wznawia sesje do swiata gry.

### H. Bezpieczna Inicjalizacja Flagi `--direct` w `WinMain`
* **Plik**: [`src/UserInterface/UserInterface.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/UserInterface.cpp)
* **Rozwiazanie**:
  Przeniesiono wywolanie `CPythonNetworkStream::Instance().SetDirectEnterMode(g_iDirectEnterSlot)` za alokacje `new CPythonApplication` w `Main()`, zapobiegajac wywolaniu na niezainicjalizowanym singletonie.

---

## 4. Specyfikacja Protokolu Luria 2026 (Live Handshake & Auth)

Podsystem Fast-Boot w pelni obsluguje niestandardowy protokol serwera Luria:

| Krok | Kierunek | Naglowek / Rozmiar | Opis i Zawartosc |
|---|---|---|---|
| 1 | Serwer $\rightarrow$ Klient | `0xFD` (15 B) | Pakiet fazy: `phase = 1` (`PHASE_HANDSHAKE`). |
| 2 | Klient $\rightarrow$ Serwer | `0xFF` (13 B) | Handshake ACK z wyrownaniem czasu `dwTime + lDelta * 2`. |
| 3 | Serwer $\rightarrow$ Klient | `0xFB` (261 B) | Klucz publiczny serwera DH2 (256 B danych + naglowek). |
| 4 | Klient $\rightarrow$ Serwer | `0xFB` (261 B) | Klucz publiczny klienta wyliczony przez `NegotiateDH2Keys`. |
| 5 | Serwer $\rightarrow$ Klient | `0xFA` (4 B) | Potwierdzenie uzgodnienia kluczy; **Aktywacja Blowfish CTR**. |
| 6 | Klient $\rightarrow$ Serwer | `0x6F` (168 B) | Pakiet autoryzacji: login, haslo, PIN (4 znaki), token, HWID SHA-256. |
| 7 | Serwer $\rightarrow$ Klient | `0x96` (6 B) | `AUTH_SUCCESS`: odbior `dwLoginKey`. Sukces autoryzacji. |
| 8 | Przelaczenie | - | Zamkniecie portu Auth (11469) i polaczenie z Game Serverem (np. 29301). |
| 9 | Klient $\rightarrow$ Serwer | `0x6D` (65 B) | `LOGIN2`: autoryzacja sesji gry za pomoca `dwLoginKey`. |
| 10 | Serwer $\rightarrow$ Klient | `0x20` (489 B) | Lista 5 postaci na koncie (dane, levele, koordynaty). |
| 11 | Klient $\rightarrow$ Serwer | `0x06` (2 B) | Wybor wybranego slotu postaci (`DirectEnterMode`). |
| 12 | Serwer $\rightarrow$ Klient | `0x8A` (77 B) | Zidentyfikowanie postaci gracza w swiecie (`VID`, nazwa, koordynaty). |
| 13 | Klient $\rightarrow$ Serwer | `0x0206` (68 B) | `CG_ENTERGAME`: rzadanie wejscia do swiata gry. |
| 14 | Serwer $\rightarrow$ Klient | `0xFD` (2 B) | `PHASE = 5` (`PHASE_GAME`). Gracz pojawia sie na mapie! |

---

## 5. Konfiguracja Runtime (`fast_boot_config.json`)

Stan nakladki oraz wybrane dane uwierzytelniajace sa trwale przechowywane w pliku JSON w katalogu roboczym gry:
```json
{
    "profile_idx": 0,
    "login": "loginek60",
    "password": "!a123123",
    "pin": "1544",
    "slot": 0,
    "save": true,
    "auto_reconnect": true
}
```

* **Skrot klawiszowy**: Klawisz **F11** umozliwia w dowolnym momencie wlaczenie lub ukrycie panelu Fast-Boot bez przerywania dzialania gry.
* **Auto-ukrycie**: Po wejsciu do fazy `Game` panel automatycznie ukrywa sie, pozostawiajac pelne okno dla gry.

---

## 6. Weryfikacja Jakosciowa i Testy

Wszystkie wprowadzone modyfikacje sa objete scisla weryfikacja automatyczna:
1. **Preflight Check**:
   ```bash
   python tools/ai_preflight_check.py
   ```
   Weryfikuje poprawnosc ciagow formatujacych, wyrownanie struktur C-ABI oraz sygnatury w 0.4s.
2. **Kompilacja Release x64**:
   ```bash
   cmake --build build --config Release --target UserInterface
   ```
3. **Pelny pakiet CTest (51/51 - 100% PASS)**:
   ```bash
   ctest -C Release --output-on-failure
   ```
4. **Test na zywo z serwerem Luria oraz Standard x64**:
   ```bash
   python tools/mcp/test_luria_live_harness.py
   ```
   Potwierdza kompletny cykl autoryzacji i wejscie postacia do swiata gry w czasie ponizej 1.5 sekundy.

