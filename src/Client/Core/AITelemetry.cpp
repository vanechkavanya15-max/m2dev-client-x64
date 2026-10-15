#include "AITelemetry.h"

#include <fstream>
#include <iostream>
#include <charconv>
#include <cstring>
#include <algorithm>
#include <chrono>

namespace Client::Core {

namespace {

/**
 * @brief Bezalokacyjny bufor pomocniczy do budowania pojedynczej linii JSON.
 * Gwarantuje brak jakichkolwiek wywolan malloc/new podczas formatowania.
 */
class FixedJsonBuffer {
public:
    char data[TelemetryRecord::MAX_PAYLOAD_SIZE];
    size_t length = 0;

    FixedJsonBuffer() noexcept {
        Clear();
    }

    void Clear() noexcept {
        length = 0;
        data[0] = '\0';
    }

    void Append(std::string_view sv) noexcept {
        size_t available = sizeof(data) - 1 - length;
        size_t toCopy = (sv.size() < available) ? sv.size() : available;
        if (toCopy > 0) {
            std::memcpy(data + length, sv.data(), toCopy);
            length += toCopy;
            data[length] = '\0';
        }
    }

    void AppendChar(char c) noexcept {
        if (length + 1 < sizeof(data)) {
            data[length++] = c;
            data[length] = '\0';
        }
    }

    void AppendUInt(uint64_t val) noexcept {
        char tmp[32];
        auto [ptr, ec] = std::to_chars(tmp, tmp + sizeof(tmp), val);
        if (ec == std::errc()) {
            Append(std::string_view(tmp, static_cast<size_t>(ptr - tmp)));
        }
    }

    void AppendInt(int64_t val) noexcept {
        char tmp[32];
        auto [ptr, ec] = std::to_chars(tmp, tmp + sizeof(tmp), val);
        if (ec == std::errc()) {
            Append(std::string_view(tmp, static_cast<size_t>(ptr - tmp)));
        }
    }

    void AppendHex8(uint8_t val) noexcept {
        Append("0x");
        if (val < 16) {
            AppendChar('0');
        }
        char tmp[8];
        auto [ptr, ec] = std::to_chars(tmp, tmp + sizeof(tmp), val, 16);
        if (ec == std::errc()) {
            for (char* p = tmp; p < ptr; ++p) {
                if (*p >= 'a' && *p <= 'f') {
                    *p = static_cast<char>(*p - 'a' + 'A');
                }
            }
            Append(std::string_view(tmp, static_cast<size_t>(ptr - tmp)));
        }
    }

    void AppendEscapedJson(std::string_view sv) noexcept {
        for (char c : sv) {
            switch (c) {
                case '\"': Append("\\\""); break;
                case '\\': Append("\\\\"); break;
                case '\b': Append("\\b"); break;
                case '\f': Append("\\f"); break;
                case '\n': Append("\\n"); break;
                case '\r': Append("\\r"); break;
                case '\t': Append("\\t"); break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        Append("\\u00");
                        static constexpr char hexDigits[] = "0123456789ABCDEF";
                        AppendChar(hexDigits[(static_cast<unsigned char>(c) >> 4) & 0xF]);
                        AppendChar(hexDigits[static_cast<unsigned char>(c) & 0xF]);
                    } else {
                        AppendChar(c);
                    }
                    break;
            }
        }
    }

    void AppendBool(bool val) noexcept {
        if (val) {
            Append("true");
        } else {
            Append("false");
        }
    }
};

uint64_t GetCurrentEpochMs() noexcept {
    const auto now = std::chrono::system_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
    );
}

} // namespace

AITelemetry& AITelemetry::Instance() noexcept {
    static AITelemetry s_instance(DEFAULT_CAPACITY);
    return s_instance;
}

AITelemetry::AITelemetry(size_t capacity)
    : m_capacity(capacity)
{
    if (m_capacity > 0) {
        m_ringBuffer.resize(m_capacity);
    }
}

void AITelemetry::StoreRecordAndDispatch(const char* jsonBuffer, size_t length, uint64_t timestamp) {
    std::ostream* streamToUse = nullptr;
    std::function<void(std::string_view)> callbackToUse = nullptr;
    bool shouldStream = false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_capacity == 0) {
            return;
        }

        const size_t index = m_writeIndex % m_capacity;
        m_ringBuffer[index].timestamp = timestamp;
        m_ringBuffer[index].length = static_cast<uint16_t>(length);
        std::memcpy(m_ringBuffer[index].json, jsonBuffer, length);
        m_ringBuffer[index].json[length] = '\0';

        m_writeIndex++;
        if (m_count < m_capacity) {
            m_count++;
        }
        m_totalEmitted++;

        shouldStream = m_streamingEnabled;
        streamToUse = m_outputStream;
        callbackToUse = m_streamCallback;
    }

    if (shouldStream) {
        if (streamToUse) {
            streamToUse->write(jsonBuffer, length);
        } else if (callbackToUse) {
            callbackToUse(std::string_view(jsonBuffer, length));
        } else {
            std::cout.write(jsonBuffer, length);
        }
    }
}

void AITelemetry::EmitPacketTx(std::string_view driver, uint8_t opcode, size_t length, uint32_t seq) {
    const uint64_t ts = GetCurrentEpochMs();
    FixedJsonBuffer buf;
    buf.Append("{\"ts\":");
    buf.AppendUInt(ts);
    buf.Append(",\"level\":\"INFO\",\"cat\":\"NET\",\"event\":\"PACKET_TX\",");
    buf.Append("\"driver\":\"");
    buf.AppendEscapedJson(driver);
    buf.Append("\",\"opcode\":");
    buf.AppendUInt(opcode);
    buf.Append(",\"op\":\"");
    buf.AppendHex8(opcode);
    buf.Append("\",\"len\":");
    buf.AppendUInt(length);
    buf.Append(",\"length\":");
    buf.AppendUInt(length);
    buf.Append(",\"seq\":");
    buf.AppendUInt(seq);
    buf.Append("}\n");

    StoreRecordAndDispatch(buf.data, buf.length, ts);
}

void AITelemetry::EmitPacketRx(std::string_view driver, uint8_t opcode, size_t length) {
    const uint64_t ts = GetCurrentEpochMs();
    FixedJsonBuffer buf;
    buf.Append("{\"ts\":");
    buf.AppendUInt(ts);
    buf.Append(",\"level\":\"INFO\",\"cat\":\"NET\",\"event\":\"PACKET_RX\",");
    buf.Append("\"driver\":\"");
    buf.AppendEscapedJson(driver);
    buf.Append("\",\"opcode\":");
    buf.AppendUInt(opcode);
    buf.Append(",\"op\":\"");
    buf.AppendHex8(opcode);
    buf.Append("\",\"len\":");
    buf.AppendUInt(length);
    buf.Append(",\"length\":");
    buf.AppendUInt(length);
    buf.Append("}\n");

    StoreRecordAndDispatch(buf.data, buf.length, ts);
}

void AITelemetry::EmitCombatEvent(uint32_t attackerVid, uint32_t victimVid, uint32_t damage, bool isDead) {
    const uint64_t ts = GetCurrentEpochMs();
    FixedJsonBuffer buf;
    buf.Append("{\"ts\":");
    buf.AppendUInt(ts);
    buf.Append(",\"level\":\"INFO\",\"cat\":\"COMBAT\",\"event\":\"COMBAT_EVENT\",");
    buf.Append("\"attackerVid\":");
    buf.AppendUInt(attackerVid);
    buf.Append(",\"attacker_vid\":");
    buf.AppendUInt(attackerVid);
    buf.Append(",\"victimVid\":");
    buf.AppendUInt(victimVid);
    buf.Append(",\"victim_vid\":");
    buf.AppendUInt(victimVid);
    buf.Append(",\"damage\":");
    buf.AppendUInt(damage);
    buf.Append(",\"isDead\":");
    buf.AppendBool(isDead);
    buf.Append(",\"is_dead\":");
    buf.AppendBool(isDead);
    buf.Append("}\n");

    StoreRecordAndDispatch(buf.data, buf.length, ts);
}

void AITelemetry::EmitPhaseTransition(std::string_view fromPhase, std::string_view toPhase) {
    const uint64_t ts = GetCurrentEpochMs();
    FixedJsonBuffer buf;
    buf.Append("{\"ts\":");
    buf.AppendUInt(ts);
    buf.Append(",\"level\":\"INFO\",\"cat\":\"PHASE\",\"event\":\"PHASE_TRANSITION\",");
    buf.Append("\"from\":\"");
    buf.AppendEscapedJson(fromPhase);
    buf.Append("\",\"fromPhase\":\"");
    buf.AppendEscapedJson(fromPhase);
    buf.Append("\",\"from_phase\":\"");
    buf.AppendEscapedJson(fromPhase);
    buf.Append("\",\"to\":\"");
    buf.AppendEscapedJson(toPhase);
    buf.Append("\",\"toPhase\":\"");
    buf.AppendEscapedJson(toPhase);
    buf.Append("\",\"to_phase\":\"");
    buf.AppendEscapedJson(toPhase);
    buf.Append("\"}\n");

    StoreRecordAndDispatch(buf.data, buf.length, ts);
}

void AITelemetry::EmitError(std::string_view category, std::string_view code, std::string_view message) {
    const uint64_t ts = GetCurrentEpochMs();
    FixedJsonBuffer buf;
    buf.Append("{\"ts\":");
    buf.AppendUInt(ts);
    buf.Append(",\"level\":\"ERROR\",\"cat\":\"");
    buf.AppendEscapedJson(category);
    buf.Append("\",\"event\":\"ERROR\",\"code\":\"");
    buf.AppendEscapedJson(code);
    buf.Append("\",\"message\":\"");
    buf.AppendEscapedJson(message);
    buf.Append("\"}\n");

    StoreRecordAndDispatch(buf.data, buf.length, ts);
}

void AITelemetry::EmitError(TelemetryCategory cat, std::string_view code, std::string_view message) {
    EmitError(TelemetryCategoryToString(cat), code, message);
}

bool AITelemetry::FlushToFile(const char* filePath) {
    if (!filePath || filePath[0] == '\0') {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    std::ofstream out(filePath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    for (size_t i = 0; i < m_count; ++i) {
        const size_t idx = (m_writeIndex - m_count + i) % m_capacity;
        out.write(m_ringBuffer[idx].json, m_ringBuffer[idx].length);
    }

    out.flush();
    return out.good();
}

bool AITelemetry::FlushToFile(std::string_view filePath) {
    const std::string pathStr(filePath);
    return FlushToFile(pathStr.c_str());
}

void AITelemetry::FlushToDisk(const char* filePath) {
    FlushToFile(filePath);
}

void AITelemetry::FlushToStream(std::ostream& stream) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (size_t i = 0; i < m_count; ++i) {
        const size_t idx = (m_writeIndex - m_count + i) % m_capacity;
        stream.write(m_ringBuffer[idx].json, m_ringBuffer[idx].length);
    }
    stream.flush();
}

void AITelemetry::EnableStreaming(bool enable) noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_streamingEnabled = enable;
}

bool AITelemetry::IsStreamingEnabled() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_streamingEnabled;
}

void AITelemetry::SetOutputStream(std::ostream* stream) noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_outputStream = stream;
}

void AITelemetry::SetStreamCallback(std::function<void(std::string_view)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_streamCallback = std::move(callback);
}

size_t AITelemetry::GetBufferedCount() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_count;
}

size_t AITelemetry::GetCapacity() const noexcept {
    return m_capacity;
}

size_t AITelemetry::GetTotalEmittedCount() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_totalEmitted;
}

bool AITelemetry::IsEmpty() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_count == 0;
}

bool AITelemetry::IsFull() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_count >= m_capacity;
}

void AITelemetry::Clear() noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_writeIndex = 0;
    m_count = 0;
}

std::string AITelemetry::GetLastEventJson() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_count == 0) {
        return {};
    }
    const size_t lastIdx = (m_writeIndex - 1) % m_capacity;
    return std::string(m_ringBuffer[lastIdx].json, m_ringBuffer[lastIdx].length);
}

std::string AITelemetry::DumpToString() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string result;
    size_t totalBytes = 0;
    for (size_t i = 0; i < m_count; ++i) {
        const size_t idx = (m_writeIndex - m_count + i) % m_capacity;
        totalBytes += m_ringBuffer[idx].length;
    }
    result.reserve(totalBytes);
    for (size_t i = 0; i < m_count; ++i) {
        const size_t idx = (m_writeIndex - m_count + i) % m_capacity;
        result.append(m_ringBuffer[idx].json, m_ringBuffer[idx].length);
    }
    return result;
}

std::vector<std::string> AITelemetry::GetEventsAsJsonLines() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> lines;
    lines.reserve(m_count);
    for (size_t i = 0; i < m_count; ++i) {
        const size_t idx = (m_writeIndex - m_count + i) % m_capacity;
        lines.emplace_back(m_ringBuffer[idx].json, m_ringBuffer[idx].length);
    }
    return lines;
}

} // namespace Client::Core
