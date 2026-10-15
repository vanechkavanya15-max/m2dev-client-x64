#if !defined(TEST_MODE_DISABLE_STDAFX)
#include "StdAfx.h"
#else
#include <windows.h>
#include <EterBase/CRC32.h>
#include <EterBase/Debug.h>
#endif
#include "ProcessScanner.h"

#include <tlhelp32.h>
#include <utf8.h>

#include <thread>
#include <stop_token>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <random>
#include <map>
#include <vector>
#include <utility>

namespace
{
    // RAII opakowanie dla uchwytow Win32 zapobiegajace wyciekom zasobow
    struct ScopedHandle
    {
        HANDLE m_handle{INVALID_HANDLE_VALUE};

        explicit ScopedHandle(HANDLE h = INVALID_HANDLE_VALUE) noexcept
            : m_handle(h)
        {
        }

        ~ScopedHandle() noexcept
        {
            Reset();
        }

        ScopedHandle(const ScopedHandle&) = delete;
        ScopedHandle& operator=(const ScopedHandle&) = delete;

        ScopedHandle(ScopedHandle&& other) noexcept
            : m_handle(other.m_handle)
        {
            other.m_handle = INVALID_HANDLE_VALUE;
        }

        ScopedHandle& operator=(ScopedHandle&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                m_handle = other.m_handle;
                other.m_handle = INVALID_HANDLE_VALUE;
            }
            return *this;
        }

        void Reset(HANDLE h = INVALID_HANDLE_VALUE) noexcept
        {
            if (m_handle != INVALID_HANDLE_VALUE && m_handle != nullptr)
            {
                CloseHandle(m_handle);
            }
            m_handle = h;
        }

        [[nodiscard]] bool IsValid() const noexcept
        {
            return m_handle != INVALID_HANDLE_VALUE && m_handle != nullptr;
        }

        [[nodiscard]] HANDLE Get() const noexcept
        {
            return m_handle;
        }

        operator HANDLE() const noexcept
        {
            return m_handle;
        }
    };

    std::vector<CRCPair> gs_kVct_crcPair;
    std::mutex gs_queueMutex;

    std::condition_variable_any gs_cvDelay;
    std::mutex gs_cvMutex;

    std::jthread gs_workerThread;
}

static void ScanProcessList(const std::stop_token& stoken, std::map<DWORD, DWORD>& rkMap_crcProc, std::vector<CRCPair>* pkVct_crcPair)
{
    if (!pkVct_crcPair || stoken.stop_requested())
        return;

    SYSTEM_INFO si{};
    GetSystemInfo(&si);

    PROCESSENTRY32W pro{};
    pro.dwSize = sizeof(PROCESSENTRY32W);

    ScopedHandle processSnap(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!processSnap.IsValid())
        return;

    BOOL bOK = Process32FirstW(processSnap, &pro);

    while (bOK)
    {
        if (stoken.stop_requested())
            break;

        ScopedHandle hProc(OpenProcess(PROCESS_VM_READ, FALSE, pro.th32ProcessID));
        if (hProc.IsValid())
        {
            ScopedHandle hModuleSnap(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pro.th32ProcessID));
            if (hModuleSnap.IsValid())
            {
                MODULEENTRY32W me32{};
                me32.dwSize = sizeof(MODULEENTRY32W);

                BOOL bRet = Module32FirstW(hModuleSnap, &me32);
                while (bRet)
                {
                    if (stoken.stop_requested())
                        break;

                    // Sciezka wide exe -> UTF-8 bajty dla CRC
                    std::string exePathUtf8 = WideToUtf8(me32.szExePath);

                    DWORD crcExtPath = GetCRC32(exePathUtf8.c_str(), static_cast<int>(exePathUtf8.size()));

                    auto f = rkMap_crcProc.find(crcExtPath);
                    if (f == rkMap_crcProc.end())
                    {
                        DWORD crcProc = GetFileCRC32(me32.szExePath);
                        rkMap_crcProc.insert(std::make_pair(crcExtPath, crcProc));

                        pkVct_crcPair->push_back(std::make_pair(crcProc, exePathUtf8));
                    }

                    // Bezpieczne, natychmiast przerywalne usypianie zamiast archaicznego Sleep(1)
                    {
                        std::unique_lock<std::mutex> lock(gs_cvMutex);
                        if (gs_cvDelay.wait_for(lock, stoken, std::chrono::milliseconds(1), [&stoken]() {
                            return stoken.stop_requested();
                        }))
                        {
                            break;
                        }
                    }

                    me32.dwSize = sizeof(MODULEENTRY32W);
                    bRet = Module32NextW(hModuleSnap, &me32);
                }
            }
        }

        pro.dwSize = sizeof(PROCESSENTRY32W);
        bOK = Process32NextW(processSnap, &pro);
    }
}

static void ProcessScanner_Thread(std::stop_token stoken)
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<uint32_t> initialDist(10000, 19000);
    std::uniform_int_distribution<uint32_t> loopDist(1000, 10000);

    uint32_t dwDelay = initialDist(rng);

    std::map<DWORD, DWORD> kMap_crcProc;
    std::vector<CRCPair> kVct_crcPair;

    while (!stoken.stop_requested())
    {
        // Oczekiwanie na uplyw czasu z natychmiastowym wybudzeniem przez stoken
        {
            std::unique_lock<std::mutex> lock(gs_cvMutex);
            if (gs_cvDelay.wait_for(lock, stoken, std::chrono::milliseconds(dwDelay), [&stoken]() {
                return stoken.stop_requested();
            }))
            {
                break;
            }
        }

        if (stoken.stop_requested())
            break;

        kVct_crcPair.clear();
        ScanProcessList(stoken, kMap_crcProc, &kVct_crcPair);

        if (!kVct_crcPair.empty())
        {
            std::lock_guard<std::mutex> lock(gs_queueMutex);
            gs_kVct_crcPair.insert(gs_kVct_crcPair.end(),
                                  std::make_move_iterator(kVct_crcPair.begin()),
                                  std::make_move_iterator(kVct_crcPair.end()));
        }

        dwDelay = loopDist(rng);
    }
}

void ProcessScanner_ReleaseQuitEvent()
{
    if (gs_workerThread.joinable())
    {
        gs_workerThread.request_stop();
        gs_cvDelay.notify_all();
    }
}

void ProcessScanner_Destroy()
{
    ProcessScanner_ReleaseQuitEvent();

    if (gs_workerThread.joinable())
    {
        gs_workerThread.request_stop();
        gs_cvDelay.notify_all();
        gs_workerThread.join();
    }

    std::lock_guard<std::mutex> lock(gs_queueMutex);
    gs_kVct_crcPair.clear();
}

bool ProcessScanner_PopProcessQueue(std::vector<CRCPair>* pkVct_crcPair)
{
    if (!pkVct_crcPair)
        return false;

    {
        std::lock_guard<std::mutex> lock(gs_queueMutex);
        *pkVct_crcPair = std::move(gs_kVct_crcPair);
        gs_kVct_crcPair.clear();
    }

    return !pkVct_crcPair->empty();
}

bool ProcessScanner_Create()
{
    if (gs_workerThread.joinable())
    {
        return true;
    }

    try
    {
        gs_workerThread = std::jthread(ProcessScanner_Thread);
        return true;
    }
    catch (const std::exception&)
    {
        LogBox("ProcessScanner_Create failed");
        return false;
    }
}
