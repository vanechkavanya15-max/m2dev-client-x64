# ULTIMATE PLAN: Uniwersalna Macierz Protokolow (Universal Protocol Matrix - UPM)

> **Status Dokumentu**: ZATWIERDZONY DO REALIZACJI  
> **Cel**: Eliminacja architektury hybrydowej (Frankenstein Architecture), unifikacja ramkowania TCP (1B vs 4B), separacja domeny od enkapsulacji bajtowej oraz pelne wsparcie multi-serwerowe dla Agentow AI (Antigravity & Jules Swarm).

---

## 1. Krytyczna Diagnoza Inzynierska: Architektura Hybrydowa ("Frankenstein")

Pomimo osiagniecia **32/32 CTest PASS** w srodowisku mockowym, w glebi kodu klienta istnialo krytyczne pekniecie architektoniczne pomiedzy kodem nadawczym (**CG - Client to Game**) a kodem odbiorczym (**GC - Game to Client**):

```
                       ┌────────────────────────────────────────────────────────┐
                       │          KOD NADAWCZY (CG - Client to Game)            │
                       │          (Zhardkodowany pod serwer BEAVIUM)            │
                       │                                                        │
                       │  • SendMoveHandler: Beavium::TPacketCGMoveBeavium (24B)│
                       │    Naglowek 0x07, kat w mikrostopniach (* 1e6)         │
                       │  • SendAttackHandler: ProxyPacketCGAttack (8B)         │
                       │    Naglowek 0x02, sekwencja 16-bit                     │
                       │  • ShopPacketCodec: TPacketCGShopBuyBeavium            │
                       └───────────────────────────┬────────────────────────────┘
                                                   │
                                                   ▼ ROZDWOJENIE JAZNI!
                                                   ▲
                       ┌───────────────────────────┴────────────────────────────┐
                       │          KOD ODBIORCZY (GC - Game to Client)           │
                       │          (Zhardkodowany pod standard x64 2026)         │
                       │                                                        │
                       │  • PythonNetworkStream::DispatchPacket zaklada:        │
                       │    TDynamicSizePacketHeader: [Header:2][Length:2] (4B) │
                       │  • Serwer Beavium wysyla: [Header:1] (1B) + Lookup     │
                       │    w tablicy beavium_gc_table.inl!                     │
                       └────────────────────────────────────────────────────────┘
```

### Konsekwencje w srodowisku produkcyjnym:
1. **Podlaczenie pod prawdziwy serwer Beavium**: Klient proboje odczytac 4 bajty naglowka w `DispatchPacket` zamiast 1 bajtu naglowka -> **natychmiastowe przesuniecie bufora TCP o 3 bajty i rozlaczenie sesji (Unknown Packet Header)**.
2. **Podlaczenie pod serwer standardowy x64 2026**: Klient wysyla pakiet ruchu Beavium (24 bajty z mikrostopniami) zamiast standardowego pakietu ruchu (19 bajtow) -> **serwer odrzuca pakiet jako exploit i rozlacza gracza**.
3. **Pulapka poznawcza dla Agentow AI**:
   - `src/UserInterface/beavium_gc_table.inl` oraz `src/Client/Network/Protocol/beavium_gc_table.inl` to dwie zduplikowane, identyczne kopie.
   - `src/UserInterface/BeaviumProtocol.h` to sztuczna wydmuszka inkludujaca wersje z `Client/`.
   - Brak jednego zrodla prawdy (Single Source of Truth) powoduje, ze agenci AI gubia sie w edycjach.

---

## 2. Docelowa Architektura: Uniwersalna Macierz Protokolow (UPM)

Wprowadzamy czysty, 3-warstwowy podzial Hexagonal Architecture (Ports and Adapters):

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      WARSTWA 1: CZYSTA DOMENA SILNIKA                  │
│                     (100% Wolna od Naglowkow i Bajtow)                  │
│                                                                         │
│   src/Client/Network/Domain/                                            │
│   ├── CombatCommands.h    -> AttackCommand { attacker, victim, type }   │
│   ├── MovementCommands.h  -> MoveCommand   { vid, x, y, rotDegrees }    │
│   └── ItemCommands.h      -> ItemUseCommand{ window, cell }             │
└────────────────────────────────────┬────────────────────────────────────┘
                                     │
                                     ▼ (Wywolanie czystego interfejsu)
┌─────────────────────────────────────────────────────────────────────────┐
│               WARSTWA 2: STEROWNIK PROTOKOLU (DRIVER PORT)              │
│                                                                         │
│   src/Client/Network/Protocol/                                          │
│   ├── IServerProtocolDriver.h   -> Czysty interfejs sterownika serwera  │
│   └── ProtocolDriverRegistry.h  -> Centralna fabryka sterownikow        │
└──────────────────┬──────────────────────────────────┬───────────────────┘
                   │                                  │
    Wybor profilu  ▼                                  ▼ Wybor profilu
┌─────────────────────────────────────┐  ┌────────────────────────────────┐
│   STEROWNIK A: StandardX64Driver    │  │   STEROWNIK B: BeaviumDriver   │
│   (Standard Metin2 x64 2026)        │  │   (Serwer Beavium i Pokrewne)  │
│                                     │  │                                │
│   • Framing: 4B [Header:2][Len:2]   │  │   • Framing: 1B + Table Lookup │
│   • Opcodes: 0x0401, 0x0511         │  │   • Opcodes: 0x02, 0x07, 0x0C  │
│   • Atak: 11B (x64 Uniform)         │  │   • Atak: 8B (Seq + Proxy)     │
│   • Ruch: 19B (Stopnie)             │  │   • Ruch: 24B (Mikrostopnie)   │
│   • Krypto: Brak / Standard         │  │   • Krypto: AES-256-CTR / IV   │
└──────────────────┬──────────────────┘  └────────────────┬───────────────┘
                   │                                      │
                   └──────────────────┬───────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────┐
│              WARSTWA 3: STRUMIEN SIECIOWY (CPythonNetworkStream)        │
│                                                                         │
│   • Dynamiczne ramkowanie: m_pActiveDriver->InspectFrame(tcpBuffer)     │
│   • Przelaczanie serwera: Stream.SetServerDriver("beavium" | "x64")     │
│   • 0 zahardkodowanych numerow opkodow w petli sieciowej                │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Plan Realizacji Krok po Kroku

### Krok 1: Likwidacja Duplikatow i Oczyszczenie Drzewa Plikow (Cleanup)
1. Usuniecie martwej kopii: `src/UserInterface/beavium_gc_table.inl`.
2. Jedynym zrodlem prawdy dla tabeli Beavium staje sie: `src/Client/Network/Protocol/beavium_gc_table.inl`.
3. Usuniecie wydmuszki `src/UserInterface/BeaviumProtocol.h` i skierowanie wszystkich referencji na `Client/Network/Protocol/BeaviumProtocol.h`.

### Krok 2: Kanoniczna Warstwa Domeny DTO (`src/Client/Network/Domain/`)
Utworzenie struktur czystej domeny (POD), bez bajtowych naglowkow, bez pragma pack, bez zaleznosci od protokolu:
- `src/Client/Network/Domain/CombatCommands.h`:
  - `AttackCommand { attackerVid, victimVid, attackType, sequence, attackMotion }`
  - `DamageInfoEvent { victimVid, damage, flag }`
- `src/Client/Network/Domain/MovementCommands.h`:
  - `MoveCommand { vid, x, y, rotationDegrees, time, func, arg }`
- `src/Client/Network/Domain/ItemCommands.h`:
  - `ItemUseCommand { window, cell }`
  - `ItemDropCommand { window, cell, count }`

### Krok 3: Definicja Interfejsu `IServerProtocolDriver`
Plik: `src/Client/Network/Protocol/IServerProtocolDriver.h`:
```cpp
namespace Network::Protocol
{
    struct FrameHeaderInfo
    {
        uint16_t unifiedOpcode; ///< Zunifikowany naglowek wewnetrzny
        uint32_t packetLength;  ///< Calkowity rozmiar pakietu do odebrania z bufora TCP
        uint32_t headerSize;    ///< Rozmiar naglowka (1B dla Beavium/Legacy, 4B dla x64)
    };

    class IServerProtocolDriver
    {
    public:
        virtual ~IServerProtocolDriver() = default;

        [[nodiscard]] virtual std::string_view GetDriverName() const noexcept = 0;

        /// @brief Bada bufor TCP i zwraca naglowek oraz dlugosc pakietu, lub nullopt jesli potrzeba wiecej danych
        [[nodiscard]] virtual std::optional<FrameHeaderInfo> InspectFrame(
            std::span<const uint8_t> buffer) const noexcept = 0;

        /// @brief Kodowanie czystych komend domenowych do bajtow sieciowych
        [[nodiscard]] virtual EterBase::PacketResult<std::vector<uint8_t>> EncodeAttack(
            const Domain::AttackCommand& cmd) const = 0;

        [[nodiscard]] virtual EterBase::PacketResult<std::vector<uint8_t>> EncodeMove(
            const Domain::MoveCommand& cmd) const = 0;

        /// @brief Dyspozycja zdeserializowanego pakietu do magistrali zdarzen
        virtual bool DispatchInbound(
            uint16_t unifiedOpcode,
            std::span<const uint8_t> payload,
            UserInterface::Contracts::IGameEventSink* pSink) = 0;
    };
}
```

### Krok 4: Wdrozenie Dwoch Dedykowanych Sterownikow
1. **`StandardX64ProtocolDriver`**:
   - `InspectFrame`: bada pierwsze 4 bajty (`[header:2][length:2]`).
   - `EncodeAttack`: produkuje standardowy pakiet `TPacketCGAttack` (11 bajtow).
   - `EncodeMove`: produkuje standardowy pakiet `TPacketCGMove` (19 bajtow).
   - `DispatchInbound`: deleguje bezposrednio do `PhaseGamePacketDispatcher`.
2. **`BeaviumProtocolDriver`**:
   - `InspectFrame`: bada 1 bajt `header` i odczytuje dokladny rozmiar z tabeli `beavium_gc_table.inl`.
   - `EncodeAttack`: produkuje `ProxyPacketCGAttack` (8 bajtow) z naglowkiem `0x02`.
   - `EncodeMove`: produkuje `Beavium::TPacketCGMoveBeavium` (24 bajty) z mikrostopniami i naglowkiem `0x07`.
   - `DispatchInbound`: mapuje naglowki 1-bajtowe na zunifikowane opkody i emituje zdarzenia domenowe.

### Krok 5: Refaktoryzacja Handlerow Nadawczych (`SendAttackHandler`, `SendMoveHandler`)
Handlery przestaja dotykac struktur Beavium bezposrednio:
```cpp
// Przyklad SendAttackHandler.cpp:
bool SendAttackHandler::SendAttack(uint32_t targetId, uint32_t attackMotion, uint16_t sequence, CNetworkStream* networkStream)
{
    auto* pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
    Network::Domain::AttackCommand cmd{
        .targetVid = targetId,
        .sequence = sequence,
        .attackMotion = attackMotion
    };
    auto encoded = pDriver->EncodeAttack(cmd);
    if (!encoded.has_value()) return false;
    return networkStream->Send(static_cast<int>(encoded->size()), encoded->data());
}
```
**Rezultat**: Zero zahardkodowanych naglowkow! Ten sam kod bezblednie obsluguje dowolny serwer.

### Krok 6: Uniwersalizacja `CPythonNetworkStream::DispatchPacket`
W petli sieciowej klienta zastepujemy sztywne `Peek(sizeof(TDynamicSizePacketHeader))` wywolaniem:
```cpp
auto frameOpt = m_pActiveDriver->InspectFrame(peekSpan);
if (!frameOpt.has_value())
    return false; // Czekamy na wiecej bajtow w buforze TCP

if (!Peek(frameOpt->packetLength))
    return false;

std::vector<uint8_t> buffer(frameOpt->packetLength);
if (!Recv(frameOpt->packetLength, buffer.data()))
    return false;

m_pActiveDriver->DispatchInbound(frameOpt->unifiedOpcode, buffer, m_pEventSink);
```

### Krok 7: Zestaw Testow Multi-Server (33/33 CTest PASS)
Plik: `tests/test_c26_multi_server_protocol.cpp`:
1. **Test A (StandardX64Driver)**: Weryfikacja ramkowania 4B, serializacji 19B/11B.
2. **Test B (BeaviumDriver)**: Weryfikacja ramkowania 1B + lookup w tabeli, serializacji 24B/8B.
3. **Test C (Dynamic Switching)**: Przelaczenie sterownika w runtime w trakcie symulowanej sesji bez desynchronizacji bufora.

---

## 4. Dlaczego To Bedzie Cudo dla Agentow AI (Jules Swarm & Antigravity)

Gdy w przyszlosci pojawi sie wymog:
> *"Dodaj obsluge serwera 'Alune' z wlasnymi opkodami i wlasna struktura pakietu ruchu"*

Agent AI nie bedzie musial analizowac 50 plikow C++ ani modyfikowac `CPythonNetworkStream`. Agent AI wykona dokladnie **dwa bezpieczne kroki**:
1. Stworzy jeden plik: `src/Client/Network/Protocol/Drivers/AluneProtocolDriver.cpp` implementujacy `IServerProtocolDriver`.
2. Zarejestruje go w fabryce: `ProtocolDriverRegistry::Instance().RegisterDriver("alune", std::make_unique<AluneProtocolDriver>());`.

**Zero konfliktow git merge, zero regresji w innych profilach serwerowych, 100% czystej inzynierii oprogramowania.**
