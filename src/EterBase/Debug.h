#ifndef __INC_ETERLIB_DEBUG_H__
#define __INC_ETERLIB_DEBUG_H__

#include <windows.h>
#include <format>
#include <string_view>
#include <string>
#include <source_location>

#if defined(_DEBUG) && !defined(_PACKETDUMP)
#define _PACKETDUMP
#endif

// ============================================================================
// LOG LEVELS I KONTROLA POZIOMU LOGOWANIA
// ============================================================================
extern void SetLogLevel(UINT uLevel);
extern UINT GetLogLevel();

// ============================================================================
// BEZPIECZNE FUNKCJE NISKOPOZIOMOWE (std::string_view, ZERO BUFFER OVERFLOW)
// ============================================================================
extern void TraceRaw(std::string_view msg, bool appendNewline = false);
extern void TraceErrorRaw(std::string_view msg, bool appendNewline = true);
extern void LogRaw(UINT uLevel, std::string_view msg, bool appendNewline = false);
extern void LogFileRaw(std::string_view msg);
extern void LogBoxRaw(std::string_view msg, std::string_view caption = "LOG", HWND hWnd = NULL);
extern void PacketDumpRaw(std::string_view msg);
extern void TempTraceRaw(std::string_view msg, bool errType = false, bool appendNewline = false);

// ============================================================================
// NOWOCZESNE SZABLONY std::format (C++20/C++23) Z WALIDACJA W CZASIE KOMPILACJI
// ============================================================================

template <typename... Args>
void TraceErrorFmt(std::format_string<Args...> fmt, Args&&... args)
{
    try
    {
        std::string s = std::format(fmt, std::forward<Args>(args)...);
        TraceErrorRaw(s, true);
    }
    catch (const std::exception& e)
    {
        TraceErrorRaw(std::format("[TraceErrorFmt exception: {}]", e.what()), true);
    }
    catch (...)
    {
        TraceErrorRaw("[TraceErrorFmt unknown exception]", true);
    }
}

template <typename... Args>
void TraceErrorWithoutEnterFmt(std::format_string<Args...> fmt, Args&&... args)
{
    try
    {
        std::string s = std::format(fmt, std::forward<Args>(args)...);
        TraceErrorRaw(s, false);
    }
    catch (const std::exception& e)
    {
        TraceErrorRaw(std::format("[TraceErrorWithoutEnterFmt exception: {}]", e.what()), false);
    }
    catch (...)
    {
        TraceErrorRaw("[TraceErrorWithoutEnterFmt unknown exception]", false);
    }
}

template <typename... Args>
void TraceFmt(std::format_string<Args...> fmt, Args&&... args)
{
    try
    {
        std::string s = std::format(fmt, std::forward<Args>(args)...);
        TraceRaw(s, false);
    }
    catch (const std::exception& e)
    {
        TraceErrorRaw(std::format("[TraceFmt exception: {}]", e.what()), true);
    }
    catch (...)
    {
        TraceErrorRaw("[TraceFmt unknown exception]", true);
    }
}

template <typename... Args>
void TracenFmt(std::format_string<Args...> fmt, Args&&... args)
{
    try
    {
        std::string s = std::format(fmt, std::forward<Args>(args)...);
        TraceRaw(s, true);
    }
    catch (const std::exception& e)
    {
        TraceErrorRaw(std::format("[TracenFmt exception: {}]", e.what()), true);
    }
    catch (...)
    {
        TraceErrorRaw("[TracenFmt unknown exception]", true);
    }
}

template <typename... Args>
void LogFmt(UINT uLevel, std::format_string<Args...> fmt, Args&&... args)
{
    if (uLevel < GetLogLevel())
        return;

    try
    {
        std::string s = std::format(fmt, std::forward<Args>(args)...);
        LogRaw(uLevel, s, false);
    }
    catch (const std::exception& e)
    {
        TraceErrorRaw(std::format("[LogFmt exception: {}]", e.what()), true);
    }
    catch (...)
    {
        TraceErrorRaw("[LogFmt unknown exception]", true);
    }
}

template <typename... Args>
void LognFmt(UINT uLevel, std::format_string<Args...> fmt, Args&&... args)
{
    if (uLevel < GetLogLevel())
        return;

    try
    {
        std::string s = std::format(fmt, std::forward<Args>(args)...);
        LogRaw(uLevel, s, true);
    }
    catch (const std::exception& e)
    {
        TraceErrorRaw(std::format("[LognFmt exception: {}]", e.what()), true);
    }
    catch (...)
    {
        TraceErrorRaw("[LognFmt unknown exception]", true);
    }
}

template <typename... Args>
void LogFileFmt(std::format_string<Args...> fmt, Args&&... args)
{
    try
    {
        std::string s = std::format(fmt, std::forward<Args>(args)...);
        LogFileRaw(s);
    }
    catch (const std::exception& e)
    {
        TraceErrorRaw(std::format("[LogFileFmt exception: {}]", e.what()), true);
    }
    catch (...)
    {
        TraceErrorRaw("[LogFileFmt unknown exception]", true);
    }
}

template <typename... Args>
void LogBoxFmt(std::format_string<Args...> fmt, Args&&... args)
{
    try
    {
        std::string s = std::format(fmt, std::forward<Args>(args)...);
        LogBoxRaw(s);
    }
    catch (const std::exception& e)
    {
        TraceErrorRaw(std::format("[LogBoxFmt exception: {}]", e.what()), true);
    }
    catch (...)
    {
        TraceErrorRaw("[LogBoxFmt unknown exception]", true);
    }
}

template <typename... Args>
void PacketDumpFmt(std::format_string<Args...> fmt, Args&&... args)
{
#ifdef _PACKETDUMP
    try
    {
        std::string s = std::format(fmt, std::forward<Args>(args)...);
        PacketDumpRaw(s);
    }
    catch (const std::exception& e)
    {
        TraceErrorRaw(std::format("[PacketDumpFmt exception: {}]", e.what()), true);
    }
    catch (...)
    {
        TraceErrorRaw("[PacketDumpFmt unknown exception]", true);
    }
#else
    (void)sizeof...(args);
#endif
}

template <typename... Args>
void TempTraceFmt(bool errType, std::format_string<Args...> fmt, Args&&... args)
{
    try
    {
        std::string s = std::format(fmt, std::forward<Args>(args)...);
        TempTraceRaw(s, errType, false);
    }
    catch (const std::exception& e)
    {
        TraceErrorRaw(std::format("[TempTraceFmt exception: {}]", e.what()), true);
    }
    catch (...)
    {
        TraceErrorRaw("[TempTraceFmt unknown exception]", true);
    }
}

template <typename... Args>
void TempTracenFmt(bool errType, std::format_string<Args...> fmt, Args&&... args)
{
    try
    {
        std::string s = std::format(fmt, std::forward<Args>(args)...);
        TempTraceRaw(s, errType, true);
    }
    catch (const std::exception& e)
    {
        TraceErrorRaw(std::format("[TempTracenFmt exception: {}]", e.what()), true);
    }
    catch (...)
    {
        TraceErrorRaw("[TempTracenFmt unknown exception]", true);
    }
}

// ============================================================================
// 100% KOMPATYBILNOSC WSTECZNA DLA ISTNIEJACYCH WYWOLAN C-STYLE (printf / va_list)
// ============================================================================
extern void Log(UINT uLevel, const char* c_szMsg);
extern void Logn(UINT uLevel, const char* c_szMsg);
extern void Logf(UINT uLevel, const char* c_szFormat, ...);
extern void Lognf(UINT uLevel, const char* c_szFormat, ...);

extern void Trace(const char* c_szMsg);
extern void Tracen(const char* c_szMsg);
extern void Tracenf(const char* c_szFormat, ...);
extern void Tracef(const char* c_szFormat, ...);
extern void TraceError(const char* c_szFormat, ...);
extern void TraceErrorWithoutEnter(const char* c_szFormat, ...);

// MR-11: Separate packet dump log from the main log file
extern void TempTrace(const char* c_szMsg, bool errType = false);
extern void TempTracef(const char* c_szFormat, bool errType = false, ...);
extern void TempTracen(const char* c_szMsg, bool errType = false);
extern void TempTracenf(const char* c_szFormat, bool errType = false, ...);

extern void PacketDump(const char* c_szMsg);
extern void PacketDumpf(const char* c_szFormat, ...);
// MR-11: -- END OF -- Separate packet dump log from the main log file

extern void LogBox(const char* c_szMsg, const char* c_szCaption = NULL, HWND hWnd = NULL);
extern void LogBoxf(const char* c_szMsg, ...);

extern void LogFile(const char* c_szMsg);
extern void LogFilef(const char* c_szMessage, ...);
extern void OpenConsoleWindow(void);
extern void CloseConsoleWindow();
extern void SetupLog(void);

extern void OpenLogFile(bool bUseLogFile = true);
extern void CloseLogFile();

extern HWND g_PopupHwnd;

#define CHECK_RETURN(flag, string)          \
    if (flag)                               \
    {                                       \
        LogBox(string);                     \
        return;                             \
    }

// ============================================================================
// NOWOCZESNA ASERCJA (std::source_location, CZYSTY ZRZUT BLEDU, BEZ throw "ffs")
// ============================================================================
namespace EterBase {
    bool HandleAssertFailure(const char* expr, const std::source_location& loc);
    bool HandleAssertFailure(const char* expr, const char* file, int line, const char* function = "");
}

#define MD_ASSERT(expr) ((expr) ? true : (::EterBase::HandleAssertFailure(#expr, ::std::source_location::current()), false))

#endif // __INC_ETERLIB_DEBUG_H__
