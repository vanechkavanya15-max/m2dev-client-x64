#include "StdAfx.h"

#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <string_view>
#include <format>
#include <source_location>

#include "Debug.h"
#include "Singleton.h"
#include "Timer.h"
#include <filesystem>
#include <utf8.h>

const DWORD DEBUG_STRING_MAX_LEN = 1024;

static int isLogFile = false;
HWND g_PopupHwnd = NULL;

// ============================================================================
// OPTIMIZED LOGGING INFRASTRUCTURE
// ============================================================================

// Cached timestamp to avoid repeated time()/localtime() syscalls
// Refreshes every ~100ms (good enough for logging, avoids syscall overhead)
struct TCachedTimestamp
{
    DWORD lastUpdateMs = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;

    void Update()
    {
        DWORD now = ELTimer_GetMSec();
        // Refresh timestamp every 100ms (not per-call)
        if (now - lastUpdateMs > 100)
        {
            time_t ct = time(0);
            struct tm ctm = *localtime(&ct);
            month = ctm.tm_mon + 1;
            day = ctm.tm_mday;
            hour = ctm.tm_hour;
            minute = ctm.tm_min;
            lastUpdateMs = now;
        }
    }

    void Format(char* buf, size_t bufSize) const
    {
        DWORD msec = ELTimer_GetMSec() % 60000;
        _snprintf_s(buf, bufSize, _TRUNCATE, "%02d%02d %02d:%02d:%05d :: ",
            month, day, hour, minute, (int)msec);
    }
};

static TCachedTimestamp g_cachedTimestamp;

// Optimized debug output: Fast path for ASCII strings (avoids Utf8ToWide allocation)
#ifdef _DEBUG
#define DBG_OUT_W_UTF8(psz)                                                   \
    do {                                                                      \
        const char* __s = (psz) ? (psz) : "";                                 \
        size_t __len = strlen(__s);                                           \
        if (Utf8Fast::IsAsciiOnly(__s, __len)) {                              \
            /* ASCII fast path: direct conversion, no allocation */           \
            wchar_t __wbuf[512];                                              \
            size_t __wlen = (__len < 511) ? __len : 511;                      \
            for (size_t __i = 0; __i < __wlen; ++__i)                         \
                __wbuf[__i] = (wchar_t)(unsigned char)__s[__i];               \
            __wbuf[__wlen] = L'\0';                                           \
            OutputDebugStringW(__wbuf);                                       \
        } else {                                                              \
            /* Non-ASCII: use full conversion */                              \
            std::wstring __w = Utf8ToWide(__s);                               \
            OutputDebugStringW(__w.c_str());                                  \
        }                                                                     \
    } while (0)
#else
#define DBG_OUT_W_UTF8(psz) do { (void)(psz); } while (0)
#endif

// MR-11: Colored console output for syserr and packet dumps
#ifdef _DEBUG
static WORD g_consoleDefaultAttrs = 0;
static bool g_consoleAttrsInit = false;

static void InitConsoleDefaultAttrs()
{
    if (g_consoleAttrsInit)
        return;

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!hConsole || hConsole == INVALID_HANDLE_VALUE)
        return;

    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(hConsole, &info))
    {
        g_consoleDefaultAttrs = info.wAttributes;
        g_consoleAttrsInit = true;
    }
}

static void WriteConsoleColored(const char* text, WORD attrs)
{
    if (!text)
        return;

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!hConsole || hConsole == INVALID_HANDLE_VALUE)
    {
        fputs(text, stdout);
        return;
    }

    InitConsoleDefaultAttrs();
    if (g_consoleAttrsInit)
        SetConsoleTextAttribute(hConsole, attrs);

    fputs(text, stdout);

    if (g_consoleAttrsInit)
        SetConsoleTextAttribute(hConsole, g_consoleDefaultAttrs);
}

static const WORD kConsoleSyserrRed = FOREGROUND_RED | FOREGROUND_INTENSITY;
static const WORD kConsolePacketDumpDim = FOREGROUND_INTENSITY;
static const WORD kConsoleTempTraceBg = BACKGROUND_BLUE | BACKGROUND_INTENSITY |
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
#endif
// MR-11: -- END OF -- Colored console output for syserr and packet dumps

// Buffered log file writer
// OPTIMIZATION: Buffered writes with periodic flush instead of per-write fflush()
class CLogFile : public CSingleton<CLogFile>
{
    public:
        CLogFile() : m_fp(NULL), m_bufferPos(0), m_lastFlushMs(0) {}

        virtual ~CLogFile()
        {
            Flush(); // Ensure all buffered data is written
            if (m_fp)
                fclose(m_fp);
            m_fp = NULL;
        }

        void Initialize()
        {
            m_fp = fopen("log/log.txt", "w");
            m_bufferPos = 0;
            m_lastFlushMs = ELTimer_GetMSec();
        }

        void Write(const char* c_pszMsg)
        {
            if (!m_fp)
                return;

            // Use cached timestamp (updated every ~100ms)
            g_cachedTimestamp.Update();
            char timestamp[32];
            g_cachedTimestamp.Format(timestamp, sizeof(timestamp));

            // Calculate total length needed
            size_t timestampLen = strlen(timestamp);
            size_t msgLen = c_pszMsg ? strlen(c_pszMsg) : 0;
            size_t totalLen = timestampLen + msgLen;

            // If this write would overflow the buffer, flush first
            if (m_bufferPos + totalLen >= BUFFER_SIZE - 1)
                Flush();

            // If message is larger than buffer, write directly (rare case)
            if (totalLen >= BUFFER_SIZE - 1)
            {
                fputs(timestamp, m_fp);
                if (c_pszMsg)
                    fputs(c_pszMsg, m_fp);
                fflush(m_fp);
                return;
            }

            // Append to buffer
            memcpy(m_buffer + m_bufferPos, timestamp, timestampLen);
            m_bufferPos += timestampLen;
            if (msgLen > 0)
            {
                memcpy(m_buffer + m_bufferPos, c_pszMsg, msgLen);
                m_bufferPos += msgLen;
            }

            // Flush immediately so crashes or exits never lose log output
            Flush();
        }

        void Flush()
        {
            if (!m_fp || m_bufferPos == 0)
                return;

            m_buffer[m_bufferPos] = '\0';
            fputs(m_buffer, m_fp);
            fflush(m_fp);
            m_bufferPos = 0;
            m_lastFlushMs = ELTimer_GetMSec();
        }

    protected:
        static const size_t BUFFER_SIZE = 8192; // 8KB buffer
        FILE* m_fp;
        char m_buffer[BUFFER_SIZE];
        size_t m_bufferPos;
        DWORD m_lastFlushMs;
};

static CLogFile gs_logfile;

// MR-11: Separate packet dump log from the main log file
class CExtraLogFile
{
    public:
        CExtraLogFile() : m_fp(NULL), m_bufferPos(0), m_lastFlushMs(0) {}

        ~CExtraLogFile()
        {
            Flush();
            if (m_fp)
                fclose(m_fp);
            m_fp = NULL;
        }

        bool Initialize(const char* path)
        {
            m_fp = fopen(path, "w");
            m_bufferPos = 0;
            m_lastFlushMs = ELTimer_GetMSec();
            return m_fp != NULL;
        }

        bool IsOpen() const
        {
            return m_fp != NULL;
        }

        void Write(const char* c_pszMsg)
        {
            if (!m_fp)
                return;

            g_cachedTimestamp.Update();
            char timestamp[32];
            g_cachedTimestamp.Format(timestamp, sizeof(timestamp));

            size_t timestampLen = strlen(timestamp);
            size_t msgLen = c_pszMsg ? strlen(c_pszMsg) : 0;
            size_t totalLen = timestampLen + msgLen;

            if (m_bufferPos + totalLen >= BUFFER_SIZE - 1)
                Flush();

            if (totalLen >= BUFFER_SIZE - 1)
            {
                fputs(timestamp, m_fp);
                if (c_pszMsg)
                    fputs(c_pszMsg, m_fp);
                fflush(m_fp);
                return;
            }

            memcpy(m_buffer + m_bufferPos, timestamp, timestampLen);
            m_bufferPos += timestampLen;
            if (msgLen > 0)
            {
                memcpy(m_buffer + m_bufferPos, c_pszMsg, msgLen);
                m_bufferPos += msgLen;
            }

            // Flush immediately
            Flush();
        }

        void Flush()
        {
            if (!m_fp || m_bufferPos == 0)
                return;

            m_buffer[m_bufferPos] = '\0';
            fputs(m_buffer, m_fp);
            fflush(m_fp);
            m_bufferPos = 0;
            m_lastFlushMs = ELTimer_GetMSec();
        }

    private:
        static const size_t BUFFER_SIZE = 8192;
        FILE* m_fp;
        char m_buffer[BUFFER_SIZE];
        size_t m_bufferPos;
        DWORD m_lastFlushMs;
};

#ifdef _PACKETDUMP
static CExtraLogFile g_packetDumpFile;
static CExtraLogFile g_pdlogFile;
static bool g_packetDumpEnabled = false;
static bool g_pdlogEnabled = false;
static bool g_pdlogRequested = false;

static void EnsurePacketDumpFiles(bool enablePdlog)
{
    if (!std::filesystem::exists("log"))
        std::filesystem::create_directory("log");

    if (!g_packetDumpEnabled)
        g_packetDumpEnabled = g_packetDumpFile.Initialize("log/packetdump.txt");

    if (enablePdlog && !g_pdlogEnabled)
        g_pdlogEnabled = g_pdlogFile.Initialize("log/pdlog.txt");
}
#endif
// MR-11: -- END OF -- Separate packet dump log from the main log file

// Buffered stderr writer for syserr (same pattern as CLogFile)
// OPTIMIZATION: Reduces fflush(stderr) from every call to every 500ms
static struct TSyserrBuffer
{
    static const size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    size_t pos = 0;
    DWORD lastFlushMs = 0;

    void Write(const char* msg, size_t len)
    {
        if (pos + len >= BUFFER_SIZE - 1)
            Flush();

        if (len >= BUFFER_SIZE - 1)
        {
            // Large message: write directly
            fwrite(msg, 1, len, stderr);
            fflush(stderr);
            return;
        }

        memcpy(buffer + pos, msg, len);
        pos += len;

        // Force flush every write to capture crash traces
        Flush();
    }

    void Flush()
    {
        if (pos == 0)
            return;
        fwrite(buffer, 1, pos, stderr);
        fflush(stderr);
        pos = 0;
        lastFlushMs = ELTimer_GetMSec();
    }
} g_syserrBuffer;

// MR-11: Separate packet dump log from the main log file
static void WriteSyserrPlain(const char* msg)
{
    if (!msg)
        return;

    g_cachedTimestamp.Update();
    char timestamp[32];
    g_cachedTimestamp.Format(timestamp, sizeof(timestamp));

    g_syserrBuffer.Write(timestamp, strlen(timestamp));
    g_syserrBuffer.Write(msg, strlen(msg));
}
// MR-11: -- END OF -- Separate packet dump log from the main log file

static UINT gs_uLevel = 0;

void SetLogLevel(UINT uLevel)
{
    gs_uLevel = uLevel;
}

UINT GetLogLevel()
{
    return gs_uLevel;
}

// ============================================================================
// SAFE FORMAT HELPER (ZERO BUFFER OVERFLOW, NO TRUNCATION, STACK FAST PATH)
// ============================================================================
namespace {

std::string VFormatHelper(const char* format, va_list args)
{
    if (!format)
        return {};

    char stackBuf[2048];
    va_list argsCopy;
    va_copy(argsCopy, args);

    int needed = vsnprintf(stackBuf, sizeof(stackBuf), format, args);
    if (needed < 0)
    {
        va_end(argsCopy);
        return {};
    }

    if (static_cast<size_t>(needed) < sizeof(stackBuf))
    {
        va_end(argsCopy);
        return std::string(stackBuf, static_cast<size_t>(needed));
    }

    // Dynamic buffer allocation for large outputs without truncation
    std::string result(static_cast<size_t>(needed), '\0');
    vsnprintf(result.data(), result.size() + 1, format, argsCopy);
    va_end(argsCopy);
    return result;
}

} // anonymous namespace

// ============================================================================
// CORE RAW LOGGING IMPLEMENTATIONS (std::string_view)
// ============================================================================

void TraceRaw(std::string_view msg, bool appendNewline)
{
    std::string formatted;
    const char* pText = nullptr;

    if (appendNewline)
    {
        if (msg.empty() || msg.back() != '\n')
        {
            formatted.reserve(msg.size() + 1);
            formatted.assign(msg);
            formatted.push_back('\n');
            pText = formatted.c_str();
        }
        else
        {
            formatted.assign(msg);
            pText = formatted.c_str();
        }
    }
    else
    {
        formatted.assign(msg);
        pText = formatted.c_str();
    }

#ifdef _DEBUG
    DBG_OUT_W_UTF8(pText);
    fputs(pText, stdout);
#endif

    if (isLogFile)
        LogFile(pText);
}

void TraceErrorRaw(std::string_view msg, bool appendNewline)
{
    std::string fullMsg;
    fullMsg.reserve(8 + msg.size() + (appendNewline ? 1 : 0));
    fullMsg.append("SYSERR: ");
    fullMsg.append(msg);
    if (appendNewline && (fullMsg.empty() || fullMsg.back() != '\n'))
        fullMsg.push_back('\n');

    // OPTIMIZED: Use cached timestamp instead of time()/localtime() per call
    g_cachedTimestamp.Update();
    char timestamp[32];
    g_cachedTimestamp.Format(timestamp, sizeof(timestamp));

    // Stderr output: timestamp + message without "SYSERR: " prefix (exact Metin2 behavior)
    std::string_view msgWithoutPrefix = std::string_view(fullMsg).substr(8);
    g_syserrBuffer.Write(timestamp, strlen(timestamp));
    g_syserrBuffer.Write(msgWithoutPrefix.data(), msgWithoutPrefix.size());
    g_syserrBuffer.Flush();

#ifdef _DEBUG
    DBG_OUT_W_UTF8(fullMsg.c_str());
    WriteConsoleColored(fullMsg.c_str(), kConsoleSyserrRed);
#endif

    if (isLogFile)
        LogFile(fullMsg.c_str());
}

void LogRaw(UINT uLevel, std::string_view msg, bool appendNewline)
{
    if (uLevel < gs_uLevel)
        return;

    TraceRaw(msg, appendNewline);
}

void TempTraceRaw(std::string_view msg, bool errType, bool appendNewline)
{
    std::string formatted;
    if (appendNewline && (msg.empty() || msg.back() != '\n'))
    {
        formatted.reserve(msg.size() + 1);
        formatted.append(msg);
        formatted.push_back('\n');
    }
    else
    {
        formatted.assign(msg);
    }

#ifdef _DEBUG
    DBG_OUT_W_UTF8(formatted.c_str());
    WriteConsoleColored(formatted.c_str(), kConsoleTempTraceBg);
#endif

    if (errType)
    {
        WriteSyserrPlain(formatted.c_str());
        return;
    }

    if (isLogFile)
        LogFile(formatted.c_str());
}

void PacketDumpRaw(std::string_view msg)
{
#ifdef _PACKETDUMP
    std::string fullMsg;
    fullMsg.reserve(13 + msg.size() + 1);
    fullMsg.append("PACKET_DUMP: ");
    fullMsg.append(msg);
    if (fullMsg.empty() || fullMsg.back() != '\n')
        fullMsg.push_back('\n');

#ifdef _DEBUG
    DBG_OUT_W_UTF8(fullMsg.c_str());
    WriteConsoleColored(fullMsg.c_str(), kConsolePacketDumpDim);
#endif

    EnsurePacketDumpFiles(g_pdlogRequested);

    if (g_packetDumpEnabled)
        g_packetDumpFile.Write(fullMsg.c_str());
    if (g_pdlogEnabled)
        g_pdlogFile.Write(fullMsg.c_str());
#else
    (void)msg;
#endif
}

void LogBoxRaw(std::string_view msg, std::string_view caption, HWND hWnd)
{
    if (!hWnd)
        hWnd = g_PopupHwnd;

    std::string sMsg(msg);
    std::string sCaption(caption.empty() ? "LOG" : caption);

    std::wstring wMsg = Utf8ToWide(sMsg.c_str());
    std::wstring wCaption = Utf8ToWide(sCaption.c_str());

    MessageBoxW(hWnd, wMsg.c_str(), wCaption.c_str(), MB_OK);

    Tracen(sMsg.c_str());
}

void LogFileRaw(std::string_view msg)
{
    std::string sMsg(msg);
    CLogFile::Instance().Write(sMsg.c_str());

#ifdef _PACKETDUMP
    if (g_pdlogEnabled)
        g_pdlogFile.Write(sMsg.c_str());
#endif
}

// ============================================================================
// BACKWARD COMPATIBLE C-STYLE LOGGING FUNCTIONS
// ============================================================================

void Log(UINT uLevel, const char* c_szMsg)
{
    if (uLevel >= gs_uLevel)
        Trace(c_szMsg);
}

void Logn(UINT uLevel, const char* c_szMsg)
{
    if (uLevel >= gs_uLevel)
        Tracen(c_szMsg);
}

void Logf(UINT uLevel, const char* c_szFormat, ...)
{
    if (uLevel < gs_uLevel || !c_szFormat)
        return;

    va_list args;
    va_start(args, c_szFormat);
    std::string formatted = VFormatHelper(c_szFormat, args);
    va_end(args);

    TraceRaw(formatted, false);
}

void Lognf(UINT uLevel, const char* c_szFormat, ...)
{
    if (uLevel < gs_uLevel || !c_szFormat)
        return;

    va_list args;
    va_start(args, c_szFormat);
    std::string formatted = VFormatHelper(c_szFormat, args);
    va_end(args);

    TraceRaw(formatted, true);
}

void Trace(const char* c_szMsg)
{
    TraceRaw(c_szMsg ? c_szMsg : "", false);
}

void Tracen(const char* c_szMsg)
{
    TraceRaw(c_szMsg ? c_szMsg : "", true);
}

void Tracef(const char* c_szFormat, ...)
{
    if (!c_szFormat)
        return;

    va_list args;
    va_start(args, c_szFormat);
    std::string formatted = VFormatHelper(c_szFormat, args);
    va_end(args);

    TraceRaw(formatted, false);
}

void Tracenf(const char* c_szFormat, ...)
{
    if (!c_szFormat)
        return;

    va_list args;
    va_start(args, c_szFormat);
    std::string formatted = VFormatHelper(c_szFormat, args);
    va_end(args);

    TraceRaw(formatted, true);
}

void TraceError(const char* c_szFormat, ...)
{
    if (!c_szFormat)
        return;

    va_list args;
    va_start(args, c_szFormat);
    std::string formatted = VFormatHelper(c_szFormat, args);
    va_end(args);

    TraceErrorRaw(formatted, true);
}

void TraceErrorWithoutEnter(const char* c_szFormat, ...)
{
    if (!c_szFormat)
        return;

    va_list args;
    va_start(args, c_szFormat);
    std::string formatted = VFormatHelper(c_szFormat, args);
    va_end(args);

    TraceErrorRaw(formatted, false);
}

void TempTrace(const char* c_szMsg, bool errType)
{
    TempTraceRaw(c_szMsg ? c_szMsg : "", errType, false);
}

void TempTracen(const char* c_szMsg, bool errType)
{
    TempTraceRaw(c_szMsg ? c_szMsg : "", errType, true);
}

void TempTracef(const char* c_szFormat, bool errType, ...)
{
    if (!c_szFormat)
        return;

    va_list args;
    va_start(args, errType);
    std::string formatted = VFormatHelper(c_szFormat, args);
    va_end(args);

    TempTraceRaw(formatted, errType, false);
}

void TempTracenf(const char* c_szFormat, bool errType, ...)
{
    if (!c_szFormat)
        return;

    va_list args;
    va_start(args, errType);
    std::string formatted = VFormatHelper(c_szFormat, args);
    va_end(args);

    TempTraceRaw(formatted, errType, true);
}

void PacketDump(const char* c_szMsg)
{
    PacketDumpRaw(c_szMsg ? c_szMsg : "");
}

void PacketDumpf(const char* c_szFormat, ...)
{
    if (!c_szFormat)
        return;

    va_list args;
    va_start(args, c_szFormat);
    std::string formatted = VFormatHelper(c_szFormat, args);
    va_end(args);

    PacketDumpRaw(formatted);
}

void LogBox(const char* c_szMsg, const char* c_szCaption, HWND hWnd)
{
    LogBoxRaw(c_szMsg ? c_szMsg : "", c_szCaption ? c_szCaption : "LOG", hWnd);
}

void LogBoxf(const char* c_szFormat, ...)
{
    if (!c_szFormat)
        return;

    va_list args;
    va_start(args, c_szFormat);
    std::string formatted = VFormatHelper(c_szFormat, args);
    va_end(args);

    LogBox(formatted.c_str());
}

void LogFile(const char* c_szMsg)
{
    LogFileRaw(c_szMsg ? c_szMsg : "");
}

void LogFilef(const char* c_szMessage, ...)
{
    if (!c_szMessage)
        return;

    va_list args;
    va_start(args, c_szMessage);
    std::string formatted = VFormatHelper(c_szMessage, args);
    va_end(args);

    LogFileRaw(formatted);
}

void SetupLog(void)
{
    // SetupLog initialization hook
}

void OpenLogFile(bool bUseLogFile)
{
    if (!std::filesystem::exists("log")) {
        std::filesystem::create_directory("log");
    }

    _wfreopen(L"log/syserr.txt", L"w", stderr);

    if (bUseLogFile)
    {
        isLogFile = true;
        CLogFile::Instance().Initialize();
    }

#ifdef _PACKETDUMP
    g_pdlogRequested = bUseLogFile;
    EnsurePacketDumpFiles(g_pdlogRequested);
#endif
}

void CloseLogFile()
{
    // Flush all buffered output before shutdown
    g_syserrBuffer.Flush();
    CLogFile::Instance().Flush();

#ifdef _PACKETDUMP
    if (g_packetDumpEnabled)
        g_packetDumpFile.Flush();
    if (g_pdlogEnabled)
        g_pdlogFile.Flush();
#endif
}

void OpenConsoleWindow()
{
    AllocConsole();

    _wfreopen(L"CONOUT$", L"a", stdout);
    _wfreopen(L"CONIN$", L"r", stdin);
}

void CloseConsoleWindow()
{
    FreeConsole();
}

// ============================================================================
// MODERN ASSERTION ENGINE (std::source_location, ZERO throw "ffs")
// ============================================================================
namespace EterBase {

bool HandleAssertFailure(const char* expr, const std::source_location& loc)
{
    std::string report = std::format(
        "Assertion Failed: ({}) in function '{}', file '{}:{}'",
        expr ? expr : "<unknown>",
        loc.function_name(),
        loc.file_name(),
        loc.line()
    );

    TraceErrorRaw(report, true);

#if defined(_DEBUG)
    if (IsDebuggerPresent())
    {
        __debugbreak();
    }
#endif

    return false;
}

bool HandleAssertFailure(const char* expr, const char* file, int line, const char* function)
{
    std::string report = std::format(
        "Assertion Failed: ({}) in function '{}', file '{}:{}'",
        expr ? expr : "<unknown>",
        function ? function : "<unknown>",
        file ? file : "<unknown>",
        line
    );

    TraceErrorRaw(report, true);

#if defined(_DEBUG)
    if (IsDebuggerPresent())
    {
        __debugbreak();
    }
#endif

    return false;
}

} // namespace EterBase
