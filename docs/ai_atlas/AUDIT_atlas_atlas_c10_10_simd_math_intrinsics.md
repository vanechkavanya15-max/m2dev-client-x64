---
task_id: "atlas_c10_10_simd_math_intrinsics"
cluster: "SYS"
module_name: "Optymalizacje Matematyczne SIMD (SSE/AVX)"
target_files:
- src/EterBase/MathSIMD.h
- src/EterBase/SIMDVector2D.h
- src/EterBase/SIMD_CRC32Hardware.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c10_10_simd_math_intrinsics.md"
architecture_layer: "VFS, Szyfrowanie, Audio, Proto i Platforma"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul ten dostarcza fundament matematyczny oraz narzedzia do weryfikacji danych, wykorzystujac instrukcje wektorowe procesora (SIMD: SSE4.1, SSE4.2, AVX2). Zastepuje on standardowe obliczenia skalarne o wiele szybszymi, zrownoleglonymi operacjami, co jest kluczowe dla wydajnosci klienta gry.
- **MathSIMD.h:** Dostarcza bezstanowe funkcje narzedziowe do operacji na wektorach 3D i 4D, obliczania dystansow, iloczynow skalarnych i wektorowych oraz zaawansowanej detekcji kolizji (np. Sphere-Sphere, Ray-AABB, Ray-Sphere, Frustum Culling). Wykorzystuje biblioteke DirectXMath, oferujac bezpieczne ladowanie/zapisywanie do pamieci bez rygorystycznych wymogow wyrownania (odporne na Unaligned Memory Loads dla pragmowanych starych struktur). Wywolywany prawdopodobnie w petli renderowania scen i aktualizacji logiki swiata.
- **SIMDVector2D.h:** C++23 obiektowy wrapper dla wektorow 2D, operujacy bezposrednio na wbudowanych typach _m128 (z procesorow x86/x64). Oferuje standardowe wektorowe operacje matematyczne (+, -, *, dot, distance, length) z dodatkiem nowoczesnych mechanizmow oblugi bledow (std::expected), zeby np. uniknac dzielenia przez zero podczas normalizacji wektora zerowego.
- **SIMD_CRC32Hardware.h:** Sprzetowy, bezstanowy akcelerator obliczania sum kontrolnych CRC32. Korzystajac z instrukcji SSE4.2 (_mm_crc32_u64, _mm_crc32_u32), pozwala na bledyskawiczna weryfikacje pakietow. Zastepuje starsze rozwiazania oparte na duzych tablicach. Publikuje rowniez asynchronicznie event (HardwareCrc32CalculatedEvent) po zakonczeniu obliczen zeby oddzielic domene weryfikacji od GUI uzywajac EventBus.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):**
  - Komponenty fizyki/kolizji (uzywajace Ray-AABB z MathSIMD).
  - Warstwa renderowania klienta uzywajaca MathSIMD do Frustum Culling i manipulacji z DirectX.
  - Moduly walidujace (siec, pakiety, VFS) subskrybujace CRC32 przez EventBus.
- **Zaleznosci wyjsciowe (Outbound):**
  - Systemy natywne (Windows / DX): `<DirectXMath.h>` (MathSIMD).
  - Rozszerzenia SIMD x86: `<immintrin.h>`, `<nmmintrin.h>`.
  - C++23 Biblioteka standardowa: `<cmath>`, `<cstdint>`, `<algorithm>`, `<span>`, `<string_view>`, `<optional>`, `<expected>`, `<cstring>`.
  - Powiazania z repozytorium (EterBase): `"Result.h"`, `"LogModern.h"`, `"StrongTypes.h"`, `"UserInterface/Core/EventBus.h"`.
- **Drzewo dyrektyw #include:**
  - `MathSIMD.h`: `<DirectXMath.h>`, `<immintrin.h>`, `<cmath>`, `<cstdint>`, `<algorithm>`
  - `SIMDVector2D.h`: `<immintrin.h>`, `<cmath>`, `<expected>`, `<optional>`, `<string_view>`, `"Result.h"`, `"LogModern.h"`
  - `SIMD_CRC32Hardware.h`: `<cstdint>`, `<span>`, `<string_view>`, `<optional>`, `<cstring>`, `<nmmintrin.h>` (dla 64bit), `"Result.h"`, `"LogModern.h"`, `"StrongTypes.h"`, `"UserInterface/Core/EventBus.h"`
- **Model pamieciowy:** Brak manualnych alokacji sterty uzywajac new/delete ani smart pointerow. Kod w calosci oparty na szybkich lokalnych wektorach stosowych (bezposrednio obslugiwanych przez rejestry m128/m256) oraz bezkopiowych widokach (np. `std::span` i `std::string_view` dla danych do hashownia CRC32). Przekazywanie wartosci po wektorach DX (`FXMVECTOR`) by umozliwic swobodne przenoszenie na rejestry CPU. Klasy bezstanowe (`SIMD_CRC32Hardware`, przestrzen nazw `MathSIMD`). Czyste struktury wielokrotnego uzytku chronione przed bledem wyrownania `Unaligned Memory Loads`.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**SIMDVector2D.h:**
- **Tabela Klas i Struktur:**
  - `SIMDVector2D`: Wrapper w C++23 wokolo instrukcji m128 dla wektora 2D. 16 bajtow.
- **Tabela Metod Publicznych (`SIMDVector2D`):**
  - `constexpr SIMDVector2D() noexcept` | (0,0)
  - `SIMDVector2D(float x, float y) noexcept` | Uzywa _mm_set_ps do ustawienia wektora.
  - `explicit SIMDVector2D(__m128 v) noexcept`
  - `float GetX() const noexcept`
  - `float GetY() const noexcept`
  - `SIMDVector2D operator+(const SIMDVector2D& other) const noexcept`
  - `SIMDVector2D operator-(const SIMDVector2D& other) const noexcept`
  - `SIMDVector2D operator*(float scalar) const noexcept`
  - `friend SIMDVector2D operator*(float scalar, const SIMDVector2D& v) noexcept`
  - `float DotProduct(const SIMDVector2D& other) const noexcept` | Obsluguje fallback z bezposrednim zapisem do unii float[4] dla maszyn bez obslugi SSE3 (np. tylko zwykle SSE).
  - `float LengthSquared() const noexcept`
  - `float Length() const noexcept`
  - `std::expected<SIMDVector2D, std::string_view> Normalize() const noexcept` | Ostrzega i zwraca std::unexpected przy wektorze o dlugosci mniejszej niz 1e-8f uzywajac logowania ModernLogger::Warn.
  - `float DistanceTo(const SIMDVector2D& other) const noexcept`
  - `float DistanceSquaredTo(const SIMDVector2D& other) const noexcept`
- **Pamieciowy Layout:** Unia z `__m128 m_vec` (16 bytes align) oraz `float m_data[4]`. Srodowisko do podpiecia bez offsetow z wyjatkiem offsetu 0x0 dla bufora 16 bajtowego.

**SIMD_CRC32Hardware.h:**
- **Tabela Klas i Struktur:**
  - `HardwareCrc32CalculatedEvent` | Zdarzenie z EventBus, dziedziczy po `UserInterface::Core::IEvent`. Posiada konstruktor pobierajacy `uint32_t checksum` i `size_t size`.
  - `SIMD_CRC32Hardware` | Kalkulator CRC32 bezstanowy (tylko metody statyczne).
- **Tabela Metod Publicznych (`SIMD_CRC32Hardware`):**
  - `static PacketResult<uint32_t> Calculate(std::span<const uint8_t> data, uint32_t initialCrc = 0xFFFFFFFF) noexcept` | Weryfikuje widok, wola wewnetrzne `ComputeInternal`. Brak danych to blad PacketError::BufferUnderflow.
  - `static PacketResult<uint32_t> CalculateString(std::string_view text) noexcept` | Rzutuje C-String przez `std::make_optional` na powyzsze.
- **Pamieciowy Layout:** Brak pol lokalnych (funkcje statyczne). `HardwareCrc32CalculatedEvent` posiada `uint32_t` + paddowanie kompilatora + `size_t` (klasyczne C++ packing, dziedziczenie VTable z IEvent, co daje vptr na poczatku obiektu).

**MathSIMD.h:** (Przestrzen `MathSIMD`)
- **Tabela Metod Publicznych:**
  - `XMVECTOR Load3(const float* pData) noexcept` | Laduje 3D ignorujac alignment poprzez reinterpretacje do XMFLOAT3 i `XMLoadFloat3`.
  - `void Store3(float* pDest, FXMVECTOR v) noexcept` | Przeciwienstwo Load3.
  - `XMVECTOR Load4(const float* pData) noexcept`
  - `void Store4(float* pDest, FXMVECTOR v) noexcept`
  - `float Distance(FXMVECTOR a, FXMVECTOR b) noexcept` | XMVectorSubtract a potem dlugosc.
  - `float DistanceSq(FXMVECTOR a, FXMVECTOR b) noexcept` | Jak wyzej, szybsze bez sqrt (XMVector3LengthSq).
  - `float Dot3(FXMVECTOR a, FXMVECTOR b) noexcept` | XMVector3Dot
  - `XMVECTOR Cross3(FXMVECTOR a, FXMVECTOR b) noexcept` | XMVector3Cross
  - `XMVECTOR Normalize3(FXMVECTOR v) noexcept` | XMVector3Normalize
  - `float Length3(FXMVECTOR v) noexcept` | Dlugosc.
  - `bool IntersectSphereSphere(FXMVECTOR centerA, float radiusA, FXMVECTOR centerB, float radiusB) noexcept` | Uzywa DistanceSq.
  - `bool IntersectRayAABB(FXMVECTOR rayOrigin, FXMVECTOR rayDir, FXMVECTOR boxMin, FXMVECTOR boxMax, float& outTMin, float& outTMax) noexcept` | Szybki raycasting Kay-Kajiya slab method z zabezpieczeniem na probe dzielenia przez 0 za pomoca uzycia granicy `1e-7f`. Zwraca Min/Max dla T na osi promienia.
  - `bool IntersectRaySphere(FXMVECTOR rayOrigin, FXMVECTOR rayDir, FXMVECTOR sphereCenter, float sphereRadius, float& outT) noexcept` | Kolizja promien-kula z wyliczeniem dyskryminanty i odleglosci wejscia.
  - `bool FrustumContainsSphere(const XMVECTOR* planes, size_t planeCount, FXMVECTOR center, float radius) noexcept` | Narzedzie Cullingu sfer, odrzuca sfery ponizej -radius.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** SIMD_CRC32Hardware stanowi fundamentalny fundament kontroli pakietow warstwy sieciowej (Game->Client/Client->Game) z weryfikacja pod katem `PacketError::BufferUnderflow` jesli CRC nie jest adekwatne lub probka jest zerowa. Zwraca bezpieczny typ PacketResult.
- **Metody Pythona (`PyMethodDef`):** Modul jest silnikiem C++ typu natywnego na bardzo niskim poziomie abstrakcji (bezposrednie interakcje SSE). Zamiast mapowac narzuty tuple'a z C-API za pomoca np. wektoryzacji na METH_FASTCALL/PyObject, warstwy wyzsze powiazane z grp/VFS same odgornie musza zrzucic obiekty jako 64bit handle/parametry do wektorow SIMD.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Funkcjonalnosc w pelni bezstanowa (Stateless) i "thread-safe". Wejscia i wyjscia uzywaja wektorow natywnych, wewnatrz brak uzycia zmiennych chronionych mutexami badz modyfikowanialnych globalsow (Global State). Zdarzenia wysylane przez CRC sa bezposrednio emitowane asynchronicznie poprzez `EventBus::Publish()`.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - **Dzielenie przez Zero (Zero-Length Vectors):** Bezposrednio zabezpieczone w funkcjach Ray-AABB (przez select 1e-7f) i Normalize (sprawdzenie kwadratu dlugosci > 1e-8f + std::expected/logowanie).
  - **Brak SSE4.2 w maszynie hosta:** W przypadku kompilacji badz docelowych maszyn na innej architekturze niz _M_X64 (ARM itp), dyrektywa preprocesora w SIMD_CRC32Hardware wyrzuca `ModernLogger::Warn("SIMD_CRC32Hardware: SSE4.2 nie jest wsprane na tej architekturze. Zwracanie niezmienionego CRC.");` uzywajac fallback. Nalezy nie kompilowac logiki pod arm bez odpowiedniej bramki soft-layer-crc dla legacy.
  - **Unaligned Memory Loads:** Nie uzywac bezposrednich castow i wczytan na zapakowanych strukturach C (Pack1). Uzywac `MathSIMD::Load3` oraz `MathSIMD::Store3`.
- **Zarzadzanie zasobami (RAII):** Kod bez zadnych wyciekow z braku uzycia sterty (tylko RAII na stosie i w rejestrach w wektorach m128/FXMVECTOR).

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** W razie implementacji nowych wektorowych metod np. w przestrzeni `MathSIMD.h`, definiuj je wylacznie uzywajac instrukcji natywnych z `DirectXMath` (lub odpowiednio sse `_mm_set_ps` w wektorach `SIMDVector2D.h`) na argumentach `FXMVECTOR`. Wymagane jest uzycie modyfikatora `noexcept` oraz uzycie C++23 monadic (`std::expected`) przy ryzykownych pod wzgledem arytmetyki metodach z narzutem bledu logicznego.
- **Jak debugowac i logowac:** Do debugu bleduw wyniklych ze slepych rotacji SIMD mozna podpinac sie w hook pod wywietlane bledy w konsoli z ModernLogger (np. `"Attempted to normalize a zero-length vector"` w SIMDVector2D). Zdarzenia obslugujace zakonczenie weryfikacji pakietow z pomoca Hashu mozna wylapac przez nasluchiwanie w GUI klasy `HardwareCrc32CalculatedEvent` posredniczac `EventBus`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Kod ten nie zalezy od graficznej warstwy klienta. Obiekty XMVECTOR i sumy CRC32 testuje sie idealnie przy uzyciu doctest. Wymagane flargi kompilacyjne pod GCC/G++ to minimalnie `-msse4.2 -mavx2` z wylaczeniem naglowkow `StdAfx.h` podczas tworzenia izolowanych srodowisk w Linuksie poprzez mockowanie interfejsow wejsciowych.
