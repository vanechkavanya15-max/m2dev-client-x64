# Architektura Fast-Boot, Pipeline Loading -> Game oraz Analiza Podzialu Odpowiedzialnosci C++ i Python

## 1. Wprowadzenie i Cel Dokumentu

Niniejszy dokument opisuje architekture przejscia miedzy fazami sieciowymi klienta gry Metin2 (x64), ze szczegolnym uwzglednieniem trybu **Fast-Boot / Direct-to-World (Dear ImGui DX9)** oraz rygorystycznego podzialu odpowiedzialnosci pomiedzy silnikiem natywnym C++ a warstwa skryptowa Pythona.

Dokument zawiera rowniez pelna analize krytyczna problemu "Terrain-Only + Chat" (stanu, w ktorym po zalogowaniu na serwer renderowany jest wylacznie teren 3D i okno czatu, bez widocznych modeli postaci gracza, NPC oraz pelnego interfejsu HUD).

---

## 2. Podzial Odpowiedzialnosci: Silnik C++ vs Warstwa Skryptowa Python

W oryginalnej architekturze silnika gry Metin2 istnieje scisly podzial obowiazkow pomiedzy rdzeniem C++ a skryptami Pythona:

### 2.1. Rola Silnika C++ (DirectX 9 / Vulkan, Core Engine)
- **Niskopoziomowy rendering 3D**: Rendering siatki terenu (`CPythonBackground`), zarzadzanie kwaterami terenu (`CTerrainQuadTree`), cieniowanie, mgla oraz obiekty statyczne mapy (budynki, drzewa SpeedTree, mosty).
- **Zarzadzanie aktorami i animacja**: `CPythonCharacterManager`, `CRaceManager`, `CActorInstance`, `CGrannyModelInstance`. C++ odpowiada za transformacje kosci, interpolacje klatek animacji `.msa`, rendering siatek `.msm` oraz nakladanie tekstur.
- **Obsluga sieci TCP i kryptografii**: `CNetworkStream`, `CPythonNetworkStream`, sterowniki protokolu (`IServerProtocolDriver`), deszyfrowanie pakietow w locie (`BlowfishCryptoProvider`, `SodiumCryptoProvider`).
- **Kamera i przestrzen**: Obliczanie macierzy widoku i rzutowania (`CCamera`), pozycjonowanie kamery wzgledem celu.

### 2.2. Rola Warstwy Pythona (Metadata, Rejestracja Zasobow i Logika UI)
- **Rejestracja definicji ras i modeli**: C++ sam z siebie NIE WIE, jakie modele i animacje odpowiadaja poszczegolnym numerom ras (`wRaceNum`). To skrypt `playerSettingModule.py` wywoluje bindingi C++:
  - `chrmgr.CreateRace(race)`
  - `chrmgr.SelectRace(race)`
  - `chrmgr.SetPathName("d:/ymir work/pc/warrior/")`
  - `chrmgr.RegisterCacheMotionData(mode, motion, "wait_1.msa", ...)`
  - `chrmgr.LoadRaceData("warrior_m.msm")`
- **Rejestracja bazy NPC i potworow**: Parsowanie pliku `npclist.txt` i powiazanie numerow VNUM z modelami 3D (`chrmgr.CreateRace`, `chrmgr.LoadRaceData`).
- **Rejestracja skilli, rang i kolorow**:
  - `playerSettingModule.RegisterSkill(race, group, empire)` - przypisanie identyfikatorow skilli do slotow gracza.
  - `chrmgr.RegisterTitleName(...)`, `chrmgr.RegisterNameColor(...)` - kolory nickow i rang nad glowami postaci.
  - `emotion.RegisterEmotionIcons()` - ikony emocji.
- **Tworzenie i zarzadzanie oknami HUD**: `interfaceModule.Interface`, `uiTaskBar.TaskBar` (orby HP/SP, pasek skilli, przyciski systemowe), `uiMiniMap.MiniMapWindow`, `uiChat.ChatWindow`.

> [!IMPORTANT]
> Jesli warstwa Pythona nie zarejestruje ras w `CRaceManager` przed nadejsciem pakietow spawnu postaci (`0x01` / `GC_CHARACTER_ADD`), silnik C++ nie wie skad pobrac pliki `.msm` i animacje `.msa`. W rezultacie postac gracza oraz byty swiata staja sie niewidzialnymi bytami bez geometrii.

---

## 3. Tradycyjny Przebieg Faz vs Tryb Fast-Boot

### 3.1. Tradycyjny Przebieg (Legacy Python FSM)
1. `introLogin.py` -> Logowanie do serwera auth, odbior sukcesu.
2. `introSelect.py` -> Wybor postaci z 5 slotow, wyslanie `CG_SELECT_CHARACTER`.
3. `introLoading.py` -> Okno ladowania z tlem i paskiem postepu:
   - Odbior `GC_PHASE(PHASE_LOADING)` -> `SetLoadingPhase()`
   - Odbior `GC_MAIN_CHARACTER` -> C++ wywoluje `introLoading.LoadData(playerX, playerY)`
   - `LoadData()` wykonuje petle krok po kroku: `INIT`, `MAP`, `SOUND`, `EFFECT`, `WARRIOR`, `ASSASSIN`, `SURA`, `SHAMAN`, `SKILL`, `ENEMY`, `NPC`
   - Na koncu `LoadData()` wywolywane jest `net.StartGame()` oraz `net.SendEnterGamePacket()`
   - Serwer odsyla `GC_PHASE(PHASE_GAME)` -> przelaczenie na `game.py:GameWindow`.

### 3.2. Przebieg w Trybie Fast-Boot (Dear ImGui Direct-to-World)
1. Uzytkownik w oknie Fast-Boot (skrot klawiszowy `F11`) wybiera profil serwera (np. Luria CH4) i klika `ZALOGUJ I WEJDZ DO GRY`.
2. `FastBootOverlay::ConnectCurrentProfile()` konfiguruje `CAccountConnector` oraz ustawia `rkNetStream.SetDirectEnterMode(slot)`.
3. `CAccountConnector` przeprowadza Handshake, wymiane kluczy DH2 oraz autoryzacje `0x6F`.
4. Po odebraniu `0x96` (Auth Success), nastepuje natychmiastowe przelaczenie na Game Server:
   - Handshake DH2 na porcie gry -> autoryzacja `0x6D`.
   - Po odebraniu fazy `Select` automatycznie wysylany jest pakiet wyboru postaci `0x06` dla wybranego slotu (bez czekania na UI Pythona).
   - Po odebraniu `GC_PHASE(PHASE_LOADING)` i pakietu postaci glownej (`0x8A` / `0x0F`), silnik natychmiast wykonuje `Warp(x, y)` i wysyla `SendEnterGame()`.
   - Klient wchodzi bezposrednio do fazy `Game` w C++ i Pythona.

---

## 4. Analiza Przyczyn Problemu "Terrain-Only + Chat" (Root Cause Analysis)

Podczas testow na serwerze zewnetrznym Luria zaobserwowano objaw:
- Teren mapy 3D wczytal sie w 100% poprawnie (tekstury, obiekty otoczenia, rzeka, mosty).
- Okno czatu bylo widoczne i wyswietlalo wiadomosci od innych graczy.
- **BRAKOWALO**: modelu postaci gracza, modeli NPC w miescie, innych graczy, orbow HP/SP, minimapy oraz paska skilli.

Szczegolowy audyt zrodel i logow transmisyjnych (`syserr.txt` oraz baza `E:\Metin2Clientless`) ujawnil 3 powiazane przyczyny:

### Przyczyna 1: Blokada Tworzenia Graczy i NPC w `RecvCharacterAppendPacketLuria`
W pliku `src/UserInterface/PythonNetworkStreamPhaseGameActor.cpp` istnial nastepujacy kod:

```cpp
// Wadliwy fragment w RecvCharacterAppendPacketLuria():
if (kNetActorData.m_bType != CActorInstance::TYPE_PC && 
    kNetActorData.m_bType != CActorInstance::TYPE_NPC)
{
    const char* c_szName = nullptr;
    if (CPythonNonPlayer::Instance().GetName(kNetActorData.m_dwRace, &c_szName))
        kNetActorData.m_stName = c_szName;

    __RecvCharacterAppendPacket(&kNetActorData);
}
else
{
    s_kNetActorData = kNetActorData;
}
```

**Mechanizm bledu**:
- Kod zakladal, ze dla kazdego gracza (`TYPE_PC = 0`) oraz NPC (`TYPE_NPC = 1`) serwer przesle pozniej pakiet `TPacketGCCharacterAdditionalInfo` (`CHAR_ADDITIONAL_INFO`), a do tego czasu obiekt jest tylko zapamietywany w zmiennej statycznej `s_kNetActorData`.
- W protokole serwera Luria pakiet `0x01` (`HEADER_GC_CHARACTER_ADD`, 37B) jest pakietem kompletnym lub dane postaci glownej sa przesylane w pakiecie `0x8A` (77B). Serwer Luria **NIE wysyla** klasycznego pakietu `CHAR_ADDITIONAL_INFO` dla kazdego bytu.
- **Skutek**: Dla glownej postaci gracza (`m_bType == TYPE_PC`) oraz dla wszystkich NPC w miescie (`m_bType == TYPE_NPC`) funkcja `__RecvCharacterAppendPacket` **NIGDY NIE ZOSTALA WYWOLANA**!
- Obiekty aktorow w ogole nie zostaly utworzone w `CPythonCharacterManager`. Silnik nie mial czego renderowac, a kamera nie miala punktu zaczepienia gracza.

### Przyczyna 2: Asynchroniczne Pominiecie `LoadData()` w Szybkim Logowaniu
- Przy natychmiastowym wejsciu do gry, gdy pakiet `0x8A` nadszedl w tym samym pakiecie co przelaczenie fazy, wywolanie `SendEnterGame()` i nadejscie `GC_PHASE(PHASE_GAME)` nastepowalo zanim `introLoading.py` zdazylo zarejestrowac rasy wojownika, sury, szamana, ninjy, potworow i NPC w `playerSettingModule.py`.
- Gdy rasy nie sa zarejestrowane, C++ `CRaceManager::GetRaceDataPointer(race)` zwraca `nullptr`, co blokuje ladowanie siatki `.msm`.

### Przyczyna 3: Przepelnienie Bufora w Fazie Loading i Odrzucanie Pakietow Statystyk
- Po wyslaniu `SendEnterGame` serwer Luria natychmiast przesyla pakiet inwentarza `0x10` (1021B), pakiet zadan `0x4C` (1531B), pakiety paskow `0x15` (64B) oraz serie pakietow `0x11` (PointChange, 17B - HP, MP, EXP, Level).
- Poniewaz klient znajdowal sie jeszcze ulamki sekund w `Phase=Loading`, pakiet sniffer odrzucal te pakiety jako `UnknownOpcode` w fazie Loading, przez co baza statystyk gracza (`CPythonPlayer`) nie otrzymala poziomu HP i punktow postaci, uniemozliwiajac wypelnienie paska statusu w UI.

---

## 5. Standard Architektoniczny Poprawki (Plan Dzialania)

Aby zapewnic stabilne, bezbledne ladowanie zarowno na serwerze Luria, serwerze lokalnym x64, jak i w trybie Headless:

1. **Poprawka w `RecvCharacterAppendPacketLuria`**:
   - Kazdy byt odbierany z naglowkiem `0x01` (37B) musi byc natychmiast przekazywany do `__RecvCharacterAppendPacket(&kNetActorData)`.
   - Jesli byt to NPC lub gracz glowny, nazwa jest pobierana z bazy `CPythonNonPlayer` lub z zapisanego profilu postaci.
2. **Game Data Bootstrap (`EnsureGameDataLoaded`)**:
   - Wdrozenie bezpiecznego, jednokrotnego mechanizmu gwarantujacego zaladowanie `playerSettingModule.LoadGameData` bezposrednio przy inicjalizacji `GameWindow.Open()` (jesli nie zostalo wywolane w `introLoading`).
3. **Dispatcher Route Buffering**:
   - Rejestracja handlerow `0x10`, `0x11`, `0x15` rowniez w fazie Loading, aby pakiety statystyk nadsylane tuz przed flaga wejscia do gry nie byly gubione.
