# System Profili Wieloserwerowych (Multi-Server Profile System & UPM)

## Spis Tresci
1. [Wprowadzenie i Koncepcja Architektury UPM](#1-wprowadzenie-i-koncepcja-architektury-upm)
2. [Klasa ProtocolDriverRegistry: Rejestr Sterownikow](#2-klasa-protocoldriverregistry-rejestr-sterownikow)
3. [4 Wbudowane Sterowniki Protokolow](#3-4-wbudowane-sterowniki-protokolow)
4. [Format Plikow Konfiguracyjnych JSON (Analiza servers/beavium.json)](#4-format-plikow-konfiguracyjnych-json-analiza-serversbeaviumjson)
5. [Integracja z Warstwa Klienta: UI, FastBoot i CPythonNetworkStream](#5-integracja-z-warstwa-klienta-ui-fastboot-i-cpythonnetworkstream)
6. [Diagram Architektury Wieloserwerowej (Mermaid)](#6-diagram-architektury-wieloserwerowej-mermaid)
7. [Powiazania Kodowe i Referencje](#7-powiazania-kodowe-i-referencje)

---

## 1. Wprowadzenie i Koncepcja Architektury UPM

W standardowych wersjach klienta Metin2 calosc protokolu sieciowego (naglowki, rozmiary struktur, szyfrowanie) byla sztywno wkompilowana w kod binarny. Zmiana docelowego serwera (np. przejscie z serwera produkcyjnego Luria na wlasny serwer deweloperski Standard x64 lub MartySama) wymagala dotad osobnych kompilacji, modyfikacji kodu zrodlowego oraz odmiennych paczek z plikami `.exe`.

W nowo zaprojektowanej architekturze klienta x64 wprowadzono **Universal Protocol Module (UPM)**. Jest to wieloserwerowy system profili, w ktorym pojedynczy plik wykonywalny potrafi w czasie rzeczywistym (w runtime) dostosowac:
- Sposob ramkowania bufora TCP (`InspectFrame`).
- Algorytm kryptograficzny (Sodium, Blowfish CTR, TEA).
- Format kodowania pakietow akcji gracza (`EncodeAttack`, `EncodeMove`, `EncodeItemUse`, `EncodeTarget`, `EncodeLogin`).
- Dyspozycje pakietow przychodzacych do odbiornikow zdarzen (`DispatchInbound`).

---

## 2. Klasa ProtocolDriverRegistry: Rejestr Sterownikow

Sercem zarzadzania profilami jest klasa [ProtocolDriverRegistry](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/ProtocolDriverRegistry.h), zaimplementowana wedlug wzorca Singleton oraz Registry:

```cpp
class ProtocolDriverRegistry
{
public:
    static ProtocolDriverRegistry& Instance() noexcept;

    void RegisterDriver(std::string_view name, std::unique_ptr<IServerProtocolDriver> pDriver);
    bool SetActiveDriver(std::string_view name);
    [[nodiscard]] IServerProtocolDriver* GetActiveDriver() const noexcept;
    [[nodiscard]] std::string GetActiveDriverName() const;
    [[nodiscard]] IServerProtocolDriver* GetDriver(std::string_view name) const noexcept;
    [[nodiscard]] bool HasDriver(std::string_view name) const noexcept;
    void InitializeDefaults();
    void SetOnDriverChanged(std::function<void(std::string_view)> callback);
    void Clear() noexcept;
};
```

### Mechanizmy Kluczowe:
- **Thread-Safety**: Wszelkie operacje wyszukiwania, rejestracji i zmiany aktywnego sterownika sa chronione muteksem `mutable std::mutex m_mutex`.
- **Zdarzenie OnDriverChanged**: Rejestr umozliwia subskrypcje zmiany aktywnego profilu. Gdy wywolane zostanie `SetActiveDriver("luria")`, zarejestrowany callback automatycznie powiadamia strumien sieciowy `CPythonNetworkStream`, ktory w locie przeladowuje tablice handlerow (`RefreshProtocolHandlers()`).
- **Leniwa Inicjalizacja Domyslna**: Metoda `InitializeDefaults()` automatycznie rejestruje 4 wbudowane sterowniki i jako domyslny profil wybiera `"standard_x64"`.

---

## 3. 4 Wbudowane Sterowniki Protokolow

Klient posiada cztery dedykowane sterowniki dziedziczace po interfejsie [IServerProtocolDriver](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/IServerProtocolDriver.h):

### 3.1 Standard x64 (`standard_x64`)
Pliki: [StandardX64ProtocolDriver.h](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/Drivers/StandardX64ProtocolDriver.h), [StandardX64ProtocolDriver.cpp](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/Drivers/StandardX64ProtocolDriver.cpp)
- **Przeznaczenie**: Nowoczesny serwer docelowy x64 (Clean Architecture / 2026).
- **Ramkowanie**: 4 bajty zunifikowanego naglowka `[Header:2][Length:2]`.
- **Format pakietow**: `TPacketCGAttack` (11B), `TPacketCGMove` (19B).
- **Zaleta**: Brak koniecznosci statycznych tabel rozmiarow, calkowita ochrona przed desynchronizacja bufora.

### 3.2 MartySama (`martysama`)
Pliki: [MartySamaProtocolDriver.h](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/Drivers/MartySamaProtocolDriver.h), [MartySamaProtocolDriver.cpp](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/Drivers/MartySamaProtocolDriver.cpp)
- **Przeznaczenie**: Serwery oparte na popularnej bazie zrodel MartySama v5.x.
- **Ramkowanie**: Klasyczne 1-bajtowe naglowki Ymir.
- **Format pakietow**: `TPacketCGAttackMarty` (8B), `TPacketCGMoveMarty` (16B), `TPacketCGTargetMarty` (5B).
- **Interwaly**: `GetMinAttackIntervalMs() = 250ms`, `GetMinActionIntervalMs() = 100ms`.

### 3.3 Luria (`luria`)
Pliki: [LuriaProtocolDriver.h](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/Drivers/LuriaProtocolDriver.h), [LuriaProtocolDriver.cpp](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/Drivers/LuriaProtocolDriver.cpp)
- **Przeznaczenie**: Zewnetrzny serwer produkcyjny Luria (IP: 46.105.73.82).
- **Kryptografia**: Wymiana kluczy DH2 w fazie Handshake (`LuriaDH2.cpp`), nastepnie aktywacja `BlowfishCryptoProvider` w trybie CTR.
- **Tablica pakietow**: 139 dedykowanych wpisow w `luria_gc_table.inl`. Wymaga autoryzacji 4-cyfrowym kodem PIN.

### 3.4 Beavium (`beavium`)
Pliki: [BeaviumProtocolDriver.h](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/Drivers/BeaviumProtocolDriver.h), [BeaviumProtocolDriver.cpp](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/Drivers/BeaviumProtocolDriver.cpp)
- **Przeznaczenie**: Nowoczesny serwer oparty na mikrouslugach i kryptografii Sodium.
- **Kryptografia**: `SodiumCryptoProvider` (libsodium X25519 + XChaCha20).
- **Tablica pakietow**: 48 wpisow w `beavium_gc_table.inl`.
- **Obsluga specjalna**: Dedykowane dynamiczne pakiety dla rankingu, pet info oraz czatu glosowego.

---

## 4. Format Plikow Konfiguracyjnych JSON (Analiza servers/beavium.json)

Poza wbudowanymi klasami C++, klient umozliwia definiowanie konfiguracji polaczen serwerowych za pomoca plikow JSON w katalogu `servers/`.

Rzeczywista zawartosc pliku [servers/beavium.json](file:///E:/m2dev-client-src-mainOryginalx64/servers/beavium.json):

```json
{
  "name": "Beavium",
  "version": "1.0.0",
  "driver": "beavium",
  "host": "127.0.0.1",
  "port": 11000,
  "auth": {
    "ip": "127.0.0.1",
    "port": 11000
  },
  "channels": [
    {
      "id": 1,
      "name": "CH1",
      "ip": "127.0.0.1",
      "port": 11011
    },
    {
      "id": 2,
      "name": "CH2",
      "ip": "127.0.0.1",
      "port": 11021
    }
  ],
  "protocol": {
    "framing": "Ymir1B",
    "headerSizeBytes": 1,
    "crypto": "Sodium",
    "keySize": 32,
    "requireCrcPiece": true,
    "magicByte": 255
  },
  "timing": {
    "moveIntervalMs": 150,
    "attackIntervalMs": 380,
    "pickupIntervalMs": 200,
    "pingIntervalMs": 5000,
    "jitterStdDevMs": 15
  },
  "opcodes": {
    "HANDSHAKE": 255,
    "TIME_SYNC": 255,
    "PHASE": 253,
    "PONG": 252,
    "ATTACK": 2,
    "ATTACK_ALT": 29,
    "MOVE": 7,
    "USE_ITEM": 12,
    "DROP_ITEM": 20,
    "PICKUP_ITEM": 14,
    "CHAT": 3,
    "WHISPER": 25,
    "EXCHANGE": 31,
    "SHOP": 27,
    "TARGET": 34,
    "USE_SKILL": 52,
    "SELECT_EMPIRE": 53,
    "KEY_AGREEMENT": 250
  }
}
```

### Wyjasnienie Sekcji Konfiguracyjnej:
- `"driver"`: Wskazuje nazwe sterownika zarejestrowanego w `ProtocolDriverRegistry` (`"beavium"`).
- `"auth"` oraz `"channels"`: Definiuja topologie serwera (adres IP serwera logowania oraz porty kanalow gry CH1, CH2...).
- `"protocol"`: Parametry ramkowania (`framing`: "Ymir1B", `headerSizeBytes`: 1, `crypto`: "Sodium", `keySize`: 32, `requireCrcPiece`: flaga CRC).
- `"timing"`: Zabezpieczenia po stronie klienta modelujace odstepy czasowe pakietow (Anti-Flood / Anti-Spam), w tym odstep ruchu (150ms), ataku (380ms) oraz pingu (5000ms).
- `"opcodes"`: Numeryczne mapowania opkodow uzywane przez adaptery translacji.

---

## 5. Integracja z Warstwa Klienta: UI, FastBoot i CPythonNetworkStream

Integracja systemu profili obejmuje trzy kluczowe warstwy aplikacji:

### 5.1 Reakcja Strumienia Sieciowego (`CPythonNetworkStream`)
W konstruktorze strumienia [PythonNetworkStream.cpp](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/PythonNetworkStream.cpp#L817-L824):
```cpp
RefreshProtocolHandlers();
Network::Protocol::ProtocolDriverRegistry::Instance().SetOnDriverChanged(
    [this](std::string_view /*name*/) {
        RefreshProtocolHandlers();
    }
);
```
Za kazdym razem, gdy uzytkownik lub nakladka zmieni sterownik, tablice `m_offlineHandlers`, `m_handshakeHandlers`, `m_loginHandlers`, `m_selectHandlers`, `m_loadingHandlers` oraz `m_gameHandlers` sa natychmiastowo przebudowywane bez koniecznosci restartowania aplikacji.

### 5.2 Nakladka Szybkiego Logowania Fast-Boot (`FastBootOverlay`)
Plik zrodlowy: [FastBootOverlay.cpp](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/FastBootOverlay.cpp#L32-L43)
Klasa udostepnia w interfejsie graficznym Dear ImGui liste predefiniowanych serwerow:
- `"Luria - CH4 [Zalecany]"` -> aktywuje `"luria"`, laczy z `46.105.73.82:29301` (Game) i `11469` (Auth).
- `"Standard x64 - CH1"` -> aktywuje `"standard_x64"`, laczy z `127.0.0.1:11011` i `11000`.
- `"Beavium - CH1"` -> aktywuje `"beavium"`, laczy z `127.0.0.1:11011` i `11000`.

Po kliknieciu przycisku "Polacz", nakladka wywoluje:
```cpp
Network::Protocol::ProtocolDriverRegistry::Instance().SetActiveDriver(prof.driverName);
rkNetStream.RefreshProtocolHandlers();
rkAccountConnector.Connect(prof.serverIp.c_str(), prof.gamePort, prof.serverIp.c_str(), prof.authPort);
```

### 5.3 Warstwa Skryptowa Pythona
Modul `PythonNetworkStreamModule.cpp` eksportuje metody do srodowiska wykonawczego Pythona, co pozwala skryptom interfejsu logowania dynamicznie sprawdzac i ustawiac profil sterownika na podstawie wybranego w menu serwera.

---

## 6. Diagram Architektury Wieloserwerowej (Mermaid)

```mermaid
flowchart TD
    subgraph UI ["Warstwa Prezentacji (UI)"]
        FBO["FastBootOverlay (Dear ImGui DX9)"]
        PyUI["Python UI (introLogin.py)"]
    end

    subgraph Registry ["ProtocolDriverRegistry (Singleton)"]
        PDR["ProtocolDriverRegistry"]
        ActivePtr["m_pActiveDriver"]
        DriverMap["m_drivers map"]
    end

    subgraph Drivers ["Zarejestrowane Sterowniki (IServerProtocolDriver)"]
        D1["StandardX64ProtocolDriver"]
        D2["MartySamaProtocolDriver"]
        D3["LuriaProtocolDriver"]
        D4["BeaviumProtocolDriver"]
    end

    subgraph NetStream ["Silnik Sieciowy (CNetworkStream / CPythonNetworkStream)"]
        CPNS["CPythonNetworkStream"]
        Tables["Tablice Handlerow (m_gameHandlers itp.)"]
        Crypto["INetCryptoProvider (Blowfish / Sodium / TEA)"]
    end

    FBO -->|SetActiveDriver(name)| PDR
    PyUI -->|SetActiveDriver(name)| PDR
    PDR --> DriverMap
    DriverMap --> D1
    DriverMap --> D2
    DriverMap --> D3
    DriverMap --> D4
    PDR --> ActivePtr
    PDR -->|OnDriverChanged Callback| CPNS
    CPNS -->|RefreshProtocolHandlers()| Tables
    D3 -->|Wstrzykuje Blowfish CTR| Crypto
    D4 -->|Wstrzykuje Sodium XChaCha20| Crypto
```

---

## 7. Powiazania Kodowe i Referencje

- Rejestr sterownikow naglowek: [ProtocolDriverRegistry.h](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/ProtocolDriverRegistry.h)
- Rejestr sterownikow implementacja: [ProtocolDriverRegistry.cpp](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/ProtocolDriverRegistry.cpp)
- Czysty interfejs sterownika: [IServerProtocolDriver.h](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/IServerProtocolDriver.h)
- Sterownik Standard x64: [StandardX64ProtocolDriver.h](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/Drivers/StandardX64ProtocolDriver.h)
- Sterownik MartySama: [MartySamaProtocolDriver.h](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/Drivers/MartySamaProtocolDriver.h)
- Sterownik Luria: [LuriaProtocolDriver.h](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/Drivers/LuriaProtocolDriver.h)
- Sterownik Beavium: [BeaviumProtocolDriver.h](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/Drivers/BeaviumProtocolDriver.h)
- Plik konfiguracji profilu Beavium: [servers/beavium.json](file:///E:/m2dev-client-src-mainOryginalx64/servers/beavium.json)
- Kontroler nakladki FastBoot: [FastBootOverlay.cpp](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/FastBootOverlay.cpp)
- Integracja ze strumieniem sieciowym: [PythonNetworkStream.cpp](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/PythonNetworkStream.cpp#L817-L860)
