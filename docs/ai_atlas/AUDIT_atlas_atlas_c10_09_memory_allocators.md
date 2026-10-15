---
task_id: "atlas_c10_09_memory_allocators"
cluster: "SYS"
module_name: "Specjalizowane Alokatory Pamieci i Pule Zero-Alloc"
target_files:
- src/EterBase/PacketMemoryPool.h
- src/EterBase/PacketZeroAllocPool.h
- src/EterBase/ZeroAllocByteSerializer.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c10_09_memory_allocators.md"
architecture_layer: "VFS, Szyfrowanie, Audio, Proto i Platforma"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

**Funkcja modulu:**
Modul stanowi fundament niskopoziomowego zarzadzania pamiecia dla podsystemu sieciowego w kliencie Metin2. Udostepnia trzy glowne komponenty:
1. `PacketMemoryPool` - pule pamieci dla prealokowanych obiektow (np. obiektow reprezentujacych pakiety), pomagajaca uniknac fragmentacji sterty (heap fragmentation) i kosztownych wywolan `new`/`delete`.
2. `PacketZeroAllocPool` - pule pamieci oparta na stalym buforze o rozmiarze okreslanym w czasie kompilacji (zero runtime allocation), sluzaca do natychmiastowego dysponowania zasobami, np. przy wysokiej czestotliwosci tickow sieciowych.
3. `ZeroAllocByteSerializer` - binarny serializator zapisujacy dane strukturalne, typy proste i stringi bezposrednio do buforow (np. `std::span<uint8_t>`) bez przeprowadzania alokacji pamieci.

**Moment wywolania i przeplyw danych (Lifecycle & Data Flow):**
Narzedzia z tego modulu wykorzystywane sa przez watki sieciowe oraz glowna petle gry podczas odbierania (deserialize) i budowania (serialize) pakietow sieciowych:
- **Alokacja/Inicjalizacja:** Obiekty lub obiekty posrednie (pakiety) pobierane sa ze wczesniej zainicjalizowanych puli (np. za pomoca metody `Acquire` / `acquire`). Metoda ta wywoluje in-place konstruktor nowo wziecgo obiektu (placement new).
- **Uzycie (Serializacja):** Zbudowane dane, lub ich zrzuty binarne, sa serializowane do stanow pakietow (lub ich wnetrznych buforow) w bezpieczny sposob przy wykorzystaniu `ZeroAllocByteSerializer` z uzyciem rygorystycznych ograniczen (bounds checking).
- **Reset/Dealokacja:** Po wyslaniu lub przetworzeniu danych obiekt jest uwalniany metoda `Release` / `release`, zdejmujaca odpowiedzialnosc z zarzadcy sterty. Typ zostaje "zniszczony" (wywolanie destruktora), a wskaznik (lub index slotu) wraca do puli. W poolu ZeroAlloc bit wskazujacy na zajecie slotu w `std::bitset` jest wyzerowywany.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
Klasy z modulu uzywane sa jako zaleznosc generyczna przez routery pakietow, dispatchery i nadajniki/odbiorniki w warstwie `Network` (np. `PhaseGamePacketDispatcher`, klasy z `src/UserInterface/Network/Senders/`).

**Zaleznosci wyjsciowe (Outbound):**
Modul ten ma minimalne zaleznosci:
- Biblioteka Standardowa: `<cstdint>`, `<vector>`, `<mutex>`, `<memory>`, `<stdexcept>`, `<utility>`, `<span>`, `<string_view>`, `<new>`, `<array>`, `<bitset>`, `<type_traits>`, `<cstddef>`, `<cstring>`, `<algorithm>`, `<limits>`.
- `EterBase/Result.h` (C++23 `std::expected` obsluga bledow, np. `PacketResult`, `PacketError`).
- `EterBase/StrongTypes.h` (dla serializatora).
- `EterBase/LogModern.h` (dla logowania bledow i problemow z przepelnieniem).

**Drzewo dyrektyw `#include`:**
Brak zaleznosci cyklicznych. Pliki korzystaja z najnowszych i relatywnie hermetycznych modulow wewnatrz `EterBase`.

**Model pamieciowy:**
Glowny nacisk na operacje bezposrednio na wektorach i surowych przestrzeniach adresowych (`std::byte`, struktury zgodne z alignas), w `PacketMemoryPool` wystepuja surowe wskazniki zwracane po wywolaniu placement new. Pola sa bezpieczne wielowatkowo dzieki wykorzystaniu `std::mutex` i `std::scoped_lock`. W `ZeroAllocByteSerializer` glowne referencje opieraja sie o widoki `std::span<uint8_t>` co zapobiega problemom wiazacym sie z null pointerami.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

#### Tabela Klas i Struktur

| Nazwa | Rola | Wielkosc w bajtach | Wlasciciel watku |
| --- | --- | --- | --- |
| `PacketMemoryPool<T>` | Dynamicznie rozszerzajaca sie pula pamieci, przetrzymujaca wolne bloki. | Zalezna od pojemnosci. | Zabezpieczone mutexem (Thread-safe) |
| `PacketZeroAllocPool<T, Capacity>` | Pula elementow o scisle zadanym romiarze nie korzystajaca z alokatora systemowego | Zalezna od typu. Posiada wewnetrznie pole `m_storage` bedace arrayem `alignas(T)` | Zabezpieczone mutexem (Thread-safe) |
| `ZeroAllocByteSerializer` | Bez-alokacyjny wrapper bufora `std::span` z kontrola wektorow przesuwania (offsetow) | ~24/32 bajty (std::span + size_t) | Dowolny (lokalny w stosie) |
| `PacketZeroAllocPool::StorageElement` | Struktura posredniczaca, zapewniajaca bajtowy layout zgodny z alignas(T) | sizeof(T) | Uzytek wewnetrzny |

#### Tabela Metod Publicznych

**PacketMemoryPool<T>:**
- `explicit PacketMemoryPool(std::size_t initial_capacity = 1024, std::size_t expansion_size = 1024)` - Inicjuje bufor i dokonuje wstepnej pre-alokacji `initial_capacity`.
- `template <typename... Args> T* acquire(Args&&... args)` - Zwraca w pelni skonstruowany in-place obiekt na przetrzymywanym bloku pamieci (lub alokuje nowy w razie braku).
- `void release(T* instance) noexcept` - Wywoluje destruktor wskaznika i oddaje element z powrotem. Brak automatycznego zapobiegania double-free, ale nie wymaga dealokacji systemu.
- `std::size_t available_count() const noexcept` - Zwraca wolne sloty z zabezpieczeniem mutexem.
- `void purge() noexcept` - Usuwa cala zapamietana pamiec z free_list, niszczy alokacje (brak automatycznego niszczenia wciaz pobranych elementow).

**PacketZeroAllocPool<T, Capacity>:**
- `PacketZeroAllocPool() noexcept` - Tworzy nowa, czysta pule zero alloc oparta na `std::array` i `std::bitset`.
- `template <typename... Args> [[nodiscard]] PacketResult<T*> Acquire(Args&&... args)` - Probuje uzyskac nowo skonstruowany obiekt ze stalego poolu; zwraca `PacketError::BufferUnderflow`, gdy pelny.
- `[[nodiscard]] PacketResult<void> Release(T* ptr) noexcept` - Waliduje `ptr`, sprawdza poprawne ofsety i zwraca zasob; zwraca bledy w przypadku invalidizacji lub podwojnego zwalniania.

**ZeroAllocByteSerializer:**
- `explicit constexpr ZeroAllocByteSerializer(std::span<uint8_t> buffer) noexcept` - Inicjalizuje na bazie spanu z zadanym rozmiarem, offset = 0.
- `template <typename T> [[nodiscard]] PacketResult<void> Write(const T& value) noexcept` - Kopiuje (`std::memcpy`) trywialnie kopiowalny typ do widoku.
- `[[nodiscard]] PacketResult<void> WriteBytes(std::span<const uint8_t> data) noexcept` - Wrzuca podany zakres bajtow.
- `[[nodiscard]] PacketResult<void> WriteStringFixed(std::string_view str, size_t maxLength) noexcept` - Pisze z okreslonym wczesniej rozmiarem, dodajac zerowe bajty w ramach paddingu.
- `template <typename LenType = uint16_t> [[nodiscard]] PacketResult<void> WriteString(std::string_view str) noexcept` - Dodaje string poprzedzony rozmiarem zdefiniowanym w `LenType`.
- `[[nodiscard]] constexpr size_t GetBytesWritten() const noexcept` - Pobiera zapisany dotad rozmiar.
- `[[nodiscard]] constexpr std::span<const uint8_t> GetWrittenData() const noexcept` - Zwraca poddany juz obrobce widok bajtow.
- `constexpr void Reset() noexcept` - Kasuje ofset do zera.

#### Pamieciowy Layout Struktur (Memory Layout & Offsets)
- W klasie `PacketZeroAllocPool`, przydzielane zasoby spoczywaja pod dokladnie stalymi ofsetami: dla `Element K`, baza m_storage to `base_ptr + (K * sizeof(T))`. Rozmiar calkowity poolu narzuca typ zdefiniowany w czasie kompilacji oraz capacity, co zapobiega przeplotom danych (data interlacing). Jest to idealne pole do zastosowania systemow hookowania, majac na uwadze brak zjawisk defragmentacji.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

**Pakiety Sieciowe:**
Same pule nie determinuja konkretnych kodow operacji (opcode) lub nazewnictwa wewnetrznego (np. CG/GC). Maja one jednak bezposrednie powiazanie z obsluga calego cyklu zycia dowolnego pakietu od obslugi gniazdka strumieniujacego po finalne dysponowanie obiektem struktury na wyzsza warstwe GamePhase (lub inne moduly np. EterPack).

**Metody Pythona (`PyMethodDef`):**
Obiekty zawarte w tym module stanowia backend i czesc silnika (C++ System Layer), wobec czego nie ma zadnych ujawnien do wartwy C-API, z ktora operuje skrypt Pythona (np. do obslugi GUI czy skryptow logicznych). Python dziala na przetworzonych juz elementach poprzez dispatchery.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

**Zasady wielowatkowosci:**
`PacketMemoryPool` jak rowniez `PacketZeroAllocPool` zawieraja `std::mutex` oraz chronia newralgiczne metody uzywajac obiektu `std::scoped_lock`. Moga byc wspoldzielone przez watek sieciowy, watek ladowania zasobow i glowny, choc najlepiej je przydzielac wylacznie jednej warstwie obslugi I/O aby zapobiec lock-contention.
`ZeroAllocByteSerializer` dziala na obiekcie lokalnym; brakuje tu blokad, stad tez uzywanie danego wlasnego buffora serializacji w obrebie dwoch watkow rownoczesnie poskutkuje niezdefiniowanym zachowaniem (undefined behavior) na zmiennej `m_offset`.

**Potencjalne punkty awarii (Crash Points & Edge Cases):**
- Proba wezwania w `ZeroAllocByteSerializer` zapisu powodujaca BufferUnderflow spowoduje natychmiastowy zwrot z `MakeError(PacketError::BufferUnderflow)`. Bledy te NIE rzucaja wyjatkow, lecz musza byc walidowane poprzez `Result::has_value()` albo uzycie `.and_then()`.
- Metoda `PacketMemoryPool::purge()` nie obsluguje niszczenia jeszcze dzialajacych zasobow. Wywolanie jej podczas aktywnej operacji na pakiecie z puli spowoduje tzw. Dangling Pointers (Use-After-Free) w innej czesci aplikacji.
- `PacketZeroAllocPool::Release` moze bezpiecznie poinformowac o bledzie (`PacketError::InvalidHeader` i log erroru), jezeli probuje sie zwolnic zly wskaznik lub przypisany do innej puli. Mimo tego, uzycie zwolnionego juz pamieciowego obiektu bez ponownego requestowania moze stworzyc korupcje stanow logicznych.

**Zarzadzanie zasobami (RAII):**
Pule wbudowane wykorzystuja dedykowane zarzadzanie pamiecia. RAII odnosi sie do wewnetrznego uzywania lockow (`std::scoped_lock`), oraz dla obiektu umieszczanego na stercie po wywolaniu bloku z destruktorem. Model alokacji wspiera `alignas`, co dba o dostosowanie ukladu bajtow dla wymogow wektorowych procesora (SSE/AVX).

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
1. Skoncentruj uwage na dodaniu metody przy zalozeniu wsparcia dla standardow C++23.
2. Jesli poszerzasz serializacje np. o inny customowy typ bazowy, edytuj plik `src/EterBase/ZeroAllocByteSerializer.h`, implementujac szablon wymuszajacy (za pomoca concepts `requires`) wlasciwa trywialnosc i zachowanie rozmiarow typow.
3. Jesli bedziesz obslugiwac nowy typ w module sieciowym, zachowaj uzycie obiektu z `Result.h`. Przetwarzaj sukces via `.and_then` by redukowac kod typu spaghetti z zaglebionymi sprawdzaniami stanu bledu na rzecz kompozycji funkcyjnej.

**Jak debugowac i logowac:**
Zastosowany mechanizm `ModernLogger::Error()` sluzy do notyfikowania wywrotowych wydarzen. Dla typow errorowych korzystaj z prekonfigurowanego parsera `std::formatter` we wlasnych miejscach podpinanych pod strumienie formatu. Breakpointy w funkcji `Release` sa najlepszym miejscem poszukiwan memory-leakow zwiazanych z gubieniem instancji z obiegu.

**Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
Jako obiekty odseparowane od starych warstw interfejsowych badz `CPythonApplication`, powyzsze pliki mozna swobodnie wpinac w standardowy `gTest` badz inna metode testowania C++ (np. CTest). Przetestuj operacje brzegowe w `ZeroAllocByteSerializer` (wprowadzanie ciagu 121 znakow przy buforze 120), weryfikujac kody bledow zadeklarowanych w `Result.h`. Skontroluj, czy proba zwolnienia falszywego lub nie-wyrownanego wskaznika na baze `PacketZeroAllocPool` poprawnie zwroci przewidywany blad wycieku zasobu.
