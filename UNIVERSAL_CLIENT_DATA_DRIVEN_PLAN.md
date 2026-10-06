# Kompleksowy Plan Budowy Uniwersalnego Klienta Data-Driven (m2dev x64 Multi-Server)

Dokument stanowi kompletna architekture i plan techniczny przeksztalcenia klienta `E:\m2dev-client-src-mainOryginalx64` w uniwersalny silnik oparty na konfiguracji (Data-Driven Client). 

Po wdrozeniu ponizszego planu, plik wykonywalny `.exe` jest kompilowany **tylko raz**. Obsluga dowolnego nowego serwera (Pandora, Glador, Alune, serwery mobilne i PC) polega wylacznie na dodaniu pliku profilu `.json` bez modyfikacji kodu zrodlowego C++ i bez rekompilacji.

---

## SPIS TRESCI
1. [Koncepcja i Architektura Data-Driven](#1-koncepcja-i-architektura-data-driven)
2. [Format Plikow Konfiguracyjnych (JSON)](#2-format-plikow-konfiguracyjnych-json)
   - [2.1. Profil glowny serwera (servers/pandora.json)](#21-profil-glowny-serwera-serverspandorajson)
   - [2.2. Rejestr pakietow serwera (servers/pandora_gc.json)](#22-rejestr-pakietow-serwera-serverspandora_gcjson)
3. [Implementacja C++ Managera Konfiguracji w m2dev](#3-implementacja-c-managera-konfiguracji-w-m2dev)
4. [Dostosowanie Warstwy Sieciowej (Network Layer)](#4-dostosowanie-warstwy-sieciowej-network-layer)
   - [4.1. Dynamiczny DispatchPacket (odbiór pakietow z tabeli JSON)](#41-dynamiczny-dispatchpacket-odbior-pakietow-z-tabeli-json)
   - [4.2. Uniwersalne wysylanie pakietow CG (Send...)](#42-uniwersalne-wysylanie-pakietow-cg-send)
   - [4.3. Zmienny Handshake i faza logowania](#43-zmienny-handshake-i-faza-logowania)
5. [Dynamiczne Limity i Funkcje Rozgrywki (Gameplay)](#5-dynamiczne-limity-i-funkcje-rozgrywki-gameplay)
   - [5.1. Ruch (Movement & Anti-Speedhack)](#51-ruch-movement--anti-speedhack)
   - [5.2. Atak i walka wrecz (Combat & CRC)](#52-atak-i-walka-wrecz-combat--crc)
   - [5.3. Skille, Pickup, OnClick, Ekwipunek, Czat](#53-skille-pickup-onclick-ekwipunek-czat)
6. [Przeplyw Pracy (Workflow) – Dodanie Nowego Serwera w 5 Minut](#6-przeplyw-pracy-workflow--dodanie-nowego-serwera-w-5-minut)
7. [Checklista Wdrozeniowa Krok po Kroku](#7-checklista-wdrozeniowa-krok-po-kroku)

---

## 1. Koncepcja i Architektura Data-Driven

W klasycznym podejsciu kazda zmiana opcodu, rozmiaru pakietu lub czestotliwosci ruchu wymaga edycji kodu C++ i rekompilacji.
W podejsciu **Data-Driven**:
- Kod C++ implementuje uniwersalne szablony akcji i zarzadzanie buforami.
- Wszystkie zmienne (adresy IP, porty, stale czasowe, limity predkosci, opcody CG i rozmiary GC) sa wstrzykiwane z pliku konfiguracyjnego w momencie startu gry:
  ```text
  Metin2Client.exe --server pandora
  ```
- Klient dynamicznie przelacza tryb kadrowania (framing klasyczny Ymir 1B naglowek vs nowoczesny 4B framing m2dev).

---

## 2. Format Plikow Konfiguracyjnych (JSON)

Pliki profilu znajduja sie w katalogu `servers/` obok pliku wykonywalnego gry.

### 2.1. Profil glowny serwera (`servers/pandora.json`)
```json
{
  "server_name": "pandora",
  "display_name": "PandoraMT2 (Crossplatform ARM64 / PC)",
  "host": "57.128.249.87",
  "fallback_host": "57.128.211.117",
  "auth_port": 13000,
  "channels": {
    "1": 13001,
    "2": 13002,
    "3": 13003,
    "4": 13004,
    "5": 13005,
    "6": 13006
  },

  "protocol": {
    "header_size_bytes": 1,
    "has_length_field_in_fixed_packets": false,
    "handshake_type": "ymir_13b",
    "key_agreement_enabled": false
  },

  "timings_and_limits": {
    "move_interval_ms": 250,
    "attack_crc_required": true,
    "pickup_interval_ms": 100,
    "max_move_speed": 1100,
    "interaction_max_range": 500
  },

  "cg_opcodes": {
    "login": 1,
    "attack": 2,
    "chat": 3,
    "character_create": 4,
    "character_delete": 5,
    "character_select": 6,
    "move": 7,
    "sync_position": 8,
    "enter_game": 10,
    "item_use": 11,
    "item_drop": 12,
    "item_move": 13,
    "item_pickup": 15,
    "quickslot_add": 16,
    "quickslot_del": 17,
    "quickslot_swap": 18,
    "whisper": 19,
    "on_click": 26,
    "exchange": 27,
    "script_answer": 29,
    "quest_input": 30,
    "quest_confirm": 31,
    "shop": 50,
    "fly_targeting": 51,
    "shoot": 53,
    "use_skill": 54,
    "target": 60,
    "key_agreement": 105
  },

  "gc_packets_file": "servers/pandora_gc.json",
  "asset_paths": {
    "maps_directory": "Maps",
    "locale_directory": "locale/pl"
  }
}
```

### 2.2. Rejestr pakietow serwera (`servers/pandora_gc.json`)
Plik generowany automatycznie z dekompilacji Ghidry (tablica skokow):
```json
{
  "1": { "name": "HEADER_GC_WARP", "size": 13, "is_dynamic": false },
  "2": { "name": "HEADER_GC_PHASE", "size": 2, "is_dynamic": false },
  "3": { "name": "HEADER_GC_NOTICE", "size": 3, "is_dynamic": true },
  "4": { "name": "HEADER_GC_CHAT", "size": 4, "is_dynamic": true },
  "5": { "name": "HEADER_GC_CHARACTER_ADD", "size": 67, "is_dynamic": false },
  "13": { "name": "HEADER_GC_SYNC_POSITION", "size": 9, "is_dynamic": false },
  "14": { "name": "HEADER_GC_LOGIN_SUCCESS", "size": 1, "is_dynamic": false },
  "16": { "name": "HEADER_GC_MAIN_CHARACTER", "size": 5, "is_dynamic": false },
  "17": { "name": "HEADER_GC_POINTS", "size": 65, "is_dynamic": false },
  "18": { "name": "HEADER_GC_POINT_CHANGE", "size": 9, "is_dynamic": false },
  "19": { "name": "HEADER_GC_ITEM_SET", "size": 42, "is_dynamic": false },
  "22": { "name": "HEADER_GC_ITEM_GROUND_ADD", "size": 15, "is_dynamic": false },
  "255": { "name": "HEADER_GC_HANDSHAKE", "size": 13, "is_dynamic": false }
}
```

---

## 3. Implementacja C++ Managera Konfiguracji w m2dev

Tworzymy lekki plik naglowkowy `src/UserInterface/ServerConfig.h`:

```cpp
#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>

struct ServerProfileConfig
{
    std::string server_name;
    std::string host;
    int auth_port = 13000;
    std::unordered_map<int, int> channels;

    // Protokol
    int header_size_bytes = 1;
    bool has_length_in_fixed = false;
    std::string handshake_type = "ymir_13b";

    // Czasy i limity
    uint32_t move_interval_ms = 250;
    bool attack_crc_required = true;
    uint32_t pickup_interval_ms = 100;
    uint32_t max_move_speed = 1100;

    // Opcody CG
    std::unordered_map<std::string, uint8_t> cg_opcodes;

    // Mapa rozmiarow GC: Opcode -> {Rozmiar, CzyDynamiczny}
    struct GcPacketInfo { uint32_t size; bool is_dynamic; };
    std::unordered_map<uint8_t, GcPacketInfo> gc_packets;

    uint8_t GetCG(const std::string& name, uint8_t defaultOpcode) const
    {
        auto it = cg_opcodes.find(name);
        return it != cg_opcodes.end() ? it->second : defaultOpcode;
    }
};

class CServerConfigManager
{
public:
    static CServerConfigManager& Instance()
    {
        static CServerConfigManager s_instance;
        return s_instance;
    }

    bool LoadServerProfile(const std::string& profileName);
    const ServerProfileConfig& GetCurrent() const { return m_currentProfile; }

private:
    ServerProfileConfig m_currentProfile;
};

#define g_ServerConfig CServerConfigManager::Instance().GetCurrent()
```

---

## 4. Dostosowanie Warstwy Sieciowej (Network Layer)

### 4.1. Dynamiczny `DispatchPacket` (odbiór pakietow z tabeli JSON)
W pliku `src/UserInterface/PythonNetworkStream.cpp` zamieniamy sztywna weryfikacje 4-bajtowego naglowka na odczyt z dynamicznego profilu:

```cpp
bool CPythonNetworkStream::DispatchPacket(const PacketHandlerMap& handlers)
{
    uint8_t header = 0;
    if (!Peek(sizeof(uint8_t), &header))
        return false;

    // Pomin zera / padding szyfrowania
    while (0 == header)
    {
        if (!Recv(sizeof(uint8_t), &header)) return false;
        if (!Peek(sizeof(uint8_t), &header)) return false;
    }

    // Pobierz informacje o rozmiarze pakietu z profilu serwera
    auto gcIt = g_ServerConfig.gc_packets.find(header);
    uint32_t uPacketSize = 0;

    if (gcIt != g_ServerConfig.gc_packets.end())
    {
        if (gcIt->second.is_dynamic)
        {
            // Pakiety dynamiczne: [header 1B][size 2B LE]
            struct TDyn { uint8_t h; uint16_t sz; };
            TDyn dyn;
            if (!Peek(sizeof(TDyn), &dyn)) return false;
            uPacketSize = dyn.sz;
        }
        else
        {
            uPacketSize = gcIt->second.size;
        }
    }
    else
    {
        TraceError("Unknown packet opcode: 0x%02X (%u) in Phase %s", header, header, m_strPhase.c_str());
        ClearRecvBuffer();
        return false;
    }

    // Oczekiwanie na pelna zawartosc pakietu w buforze
    if (m_recvBuf.ReadableBytes() < uPacketSize)
        return false;

    // Szukamy w zarejestrowanych handlerach
    auto it = handlers.find(header);
    if (it != handlers.end())
    {
        LogRecvPacket(header, uPacketSize);
        bool ret = (this->*(it->second.handler))();
        if (!ret || it->second.exitPhase) return false;
    }
    else
    {
        // Pakiet znany z profilu, ale nieobslugiwany w UI (Dummy Handler) -> bezpieczne pominiecie
        m_recvBuf.Skip(uPacketSize);
    }

    return true;
}
```

### 4.2. Uniwersalne wysylanie pakietow CG (`Send...`)
W funkcjach `SendCharacterStatePacket`, `SendAttackPacket`, `SendItemPickUpPacket` itp. uzywamy opcodow z konfiguracji:

```cpp
// Ruch postaci
kStatePacket.header = g_ServerConfig.GetCG("move", 7);

// Atak
kPacketAtk.header = g_ServerConfig.GetCG("attack", 2);

// Podnoszenie dropu
kItemPickUp.header = g_ServerConfig.GetCG("item_pickup", 15);

// Interakcja NPC
kOnClick.header = g_ServerConfig.GetCG("on_click", 26);
```

### 4.3. Zmienny Handshake i faza logowania
W zaleznosci od `g_ServerConfig.handshake_type`:
- Jesli `"ymir_13b"`: klient oczekuje 13B powitania (`0xFF`/`0xFD`), synchronizuje `ELTimer_SetServerMSec(dwTime + lDelta)`.
- Jesli `"m2dev_challenge"`: klient uruchamia procedury `KEY_CHALLENGE`/`KEY_RESPONSE`.

---

## 5. Dynamiczne Limity i Funkcje Rozgrywki (Gameplay)

### 5.1. Ruch (Movement & Anti-Speedhack)
W `PythonPlayerEventHandler.cpp`:
```cpp
void CPythonPlayerEventHandler::OnMoving(const SState& c_rkState)
{
    DWORD dwCurTime = ELTimer_GetMSec();
    if (m_dwNextMovingNotifyTime > dwCurTime)
        return;

    // Dynamiczny interwal z profilu serwera (np. 250 ms dla Pandory, 300 ms dla zwyklych)
    m_dwNextMovingNotifyTime = dwCurTime + g_ServerConfig.move_interval_ms;

    CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
    rkNetStream.SendCharacterStatePacket(c_rkState.kPPosSelf, c_rkState.fAdvRotSelf, CInstanceBase::FUNC_MOVE, 0);
}
```

### 5.2. Atak i walka wrecz (Combat & CRC)
W `SendAttackPacket`:
```cpp
bool CPythonNetworkStream::SendAttackPacket(UINT uMotAttack, DWORD dwVIDVictim)
{
    if (!__CanActMainInstance()) return true;

    TPacketCGAttack kPacketAtk;
    kPacketAtk.header = g_ServerConfig.GetCG("attack", 2);
    kPacketAtk.bType = uMotAttack;
    kPacketAtk.dwVictimVID = dwVIDVictim;

    if (g_ServerConfig.attack_crc_required)
        kPacketAtk.bCRCHash = (uint16_t)(14 + (ELTimer_GetMSec() % 4));
    else
        kPacketAtk.bCRCHash = 0;

    return Send(sizeof(kPacketAtk), &kPacketAtk);
}
```

### 5.3. Skille, Pickup, OnClick, Ekwipunek, Czat
- **Skille:** Opcode pobierany z `g_ServerConfig.GetCG("use_skill", 54)`.
- **Pickup:** Opcode z `g_ServerConfig.GetCG("item_pickup", 15)`.
- **Ekwipunek i Czat:** Logika manipulacji ikonami i oknem czatu pozostaje **w 100% nienaruszona**; jedyna zmiana to dynamiczny opcode.

---

## 6. Przeplyw Pracy (Workflow) – Dodanie Nowego Serwera w 5 Minut

Gdy pojawia sie nowy serwer:
1. **Zrzut binarnego pliku serwera:** Pobierasz `libmain.so` (Android) lub `.exe` (PC) serwera.
2. **Auto-analiza Ghidra:** Odpalasz skrypt ekstrakcji tablicy skokow (taki sam jak wygenerowal `E:\PandoraRepo\packets_gc_specification.json`).
3. **Utworzenie plikow JSON:**
   - Kopiujesz `servers/pandora.json` jako `servers/nowy_serwer.json`.
   - Zmieniasz IP, porty oraz zrzut rozmiarow pakietow.
4. **Start gry:**
   Uruchamiasz:
   ```cmd
   Metin2Client.exe --server nowy_serwer
   ```
   **Klient od razu laczy sie z nowym serwerem bez koniecznosci kompilacji kodu C++!**

---

## 7. Checklista Wdrozeniowa Krok po Kroku

- [ ] **Krok 1:** Dodanie klasy `CServerConfigManager` i struktury `ServerProfileConfig` w `src/UserInterface/ServerConfig.h`.
- [ ] **Krok 2:** Wczytanie profilu z JSON w `PythonApplication.cpp` na podstawie parametru wiersza polecen (`--server <nazwa>`).
- [ ] **Krok 3:** Przebudowa `CPythonNetworkStream::DispatchPacket` na czytanie rozmiarow z mapy `gc_packets`.
- [ ] **Krok 4:** Zamiana sztywnych opcodow `CG::` w metodach `Send...` na wywolania `g_ServerConfig.GetCG(...)`.
- [ ] **Krok 5:** Parametryzacja stalej interwalu ruchu `300` na `g_ServerConfig.move_interval_ms` w `PythonPlayerEventHandler.cpp`.
- [ ] **Krok 6:** Obsluga sumy kontrolnej w `SendAttackPacket` sterowana flaga `attack_crc_required`.
- [ ] **Krok 7:** Stworzenie folderu `servers/` i umieszczenie w nim `pandora.json` oraz `pandora_gc.json`.
- [ ] **Krok 8:** Jednorazowa kompilacja `Metin2Client.exe` w Visual Studio.

Gotowe! Posiadasz uniwersalnego klienta, ktorego mozesz podlaczyc pod dowolny serwer gry Metin2 za pomoca prostych plikow konfiguracyjnych.
