# Przewodnik Dostosowania Klienta m2dev (x64) do Serwera PandoraMT2

Dokument zawiera kompletna liste wymogow, plikow zrodlowych, zmian architektonicznych oraz szczegolowy opis glownych funkcji rozgrywki (ruch, walka, skille, podnoszenie przedmiotow, klikanie) wraz z ich limitami czestotliwosci (Rate Limits) i zasiegu (Range Checks).

Dzieki temu przewodnikowi klient PC (`m2dev`) bedzie komunikowal sie z serwerem PandoraMT2 w 100% natywnie, zachowujac identyczna mechanike jak na telefonie.

---

## SPIS TRESCI
1. [Podsumowanie Architektury: m2dev vs Pandora](#1-podsumowanie-architektury-m2dev-vs-pandora)
2. [Krok 1: Zmiana Definicji Naglowkow i Struktur (Packet.h)](#krok-1-zmiana-definicji-naglowkow-i-struktur-packeth)
3. [Krok 2: Przebudowa Odbiornika Pakietow (DispatchPacket w PythonNetworkStream.cpp)](#krok-2-przebudowa-odbiornika-pakietow-dispatchpacket-w-pythonnetworkstreamcpp)
4. [Krok 3: Glowne Funkcje Rozgrywki, Ich Limity i Aktualizacja](#krok-3-glowne-funkcje-rozgrywki-ich-limity-i-aktualizacja)
   - [3.1. Poruszanie sie postaci (Movement)](#31-poruszanie-sie-postaci-movement)
   - [3.2. Zwykly atak i walka wrecz (Combat & Attack)](#32-zwykly-atak-i-walka-wrecz-combat--attack)
   - [3.3. Uzywanie umiejetnosci (Skills & Fly Targeting)](#33-uzywanie-umiejetnosci-skills--fly-targeting)
   - [3.4. Podnoszenie dropu z ziemi (Item Pickup)](#34-podnoszenie-dropu-z-ziemi-item-pickup)
   - [3.5. Interakcja i klikanie obiektow (On Click NPC/Metin)](#35-interakcja-i-klikanie-obiektow-on-click-npcmetin)
   - [3.6. Handel i sklepy NPC (Shop Buy & Sell)](#36-handel-i-sklepy-npc-shop-buy--sell)
   - [3.7. Dialogi misji i skryptow (Quest Script Answer)](#37-dialogi-misji-i-skryptow-quest-script-answer)
5. [Krok 4: Wymiana Handshake i Logowania (PhaseHandShake & PhaseLogin)](#krok-4-wymiana-handshake-i-logowania-phasehandshake--phaselogin)
6. [Krok 5: Rejestracja Dummy Handlers dla Nowych Systemow](#krok-5-rejestracja-dummy-handlers-dla-nowych-systemow)
7. [Krok 6: Synchronizacja Danych Gry (Mapy, Kolizje, Proto)](#krok-6-synchronizacja-danych-gry-mapy-kolizje-proto)
8. [Pelna Checklista Wdrozeniowa](#8-pelna-checklista-wdrozeniowa)

---

## 1. Podsumowanie Architektury: m2dev vs Pandora

Klient Pandora na Androida to klasyczny silnik C++ Ymir Metin2 skompilowany na architekture ARM64. 
Wersja `m2dev-client-src-mainOryginalx64` to nowoczesny refaktor silnika pod architekture 64-bitowa (x64), w ktorym wprowadzono wlasny, niestandardowy framing sieciowy.

| Obszar | m2dev-client-src-mainOryginalx64 | PandoraMT2 (Serwer / libmain.so) | Wymagana Zmiana w m2dev |
| :--- | :--- | :--- | :--- |
| **Typ naglowka** | `uint16_t` (2 bajty, `TPacketHeader`) | `uint8_t` / `BYTE` (1 bajt) | Zmiana typu na `uint8_t` |
| **Zakres opcodow** | Modularne `0x0100` - `0x0C00` | Tradycyjne opcody `1 - 255` | Przywrocenie mapy opcodow Ymir |
| **Kadrowanie (Framing)** | `[header: 2B][length: 2B][dane]` w **kazdym** pakiecie | Pakiety stale: `[header: 1B][dane]`<br>Pakiety zmienne: `[header: 1B][len: 2B][dane]` | Usuniecie pola `length` z pakietow stalych |
| **Dispatch pakietow** | Wymaga pola `length` w strumieniu | Oparty o tabele rozmiarow (Header Map) | Odtworzenie tabelarycznego sprawdzania rozmiaru |
| **Handshake** | 32B `server_pk` + `challenge` (Custom m2dev) | 13B powitanie (`dwHandshake`, `dwTime`, `lDelta`) | Wdrozenie standardowego handshake 13B |
| **Gameplay i fizyka** | 100% identyczny (Ymir C++) | 100% identyczny (Ymir C++) | **Brak zmian!** Logika dziala 1:1 |

---

## 2. Krok 1: Zmiana Definicji Naglowkow i Struktur (Packet.h)

Lokalizacja pliku: `src/UserInterface/Packet.h` oraz `src/EterLib/ControlPackets.h`.

### 2.1. Zmiana typu naglowka i usuniecie 4-bajtowego wymogu
W `src/UserInterface/Packet.h`:
```cpp
// ZAMIAST:
// typedef uint16_t TPacketHeader;
// constexpr uint16_t PACKET_HEADER_SIZE = 4;

// ZMIENIC NA:
typedef uint8_t TPacketHeader;
constexpr uint16_t PACKET_HEADER_SIZE = 1;
```

### 2.2. Przywrocenie opcodow Pandory
Zamiast 16-bitowych zakresow (`0x0301`, `0x0401` itp.), wprowadz opcody zgodne ze specyfikacja z `E:\PandoraRepo\PandoraProtocol.h`:
```cpp
namespace CG
{
    constexpr uint8_t LOGIN            = 1;   // 0x01
    constexpr uint8_t ATTACK           = 2;   // 0x02
    constexpr uint8_t CHAT             = 3;   // 0x03
    constexpr uint8_t CHARACTER_CREATE = 4;   // 0x04
    constexpr uint8_t CHARACTER_DELETE = 5;   // 0x05
    constexpr uint8_t CHARACTER_SELECT = 6;   // 0x06
    constexpr uint8_t MOVE             = 7;   // 0x07
    constexpr uint8_t SYNC_POSITION    = 8;   // 0x08
    constexpr uint8_t ENTERGAME        = 10;  // 0x0A
    constexpr uint8_t ITEM_USE         = 11;  // 0x0B
    constexpr uint8_t ITEM_DROP        = 12;  // 0x0C
    constexpr uint8_t ITEM_MOVE        = 13;  // 0x0D
    constexpr uint8_t ITEM_PICKUP      = 15;  // 0x0F
    constexpr uint8_t QUICKSLOT_ADD    = 16;  // 0x10
    constexpr uint8_t QUICKSLOT_DEL    = 17;  // 0x11
    constexpr uint8_t QUICKSLOT_SWAP   = 18;  // 0x12
    constexpr uint8_t WHISPER          = 19;  // 0x13
    constexpr uint8_t ON_CLICK         = 26;  // 0x1A
    constexpr uint8_t EXCHANGE         = 27;  // 0x1B
    constexpr uint8_t SCRIPT_ANSWER    = 29;  // 0x1D
    constexpr uint8_t QUEST_INPUT      = 30;  // 0x1E
    constexpr uint8_t QUEST_CONFIRM    = 31;  // 0x1F
    constexpr uint8_t SHOP             = 50;  // 0x32
    constexpr uint8_t FLY_TARGETING    = 51;  // 0x33
    constexpr uint8_t SHOOT            = 53;  // 0x35
    constexpr uint8_t USE_SKILL        = 54;  // 0x36
    constexpr uint8_t TARGET           = 60;  // 0x3C
    constexpr uint8_t KEY_AGREEMENT    = 105; // 0x69
}

namespace GC
{
    constexpr uint8_t WARP             = 1;   // 0x01 (13B)
    constexpr uint8_t PHASE            = 2;   // 0x02 (2B)
    constexpr uint8_t NOTICE           = 3;   // 0x03 (dynamic)
    constexpr uint8_t CHAT             = 4;   // 0x04 (dynamic)
    constexpr uint8_t CHARACTER_ADD    = 5;   // 0x05 (67B)
    constexpr uint8_t SYNC_POSITION    = 13;  // 0x0D (9B)
    constexpr uint8_t LOGIN_SUCCESS    = 14;  // 0x0E (1B)
    constexpr uint8_t MAIN_CHARACTER   = 16;  // 0x10 (5B)
    constexpr uint8_t POINTS           = 17;  // 0x11 (65B)
    constexpr uint8_t POINT_CHANGE     = 18;  // 0x12 (9B)
    constexpr uint8_t ITEM_SET         = 19;  // 0x13 (42B)
    constexpr uint8_t ITEM_USE         = 20;  // 0x14 (3B)
    constexpr uint8_t ITEM_UPDATE      = 21;  // 0x15 (8B)
    constexpr uint8_t ITEM_GROUND_ADD  = 22;  // 0x16 (15B)
    constexpr uint8_t SHOP             = 28;  // 0x1C (dynamic)
    constexpr uint8_t EXCHANGE         = 30;  // 0x1E (dynamic)
    constexpr uint8_t PING             = 31;  // 0x1F (1B)
    constexpr uint8_t DAMAGE_INFO      = 34;  // 0x22 (9B)
    constexpr uint8_t TARGET           = 36;  // 0x24 (5B)
    constexpr uint8_t WHISPER          = 40;  // 0x28 (dynamic)
    constexpr uint8_t DEAD             = 76;  // 0x4C (5B)
    constexpr uint8_t HANDSHAKE        = 255; // 0xFF (13B)
}
```

---

## 3. Krok 2: Przebudowa Odbiornika Pakietow (DispatchPacket w PythonNetworkStream.cpp)

W pliku `src/UserInterface/PythonNetworkStream.cpp` funkcja `DispatchPacket` zaklada z gory obecnosc 4-bajtowego naglowka z dlugoscia. 
Nalezy zaimplementowac sprawdzanie rozmiaru na podstawie zarejestrowanych pakietow.

### Kod zamienny dla `CPythonNetworkStream::DispatchPacket`:
```cpp
bool CPythonNetworkStream::DispatchPacket(const PacketHandlerMap& handlers)
{
    TPacketHeader header;
    if (!Peek(sizeof(TPacketHeader), &header))
        return false;

    // Pomin zera / padding szyfrowania
    while (0 == header)
    {
        if (!Recv(sizeof(TPacketHeader), &header))
            return false;
        if (!Peek(sizeof(TPacketHeader), &header))
            return false;
    }

    auto it = handlers.find(header);
    if (it == handlers.end())
    {
        TraceError("Unknown packet header: 0x%02X (%u), Phase: %s", header, header, m_strPhase.c_str());
        ClearRecvBuffer();
        return false;
    }

    const auto& handlerEntry = it->second;
    uint32_t uPacketSize = 0;

    // Sprawdzenie czy pakiet jest dynamiczny czy staly
    if (handlerEntry.isDynamic)
    {
        // Pakiety dynamiczne maja [header 1B][size 2B LE]
        struct TDynHeader { uint8_t h; uint16_t size; };
        TDynHeader dyn;
        if (!Peek(sizeof(TDynHeader), &dyn))
            return false;
        uPacketSize = dyn.size;
    }
    else
    {
        uPacketSize = handlerEntry.expectedSize;
    }

    // Oczekiwanie na pelny pakiet w buforze
    if (m_recvBuf.ReadableBytes() < uPacketSize)
        return false;

    // Wywolanie wlasciwego handlera
    LogRecvPacket(header, uPacketSize);
    bool ret = (this->*(handlerEntry.handler))();

    if (!ret || handlerEntry.exitPhase)
        return false;

    return true;
}
```

---

## 4. Krok 3: Glowne Funkcje Rozgrywki, Ich Limity i Aktualizacja

Oto szczegolowa specyfikacja 7 glownych funkcji styku z serwerem, ktore realizuja interakcje gracza ze swiatem:

---

### 3.1. Poruszanie sie postaci (Movement)

* **Pliki zrodlowe:**
  - `src/UserInterface/PythonPlayerEventHandler.cpp` (obsluga zdarzen)
  - `src/UserInterface/PythonNetworkStreamPhaseGame.cpp` (wysylka pakietu)
  - `src/UserInterface/InstanceBaseMovement.cpp` (predkosc i animacje)

* **Limity i weryfikacja serwera:**
  - **Interwal wysylania pakietow:** Na telefonie i Pandorze interwal wynosi **250 ms** (w m2dev domyslnie 300 ms).
  - **Speedhack Range Check:** Serwer sprawdza `Dystans <= (MoveSpeed * DeltaCzasu / 1000) * 1.15`.
  - **Limit predkosci klienta:** `uMovSpd` ma limit 1100 (zabezpieczenie przed overflow w `InstanceBaseMovement.cpp`).

* **Co zmienic w kodzie:**
  1. W `PythonPlayerEventHandler.cpp` zmien interwal z `300` na `250`:
     ```cpp
     void CPythonPlayerEventHandler::OnMoving(const SState& c_rkState)
     {
         DWORD dwCurTime = ELTimer_GetMSec();
         if (m_dwNextMovingNotifyTime > dwCurTime)
             return;

         m_dwNextMovingNotifyTime = dwCurTime + 250; // ZMIANA Z 300 NA 250

         CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
         rkNetStream.SendCharacterStatePacket(c_rkState.kPPosSelf, c_rkState.fAdvRotSelf, CInstanceBase::FUNC_MOVE, 0);
     }
     ```
  2. W `PythonNetworkStreamPhaseGame.cpp` usun przypisanie `length`:
     ```cpp
     bool CPythonNetworkStream::SendCharacterStatePacket(const TPixelPosition& c_rkPPosDst, float fDstRot, UINT eFunc, UINT uArg)
     {
         if (fDstRot < 0.0f) fDstRot = 360 + fDstRot;
         else if (fDstRot > 360.0f) fDstRot = fmodf(fDstRot, 360.0f);

         TPacketCGMove kStatePacket;
         kStatePacket.header = CG::MOVE; // 0x07 (1B)
         // kStatePacket.length = sizeof(kStatePacket); <-- USUNIETE
         kStatePacket.bFunc = eFunc;
         kStatePacket.bArg = uArg;
         kStatePacket.bRot = (uint8_t)(fDstRot / 5.0f);
         kStatePacket.lX = long(c_rkPPosDst.x);
         kStatePacket.lY = long(c_rkPPosDst.y);
         kStatePacket.dwTime = ELTimer_GetServerMSec();

         __LocalPositionToGlobalPosition(kStatePacket.lX, kStatePacket.lY);
         return Send(sizeof(kStatePacket), &kStatePacket);
     }
     ```

---

### 3.2. Zwykly atak i walka wrecz (Combat & Attack)

* **Pliki zrodlowe:**
  - `src/UserInterface/PythonNetworkStreamPhaseGame.cpp` -> `SendAttackPacket`
  - `src/UserInterface/PythonPlayerEventHandler.cpp` -> `OnAttack`

* **Limity i weryfikacja serwera:**
  - **Zasieg ataku (Attack Range):** Dystans gracza do ofiary `dwVIDVictim` nie moze przekraczac zasiegu broni (zwykle ok. **300–400 jednostek**).
  - **Czestotliwosc atakow:** Serwer sprawdza czas miedzy pakietami ataku w oparciu o statystyke `POINT_ATTACK_SPEED`. Spamowanie atakami z pomieciem animacji skutkuje brakiem obrazen na serwerze.
  - **Suma kontrolna ataku (bCRCHash):** Struktura Pandory ma pole `uint16_t bCRCHash`. Klient musi wysylac rotacyjny licznik sekwencji.

* **Co zmienic w kodzie:**
  ```cpp
  bool CPythonNetworkStream::SendAttackPacket(UINT uMotAttack, DWORD dwVIDVictim)
  {
      if (!__CanActMainInstance())
          return true;

      TPacketCGAttack kPacketAtk;
      kPacketAtk.header = CG::ATTACK; // 0x02 (1B)
      // kPacketAtk.length = sizeof(kPacketAtk); <-- USUNIETE
      kPacketAtk.bType = uMotAttack;
      kPacketAtk.dwVictimVID = dwVIDVictim;
      kPacketAtk.bCRCHash = (uint16_t)(14 + (ELTimer_GetMSec() % 4)); // Suma kontrolna Pandory

      return Send(sizeof(kPacketAtk), &kPacketAtk);
  }
  ```

---

### 3.3. Uzywanie umiejetnosci (Skills & Fly Targeting)

* **Pliki zrodlowe:**
  - `src/UserInterface/PythonNetworkStreamPhaseGame.cpp` -> `SendUseSkillPacket`
  - `src/UserInterface/PythonPlayerEventHandler.cpp` -> `OnUseSkill`

* **Limity i weryfikacja serwera:**
  - **Cooldown Check:** Serwer odrzuca pakiet, jesli umiejetnosc nie ostygla (dane z `skill_proto`).
  - **Target Check:** Dystans do celu i kat w przypadku skilli kierunkowych (szarza, wir miecza itp.).

* **Co zmienic w kodzie:**
  ```cpp
  bool CPythonNetworkStream::SendUseSkillPacket(DWORD dwSkillIndex, DWORD dwTargetVID)
  {
      TPacketCGUseSkill UseSkillPacket;
      UseSkillPacket.header = CG::USE_SKILL; // 0x36 / 54 (1B)
      // UseSkillPacket.length = sizeof(UseSkillPacket); <-- USUNIETE
      UseSkillPacket.dwVnum = dwSkillIndex;
      UseSkillPacket.dwTargetVID = dwTargetVID;

      return Send(sizeof(UseSkillPacket), &UseSkillPacket);
  }
  ```

---

### 3.4. Podnoszenie dropu z ziemi (Item Pickup)

* **Pliki zrodlowe:**
  - `src/UserInterface/PythonNetworkStreamPhaseGame.cpp` -> `SendItemPickUpPacket`
  - `src/UserInterface/PythonPlayerInputMouse.cpp` -> obsluga klawisza Z / podnoszenia

* **Limity i weryfikacja serwera:**
  - **Zasieg (Range Check):** Dystans gracza do przedmiotu na ziemi musi byc `<= 300 jednostek`.
  - **Rate Limit (Flood):** Serwer ignoruje podnoszenie czestsze niz ok. 100 ms na przedmiot.

* **Co zmienic w kodzie:**
  ```cpp
  bool CPythonNetworkStream::SendItemPickUpPacket(DWORD dwVID)
  {
      if (!__CanActMainInstance())
          return true;

      TPacketCGItemPickUp kItemPickUp;
      kItemPickUp.header = CG::ITEM_PICKUP; // 0x0F (1B)
      // kItemPickUp.length = sizeof(kItemPickUp); <-- USUNIETE
      kItemPickUp.dwVID = dwVID;

      return Send(sizeof(kItemPickUp), &kItemPickUp);
  }
  ```

---

### 3.5. Interakcja i klikanie obiektow (On Click NPC/Metin)

* **Pliki zrodlowe:**
  - `src/UserInterface/PythonNetworkStreamPhaseGame.cpp` -> `SendOnClickPacket`

* **Limity i weryfikacja serwera:**
  - **Zasieg interakcji:** Maksymalnie **400–500 jednostek** do NPC / potwora / zyl rudy. Przy wiekszym dystansie serwer nie otwiera dialogu ani okna sklepu.

* **Co zmienic w kodzie:**
  ```cpp
  bool CPythonNetworkStream::SendOnClickPacket(DWORD dwVID)
  {
      if (!__CanActMainInstance())
          return true;

      TPacketCGOnClick kOnClick;
      kOnClick.header = CG::ON_CLICK; // 0x1A / 26 (1B)
      // kOnClick.length = sizeof(kOnClick); <-- USUNIETE
      kOnClick.dwVID = dwVID;

      return Send(sizeof(kOnClick), &kOnClick);
  }
  ```

---

### 3.6. Handel i sklepy NPC (Shop Buy & Sell)

* **Pliki zrodlowe:**
  - `src/UserInterface/PythonNetworkStreamPhaseGame.cpp` -> `SendShopBuyPacket`, `SendShopSellPacket`

* **Limity i weryfikacja serwera:**
  - **Stan okna sklepu:** Gracz musi miec aktywne otwarte okno sklepu (otwarte przez `HEADER_GC_SHOP`).
  - **Limit slotu:** Indeks przedmiotu od 0 do 39 (Standard) lub do 79 (Extended).

* **Co zmienic w kodzie:**
  ```cpp
  bool CPythonNetworkStream::SendShopBuyPacket(BYTE bPos)
  {
      TPacketCGShop kShop;
      kShop.header = CG::SHOP; // 0x32 / 50 (1B)
      kShop.bSubHeader = 0; // SUBHEADER_CG_SHOP_BUY
      kShop.bPos = bPos;

      return Send(sizeof(kShop), &kShop);
  }
  ```

---

### 3.7. Dialogi misji i skryptow (Quest Script Answer)

* **Pliki zrodlowe:**
  - `src/UserInterface/PythonNetworkStreamPhaseGame.cpp` -> `SendScriptAnswerPacket`, `SendQuestConfirmPacket`

* **Limity i weryfikacja serwera:**
  - Weryfikacja aktywnego identyfikatora questa w kolejce zadan.

* **Co zmienic w kodzie:**
  ```cpp
  bool CPythonNetworkStream::SendScriptAnswerPacket(BYTE bAnswer)
  {
      TPacketCGScriptAnswer kAnswer;
      kAnswer.header = CG::SCRIPT_ANSWER; // 0x1D / 29 (1B)
      kAnswer.bAnswer = bAnswer;

      return Send(sizeof(kAnswer), &kAnswer);
  }
  ```

---

## 5. Krok 4: Wymiana Handshake i Logowania (PhaseHandShake & PhaseLogin)

### 5.1. Handshake w `PythonNetworkStreamPhaseHandShake.cpp`
Zamiast pakietu `KEY_CHALLENGE` z 32B kluczami, zaimplementuj odbior 13-bajtowego pakietu powitalnego Ymir:
```cpp
bool CPythonNetworkStream::RecvHandshake()
{
    struct TPacketGCHandshake
    {
        uint8_t  header;       // 0xFF lub 0xFD
        uint32_t dwHandshake;  // Losowy token sesji
        uint32_t dwTime;       // Czas serwera
        int32_t  lDelta;      // Przesuniecie zegara
    } hs;

    if (!Recv(sizeof(hs), &hs))
        return false;

    // Synchronizacja zegara klienta z czasem serwera (ZAPOBIEGA DETEKCJI SPEEDHACKA!)
    ELTimer_SetServerMSec(hs.dwTime + hs.lDelta);

    return true;
}
```

### 5.2. Logowanie w `PythonNetworkStreamPhaseLogin.cpp`
Zastap `SendLoginPacketNew` (z pakietem `LOGIN2` 16B) klasycznym wyslaniem loginu i hasla:
```cpp
bool CPythonNetworkStream::SendLoginPacketNew(const char * c_szName, const char * c_szPassword)
{
    TPacketCGLogin LoginPacket;
    memset(&LoginPacket, 0, sizeof(LoginPacket));
    LoginPacket.header = CG::LOGIN; // 0x01
    strncpy(LoginPacket.name, c_szName, sizeof(LoginPacket.name) - 1);
    strncpy(LoginPacket.pwd, c_szPassword, sizeof(LoginPacket.pwd) - 1);

    return Send(sizeof(LoginPacket), &LoginPacket);
}
```

---

## 6. Krok 5: Rejestracja Dummy Handlers dla Nowych Systemow

Pandora posiada systemy nieobecne w bazowym `m2dev` (Szarfy, Sklepy Offline, Aury, Battle Pass, Wiki, Captcha). 
Jesli serwer wysle taki pakiet, `m2dev` bez rejestracji wywola `PostQuitMessage(0)` i wylaczy gre!

W `CPythonNetworkStream::SetGamePhase()` zarejestruj proste handlery odrzucajace dane (Dummy Handlers):

```cpp
bool CPythonNetworkStream::__RecvDummyPacket(size_t packetSize)
{
    return m_recvBuf.Skip(packetSize);
}

void CPythonNetworkStream::__RegisterPandoraDummyHandlers()
{
    // Opcody wyciagniete z E:\PandoraRepo\packets_gc_specification.json
    RegisterHandler(96,  &CPythonNetworkStream::__RecvOfflineShopDummy, 12);
    RegisterHandler(110, &CPythonNetworkStream::__RecvCaptchaDummy, 64);
    RegisterHandler(111, &CPythonNetworkStream::__RecvAcceDummy, 24);
    RegisterHandler(112, &CPythonNetworkStream::__RecvAuraDummy, 16);
    RegisterHandler(114, &CPythonNetworkStream::__RecvSwitchbotDummy, 32);
    RegisterHandler(115, &CPythonNetworkStream::__RecvBattlePassDummy, 48);
    RegisterHandler(116, &CPythonNetworkStream::__RecvWikiDummy, 20);
    RegisterHandler(131, &CPythonNetworkStream::__RecvAutoHuntDummy, 10);
}
```

---

## 7. Krok 6: Synchronizacja Danych Gry (Mapy, Kolizje, Proto)

Same pakiety umozliwia polaczenie i bieg, ale aby uniknac cofek (rubberbandingu) i bledow animacji:

1. **Mapy i Kolizje (`Maps` / `pack/map.epk`):**
   - Wypakuj mapy z klienta Pandory (pliki `.atr`, `.raw`, `.prb`).
   - Wrzuc je do folderu `pack` lub `Maps` klienta `m2dev`.
   - Klient PC bedzie uzywal dokladnie tej samej siatki kolizji, co serwer Pandory.

2. **Baza przedmiotow i potworow (`item_proto` / `mob_proto`):**
   - Przekopiuj `item_proto` oraz `mob_proto` z Pandory do folderu `locale/pl/` klienta `m2dev`.
   - Zapewni to dokladnie te sama predkosc ruchu bazowa (`POINT_MOV_SPEED`), predkosc ataku i poprawne nazwy bytow.

---

## 8. Pelna Checklista Wdrozeniowa

- [ ] **1. `Packet.h`:** Zmiana `TPacketHeader` na `uint8_t`.
- [ ] **2. `Packet.h`:** Podmiana enumow `CG::` i `GC::` na 8-bitowe opcody Pandory (1 - 255).
- [ ] **3. `Packet.h`:** Usuniecie pola `length` ze struktur stalych.
- [ ] **4. `PythonNetworkStream.cpp`:** Dostosowanie `DispatchPacket` do sprawdzania rozmiarow z tabeli.
- [ ] **5. `PythonPlayerEventHandler.cpp`:** Zmiana interwalu ruchu w `OnMoving` z 300 ms na 250 ms.
- [ ] **6. `PythonNetworkStreamPhaseGame.cpp`:** Aktualizacja funkcji `SendCharacterStatePacket` (ruch 16B).
- [ ] **7. `PythonNetworkStreamPhaseGame.cpp`:** Aktualizacja funkcji `SendAttackPacket` (atak 8B z CRC hash).
- [ ] **8. `PythonNetworkStreamPhaseGame.cpp`:** Aktualizacja funkcji `SendUseSkillPacket` (skille 0x36).
- [ ] **9. `PythonNetworkStreamPhaseGame.cpp`:** Aktualizacja funkcji `SendItemPickUpPacket` (pickup 5B).
- [ ] **10. `PythonNetworkStreamPhaseGame.cpp`:** Aktualizacja funkcji `SendOnClickPacket` (klikanie 5B).
- [ ] **11. `PythonNetworkStreamPhaseHandShake.cpp`:** Obsluga 13B handshake i synchronizacja czasu `ELTimer_SetServerMSec`.
- [ ] **12. `PythonNetworkStreamPhaseLogin.cpp`:** Obsluga `TPacketCGLogin` (0x01).
- [ ] **13. `PythonNetworkStreamPhaseGame.cpp`:** Rejestracja `Dummy Handlers` dla opcodow 96-135.
- [ ] **14. Assety:** Skopiowanie folderu map oraz `item_proto` i `mob_proto` z klienta Pandory.

Wszystkie powyzsze kroki zapewnia pelna stabilnosc polaczenia oraz plynna rozgrywke na kliencie PC bez ryzyka cofek i bledow desynchronizacji.
