#include "CrashSentinel.h"
#include <DbgHelp.h>
#include <format>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <iostream>

#pragma comment(lib, "Dbghelp.lib")

namespace UserInterface::TestHarness
{
    CrashSentinel& CrashSentinel::Instance() noexcept
    {
        static CrashSentinel s_instance;
        return s_instance;
    }

    CrashSentinel::~CrashSentinel()
    {
        Shutdown();
    }

    bool CrashSentinel::Initialize(std::string_view reportPath)
    {
        if (m_isInitialized.load(std::memory_order_relaxed))
            return true;

        SetReportPath(reportPath);

        // Inicjalizacja DbgHelp z opcjami ladowania linii i symboli
        HANDLE hProcess = GetCurrentProcess();
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
        SymInitialize(hProcess, nullptr, TRUE);

        // Rejestracja wektorowego handlera wyjatkow na poczatku lancucha (First = 1)
        m_pHandlerHandle = AddVectoredExceptionHandler(1, VectoredExceptionHandler);
        if (!m_pHandlerHandle)
        {
            return false;
        }

        m_isInitialized.store(true, std::memory_order_relaxed);
        return true;
    }

    void CrashSentinel::Shutdown()
    {
        if (!m_isInitialized.exchange(false, std::memory_order_relaxed))
            return;

        if (m_pHandlerHandle)
        {
            RemoveVectoredExceptionHandler(m_pHandlerHandle);
            m_pHandlerHandle = nullptr;
        }

        SymCleanup(GetCurrentProcess());
    }

    void CrashSentinel::SetReportPath(std::string_view reportPath)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_reportPath = std::string(reportPath);
    }

    std::string CrashSentinel::GetLastCrashJson() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_lastCrashJson;
    }

    LONG WINAPI CrashSentinel::VectoredExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo)
    {
        return Instance().HandleException(pExceptionInfo);
    }

    LONG CrashSentinel::HandleException(PEXCEPTION_POINTERS pExceptionInfo)
    {
        if (!pExceptionInfo || !pExceptionInfo->ExceptionRecord)
            return EXCEPTION_CONTINUE_SEARCH;

        const DWORD code = pExceptionInfo->ExceptionRecord->ExceptionCode;

        // Filtrujemy tylko krytyczne bledy pamieci i naruszenia dostepu
        // Ignorujemy wyjatki C++ (0xE06D7363) i wyjatki debuggera
        if (code != EXCEPTION_ACCESS_VIOLATION &&
            code != EXCEPTION_ARRAY_BOUNDS_EXCEEDED &&
            code != EXCEPTION_ILLEGAL_INSTRUCTION &&
            code != EXCEPTION_IN_PAGE_ERROR &&
            code != EXCEPTION_STACK_OVERFLOW &&
            code != STATUS_INTEGER_DIVIDE_BY_ZERO)
        {
            return EXCEPTION_CONTINUE_SEARCH;
        }

        // Zabezpieczenie przed reentrancy / zapetleniem przy awarii w samym handlerze
        bool expected = false;
        if (!m_isHandlingCrash.compare_exchange_strong(expected, true))
        {
            return EXCEPTION_CONTINUE_SEARCH;
        }

        m_hasCrashed.store(true, std::memory_order_relaxed);

        std::string jsonReport = BuildCrashJson(pExceptionInfo);

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_lastCrashJson = jsonReport;
        }

        WriteReportToFile(jsonReport);

        m_isHandlingCrash.store(false, std::memory_order_relaxed);

        return EXCEPTION_CONTINUE_SEARCH;
    }

    std::string CrashSentinel::BuildCrashJson(PEXCEPTION_POINTERS pExceptionInfo)
    {
        const auto* pRecord = pExceptionInfo->ExceptionRecord;
        const DWORD code = pRecord->ExceptionCode;
        const void* pAddress = pRecord->ExceptionAddress;
        const DWORD threadId = GetCurrentThreadId();

        std::string exceptionName;
        switch (code)
        {
        case EXCEPTION_ACCESS_VIOLATION:
            exceptionName = "EXCEPTION_ACCESS_VIOLATION";
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            exceptionName = "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
            break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            exceptionName = "EXCEPTION_ILLEGAL_INSTRUCTION";
            break;
        case EXCEPTION_IN_PAGE_ERROR:
            exceptionName = "EXCEPTION_IN_PAGE_ERROR";
            break;
        case EXCEPTION_STACK_OVERFLOW:
            exceptionName = "EXCEPTION_STACK_OVERFLOW";
            break;
        case STATUS_INTEGER_DIVIDE_BY_ZERO:
            exceptionName = "EXCEPTION_INT_DIVIDE_BY_ZERO";
            break;
        default:
            exceptionName = std::format("0x{:08X}", code);
            break;
        }

        // Szczegoly dla EXCEPTION_ACCESS_VIOLATION
        std::string avType = "N/A";
        std::string faultingAddress = "0x0";
        if (code == EXCEPTION_ACCESS_VIOLATION && pRecord->NumberParameters >= 2)
        {
            const ULONG_PTR accessType = pRecord->ExceptionInformation[0];
            const ULONG_PTR targetAddr = pRecord->ExceptionInformation[1];
            if (accessType == 0)
                avType = "READ";
            else if (accessType == 1)
                avType = "WRITE";
            else if (accessType == 8)
                avType = "EXECUTE_DEP";
            else
                avType = std::format("UNKNOWN_{}", accessType);

            faultingAddress = std::format("0x{:016X}", static_cast<uint64_t>(targetAddr));
        }

        // Timestamp ISO-8601
        const auto now = std::chrono::system_clock::now();
        const auto timePoint = std::chrono::system_clock::to_time_t(now);
        std::tm tmBuffer{};
        gmtime_s(&tmBuffer, &timePoint);
        char timeStr[64] = {0};
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%dT%H:%M:%SZ", &tmBuffer);

        // Rozwijanie stosu za pomoca CaptureStackBackTrace i symboli DbgHelp
        constexpr USHORT MAX_FRAMES = 64;
        void* backTrace[MAX_FRAMES] = {nullptr};
        const USHORT framesCaptured = CaptureStackBackTrace(0, MAX_FRAMES, backTrace, nullptr);

        HANDLE hProcess = GetCurrentProcess();

        std::string stackJson = "[\n";
        alignas(SYMBOL_INFO) char symbolBuffer[sizeof(SYMBOL_INFO) + 512 * sizeof(char)] = {0};
        auto* pSymbol = reinterpret_cast<PSYMBOL_INFO>(symbolBuffer);
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = 511;

        for (USHORT i = 0; i < framesCaptured; ++i)
        {
            const auto addr = reinterpret_cast<DWORD64>(backTrace[i]);
            DWORD64 displacement = 0;

            std::string symbolName = "unknown";
            if (SymFromAddr(hProcess, addr, &displacement, pSymbol))
            {
                symbolName = pSymbol->Name;
            }

            std::string fileName = "";
            DWORD lineNum = 0;
            IMAGEHLP_LINE64 lineInfo{};
            lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            DWORD lineDisplacement = 0;
            if (SymGetLineFromAddr64(hProcess, addr, &lineDisplacement, &lineInfo))
            {
                fileName = lineInfo.FileName ? lineInfo.FileName : "";
                lineNum = lineInfo.LineNumber;
            }

            // Normalizacja sciezki pliku
            for (char& c : fileName)
            {
                if (c == '\\') c = '/';
            }

            // Pobranie nazwy modulu DLL / EXE
            std::string moduleName = "unknown";
            HMODULE hModule = nullptr;
            if (GetModuleHandleExA(
                    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    reinterpret_cast<LPCSTR>(addr),
                    &hModule))
            {
                char modPath[MAX_PATH] = {0};
                if (GetModuleFileNameA(hModule, modPath, MAX_PATH))
                {
                    moduleName = std::filesystem::path(modPath).filename().string();
                }
            }

            stackJson += std::format(
                "    {{\n"
                "      \"frame\": {},\n"
                "      \"address\": \"0x{:016X}\",\n"
                "      \"module\": \"{}\",\n"
                "      \"symbol\": \"{}\",\n"
                "      \"displacement\": \"0x{:X}\",\n"
                "      \"file\": \"{}\",\n"
                "      \"line\": {}\n"
                "    }}{}",
                i,
                addr,
                moduleName,
                symbolName,
                displacement,
                fileName,
                lineNum,
                (i + 1 < framesCaptured) ? ",\n" : "\n"
            );
        }
        stackJson += "  ]";

        // Skladanie kompletnego dokumentu JSON
        return std::format(
            "{{\n"
            "  \"crash_sentinel\": {{\n"
            "    \"exception_code\": \"0x{:08X}\",\n"
            "    \"exception_name\": \"{}\",\n"
            "    \"instruction_address\": \"0x{:016X}\",\n"
            "    \"thread_id\": {},\n"
            "    \"timestamp\": \"{}\",\n"
            "    \"access_violation_type\": \"{}\",\n"
            "    \"faulting_address\": \"{}\",\n"
            "    \"stack_trace\": {}\n"
            "  }}\n"
            "}}\n",
            code,
            exceptionName,
            reinterpret_cast<uint64_t>(pAddress),
            threadId,
            timeStr,
            avType,
            faultingAddress,
            stackJson
        );
    }

    void CrashSentinel::WriteReportToFile(const std::string& jsonContent)
    {
        std::string path;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            path = m_reportPath;
        }

        try
        {
            std::ofstream outFile(path, std::ios::out | std::ios::trunc);
            if (outFile.is_open())
            {
                outFile << jsonContent;
                outFile.flush();
                outFile.close();
            }

            // Opcjonalna kopia zapasowa w katalogu log/
            if (std::filesystem::exists("log"))
            {
                std::ofstream logFile("log/crash_sentinel.json", std::ios::out | std::ios::trunc);
                if (logFile.is_open())
                {
                    logFile << jsonContent;
                    logFile.flush();
                    logFile.close();
                }
            }
        }
        catch (...)
        {
            // Bezpieczenstwo wewnatrz obslugi awarii - pomijamy rzucanie wyjatkow
        }
    }
}
