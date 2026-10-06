# Dokumentacja Podsystemu: Python Bindings - Moduly 'player' oraz 'skill'

## 1. Cel Architektoniczny i Rola Modulu

Moduly `player` oraz `skill` stanowia glowny pomost komunikacyjny pomiedzy skryptami interfejsu gry napisanymi w jezyku Python a zarzadzaniem stanem lokalnego gracza w C++ (`CPythonPlayer`) oraz baza danych umiejetnosci (`CPythonSkill`).

### Glowne Odpowiedzialnosci:
1. **CPythonPlayer (modul `player`)**:
   - Reprezentuje postac aktualnie sterowana przez uzytkownika na maszynie klienckiej.
   - Trzyma lokalny cache ekwipunku (`TItemData`), pasywnego i aktywnego wyposazenia, smoczych kamieni (DS) oraz walut (Gold, Cheque, Gaya).
   - Steruje ruchem postaci na podstawie wejscia myszy i klawiatury (PickCloseItem, SetMouseState, SetAttackKeyState).
   - Obsluguje stany umiejetnosci (cooldowny, ladowanie, sloty quickslotow).
   - Rejestruje efekty wizualne gracza (auto-potiony, efekty buffow, emotki).
2. **CPythonSkill (modul `skill`)**:
   - Rejestr konfiguracji umiejetnosci ladowanych z `SkillDesc.txt` oraz `SkillTable.txt`.
   - Zapewnia dostep do ikon skilli, nazw, poziomow zaawansowania (Master, Grand Master, Perfect Master).
   - Wylicza dynamiczne wartosci formul obrazen i zuzycia many dla interfejsu (Tooltips).

## 2. Diagram Architektury i Przeplywu Danych

```mermaid
graph TD
    subgraph Warstwa Pythona UI
        UI_INV[uiInventory.py]
        UI_TASKBAR[uiTaskBar.py]
        UI_CHAR[uiCharacter.py]
    end

    subgraph Modul player (C++ Binding)
        PY_PLAYER[player Module Methods]
        CPP_PLAYER[CPythonPlayer Singleton]
        CACHE_INV[Inventory & Equipment Cache]
        SKILL_STATE[Skill State & Cooldowns]
    end

    subgraph Modul skill (C++ Binding)
        PY_SKILL[skill Module Methods]
        CPP_SKILL[CPythonSkill Singleton]
        SKILL_TABLE[Skill Data Registry]
    end

    subgraph Silnik C++ i Swiat Gry
        INST_BASE[CInstanceBase - Aktor Gracza]
        NET_STREAM[CPythonNetworkStream]
    end

    UI_INV -->|player.GetItemIndex, player.SetItemData| PY_PLAYER
    UI_TASKBAR -->|player.ActivateSkill, player.GetQuickslot| PY_PLAYER
    UI_CHAR -->|skill.GetSkillName, skill.GetSkillAffect| PY_SKILL

    PY_PLAYER --> CPP_PLAYER
    CPP_PLAYER --> CACHE_INV
    CPP_PLAYER --> SKILL_STATE
    CPP_PLAYER -->|Sterowanie ruchem i animacja| INST_BASE
    CPP_PLAYER -->|Wysylanie komend ataku/uzycia| NET_STREAM

    PY_SKILL --> CPP_SKILL
    CPP_SKILL --> SKILL_TABLE
```

## 3. Rejestr Struktur Danych i Pamieci (Memory & Struct Layout)

### 3.1 SAutoPotionInfo
Struktura zarzadzajaca automatycznym uzywaniem mikstur zycia i many:
- `bool bActivated` (1 bajt): Flaga czy auto-potion jest wlaczony.
- `long currentAmount` (4 bajty): Aktualny stan napelnienia eliksiru.
- `long totalAmount` (4 bajty): Maksymalna pojemnosc eliksiru.
- `long inventorySlotIndex` (4 bajty): Numer slotu w ekwipunku przypisany do mikstury.

### 3.2 TItemData (Lokalny Cache Ekwipunku w CPythonPlayer)
- `DWORD vnum` (4 bajty): ID przedmiotu w bazie.
- `BYTE count` (1 bajt): Liczba sztuk.
- `DWORD flags` (4 bajty): Flagi przedmiotu.
- `DWORD anti_flags` (4 bajty): Flagi restrykcji.
- `long alSockets[ITEM_SOCKET_SLOT_MAX_NUM]` (3 x 4 bajty = 12 bajtow): Kamienie duszy / przetopy.
- `TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_SLOT_MAX_NUM]` (7 x 4 bajty = 28 bajtow): Bonusy 1-5 oraz 6-7.

### 3.3 TSkillData (Wpis Umiejetnosci w CPythonSkill)
- `DWORD dwSkillIndex` (4 bajty): VNUM umiejetnosci (np. 1 = Trzystronne Ciecie, 2 = Wir Miecza).
- `BYTE bType` (1 bajt): Typ skilla (aktywny, pasywny, gildyjny, jezykowy).
- `BYTE bLevelStep` (1 bajt): Stopien zaawansowania (0..40).
- `DWORD dwIconFileName` (string/ID): Zasob graficzny ikony.
- `std::string strName`: Nazwa umiejetnosci.

## 4. Wykaz Kluczowych Metod Modulu 'player' (API Reference)

| Metoda Python | Sygnatura C++ | Parametry | Zwracana Wartosc | Logika Biznesowa |
|---|---|---|---|---|
| `player.GetAutoPotionInfo(type)` | `playerGetAutoPotionInfo` | `int potionType` | `(bActivated, cur, total, slot)` | Zwraca stan auto-potiona HP (0) lub SP (1). |
| `player.SetAutoPotionInfo(type, act, cur, max, slot)` | `playerSetAutoPotionInfo` | `int, bool, long, long, long` | `None` | Konfiguruje stan automatycznej mikstury z serwera. |
| `player.PickCloseItem()` | `playerPickCloseItem` | brak | `None` | Wywoluje `CPythonPlayer::PickCloseItem()` – automatyczny podnoszenie przedmiotow w zasiegu gracza. |
| `player.SetGameWindow(pyHandle)` | `playerSetGameWindow` | `PyObject* pyHandle` | `None` | Rejestruje instancje okna glownego gry (game.py) do odbierania zdarzen. |
| `player.RegisterEffect(eEft, path)` | `playerRegisterEffect` | `int iEft, char* szFileName` | `None` | Wczytuje i rejestruje efekt graficzny czasteczek pod podanym ID. |
| `player.SetMouseState(eMBT, eMBS)` | `playerSetMouseState` | `int eMBT, int eMBS` | `None` | Informuje silnik o kliknieciu lub zwolnieniu przycisku myszy (ruch / obrot kamery). |
| `player.GetItemIndex(cell)` | `playerGetItemIndex` | `TItemPos cell` | `int dwVnum` | Zwraca VNUM przedmiotu znajdujacego sie w podanym slocie ekwipunku / magazynu. |
| `player.GetItemCount(cell)` | `playerGetItemCount` | `TItemPos cell` | `int count` | Zwraca ilosc sztuk w slocie. |
| `player.GetItemMetinSocket(cell, idx)` | `playerGetItemMetinSocket` | `TItemPos cell, int idx` | `int socketVal` | Zwraca wartosc gniazda (KD lub czas trwania). |
| `player.GetItemAttribute(cell, idx)` | `playerGetItemAttribute` | `TItemPos cell, int idx` | `(type, value)` | Zwraca typ i wartosc bonusu w przedmiocie. |
| `player.GetStatus(dwType)` | `playerGetStatus` | `int dwType` | `long lVal` | Zwraca aktualna wartosc punktow postaci (HP, SP, EXP, Level, Str, Dex, Def). |
| `player.SetAttackKeyState(bState)` | `playerSetAttackKeyState` | `bool bState` | `None` | Wlacza/wylacza stan ataku ze spacji. |
| `player.ActivateSkill(slot)` | `playerActivateSkill` | `int dwSlot` | `None` | Uruchamia rzucenie umiejetnosci przypisanej do wskazanego slotu. |
| `player.GetSkillCoolTime(slot)` | `playerGetSkillCoolTime` | `int dwSlot` | `(curCooldown, maxCooldown)` | Zwraca czas odnowienia czaru dla paska interfejsu. |

## 5. Wykaz Kluczowych Metod Modulu 'skill' (API Reference)

| Metoda Python | Sygnatura C++ | Parametry | Zwracana Wartosc | Logika Biznesowa |
|---|---|---|---|---|
| `skill.GetSkillName(vnum)` | `skillGetSkillName` | `int dwVnum` | `string` | Zwraca zlokalizowana nazwe umiejetnosci. |
| `skill.GetSkillType(vnum)` | `skillGetSkillType` | `int dwVnum` | `int bType` | Zwraca typ umiejetnosci (aktywna, pasywna, buff). |
| `skill.GetSkillIconPath(vnum)` | `skillGetSkillIconPath` | `int dwVnum` | `string` | Zwraca sciezke pliku graficznego ikony w VFS (d:/ymir work/ui/skill/...). |
| `skill.GetSkillAffect(vnum)` | `skillGetSkillAffect` | `int dwVnum` | `string` | Zwraca opis wplywu/efektu czaru do wyswietlenia w okienku postaci. |
| `skill.GetSkillCoolTime(vnum, lvl)` | `skillGetSkillCoolTime` | `int dwVnum, int level` | `float` | Wylicza bazowy czas odnowienia czaru dla wskazanego poziomu zaawansowania. |
| `skill.CanUseSkill(vnum)` | `skillCanUseSkill` | `int dwVnum` | `bool` | Sprawdza wymagania broni (np. czy gracz trzyma luk dla skilli Lucznika). |

## 6. Punkty Styku (Cross-Subsystem Integration)

1. **CPythonNetworkStream**:
   - Akcje inicjowane w `player` (np. uzycie przedmiotu, klikniecie NPC, rzucenie czaru) konwertowane sa na pakiety `CPacketCGItemUse`, `CPacketCGOnClick`, `CPacketCGUseSkill` i wysylane przez strumien sieciowy do serwera.
2. **CInstanceBase (Aktor Gracza)**:
   - `CPythonPlayer` posiada bezposredni wskaznik na wlasny obiekt `CInstanceBase` w swiecie 3D. Aktualizuje jego wektory predkosci, cel ataku (`SetTarget`), oraz animacje walki i rzucania czarow w Granny 3D.
3. **EterPack VFS**:
   - Wszystkie ikony umiejetnosci i efekty particle pobierane sa transparentnie przez wirtualny system plikow `CEterPackManager`.

## 7. Pulapki, Antywzorce i Ograniczenia

1. **Wielowatkowosc i Bezpieczenstwo GIL Pythona**:
   - Wywolania metod `player` nastepuja w watku renderowania/petli gry. Modyfikowanie stanu gracza z innych watkow bez trzymania Pythona GIL prowadzi do natychmiastowego crashu interpretera CPython.
2. **Desynchronizacja Slotow Ekwipunku**:
   - Lokalny cache `CPythonPlayer::m_playerStatus.aItem` jest aktualizowany wylacznie na podstawie pakietow przychodzacych z serwera (`TPacketGCItemSet`). Skrypty Pythona nie moga bezposrednio zmieniac zawartosci slotow, a jedynie wysylac zadania przemieszczenia (`CPacketCGItemMove`).
3. **Zaleznosc od Broni dla Skilli**:
   - Wywolanie `skill.CanUseSkill()` weryfikuje aktualnie zalozony typ broni. Brak synchronizacji typu broni przed proba uzycia czaru skutkuje cichym odrzuceniem pakietu przez serwer.
