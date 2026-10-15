#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <mutex>
#include <functional>
#include <iosfwd>

namespace Client::Core {

/**
 * @brief Poziomy istotnosci zdarzen telemetrii AI.
 */
enum class TelemetryLevel : uint8_t {
    Debug = 0,
    Info,
    Warn,
    Error,
    Fatal
};

/**
 * @brief Kategorie domenowe zdarzen telemetrii.
 */
enum class TelemetryCategory : uint8_t {
    Network = 0,
    Combat,
    Movement,
    Phase,
    Python,
    Memory,
    System
};

[[nodiscard]] constexpr std::string_view TelemetryLevelToString(TelemetryLevel level) noexcept {
    switch (level) {
        case TelemetryLevel::Debug: return "DEBUG";
        case TelemetryLevel::Info:  return "INFO";
        case TelemetryLevel::Warn:  return "WARN";
        case TelemetryLevel::Error: return "ERROR";
        case TelemetryLevel::Fatal: return "FATAL";
        default:                    return "INFO";
    }
}

[[nodiscard]] constexpr std::string_view TelemetryCategoryToString(TelemetryCategory cat) noexcept {
    switch (cat) {
        case TelemetryCategory::Network:  return "NET";
        case TelemetryCategory::Combat:   return "COMBAT";
        case TelemetryCategory::Movement: return "MOVEMENT";
        case TelemetryCategory::Phase:    return "PHASE";
        case TelemetryCategory::Python:   return "PYTHON";
        case TelemetryCategory::Memory:   return "MEMORY";
        case TelemetryCategory::System:   return "SYSTEM";
        default:                          return "SYSTEM";
    }
}

/**
 * @brief Pojedynczy slot telemetrii w buforze pierscieniowym.
 * Prealokowany bufor zapewnia calkowity brak alokacji dynamicznych
 * na stercie podczas normalnego emitu zdarzenia (zero-allocation).
 */
struct TelemetryRecord {
    static constexpr size_t MAX_PAYLOAD_SIZE = 1024;
    uint64_t timestamp{0};
    uint16_t length{0};
    char json[MAX_PAYLOAD_SIZE]{0};
};

/**
 * @brief Glowny system ustrukturyzowanej telemetrii AI w formacie JSON-Lines.
 * Wyposazony w bufor pierscieniowy, zrzut do plikow oraz strumieniowanie w locie.
 */
class AITelemetry {
public:
    static constexpr size_t DEFAULT_CAPACITY = 2048;

    /**
     * @brief Singleton dostepny dla calego klienta.
     */
    static AITelemetry& Instance() noexcept;

    /**
     * @brief Konstruktor z konfigurowalnym rozmiarem bufora pierscieniowego.
     */
    explicit AITelemetry(size_t capacity = DEFAULT_CAPACITY);
    ~AITelemetry() = default;

    AITelemetry(const AITelemetry&) = delete;
    AITelemetry& operator=(const AITelemetry&) = delete;
    AITelemetry(AITelemetry&&) noexcept = default;
    AITelemetry& operator=(AITelemetry&&) noexcept = default;

    // Metody emisji zdarzen (zero-heap allocation)
    void EmitPacketTx(std::string_view driver, uint8_t opcode, size_t length, uint32_t seq);
    void EmitPacketRx(std::string_view driver, uint8_t opcode, size_t length);
    void EmitCombatEvent(uint32_t attackerVid, uint32_t victimVid, uint32_t damage, bool isDead);
    void EmitPhaseTransition(std::string_view fromPhase, std::string_view toPhase);
    void EmitError(std::string_view category, std::string_view code, std::string_view message);
    void EmitError(TelemetryCategory cat, std::string_view code, std::string_view message);

    // Zrzut bufora do pliku
    bool FlushToFile(const char* filePath);
    bool FlushToFile(std::string_view filePath);
    void FlushToDisk(const char* filePath);
    void FlushToStream(std::ostream& stream);

    // Obsluga strumieniowania na zywo
    void EnableStreaming(bool enable) noexcept;
    [[nodiscard]] bool IsStreamingEnabled() const noexcept;
    void SetOutputStream(std::ostream* stream) noexcept;
    void SetStreamCallback(std::function<void(std::string_view)> callback);

    // Zarzadzanie i stan bufora
    [[nodiscard]] size_t GetBufferedCount() const noexcept;
    [[nodiscard]] size_t GetCapacity() const noexcept;
    [[nodiscard]] size_t GetTotalEmittedCount() const noexcept;
    [[nodiscard]] bool IsEmpty() const noexcept;
    [[nodiscard]] bool IsFull() const noexcept;
    void Clear() noexcept;

    // Odczyt danych w formatach JSON
    [[nodiscard]] std::string GetLastEventJson() const;
    [[nodiscard]] std::string DumpToString() const;
    [[nodiscard]] std::vector<std::string> GetEventsAsJsonLines() const;

private:
    void StoreRecordAndDispatch(const char* jsonBuffer, size_t length, uint64_t timestamp);

    size_t m_capacity;
    std::vector<TelemetryRecord> m_ringBuffer;
    size_t m_writeIndex{0};
    size_t m_count{0};
    size_t m_totalEmitted{0};

    bool m_streamingEnabled{false};
    std::ostream* m_outputStream{nullptr};
    std::function<void(std::string_view)> m_streamCallback{nullptr};

    mutable std::mutex m_mutex;
};

} // namespace Client::Core
