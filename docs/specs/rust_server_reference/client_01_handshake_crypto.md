# Specyfikacja Protokolu - Faza 1: Handshake i Kryptografia

## 1. Wprowadzenie
Dokument okresla struktury pakietow oraz procedury zwiazane z inicjalizacja polaczenia, w tym proces synchronizacji czasu, mechanizm ping/pong oraz wymiane kluczy protokolu (TEA/SecurityMode). Specyfikacja bazuje na analizie klienta referencyjnego Metin2 (C++). 

## 2. Pakiety i Struktury

Z uwagi na zastosowanie `#pragma pack(1)` (lub brak paddingu w pakietach), struktury sa mapowane z odpowiednimi typami dla Rusta w architekturze Little-Endian.

### 2.1. Handshake i Synchronizacja Czasu

#### `TPacketGCHandshake` (Z serwera do klienta / Z klienta do serwera)
Wykorzystywany do synchronizacji czasu. Klient odbiera z naglowkiem `HEADER_GC_HANDSHAKE` i odsyla z naglowkiem `HEADER_CG_TIME_SYNC`.

- **Naglowek Serwera (`HEADER_GC_HANDSHAKE`):** `0xFF` (255)
- **Naglowek Klienta (`HEADER_CG_TIME_SYNC`):** `0xFC` (252)
- **Rozmiar calkowity:** 13 bajtow

| Pole | Typ C++ | Typ Rust | Mapowanie (LE) | Rozmiar (bajty) | Opis |
|---|---|---|---|---|---|
| header | BYTE | u8 | `<B` | 1 | Naglowek (0xFF z serwera, 0xFC od klienta) |
| dwHandshake | DWORD | u32 | `<I` | 4 | Wartosc handshake |
| dwTime | DWORD | u32 | `<I` | 4 | Czas serwera (z uwzglednieniem lDelta) |
| lDelta | LONG | i32 | `<i` | 4 | Opoznienie / roznica w czasie |

**Przebieg po stronie klienta:**
1. Odbiera `TPacketGCHandshake` (0xFF).
2. Aktualizuje wewnetrzny czas: `m_dwChangeServerTime = dwTime + lDelta`, a `m_dwChangeClientTime` pobiera z lokalnego timera.
3. Oblicza nowy czas dla serwera: `dwTime = dwTime + lDelta + lDelta`, a `lDelta` ustawia na 0.
4. Odsyla ten sam struct `TPacketGCHandshake`, ale ze zmienionym polem `header` na `HEADER_CG_TIME_SYNC` (0xFC).

#### `TPacketGCBlank` (Potwierdzenie Handshake)
Odbierany po poprawnym przetworzeniu `TPacketCGTimeSync` przez serwer.

- **Naglowek (`HEADER_GC_HANDSHAKE_OK`):** `0xFC` (252)
- **Rozmiar calkowity:** 1 bajt

| Pole | Typ C++ | Typ Rust | Mapowanie (LE) | Rozmiar (bajty) | Opis |
|---|---|---|---|---|---|
| header | BYTE | u8 | `<B` | 1 | Naglowek (0xFC) |

**Przebieg:** 
Klient dokonuje ostatecznej korekty delta czasu (`dwDelta = ELTimer_GetMSec() - m_dwChangeClientTime`) i ostatecznie synchronizuje z serwerem.

### 2.2. Mechanizm Ping / Pong

Sluzy do podtrzymywania polaczenia i weryfikacji opoznien.

#### `TPacketGCPing` (Z serwera)
- **Naglowek (`HEADER_GC_PING`):** Zalezny od konfiguracji, pakiet w `Packet.h` (brak zdefiniowanego, jednak w C++ sprawdzany jako opkod z serwera, czesto uzywany np. `HEADER_GC_PING`)
- **Rozmiar calkowity:** 1 bajt

| Pole | Typ C++ | Typ Rust | Mapowanie (LE) | Rozmiar (bajty) | Opis |
|---|---|---|---|---|---|
| header | BYTE | u8 | `<B` | 1 | Naglowek pakietu Ping |

#### `TPacketCGPong` (Odpowiedz klienta)
- **Naglowek (`HEADER_CG_PONG`):** `0xFE` (254)
- **Rozmiar calkowity:** 1 bajt

| Pole | Typ C++ | Typ Rust | Mapowanie (LE) | Rozmiar (bajty) | Opis |
|---|---|---|---|---|---|
| bHeader | BYTE | u8 | `<B` | 1 | Naglowek (0xFE) |

**Przebieg:**
Po odebraniu `TPacketGCPing`, klient zapisuje czas odebrania (`m_dwLastGamePingTime = ELTimer_GetMSec()`) i natychmiast odsyla pakiet `TPacketCGPong`.

### 2.3. Zmiana Fazy Polaczenia (Phase)

Pakiet uzywany do poinstruowania klienta, na jakim etapie znajduje sie aktualnie polaczenie (Login, Select, Game, etc.).

#### `TPacketGCPhase`
- **Naglowek (`HEADER_GC_PHASE`):** `0xFD` (253)
- **Rozmiar calkowity:** 2 bajty

| Pole | Typ C++ | Typ Rust | Mapowanie (LE) | Rozmiar (bajty) | Opis |
|---|---|---|---|---|---|
| header | BYTE | u8 | `<B` | 1 | Naglowek (0xFD) |
| phase | BYTE | u8 | `<B` | 1 | Identyfikator fazy (np. 2 = PHASE_LOGIN) |

### 2.4. Ulepszone Szyfrowanie (Improved Packet Encryption - TEA/SecurityMode)

Nowy mechanizm generowania wektorow inicjalizacyjnych wykorzystywany przy ulepszonym szyfrowaniu (jesli skompilowano z `_IMPROVED_PACKET_ENCRYPTION_`).

#### `TPacketKeyAgreement` (Obustronny)
Wykorzystywany do obustronnej wymiany kluczy (Diffie-Hellman / Key Agreement).

- **Naglowek serwera (`HEADER_GC_KEY_AGREEMENT`):** `0xFB` (251)
- **Naglowek klienta (`HEADER_CG_KEY_AGREEMENT`):** `0xFB` (251)
- **Rozmiar calkowity:** 261 bajtow

| Pole | Typ C++ | Typ Rust | Mapowanie (LE) | Rozmiar (bajty) | Opis |
|---|---|---|---|---|---|
| bHeader | BYTE | u8 | `<B` | 1 | Naglowek (0xFB) |
| wAgreedLength | WORD | u16 | `<H` | 2 | Faktyczna dlugosc danych klucza |
| wDataLength | WORD | u16 | `<H` | 2 | Rozmiar przesylanych danych (Max 256) |
| data | BYTE[256] | [u8; 256] | tablica u8 | 256 | Dane wynegocjowanego klucza |

**Przebieg:**
1. Klient odbiera z serwera wyzwanie `TPacketKeyAgreement` (z opkodem `HEADER_GC_KEY_AGREEMENT`).
2. Przeprowadza operacje generowania odpowiedzi poprzez `Prepare()` i zatwierdzenie `Activate()`.
3. Konstruuje odpowiedz (wlasny `TPacketKeyAgreement`) ustawiajac opkod `HEADER_CG_KEY_AGREEMENT`.
4. Odsyla odpowiedz do serwera.

#### `TPacketKeyAgreementCompleted` (Potwierdzenie z serwera)
Pakiet potwierdzajacy ukonczenie wymiany kluczy i poinstruowanie klienta o wlaczeniu trybu szyfrowanego.

- **Naglowek (`HEADER_GC_KEY_AGREEMENT_COMPLETED`):** `0xFA` (250)
- **Rozmiar calkowity:** 4 bajty

| Pole | Typ C++ | Typ Rust | Mapowanie (LE) | Rozmiar (bajty) | Opis |
|---|---|---|---|---|---|
| bHeader | BYTE | u8 | `<B` | 1 | Naglowek (0xFA) |
| data | BYTE[3] | [u8; 3] | tablica u8 | 3 | Dane puste (dummy, not used) |

**Przebieg:**
Gdy klient odbierze ten pakiet, wlacza zabezpieczenia wywolaniem wewnetrznej funkcji szyfrowania `ActivateCipher()`. Od tego momentu wszystkie pakiety sa poddawane procedurze rozszyfrowywania.

## 3. Podsumowanie dzialan w petli Handshake (PythonNetworkStreamPhaseHandShake)
Faza `HandShakePhase` obsluguje kilka krytycznych etapow przygotowania sieci. Wymaga obslugi opkodow od serwera i w zaleznosci od stanu moze:
- Zarzadzac uwierzytelnieniem szyfrowania hybrydowego (`HEADER_GC_HYBRIDCRYPT_KEYS`, `HEADER_GC_HYBRIDCRYPT_SDB`).
- Dokonac wynegocjowania kluczy TEA, gdy serwer zazada tego poprzez `HEADER_GC_KEY_AGREEMENT`.
- Przelaczyc faze dzialania klienta po odebraniu `HEADER_GC_PHASE` do np. `PHASE_LOGIN`.
- Synchornizowac czas wysylajac `HEADER_CG_TIME_SYNC`.
