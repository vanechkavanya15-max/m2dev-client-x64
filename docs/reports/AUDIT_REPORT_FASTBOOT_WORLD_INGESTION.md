# Raport Audytu Technicznego: Mechanizm Fast-Boot, Faza Wejscia do Swiata i Diagnoza Problemu "Terrain-Only + Chat"

Data audytu: 2026-10-12  
Srodowisko: C++23 x64, Visual Studio 2022 Release, Python 3.14  
Repozytorium: `E:\m2dev-client-src-mainOryginalx64`  

---

## 1. Cel Audytu i Zgloszony Problem

Uzytkownik zglosil nastepujace zachowanie po uruchomieniu gry z logowaniem Fast-Boot ImGui:
> *"jest taki problem miedzy przejsciem z enter to game jakby klient sie wczytal swiat bo widzialem wiadomosci innych ale tak graficznie jest teraz sie gra zaladowala ale tylko mapa teren i nic wiecej procz czatu, czyli cos jest zle po tym jak logowanie obslugujemy imgui tak, czyli to python wczytuje caly swiat gry pokazuje w grze na ekranie czy jak to jest przeanalizuj krytycznie stworz raport"*

Audyt mial na celu wyjasnienie:
1. Jaki jest dokladny podzial odpowiedzialnosci pomiedzy silnikiem natywnym C++ a warstwa Pythona w architekturze Metin2.
2. Dlaczego po zaladowaniu widoczny byl wylacznie teren mapy 3D oraz wiadomosci czatu, natomiast brakowalo:
   - Modeli 3D glownej postaci gracza
   - Modeli 3D innych graczy oraz NPC w miescie
   - Paskow interfejsu (TaskBar, orb HP/SP, pasek skilli, minimapa)
3. Jakie konkretne wady w kodzie C++ oraz Pythonie doprowadzily do tego stanu i jak nalezy je wyeliminowac.

---

## 2. Architektoniczny Podzial Rol: C++ vs Python w Metin2

Wbrew powszechnemu wyobrazeniu, Python w Metin2 **NIE renderuje** bezposrednio swiata 3D (rendering terenu, cieniowanie, macierze widoku, rendering geometrii postaci i siatek wykonuje w 100% C++ w DirectX 9). Jednakze:

| Warstwa | Odpowiedzialnosc | Skutek braku wykonania |
|---|---|---|
| **C++ (Silnik 3D & Siec)** | Renderowanie geometrii terenu (`CPythonBackground`), obsluga socketu TCP, deszyfrowanie pakietow (`BlowfishCryptoProvider`), transformacja kosci (`CGrannyModelInstance`), zarzadzanie instancjami (`CPythonCharacterManager`). | Jesli nie ma pakietu lub instancji, na ekranie nie ma modelu. |
| **Python (Metadane & Rejestr)** | **Zarejestrowanie ras, sciezki do modeli `.msm`, przypisanie animacji `.msa` do stanow ruchu, parsowanie `npclist.txt`, rejestracja skilli, rang, kolorow nickow oraz budowanie okien UI.** | Silnik C++ ma pusta tablice ras w `CRaceManager`. Gdy serwer przesle pakiet dodania aktora (`GC_CHARACTER_ADD`), C++ nie wie skad wczytac model, tworzac pustego "ducha". |

---

## 3. Identyfikacja Przyczyn Zrodlowych (Root Causes)

Przeprowadzona analiza logow sieciowych `syserr.txt`, kodu zrodlowego oraz bazy pakietow w `E:\Metin2Clientless` ujawnila **trzy bezposrednie przyczyny**:

### Przyczyna 1 (KRYTYCZNA): Wadliwe Filtrowanie `RecvCharacterAppendPacketLuria` w C++
W pliku [`src/UserInterface/PythonNetworkStreamPhaseGameActor.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/PythonNetworkStreamPhaseGameActor.cpp):

```cpp
// Linie 536-548 w RecvCharacterAppendPacketLuria():
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

- **Blad**: Kod celowo pomijal wywolanie `__RecvCharacterAppendPacket` dla `TYPE_PC` (gracze, w tym postac glowna!) oraz `TYPE_NPC` (wszyscy handlarze i NPC w miescie), oczekujac na wtorny pakiet `TPacketGCCharacterAdditionalInfo`.
- **Fakt serwera Luria**: Serwer Luria wysyla samowystarczalny pakiet `0x01` (37B) i **NIE WYSYLA** pakietu `CHAR_ADDITIONAL_INFO` dla kazdego bytu na mapie.
- **Konsekwencja**: Zaden gracz ani zaden NPC w miescie **nie zostal zarejestrowany w `CPythonCharacterManager`**! Ani postac gracza, ani NPC nie mialy utworzonej instancji `CInstanceBase`. Z tego powodu w swiecie gry nie bylo widac zadnych postaci.

### Przyczyna 2: Pomijanie Wywolania `playerSettingModule.LoadGameData`
W standardowym kliencie funkcja `LoadData()` w [`introloading.py`](file:///E:/m2dev-client-src-mainOryginalx64/m2dev-client-main/assets/root/introloading.py) rejestruje:
- `WARRIOR`, `ASSASSIN`, `SURA`, `SHAMAN` (modele `.msm`, animacje `.msa`)
- `NPC`, `ENEMY` (parsowanie `npclist.txt`)
- `RegisterSkill(race, group, empire)`
- `RegisterTitleName`, `RegisterNameColor`

W trybie Fast-Boot (DirectEnterMode), poniewaz C++ natychmiast po odebraniu `0x8A` wysylal `SendEnterGame()`, a serwer od razu odsyalal faze `Game`, okno `introLoading` bylo natychmiast zamykane i niszczone, zanim `LoadData()` zdolalo zarejestrowac rasy w C++ `CRaceManager`.

### Przyczyna 3: Odrzucanie Pakietow Statystyk w Fazie Loading
Przed zakonczeniem przelaczenia na `Phase=Game`, serwer przesyla pakiety inicjalizacyjne:
- `0x10` (InventorySync, 1021B)
- `0x11` (PointChange, 17B - aktualizacje HP, MP, STAMINA, GOLD, EXP)
- `0x15` (QuickslotSync, 64B)
- `0x4C` (QuestSync, 1531B)

Poniewaz silnik znajdowal sie jeszcze ulamki sekund w `Phase=Loading`, pakiet sniffer w C++ zrzucal te pakiety jako `UnknownOpcode` (widoczne w `syserr.txt` linie 222-260). W efekcie baza statystyk gracza w C++ `CPythonPlayer` miala wartosci zerowe, co uniemozliwialo prawidlowe wypelnienie orba HP/SP i paska statusu w interfejsie Pythona.

---

## 4. Wdrozone i Zarejestrowane Materialy Dokumentacyjne

W ramach biezacej aktualizacji dokumentacji przygotowano i zaktualizowano:
1. [`docs/03_network_protocol/09_fastboot_loading_to_game_pipeline.md`](file:///E:/m2dev-client-src-mainOryginalx64/docs/03_network_protocol/09_fastboot_loading_to_game_pipeline.md):
   - Pelna specyfikacja podzialu rol C++ vs Python.
   - Diagram przeplywu fazy Loading do Game w trybie standardowym i Fast-Boot.
   - Szczegolowa dokumentacja bledow braku widocznosci modeli i HUD.
2. [`docs/03_network_protocol/06_luria_protocol_driver_dh2.md`](file:///E:/m2dev-client-src-mainOryginalx64/docs/03_network_protocol/06_luria_protocol_driver_dh2.md):
   - Uaktualniono tabele pakietow odbiorczych GC: dodano `0x8A` (77B MainCharacter), `0x0F` (47B Alt MainCharacter), `0x10` (1021B Inventory), `0x11` (17B PointChange), `0x15` (64B Quickslot), `0x4C` (1531B Quests).
   - Oznaczono pakiet `0x01` jako samowystarczalny (bez oczekiwania na `CHAR_ADDITIONAL_INFO`).
3. [`docs/03_network_protocol/README.md`](file:///E:/m2dev-client-src-mainOryginalx64/docs/03_network_protocol/README.md):
   - Zaktualizowano glowny spis tresci pakietu dokumentacji sieciowej o pozycje 09.

---

## 5. Rekomendowane Kroki Naprawcze (Gotowe do Implementacji)

1. **W `src/UserInterface/PythonNetworkStreamPhaseGameActor.cpp` (`RecvCharacterAppendPacketLuria`)**:
   Zastapic blokujacy warunek `if (m_bType != TYPE_PC && m_bType != TYPE_NPC)` bezposrednim wywolaniem:
   ```cpp
   if (kNetActorData.m_stName.empty())
   {
       const char* c_szName = nullptr;
       if (CPythonNonPlayer::Instance().GetName(kNetActorData.m_dwRace, &c_szName))
           kNetActorData.m_stName = c_szName;
       else if (kNetActorData.m_dwVID == GetMainActorVID())
           kNetActorData.m_stName = CPythonPlayer::Instance().GetName();
   }
   __RecvCharacterAppendPacket(&kNetActorData);
   ```
2. **W `m2dev-client-main/assets/root/game.py` (`GameWindow.Open`)**:
   Dodac mechanizm `EnsureGameDataLoaded()` na samym poczatku `Open()`:
   Jezeli rasy nie zostaly jeszcze zarejestrowane przez `introLoading`, wykonac natychmiastowa, synchroniczna rejestracje `playerSettingModule.LoadGameData(...)` dla 4 klas, NPC, efektow, skilli i rang.
3. **W `src/UserInterface/PythonNetworkStream.cpp`**:
   Wlaczyc routing pakietow `0x10`, `0x11`, `0x15` rowniez w fazie Loading, aby wstepne dane inwentarza i punktow statystyk nie byly odrzucane przez sniffer.
