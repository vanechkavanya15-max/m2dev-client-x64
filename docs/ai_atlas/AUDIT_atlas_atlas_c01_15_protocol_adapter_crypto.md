---
task_id: "atlas_c01_15_protocol_adapter_crypto"
cluster: "NET"
module_name: "Kryptografia Protokolu i Dynamiczny Framer"
target_files:
- src/Client/Network/ProtocolAdapter.h
- src/Client/Network/DynamicPacketFramer.h
- src/Client/Network/TeaCryptoProvider.h
- src/Client/Network/AesCryptoProvider.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c01_15_protocol_adapter_crypto.md"
architecture_layer: "Silnik Sieciowy, Maszyna Faz i Protokol"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Modul jest odpowiedzialny za obsluge warstwy sieciowej klienta Metin2, dostarczajac abstrakcje nad kryptografia (TEA, AES), framingiem pakietow (DynamicPacketFramer) oraz tlumaczeniem opcode'ow (ProtocolAdapter) zaleznym od profilu serwera (np. ClassicYmir, Pandora).
- **Zastosowanie w petli gry:** Kod jest uzywany podczas odbierania (Network Tick / OnReceive) i wysylania (OnSend) pakietow. Framer akumuluje fragmenty danych i wydziela cale pakiety z uwzglednieniem roznych konwencji ramkowania (np. Ymir1B, M2Dev4B).
- **Przeplyw danych (Data Flow):** 
  1. Surowe dane sa wczytywane z socketa.
  2. Strumien przechodzi przez providera kryptograficznego (ICryptoProvider -> ProcessStream dla AES lub blokowo dla TEA), gdzie dane sa deszyfrowane w miejscu (in-place).
  3. Odszyfrowany bufor trafia do `DynamicPacketFramer::AppendData()`.
  4. Klient odpytuje `FrameNextPacket()` aby uzyskac kompletny pakiet (std::span).
  5. Pobrany pakiet zyskuje mapowany opcode (przez `ProtocolAdapter::TranslateServerToClient`).
- **Cykl zycia obiektow:** 
  - `ProtocolAdapter` jest najprawdopodobniej tworzony raz (lub raz na sesje serwera) i konfiguruje profil serwera.
  - `DynamicPacketFramer` to trwala warstwa buforowania strumienia. Resetowany (`Clear()`) glownie podczas rozlaczenia lub resetu polaczenia (desynchronizacji).
  - Obiekty typu `ICryptoProvider` (np. `AesCryptoProvider`, `TeaCryptoProvider`) sa najpewniej powiazane z czasem zycia polaczenia, i powinny byc resetowane, aby uniknac desynchronizacji klucza/licznika.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):**
  - Menedzery polaczen sieciowych (np. `CPythonNetworkStream` lub pochodne nowej architektury).
  - Klasy decydujace o profilu polaczenia (logowanie, wybor postaci).
- **Zaleznosci wyjsciowe (Outbound):**
  - `EterBase::Result`, `EterBase::PacketResult`, `EterBase::VoidResult` do obslugi bledow bez wyjatkow.
  - Opcody zdefiniowane w `Protocol/Protocol.h` i stale z `Protocol/BeaviumProtocol.h` (w przypadku AES).
- **Drzewo dyrektyw `#include`:**
  - `<EterBase/Result.h>`, `<expected>`, `<span>`, `<vector>`, `<array>`, `<unordered_map>`, `<cstdint>`, `<string_view>`, `"ICryptoProvider.h"`, `"Protocol/Protocol.h"`, `"Protocol/BeaviumProtocol.h"`.
  - Zagrozenie cyklicznymi zaleznosciami jest znikome, w wiekszosci wlasne wyizolowane pliki naglowkowe z minimalna inkluzja zewnetrzna.
- **Model pamieciowy:**
  - `DynamicPacketFramer` uzywa `std::vector` jako glownego bufora.
  - Zwracane sa lekkie widoki `std::span` zamiast kopii buforow.
  - `ICryptoProvider` moga byc tworzone za pomoca `std::unique_ptr` (jak factory w `CreateAesCryptoProvider`).
  - Szyfrowanie TEA dziala blokowo na spanach, AES szyfruje w miejscu dzialajac na `uint8_t*`.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
| Nazwa | Rola |
|---|---|
| `ProtocolAdapter` | Adapter konwertujacy opcody pomiedzy klientem a danym serwerem (ClassicYmir, Pandora). |
| `DynamicPacketFramer` | Framer laczacy fragmenty pakietow TCP w cale pakiety na podstawie trybow (Ymir1B, M2Dev4B). |
| `ICryptoProvider` | Interfejs bazowy do strumieniowego przetwarzania danych (szyfrowanie/deszyfrowanie w-miejscu). |
| `TeaCryptoProvider` | Klasa dostarczajaca szyfrowanie/deszyfrowanie algorytmem TEA na podstawie blokow 8B. |
| `AesCryptoProvider` | Klasa dostarczajaca szyfrowanie strumieniowe AES-CTR bez uszkadzania naglowkow pakietow. |
| `ServerProfile` (Enum) | Definiuje logike mapowania pakietow (ClassicYmir = 0, Pandora = 1). |
| `FramingMode` (Enum) | Definiuje sposob ramkowania (Ymir1B, M2Dev4B). |

**Tabela Metod Publicznych:**
- `ProtocolAdapter::TranslateClientToServer(uint16_t)`: Tlumaczy opcode klienta na opcode serwera (zwraca `PacketResult<uint16_t>`).
- `ProtocolAdapter::TranslateServerToClient(uint16_t)`: Tlumaczy opcode serwera na opcode klienta (zwraca `PacketResult<uint16_t>`).
- `DynamicPacketFramer::AppendData(std::span<const uint8_t>)`: Akumuluje bajty z socketa.
- `DynamicPacketFramer::FrameNextPacket()`: Probuje zwrocic kolejny pelen pakiet (jako `std::span`).
- `TeaCryptoProvider::Initialize(std::span<const uint8_t>)`: Inicjalizuje TEA z 16-bajtowem kluczem.
- `TeaCryptoProvider::EncryptBlock/DecryptBlock`: Przetwarzaja w-miejscu 8-bajtowy blok uzywajac TEA.
- `AesCryptoProvider::ProcessStream(uint8_t*, size_t)`: XOR-uje podany bufor na podstawie wczesniej przygotowanego klucza i wektora AES-CTR (strumieniowe, dziala zarowno na enc/dec).

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe (Opcody zdefiniowane/mapowane dla Pandora):**
  - CG: `ATTACK` (0x02), `USE_SKILL` (0x36), `ITEM_PICKUP` (0x0F), `ON_CLICK` (0x1A), `SHOP` (0x32), `SCRIPT_ANSWER` (0x1D), `LOGIN2`/`LOGIN3` (0x01), `MOVE` (0x07).
  - W klasycznym profilu `ClassicYmir` wystepuje mapowanie 1:1, bez nadpisania stalych z `Protocol/Protocol.h`.
- **Tryby ramkowania (Framing):**
  - `Ymir1B`: opcode ma 1 bajt, rozmiar jest pobierany ze znanej mapy rejestru (`RegisterExpectedSize`).
  - `M2Dev4B`: naglowek ma 4 bajty (2B opcode, 2B dynamiczna dlugosc). Wymaga uzycia little-endian (odczyt bitowy).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Buforowanie oraz szyfrowanie i stan (AES counter, reszty) nie sa zabezpieczone (brak mutexow). Tradycyjnie obsluga sieci (lub zrzucanie z IO do glownego logicznego watku) jest jednowatkowa na jedna instancje polaczenia. Nalezy trzymac cykl odczytu na wyznaczonym watku (np. Async Network Thread), ew. zwracane spany kopiowac przy przekazywaniu na glowny watek.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Brak lub bledny wskaznik przekazany do `AesCryptoProvider::ProcessStream` obslugiwany jest jako zgloszenie bledu.
  - `DynamicPacketFramer` radzi sobie z niedomiarem `BufferUnderflow`, lecz przy zlych danych i odczycie blednej dlugosci ramki z M2Dev4B naglowka, moze doprowadzic do out-of-memory z powodu rosnacego bufora `m_buffer` oczekujacego na gigantyczna dlugosc.
- **Zarzadzanie zasobami (RAII):** Klasy korzystaja ze zlozonych wektorow i stalej wielkosci buforow w klasach (np. `AesCryptoProvider` w calosci opiera sie na `std::array`).

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  - **Nowe Opcody (Adapter):** Dodaj je najpierw w `Protocol/Protocol.h` (jesli jeszcze nie istnieja), a pozniej w zaleznosci od profilu dopisz w funkcji switch w `ProtocolAdapter.cpp`.
  - **Nowe krypto (Provider):** Odziedzicz po `ICryptoProvider`. Dla szyfratorow zaleznych od kontekstu, postepuj jak `AesCryptoProvider` (pobieranie stalych, inicjalizacja w ctor lub Init). Nastepnie uaktualnij fabryke jesli istnieje lub podmien uzywane factory w obiekcie sieciowym klienta.
- **Jak debugowac:** Mozna wykorzystac proste printy dlugosci wczytanych/skonstruowanych frame'ow w framerze. Szczegolnie kluczowy bedzie podglad poczatku tablicy `m_buffer` (lub span z `FrameNextPacket`) przed zdekodowaniem i po nim. W wypadku zrywania polaczen nalezy sprawdzic czy nie psuje sie `m_blockOffset` w CTR AES-u.
- **Jak testowac:** Mozna w calosci pisac Headless testy dla kazdego z trzech glownych elementow. Przykrojenie spreparowanego pakietu uzywajac std::vector lub byte-array pozwoli wprost przetestowac takze framer jak i stream cipher bez instancjonowania CPythonNetworkStream.
