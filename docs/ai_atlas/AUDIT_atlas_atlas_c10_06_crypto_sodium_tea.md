---
task_id: "atlas_c10_06_crypto_sodium_tea"
cluster: "SYS"
module_name: "Implementacje Kryptograficzne TEA i LibSodium"
target_files:
- src/EterBase/CryptoSodiumModern.h
- src/EterBase/TeaModern.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c10_06_crypto_sodium_tea.md"
architecture_layer: "VFS, Szyfrowanie, Audio, Proto i Platforma"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport z audytu kodu: Implementacje Kryptograficzne TEA i LibSodium

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Modul ten dostarcza bezpieczne mechanizmy kryptograficzne dla klienta gry Metin2, zastepujac starsze, mniej bezpieczne lub przestarzale implementacje (jak Crypto++). Sluzy on glownie do zabezpieczania komunikacji sieciowej (pakiety wysylane i odbierane od serwera gry) oraz pamieci procesu klienta (np. ukrywanie prawdziwych wartosci ID podmiotow lub VNUM przedmiotow w pamieci klienta).

Implementacja sklada sie z dwoch filarow:
1. `CryptoSodiumModern` - wrapper na biblioteke `libsodium`, realizujacy szyfrowanie strumieniowe XChaCha20, z kluczami sesyjnymi wymienianymi bezpiecznie za pomoca mechanizmow libsodium. Komponent ten zyje w trakcie aktywnej sesji uzytkownika i modyfikuje swoj stan, powiadamiajajac pozostale warstwy przez `EventBus` w momencie aktywacji/deaktywacji sesji.
2. `TeaModern` - nowoczesna implementacja TEA (Tiny Encryption Algorithm) zgodna ze standardem C++20, oferujaca statyczne funkcje dla bezpiecznych operacji pamieciowych wykorzystujacych `std::span` zamiast stalych wskaznikow C, co zabezpiecza przed wieloma wektorami atakow (Buffer Overflow).

W petli gry te metody beda glownie wywolywane w warstwie Network Tick (do szyfrowania i deszyfrowania framow pakietow w locie) oraz rzadziej w glownym watku / w ui podczas manipulacji z vnumami (ekwipunek, tooltipy) lub entity ID (rendering jednostek, klikniecia na target).

Przeplyw danych w `CryptoSodiumModern`:
1. Tworzenie instancji (`CryptoSodiumModern`) - alokowane sa bufory i zmienne.
2. `Initialize()` - odpala sie generacja wlasnej pary kluczy asymetrycznych `crypto_kx_keypair`.
3. Odebranie z sieci klucza publicznego serwera i weryfikacja przez `ComputeClientKeys`. To wylicza symetryczne klucze sesji (RX i TX). Wystrzelenie `CryptoStateChangedEvent(true)` przez EventBus.
4. Uzycie `EncryptInPlace` i `DecryptInPlace` do modyfikowania ladunkow, z automatu inkrementujac wewnetrzne liczniki noncow (txNonce_ i rxNonce_) pod spodem korzystajac z operacji `crypto_stream_xchacha20_xor`.
5. `CleanUp()` / Destruktor - niszczenie danych kluczy w pamieci uzywajac `sodium_memzero` do uniemozliwienia ataku typu "Cold Boot" / zrzutu pamieci. Zglasza deaktywacje.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

- **Zaleznosci wejsciowe (Inbound):** Modul jest inicjalizowany i podpinany do komunikacji sieciowej przez komponenty zarzadzajace siecia (Network Adapter, DynamicPacketFramer). Obiekty z InventoryDomain moga rowniez polegac na `EncryptItemVnum` / `DecryptEntityId` z `CryptoSodiumModern` dla operacji pamieciowych. Obserwatorzy z API UI (Python/C++) sluchaja na `EventBus` uzywajac typu `CryptoStateChangedEvent` aby wiedziec kiedy szyfrowanie z serwerem startuje i czy sa prawidlowo polaczeni.
- **Zaleznosci wyjsciowe (Outbound):** 
  - `libsodium` API (C API) dluze pod spodem z `sodium_init`, `crypto_kx_keypair`, `crypto_kx_client_session_keys`, `crypto_stream_xchacha20_xor`, `sodium_memzero`.
  - EventBus UI klienta (`UserInterface::Core::EventBus`).
  - System logowania z C++23: `ModernLogger::Log`.
  - C++ mechanizmy bledow z `EterBase::Result` (`PacketResult`, `PacketError`).
- **Drzewo dyrektyw `#include`:**
  - `CryptoSodiumModern.h`: `<sodium.h>`, `<cstdint>`, `<cstring>`, `<span>`, `<optional>`, `<vector>`, `<array>`, `"Result.h"`, `"StrongTypes.h"`, `"LogModern.h"`, `"../UserInterface/Core/EventBus.h"`
  - `TeaModern.h`: `<cstdint>`, `<span>`, `<cstring>`, `<cstddef>`
  - Nie zidentyfikowano zadnego ryzyka zaleznosci cyklicznych, poniewaz klasy dzialaja jako najnizsze ogniwa bez referencji powrotnych (poza uzyciem generycznego EventBus-a).
- **Model pamieciowy:** Implementacja unika zarzadzania brudnymi wskaznikami (raw pointers) gdziekolwiek jest to mozliwe w warstwie publicznej, poslugujac sie instancjami pamieciowymi ze stosu `std::array`, dynamicznie przez `std::vector` i tworzac niekopiowalne i nieprzedluzajace zycia bezpieczne widoki pamieci w postaci `std::span` z C++20. Zwarcia uzywaja `std::optional` oraz obiekty oczekiwania bledow `std::expected` reprezentowane w aliasie `PacketResult`.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabele Klas i Struktur

| Nazwa | Rola | Wlasciciel Watku |
| --- | --- | --- |
| `EterBase::CryptoStateChangedEvent` | Zdarzenie przekazujace informcje do EventBus o stanie aktywacji sesji. | Watek sieciowy / glowny (zalezy ktory podniosl zrodlo wywolania). |
| `EterBase::CryptoSodiumModern` | Manager kontekstu kryptograficznego Libsodium z obsluga kluczy, noncow, szyfrowania in-place i eventow | Skonkretyzowana instancja pod menadzerem serwera, Network Thread lub Main Thread |
| `TeaModern` | Przestrzen nazw w formie statycznej klasy z algorytmem TEA. | Bezstanowy, dowolny watek bez wyscigow danych. |

### Tabela Metod Publicznych

**`EterBase::CryptoSodiumModern`**
| Sygnatura | Wartosc Zwracana | Warunki Wstepne i Skutki |
| --- | --- | --- |
| `Initialize()` | `PacketResult<void>` | Aktywuje sodum i generuje klucze asymetryczne. Oznacza `isInitialized_` na true. W przypadku bledu sodium uzywa ModernLogger |
| `CleanUp()` | `void` | Uzywa `sodium_memzero` zeby wyczyscic klucze RX/TX z pamieci operacyjnej i wylacza tryb dzialania klienta (publikuje false do obslugi stanu EventBus). |
| `GetPublicKey() const` | `std::optional<std::span<const uint8_t>>` | Sprawdza isInitialized_. Jezeli jest prawda, daje span do widoku klucza publicznego. Nie daje the ownership z pamieci. |
| `ComputeClientKeys(std::span<const uint8_t> serverKey)` | `PacketResult<void>` | Zwraca blad `PacketError::SessionClosed` jezeli wywolano przed Initialize, weryfikuje dlugosc. Na sukces, wpisuje RX/TX keys oraz odpala status na `true`. |
| `EncryptInPlace(std::span<uint8_t> buffer)` | `PacketResult<void>` | `isActivated_` musi byc prawda. Modifikuje buffer in-place XChaCha20 oraz robi counter increment: `txNonce_++`. |
| `DecryptInPlace(std::span<uint8_t> buffer)` | `PacketResult<void>` | `isActivated_` musi byc prawda. Modifikuje buffer in-place w locie. Inkrementuje `rxNonce_++`. |
| `EncryptItemVnum(ItemVnum vnum)` | `std::optional<std::vector<uint8_t>>` | Kopiuje pod spodem do wektora bufora uint32_t. Szyfruje go jako in-place i rzuca gotowy zaszyfrowany wektor. |
| `DecryptEntityId(std::span<uint8_t> buffer)` | `std::optional<EntityId>` | Koniecznie sizeof musi wynosic dokladnie sizeof(uint32_t). Odpakowuje entity do strong typed wartosci. |
| `IsActivated() const` | `bool` | Getter flagi startu `isActivated_`. |

**`TeaModern`**
| Sygnatura | Wartosc Zwracana | Warunki Wstepne i Skutki |
| --- | --- | --- |
| `static bool Encrypt(std::span<uint8_t> buffer, std::span<const uint8_t> key) noexcept` | `bool` (sukces/porazka) | Wymaga aby `key` mialo dokladnie dlugosc 16, a `buffer` byl potega 8 (BlockSize). Calkowicie in-place - brak alokacji zewnetrznej. Brak logow pobocznych. |
| `static bool Decrypt(std::span<uint8_t> buffer, std::span<const uint8_t> key) noexcept` | `bool` (sukces/porazka) | Deszyfrowanie blokowe, weryfikacje pamieci z spanow dzialaja rowniez identycznie jak szyfrowanie - 16 dla key i mod 8. |

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
Klasa `CryptoSodiumModern` posiada szereg buforow typu `std::array` dla stanow oraz pare flag logicznych.
Layout ma charakter wewnetrznego przechowywania w ciele klasy, stany sa chronione, przez to proba odczytania wartosci przez zewnetrzne moduly FFI / Arthion bedzie zalezna w pelni od ABI, ktore zaalokowalo obiekt, niemniej wartosci:
- `bool isInitialized_` i `bool isActivated_` (czesciowo zalezny od packingu struktury).
- `txNonce_` / `rxNonce_` po 8 bajtow.
- Tablice `publicKey_`, `secretKey_`, `rxKey_`, `txKey_` (zalezne od zdefiniowanych stalej z libsodium np. `crypto_kx_PUBLICKEYBYTES`, `crypto_kx_SESSIONKEYBYTES`). W przypadku pamieciowych modyfikacji, zeby dobrac sie do tajnego klucza za pomoca narzedzia cheatowego bedzie wymagalo wejscia offsetowo wg tablic od gory do dolu. Niemniej po `CleanUp()`, zawartosci sa niszczone w locie do zera.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

- **Pakiety Sieciowe:** Powiazane blisko z pakietami startowymi Handshake miedzy Client a Game (opcody sa zalezne od wariantu np. wymiana publicznych kluczy sesji uzywajaca `crypto_kx_PUBLICKEYBYTES`). Klasa dziala ponizej warstwy opcodow - enkapsuluje caly strumien socketow na protokole TCP, modyfikujac go globalnie kiedy jest `isActivated_`.
- **Metody Pythona:** Nie zidentyfikowano bezposredniego mapowania PyMethodDef w tych plikach naglowkowych. Jednakze wysylka Eventow przez `UserInterface::Core::EventBus` mozy byc obserwowana przez istniejace bindingi UI Pythona, dla przykladu interfejs ladowania (ekran wczytywania gry) moze nasluchiwac aktywacji aby wylaczyc krecacy sie stoper autoryzacji serwerowej.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Zasady wielowatkowosci:** Kod sam w sobie (poza TEA ktory jest bezstanowy) nie stosuje `std::mutex` ani narzedzi thread-safe w `CryptoSodiumModern`. Musi byc zablokowany z zewnatrz jezeli watek logiki gry bedzie modyfikowac bufory ID podczas gdy watek asynchronicznego NetWorker'a dokonuje odbioru danych sieciowych, ewentualnie instancje sa dedykowane na specificzny watek obslugi buforow w 1 domenie danych.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Brak mozliwosci wystapienia przeplnien pamieci (Buffer overflow / underflow) i zadan na nullowe wejscia dzieki uzyciu narzedzia C++20 `std::span` wymuszajacemu posiadanie waznej pamieci z jej rozmiarami przy wejsciu na interfejs publiczny.
  - Otrzymanie uszkodzonego bledu na `ComputeClientKeys` ze spanem mniejszym niz required size spowoduje lagodna degradacje poprzez `PacketResult` (error `PacketError::MalformedPayload`), co bedzie obsulugiwane po chamsku przez framera gubiac polaczenie.
- **Zarzadzanie zasobami (RAII):** Kod swietnie chroni klucze. Zadeklarowanie pamieci z sterty `secretKey_`, `txKey_`, oraz zerowanie natychmiastowe destruktorem z zabezpieczeniem specyficznym `sodium_memzero` nie generuje sladkow po zoltej stronie procesu uniemozliwiajac skanowanie dumpow przez reverserow gamedev.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Jezeli potrzebujesz dodac nowa metode operujaca na kryptografii, uzywaj wylacznie `std::span` dla dostepu modyfikowalnego/niemodyfikowalnego zamiast argumentow raw pointer, pamietaj o walidacji dlugosci tablic na samym poczatku funkcji.
  2. Sprawdz czy instancja jest aktywna (`isActivated_`).
  3. Bledy logiki zamykaj zwrotami przez `MakeError(PacketError::XXX)`, aby zapobiec throw()om z exceptions, poniewaz kod w kliencie oparty jest na determinizmie Error Based.
- **Jak debugowac i logowac:**
  - W klasie zaimplementowano powiadomienia z modern systemu na globalne zdarzenia: np. "Crypto session state changed to:". Szukaj ich na poziomach konsoli stdout od levelu INFO/ERROR po logach `ModernLogger::Log`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  - Implementacja obu narzedzi nie importuje nic mocno zwiazanego ze sztucznym UI / direct3D. Testowanie mozna wykonac bezposrednio w frameworku `doctest.h` zapisanym w `tests/`. Klasy wystarczy podlaczyc includami, zaladowac w pamieci dwa zestawy jako `klient` i `serwer`, przerzucic klucze uzywajac `GetPublicKey` -> `ComputeClientKeys` i puscic ciag bitow celem wykonania testu na szyfrowanie vs deszyfrowanie poprawnosci. Do poprawnego zaladowania nalezy dodac zaleznosc testu unit na zewnetrzny kod `<sodium.h>`.
