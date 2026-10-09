---
task_id: "atlas_c01_14_ring_packet_buffer"
cluster: "NET"
module_name: "Bufory Kolowe Pakietow i Obsluga Strumienia (Packet Ring Buffer)"
target_files:
- src/EterBase/FastLockFreeRingBuffer.h
- src/Client/Network/PacketRingBuffer.h
- src/Client/Network/ZeroCopyPacketBuffer.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c01_14_ring_packet_buffer.md"
architecture_layer: "Silnik Sieciowy, Maszyna Faz i Protokol"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport Audytu: Bufory Kolowe Pakietow i Obsluga Strumienia (Packet Ring Buffer)

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Modul ten realizuje fundamentalna warstwe transportowa i buforujaca dla strumienia pakietow sieciowych miedzy klientem a serwerem. Zapewnia on odpornosc na fragmentacje TCP, co jest krytyczne, poniewaz pakiety z serwera moga docierac w czesciach.
Kod jest wywolywany w petli sieciowej klienta (Network Tick / OnUpdate fazy sieciowej). Odbiera surowe bajty z gniazd sieciowych (sockets), buforuje je, a nastepnie udostepnia wyzszym warstwom logiki gry w postaci ustrukturyzowanych pakietow (lub jako ciagle bloki pamieci) za pomoca zero-copy z mechanizmami fallbacku.
Przeplyw danych:
1. Gniazdo sieciowe odbiera bajty.
2. Bajty sa zapisywane do jednego z buforow (np. przez GetWritableSpan i CommitWrite w ZeroCopyPacketBuffer).
3. Logika protokolu podglada/odczytuje pakiety (np. PeekPacket, RecvPacket).
4. Bufor zwalnia miejsce (CommitRead / Skip) dla kolejnych danych, zachowujac strukture pierscienia.
Cykl zycia: Bufory sa pre-alokowane podczas inicjalizacji (np. na rozmiar 2MB w ZeroCopyPacketBuffer) i dzialaja bez dalszych alokacji w petli (zero-GC). Resetowane (Clear) sa przy rozlaczeniach sieciowych lub przy bledach synchronizacji strumienia.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

*   **Zaleznosci wejsciowe (Inbound):** Kod wywolywany jest przez warstwe obslugi gniazd sieciowych (warstwa I/O Winsock/sieciowa klienta) w celu zrzucenia odczytanych bajtow oraz przez maszyny faz gry i obsluge protokolu sieciowego, ktore pobieraja wyparsowane pakiety, aby przeslac je dalej (np. EventBus, logika postaci).
*   **Zaleznosci wyjsciowe (Outbound):** Zaleznosci sa minimalne. Klasy uzywaja EterBase::Result (EterBase::VoidResult, EterBase::PacketResult), nowozytnych struktur standardowych (std::span, std::expected, std::atomic, std::vector) oraz mechanizmow logowania (LogModern.h w FastLockFreeRingBuffer).
*   **Drzewo dyrektyw `#include`:**
    *   `<atomic>`, `<vector>`, `<span>`, `<cstdint>`, `<algorithm>`, `<expected>`, `<cstring>`, `<cassert>`
    *   `"Result.h"` / `"../../EterBase/Result.h"`
    *   `"LogModern.h"`
    Ryzyko zaleznosci cyklicznych jest minimalne, poniewaz pliki to naglowki narzedziowe nie importujace modulow silnika.
*   **Model pamieciowy:** Wewnetrzna pre-alokacja via `std::vector<uint8_t>`. Udostepnianie surowych wskaznikow C i `std::span` podczas odczytu. Mechanizmy `ZeroCopyPacketBuffer` uzywaja tymczasowego bufora `m_scratchBuffer` jako fallbacku do rzadkich przypadkow przeplotu z koncem pierscienia. W przypadku lock-free mamy scisle zdefiniowane wlasnosci pamieci za pomoca `std::memory_order_relaxed`, `std::memory_order_acquire`, `std::memory_order_release`.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### EterBase::FastLockFreeRingBuffer
*   **Rola:** Lock-free, pre-alokowany ring buffer dla relacji SPSC (Single-Producer, Single-Consumer).
*   **Wielkosc:** ~zalezy od zadeklarowanego wektora + atomowe liczniki wyrownane do 64-bajtow (alignas(64) by zapobiec false sharing). Z reguly zarzadzane w watku sieciowym / glownym w zaleznosci od obslugi petli.
*   **Metody Publiczne:**
    *   `explicit FastLockFreeRingBuffer(size_t capacity)`: Konstruktor alokujacy strukture z wektorem i zerujac m_head, m_tail.
    *   `PacketResult<void> Write(std::span<const uint8_t> data) noexcept`: Zapis, warunek wstepny to dostepna pamiec wolna; skutek to aktualizacja m_tail.
    *   `PacketResult<void> Read(std::span<uint8_t> dest) noexcept`: Czyta i niszczy dane z kolejki.
    *   `PacketResult<void> Peek(std::span<uint8_t> dest) const noexcept`: Zwraca kopie na widok (fallback).
    *   `PacketResult<void> Skip(size_t count) noexcept`: Aktualizuje m_head symulujac usuniecie elementow z brzegu.
    *   `size_t GetSize() const noexcept` oraz `size_t GetFreeSpace() const noexcept` i `void Clear() noexcept`.
*   **Memory Layout:**
    *   `size_t m_capacity;`
    *   `std::vector<uint8_t> m_buffer;`
    *   `alignas(64) std::atomic<size_t> m_head;`
    *   `alignas(64) std::atomic<size_t> m_tail;`

### Client::Network::PacketRingBuffer
*   **Rola:** Podstawowy, narzedziowy ring buffer operujacy na wskaznikach pozycyjnych dla struktur sieciowych.
*   **Metody Publiczne:**
    *   `EterBase::VoidResult<EterBase::PacketError> Write(std::span<const uint8_t> data);`
    *   `EterBase::VoidResult<EterBase::PacketError> Read(std::span<uint8_t> outBuffer);`
    *   `EterBase::VoidResult<EterBase::PacketError> Peek(std::span<uint8_t> outBuffer) const;`
    *   `EterBase::VoidResult<EterBase::PacketError> Skip(size_t size);`
    *   `EterBase::VoidResult<EterBase::PacketError> PeekDynamicSize(size_t expectedSize) const;`
*   **Memory Layout:** `std::vector<uint8_t> m_buffer; size_t m_readPos; size_t m_writePos; size_t m_capacity; bool m_isFull;`

### Client::Network::ZeroCopyPacketBuffer
*   **Rola:** Zoptymalizowany pod zero-copy buffer rzutujacy ramki prosto z odebranego bufora na struktury `TPacket` o ile leza w ciaglym bloku.
*   **Metody Publiczne:**
    *   `std::span<uint8_t> GetWritableSpan(size_t maxRequested = 65536) noexcept;` (wejscie dla gniazda)
    *   `void CommitWrite(size_t bytesWritten) noexcept;`
    *   `bool Write(std::span<const uint8_t> data) noexcept;`
    *   `std::span<const uint8_t> PeekContiguous(size_t size) const noexcept;`
    *   `template <typename TPacket> const TPacket* PeekPacket() const noexcept;`
    *   `template <typename TPacket> const TPacket* RecvPacket() noexcept;`
    *   `bool CommitRead(size_t size) noexcept;`
*   **Memory Layout:** Wektor glowny (`m_buffer`), wewnetrzne pozycje read/write, a dodatkowo `mutable std::vector<uint8_t> m_scratchBuffer` na krawedziowe polaczenia pamieci z wrap-around. Zmienne telemetryczne `m_zeroCopyHits`, `m_wrapAroundFallbacks`.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

*   **Pakiety Sieciowe:** Moduly implementuja uniwersalne buforowanie dla struktur dziedziczacych i wzorowanych na TPacket (zarowno w strone CG - Client->Game, jak i GC - Game->Client). Dzieki ZeroCopyPacketBuffer pakiety moglyby byc bezposrednio uzywane w obsludze sieciowej przez proste parsowanie naglowkow i rozmiarow.
*   **Metody Pythona (`PyMethodDef`):** Te pliki nie bezposrednio eksportuja zadnego z interfejsow C-API Pythona. Stanowia jedynie niskopoziomowa podstawe dzialania strumienia na poziomie kodu C++23.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

*   **Zasady wielowatkowosci:** Klasa `FastLockFreeRingBuffer` gwarantuje SPSC za pomoca std::atomic - jeden watek zapisujacy (np. siec) i jeden czytajacy (np. gra). Klasy `PacketRingBuffer` i `ZeroCopyPacketBuffer` nie maja gwarancji atomowosci, zaklada sie ich uzytkowanie w zamknietym watku (np. w jednej instrukcji obslugi sieci Network Tick/Poll).
*   **Potencjalne punkty awarii (Crash Points & Edge Cases):**
    *   **Underflow / Overflow:** Czytanie poza pojemnosc (zabezpieczone przez PacketResult zwracajacy BufferUnderflow). Zapis poza m_capacity (odpowiednio buforowane i ucinane do limitu z wyrzucaniem bledu uzytkownikowi).
    *   **Bledne rzutowanie:** `ZeroCopyPacketBuffer::PeekPacket` uzywa `reinterpret_cast`. Trzeba bezwglednie polegac na `HasBytes(sizeof(TPacket))` i tym, ze bufor poprawnie zbudowal przestrzen ciagla (np. fallback na scratchpad w przypadku podzielonego pakietu miedzy poczatkiem a koncem wektora).
*   **Zarzadzanie zasobami (RAII):** Zasoby wektorowe alokowane sa automatycznie w czasie trwania (lifetime) instancji obiektu klasy - zadnych `new` i `delete`. Nalezy jednak powstrzymywac sie przed kopiowaniem (usuniety konstruktor kopiujacy) preferujac semantyke przenoszenia (std::move).

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

*   **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
    1. Rozszerzenia dla nowej logiki (np. pakietow kompresowanych) powiazac przez dekoracje nowej klasy lub dodanie metody na wektor danych i std::span.
    2. ZeroCopyPacketBuffer moze wymuszac aktualizacje rzutowania wiec jesli struktura nie ma stalego rozmiaru (dynamic header) uzyc `PeekDynamicSize()` dla PRB lub obslugiwac dany fragment bajtowo i wywolywac PeekContiguous z parametrami naglowka z reki.
    3. Dodac unit test oparty o docTest symulujac wejscie czesciowe.
*   **Jak debugowac i logowac:** Do monitorowania wydajnosci przy obslugach Zero-copy w module przydadza sie gettersy do telemetrii: `GetZeroCopyHits` oraz `GetWrapAroundFallbacks`. Dodawaj pulapki (breakpoints) w sekcjach kopiowania (np. gdy m_scratchBuffer zostaje wywolywany), co pozwoli zlapac rzadki wrap-around pod koniec 2MB bufora.
*   **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Inicjalizuj bufor ze statycznymi rozmiarami. Wprowadz losowe porcje danych (od 1 bajtu do 4KB) za pomoca `Write` symulujac socket recv, po czym w petli zrob sprawdzanie poprawnosci `Read` na wyjscie oraz ciaglosc dla `PeekPacket`.
