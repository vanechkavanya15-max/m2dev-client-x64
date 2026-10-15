// FastBootOverlay.cpp
// Implementacja kontrolera Fast-Boot / Direct-to-World (Dear ImGui DX9)

#include "StdAfx.h"
#include "FastBootOverlay.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx9.h"
#include "AccountConnector.h"
#include "PythonNetworkStream.h"
#include "Client/Network/Protocol/ProtocolDriverRegistry.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <fstream>
#include <filesystem>
#include <algorithm>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace UserInterface::FastBoot
{
    FastBootOverlay& FastBootOverlay::Instance()
    {
        static FastBootOverlay s_instance;
        return s_instance;
    }

    FastBootOverlay::FastBootOverlay()
    {
        // Domyslna lista profili serwerow
        m_profiles = {
            {"Luria - CH4 [Zalecany]", "luria", "46.105.73.82", 11469, 29301},
            {"Luria - CH1",            "luria", "46.105.73.82", 11469, 29001},
            {"Luria - CH2",            "luria", "46.105.73.82", 11469, 29101},
            {"Luria - CH3",            "luria", "46.105.73.82", 11469, 29201},
            {"Luria - CH5",            "luria", "46.105.73.82", 11469, 29401},
            {"Luria - CH6",            "luria", "46.105.73.82", 11469, 29501},
            {"Standard x64 - CH1",     "standard_x64", "127.0.0.1", 11000, 11011},
            {"Standard x64 - CH2",     "standard_x64", "127.0.0.1", 11000, 11021},
            {"Beavium - CH1",          "beavium", "127.0.0.1", 11000, 11011}
        };

        LoadConfig();
    }

    FastBootOverlay::~FastBootOverlay()
    {
        Shutdown();
    }

    void FastBootOverlay::ApplyStyle()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 8.0f;
        style.FrameRounding = 5.0f;
        style.PopupRounding = 6.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding = 5.0f;
        style.TabRounding = 5.0f;

        style.WindowPadding = ImVec2(16.0f, 16.0f);
        style.FramePadding = ImVec2(10.0f, 6.0f);
        style.ItemSpacing = ImVec2(10.0f, 10.0f);

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_WindowBg]             = ImVec4(0.08f, 0.09f, 0.12f, 0.94f);
        colors[ImGuiCol_Border]               = ImVec4(0.20f, 0.25f, 0.35f, 0.60f);
        colors[ImGuiCol_FrameBg]              = ImVec4(0.14f, 0.16f, 0.22f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.20f, 0.24f, 0.34f, 1.00f);
        colors[ImGuiCol_FrameBgActive]        = ImVec4(0.26f, 0.30f, 0.42f, 1.00f);
        colors[ImGuiCol_TitleBg]              = ImVec4(0.10f, 0.12f, 0.18f, 1.00f);
        colors[ImGuiCol_TitleBgActive]        = ImVec4(0.14f, 0.18f, 0.28f, 1.00f);
        colors[ImGuiCol_Button]               = ImVec4(0.16f, 0.40f, 0.28f, 1.00f); // Subtelna zielen
        colors[ImGuiCol_ButtonHovered]        = ImVec4(0.22f, 0.52f, 0.36f, 1.00f);
        colors[ImGuiCol_ButtonActive]         = ImVec4(0.12f, 0.32f, 0.22f, 1.00f);
        colors[ImGuiCol_Header]               = ImVec4(0.20f, 0.25f, 0.38f, 1.00f);
        colors[ImGuiCol_HeaderHovered]        = ImVec4(0.26f, 0.34f, 0.50f, 1.00f);
        colors[ImGuiCol_HeaderActive]         = ImVec4(0.32f, 0.42f, 0.60f, 1.00f);
        colors[ImGuiCol_CheckMark]            = ImVec4(0.35f, 0.75f, 0.45f, 1.00f);
        colors[ImGuiCol_Text]                 = ImVec4(0.92f, 0.94f, 0.96f, 1.00f);
    }

    bool FastBootOverlay::Initialize(HWND hWnd, IDirect3DDevice9* pDevice)
    {
        if (m_bInitialized)
            return true;

        if (!hWnd || !pDevice)
            return false;

        m_hWnd = hWnd;
        m_pDevice = pDevice;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        io.IniFilename = nullptr; // Bez zbednych plikow imgui.ini

        ApplyStyle();

        if (!ImGui_ImplWin32_Init(hWnd))
            return false;

        if (!ImGui_ImplDX9_Init(pDevice))
        {
            ImGui_ImplWin32_Shutdown();
            return false;
        }

        m_bInitialized = true;
        m_statusMessage = "Gotowy do polaczenia";
        m_statusColor = 0xFF88CC88;

        if (m_selectedProfileIdx >= 0 && m_selectedProfileIdx < static_cast<int>(m_profiles.size()))
        {
            Network::Protocol::ProtocolDriverRegistry::Instance().SetActiveDriver(m_profiles[m_selectedProfileIdx].driverName);
        }

        extern bool g_bFastBootAutoConnect;
        extern bool g_bHeadlessMode;
        if (g_bFastBootAutoConnect && !g_bHeadlessMode)
        {
            ConnectCurrentProfile();
        }

        return true;
    }

    void FastBootOverlay::Shutdown()
    {
        if (!m_bInitialized)
            return;

        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        m_bInitialized = false;
        m_hWnd = nullptr;
        m_pDevice = nullptr;
    }

    void FastBootOverlay::OnLostDevice()
    {
        if (m_bInitialized)
            ImGui_ImplDX9_InvalidateDeviceObjects();
    }

    void FastBootOverlay::OnResetDevice()
    {
        if (m_bInitialized)
            ImGui_ImplDX9_CreateDeviceObjects();
    }

    bool FastBootOverlay::ProcessWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_KEYDOWN && wParam == VK_F11)
        {
            ToggleVisible();
            return true;
        }

        if (m_bInitialized && m_bVisible)
        {
            if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
                return true;

            ImGuiIO& io = ImGui::GetIO();
            if ((io.WantCaptureMouse && (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)) ||
                (io.WantCaptureKeyboard && (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST)))
            {
                return true;
            }
        }

        return false;
    }

    void FastBootOverlay::SetAutoReconnect(bool bEnable, DWORD delayMs, int maxAttempts) noexcept
    {
        m_bAutoReconnect = bEnable;
        m_reconnectDelayMs = delayMs;
        m_maxReconnectAttempts = maxAttempts;
        if (!bEnable)
        {
            m_bPendingReconnect = false;
        }
    }

    void FastBootOverlay::TriggerAutoReconnect()
    {
        m_bPendingReconnect = true;
        m_reconnectTriggerTime = GetTickCount() + m_reconnectDelayMs;
        m_bVisible = true;
        m_statusColor = 0xFFFFAA00;
        m_statusMessage = std::format("Wymuszono ponowne polaczenie za {:.1f}s...", static_cast<float>(m_reconnectDelayMs) / 1000.0f);
    }

    void FastBootOverlay::CancelAutoReconnect()
    {
        m_bPendingReconnect = false;
        m_reconnectAttempts = 0;
        m_statusMessage = "Auto-reconnect anulowany.";
        m_statusColor = 0xFFCCCCCC;
    }

    void FastBootOverlay::OnRemoteDisconnect()
    {
        TraceError("[FAST_BOOT] OnRemoteDisconnect zgloszone przez siec (m_bWasInGame=%d, m_bAutoReconnect=%d)", (int)m_bWasInGame, (int)m_bAutoReconnect);
        if (m_bWasInGame && m_bAutoReconnect)
        {
            if (m_maxReconnectAttempts == 0 || m_reconnectAttempts < m_maxReconnectAttempts)
            {
                m_bPendingReconnect = true;
                m_reconnectTriggerTime = GetTickCount() + m_reconnectDelayMs;
                m_reconnectAttempts++;
                m_bVisible = true;
                m_statusColor = 0xFFFFAA00;
                m_statusMessage = std::format("Polaczenie zerwane (Kick/Disconnect)! Auto-reconnect ({}/{}) za {:.1f}s...",
                    m_reconnectAttempts, m_maxReconnectAttempts, static_cast<float>(m_reconnectDelayMs) / 1000.0f);
            }
            else
            {
                m_statusMessage = std::format("Osiagnieto limit prob reconnectu ({})! Polacz recznie.", m_maxReconnectAttempts);
                m_statusColor = 0xFFFF4444;
                m_bWasInGame = false;
                m_bVisible = true;
            }
        }
        else
        {
            m_statusMessage = "Polaczenie z serwerem zostalo zerwane (Remote Disconnect).";
            m_statusColor = 0xFFFF4444;
            m_bVisible = true;
        }
    }

    void FastBootOverlay::SetLoginFailure(const char* szStatus)
    {
        m_bConnecting = false;
        m_connectStartTime = 0;
        m_bVisible = true;
        m_statusColor = 0xFFFF4444; // Czerwony kolor bledu

        std::string status = (szStatus && szStatus[0] != '\0') ? szStatus : "UNKNOWN";

        if (status == "ALREADY")
        {
            m_statusMessage = "Blad: Konto jest juz zalogowane na serwerze! (ALREADY). Poczekaj 15 sekund i sprobuj ponownie.";
        }
        else if (status == "WRONGPWD")
        {
            m_statusMessage = "Blad: Nieprawidlowy login lub haslo! (WRONGPWD)";
        }
        else if (status == "NOID")
        {
            m_statusMessage = "Blad: Konto o podanym loginie nie istnieje! (NOID)";
        }
        else if (status == "BLOCK")
        {
            m_statusMessage = "Blad: To konto zostalo zablokowane! (BLOCK)";
        }
        else if (status == "WRONGMAT")
        {
            m_statusMessage = "Blad: Nieprawidlowy PIN lub kod usuniecia postaci! (WRONGMAT)";
        }
        else if (status == "BESAMEKEY")
        {
            m_statusMessage = "Blad: Klucz sesji zostal juz wykorzystany! (BESAMEKEY)";
        }
        else if (status == "CONNECT_FAIL")
        {
            m_statusMessage = "Blad: Nie udalo sie polaczyc z serwerem (Serwer wylaczony lub nieosiagalny)!";
        }
        else if (status == "SHUTDOWN")
        {
            m_statusMessage = "Blad: Polaczenie zostalo zerwane przez serwer (SHUTDOWN / Restart serwera)!";
        }
        else if (status == "TIMEOUT")
        {
            m_statusMessage = "Blad: Uplynal limit czasu odpowiedzi serwera (Timeout 15s)! Sprawdz stan serwera.";
        }
        else
        {
            m_statusMessage = std::format("Blad logowania z serwera: {}!", status);
        }

        if (m_bWasInGame && m_bAutoReconnect && (status == "CONNECT_FAIL" || status == "SHUTDOWN" || status == "TIMEOUT"))
        {
            if (m_maxReconnectAttempts == 0 || m_reconnectAttempts < m_maxReconnectAttempts)
            {
                m_bPendingReconnect = true;
                m_reconnectTriggerTime = GetTickCount() + m_reconnectDelayMs;
                m_reconnectAttempts++;
                m_statusMessage += std::format(" Auto-reconnect ({}/{}) za {:.1f}s...",
                    m_reconnectAttempts, m_maxReconnectAttempts, static_cast<float>(m_reconnectDelayMs) / 1000.0f);
                m_statusColor = 0xFFFFAA00;
            }
        }

        TraceError("[FAST_BOOT] Login failure zgloszony: '%s' -> '%s'", status.c_str(), m_statusMessage.c_str());
    }

    void FastBootOverlay::UpdateNetworkStatus()
    {
        CPythonNetworkStream& rkNet = CPythonNetworkStream::Instance();
        std::string currentPhase = rkNet.GetPhase();

        // 1. Sprawdzenie watchdog timeoutu 15s podczas lączenia
        if (m_bConnecting && m_connectStartTime != 0)
        {
            if (GetTickCount() - m_connectStartTime > 15000)
            {
                SetLoginFailure("TIMEOUT");
                CAccountConnector::Instance().Disconnect();
                CPythonNetworkStream::Instance().Disconnect();
                return;
            }
        }

        // 1b. Obsluga oczekujacego auto-reconnectu
        if (m_bPendingReconnect)
        {
            DWORD dwNow = GetTickCount();
            if (dwNow >= m_reconnectTriggerTime)
            {
                m_bPendingReconnect = false;
                m_statusMessage = std::format("Ponawianie polaczenia (proba {})...", m_reconnectAttempts);
                m_statusColor = 0xFFFFFF55;
                ConnectCurrentProfile();
                return;
            }
            else
            {
                float remainSec = static_cast<float>(m_reconnectTriggerTime - dwNow) / 1000.0f;
                m_statusMessage = std::format("Polaczenie zerwane! Auto-reconnect ({}) za {:.1f}s...", m_reconnectAttempts, remainSec);
                m_statusColor = 0xFFFFAA00;
            }
        }

        if (currentPhase == "Game")
        {
            m_bConnecting = false;
            m_connectStartTime = 0;
            m_bWasInGame = true;
            m_reconnectAttempts = 0;
            m_bPendingReconnect = false;
            m_statusMessage = "W grze: Faza GAME aktywna!";
            m_statusColor = 0xFF55FF55;

            if (!m_bAutoHidden)
            {
                m_bAutoHidden = true;
                m_bVisible = false;
                m_enterGameTime = 0;
            }
        }
        else
        {
            m_bAutoHidden = false;
            m_enterGameTime = 0;

            // Wykrycie zerwania sesji gdy bylismy w grze
            if (m_bWasInGame && m_bAutoReconnect && !m_bPendingReconnect && !m_bConnecting &&
                (currentPhase == "OffLine" || currentPhase == "Login" || currentPhase.empty()))
            {
                if (m_maxReconnectAttempts == 0 || m_reconnectAttempts < m_maxReconnectAttempts)
                {
                    m_bPendingReconnect = true;
                    m_reconnectTriggerTime = GetTickCount() + m_reconnectDelayMs;
                    m_reconnectAttempts++;
                    m_bVisible = true;
                    m_statusColor = 0xFFFFAA00;
                    m_statusMessage = std::format("Polaczenie zerwane! Auto-reconnect ({}/{}) za {:.1f}s...",
                        m_reconnectAttempts, m_maxReconnectAttempts, static_cast<float>(m_reconnectDelayMs) / 1000.0f);
                    TraceError("[FAST_BOOT] Inicjalizacja auto-reconnect za %u ms (proba %d)", m_reconnectDelayMs, m_reconnectAttempts);
                }
            }

            if (currentPhase == "Loading" || currentPhase == "Load")
            {
                m_statusMessage = "Ladowanie swiata gry (Phase Loading)...";
                m_statusColor = 0xFF55FFFF;
            }
            else if (currentPhase == "Select")
            {
                if (rkNet.IsCharacterSlotEmpty(m_characterSlot))
                {
                    m_bNeedCreateCharacter = true;
                    m_statusMessage = "Brak postaci na slocie! Wypelnij formularz ponizej i kliknij Stworz:";
                    m_statusColor = 0xFFFFAA00;
                }
                else
                {
                    m_bNeedCreateCharacter = false;
                    m_statusMessage = "Faza Select: Automatyczny wybor postaci (DirectEnter)...";
                    m_statusColor = 0xFFFFAA00;
                }
            }
            else if (currentPhase == "Login" && m_bConnecting)
            {
                m_statusMessage = "Autoryzacja udana. Trwa logowanie do serwera gry...";
                m_statusColor = 0xFFFFAA00;
            }
            else if (m_bConnecting)
            {
                m_statusMessage = "Trwa autoryzacja i handshake DH2...";
                m_statusColor = 0xFFFFFF00;
            }
        }
    }

    void FastBootOverlay::CreateCharacterAndEnter()
    {
        CPythonNetworkStream& rkNet = CPythonNetworkStream::Instance();

        // 1. Bezpieczna sanityzacja i walidacja nazwy nowej postaci
        std::string charName(m_szNewCharName);
        while (!charName.empty() && (charName.front() == ' ' || charName.front() == '\t'))
            charName.erase(charName.begin());
        while (!charName.empty() && (charName.back() == ' ' || charName.back() == '\t'))
            charName.pop_back();

        if (charName.empty())
        {
            charName = "NewChar" + std::to_string(GetTickCount() % 10000);
        }
        snprintf(m_szNewCharName, sizeof(m_szNewCharName), "%s", charName.c_str());

        // 2. Walidacja slotu postaci (0..4)
        BYTE bySlot = static_cast<BYTE>(std::clamp(m_characterSlot, 0, 4));

        // 3. Walidacja plci (0 = Male, 1 = Female)
        BYTE byGender = (m_newCharGender == 1) ? 1 : 0;

        // 4. Walidacja ksztaltu postaci (0..1)
        BYTE byShape = static_cast<BYTE>(std::clamp(m_newCharShape, 0, 1));

        // 5. Elastyczna i bezpieczna walidacja m_newCharJob oraz statystyk poczatkowych.
        // Kazda kombinacja statystyk bazowych sumuje sie dokladnie do 16 punktow (wymog protokolu Metin2).
        BYTE byJob = 0;
        BYTE byCON = 4, byINT = 3, bySTR = 6, byDEX = 3; // Domyslne bezpieczne statystyki (Wojownik)

        switch (m_newCharJob)
        {
            case 0: // Wojownik (Warrior)
                byJob = (byGender == 0) ? 0 : 4;
                byCON = 4; byINT = 3; bySTR = 6; byDEX = 3;
                break;

            case 1: // Ninja (Assassin)
                byJob = (byGender == 0) ? 5 : 1;
                byCON = 3; byINT = 3; bySTR = 4; byDEX = 6;
                break;

            case 2: // Sura
                byJob = (byGender == 0) ? 2 : 6;
                byCON = 3; byINT = 5; bySTR = 5; byDEX = 3;
                break;

            case 3: // Szaman (Shaman)
                byJob = (byGender == 0) ? 7 : 3;
                byCON = 4; byINT = 6; bySTR = 3; byDEX = 3;
                break;

            case 4: // Wolfman / Lycan (klasa 4 w nowszych wersjach protokolu)
                byJob = 8;
                byCON = 6; byINT = 2; bySTR = 6; byDEX = 2;
                break;

            // Bezposrednie identyfikatory ras (gdyby m_newCharJob zostal przekazany jako bezposredni race ID 5..8)
            case 5: // Ninja meski (race 5)
                byJob = 5;
                byCON = 3; byINT = 3; bySTR = 4; byDEX = 6;
                break;

            case 6: // Sura zenska (race 6)
                byJob = 6;
                byCON = 3; byINT = 5; bySTR = 5; byDEX = 3;
                break;

            case 7: // Szaman meski (race 7)
                byJob = 7;
                byCON = 4; byINT = 6; bySTR = 3; byDEX = 3;
                break;

            case 8: // Lycan meski (race 8)
                byJob = 8;
                byCON = 6; byINT = 2; bySTR = 6; byDEX = 2;
                break;

            default:
                // Niestandardowa lub ujemna wartosc m_newCharJob: bezpieczny fallback z logowaniem
                TraceError("[FAST_BOOT] CreateCharacterAndEnter: Niestandardowa klasa m_newCharJob=%d! Uzycie bezpiecznych domyslnych statystyk Wojownika.", m_newCharJob);
                byJob = (byGender == 0) ? 0 : 4;
                byCON = 4; byINT = 3; bySTR = 6; byDEX = 3;
                break;
        }

        m_statusMessage = std::format("Tworzenie postaci '{}' na slocie {} (job={}, shape={})...", charName, bySlot, byJob, byShape);
        m_statusColor = 0xFF55FFFF;

        rkNet.SendCreateCharacterPacket(
            bySlot,
            charName.c_str(),
            byJob,
            byShape,
            byCON, byINT, bySTR, byDEX
        );
    }

    void FastBootOverlay::ConnectCurrentProfile()
    {
        if (m_selectedProfileIdx < 0 || m_selectedProfileIdx >= static_cast<int>(m_profiles.size()))
            return;

        const auto& prof = m_profiles[m_selectedProfileIdx];

        // 1. Ustawienie sterownika protokolu
        Network::Protocol::ProtocolDriverRegistry::Instance().SetActiveDriver(prof.driverName);

        // 2. Skonfigurowanie automatu sieciowego w C++
        CAccountConnector& rkAccountConnector = CAccountConnector::Instance();
        CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
        rkNetStream.RefreshProtocolHandlers();

        rkAccountConnector.SetLoginInfo(m_szLogin, m_szPassword, m_szPin);
        rkNetStream.SetLoginInfo(m_szLogin, m_szPassword, m_szPin);
        rkNetStream.SetDirectEnterMode(m_characterSlot);

        m_bConnecting = true;
        m_connectStartTime = GetTickCount();
        m_enterGameTime = 0;
        m_statusMessage = std::format("Laczenie z: {} ({}:{})...", prof.name, prof.serverIp, prof.authPort);
        m_statusColor = 0xFFFFFF55;

        bool connOk = rkAccountConnector.Connect(prof.serverIp.c_str(), prof.gamePort, prof.serverIp.c_str(), prof.authPort);
        if (!connOk)
        {
            m_bConnecting = false;
            m_connectStartTime = 0;
            m_statusMessage = "Blad inicjalizacji polaczenia TCP!";
            m_statusColor = 0xFFFF4444;
        }

        if (m_bSaveCredentials)
        {
            SaveConfig();
        }
    }

    void FastBootOverlay::ConnectLocalDev()
    {
        Network::Protocol::ProtocolDriverRegistry::Instance().SetActiveDriver("standard_x64");
        CAccountConnector& rkAccountConnector = CAccountConnector::Instance();
        CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();

        rkAccountConnector.SetLoginInfo("admin", "admin", "");
        rkNetStream.SetLoginInfo("admin", "admin", "");
        rkNetStream.SetDirectEnterMode(0);
        rkNetStream.RefreshProtocolHandlers();

        m_bConnecting = true;
        m_connectStartTime = GetTickCount();
        m_enterGameTime = 0;
        m_statusMessage = "Szybkie polaczenie lokalne Standard x64 (127.0.0.1:11000)...";
        m_statusColor = 0xFFFFFF55;

        rkAccountConnector.Connect("127.0.0.1", 11011, "127.0.0.1", 11000);
    }

    void FastBootOverlay::Render()
    {
        if (!m_bInitialized || !m_bVisible)
            return;

        UpdateNetworkStatus();

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(480.0f, 430.0f), ImGuiCond_Appearing);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;

        if (ImGui::Begin("METIN2 FAST-BOOT - DIRECT TO WORLD (2026)", &m_bVisible, flags))
        {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Architektura Fast-Boot / Direct-to-World");
            ImGui::TextDisabled("Pominiecie starych faz Pythona. Skrot klawiszowy: F11");
            ImGui::Separator();

            // 1. Wybor profilu serwera
            ImGui::Spacing();
            ImGui::Text("Serwer Docelowy:");
            const char* currentProfileName = (m_selectedProfileIdx >= 0 && m_selectedProfileIdx < static_cast<int>(m_profiles.size())) 
                ? m_profiles[m_selectedProfileIdx].name.c_str() : "Wybierz...";

            if (ImGui::BeginCombo("##ServerProfile", currentProfileName))
            {
                for (int i = 0; i < static_cast<int>(m_profiles.size()); ++i)
                {
                    bool isSelected = (m_selectedProfileIdx == i);
                    if (ImGui::Selectable(m_profiles[i].name.c_str(), isSelected))
                    {
                        m_selectedProfileIdx = i;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 2. Pola wprowadzania danych logowania
            ImGui::Text("Login:");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##LoginInput", m_szLogin, sizeof(m_szLogin));

            ImGui::Spacing();
            ImGui::Text("Haslo:");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##PasswordInput", m_szPassword, sizeof(m_szPassword), ImGuiInputTextFlags_Password);

            // 3. Kod PIN (wymagany dla Luria)
            bool isLuria = (m_selectedProfileIdx < static_cast<int>(m_profiles.size()) && 
                            m_profiles[m_selectedProfileIdx].driverName == "luria");

            if (isLuria)
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Kod PIN (4 cyfry dla Luria):");
                ImGui::SetNextItemWidth(120.0f);
                ImGui::InputText("##PinInput", m_szPin, sizeof(m_szPin), ImGuiInputTextFlags_Password | ImGuiInputTextFlags_CharsDecimal);
            }

            ImGui::Spacing();

            // 4. Wybor slotu postaci (DirectEnter)
            ImGui::Text("Postac (DirectEnter Slot):");
            const char* slotNames[] = {
                "Slot 0 (Glowna postac - np. Kradneradia)",
                "Slot 1",
                "Slot 2",
                "Slot 3",
                "Slot 4"
            };
            ImGui::SetNextItemWidth(-1);
            ImGui::Combo("##CharacterSlot", &m_characterSlot, slotNames, IM_ARRAYSIZE(slotNames));

            ImGui::Spacing();
            ImGui::Checkbox("Zapamietaj dane", &m_bSaveCredentials);
            ImGui::SameLine();
            ImGui::Checkbox("Auto-Reconnect po zerwaniu", &m_bAutoReconnect);
            if (m_bPendingReconnect)
            {
                ImGui::SameLine();
                if (ImGui::Button("Anuluj"))
                {
                    CancelAutoReconnect();
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 5. Status polaczenia
            ImVec4 col(
                ((m_statusColor >> 16) & 0xFF) / 255.0f,
                ((m_statusColor >> 8) & 0xFF) / 255.0f,
                (m_statusColor & 0xFF) / 255.0f,
                1.0f
            );
            ImGui::Text("Status:");
            ImGui::SameLine();
            ImGui::TextColored(col, "%s", m_statusMessage.c_str());

            ImGui::Spacing();

            // 6. Przyciski akcji
            if (ImGui::Button(">> POLACZ I WEJDZ DO GRY (FAST-BOOT) <<", ImVec2(-1, 40.0f)))
            {
                ConnectCurrentProfile();
            }

            ImGui::Spacing();
            if (ImGui::Button("Szybkie Logowanie Lokalne (admin/admin)", ImVec2(-1, 28.0f)))
            {
                ConnectLocalDev();
            }

            // 7. Formularz tworzenia postaci (Zero-Python ImGui)
            if (m_bNeedCreateCharacter)
            {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), ">> KREATOR NOWEJ POSTACI (Zero-Python) <<");

                ImGui::Text("Nazwa nowej postaci:");
                ImGui::SetNextItemWidth(-1);
                ImGui::InputText("##NewCharName", m_szNewCharName, sizeof(m_szNewCharName));

                ImGui::Spacing();
                const char* jobNames[] = { "Wojownik (Warrior)", "Ninja (Assassin)", "Sura (Sura)", "Szaman (Shaman)" };
                ImGui::Text("Klasa:");
                ImGui::SetNextItemWidth(-1);
                ImGui::Combo("##NewCharJob", &m_newCharJob, jobNames, IM_ARRAYSIZE(jobNames));

                ImGui::Spacing();
                const char* genderNames[] = { "Mezczyzna", "Kobieta" };
                ImGui::Text("Plec:");
                ImGui::SetNextItemWidth(-1);
                ImGui::Combo("##NewCharGender", &m_newCharGender, genderNames, IM_ARRAYSIZE(genderNames));

                ImGui::Spacing();
                if (ImGui::Button(">> STWORZ POSTAC I WEJDZ DO GRY <<", ImVec2(-1, 38.0f)))
                {
                    CreateCharacterAndEnter();
                }
            }
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    }

    void FastBootOverlay::LoadConfig()
    {
        const char* configPath = "fast_boot_config.json";
        if (!std::filesystem::exists(configPath))
            return;

        try
        {
            std::ifstream ifs(configPath);
            std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            rapidjson::Document doc;
            doc.Parse(content.c_str());

            if (!doc.HasParseError() && doc.IsObject())
            {
                if (doc.HasMember("profile_idx") && doc["profile_idx"].IsInt())
                    m_selectedProfileIdx = doc["profile_idx"].GetInt();

                if (doc.HasMember("login") && doc["login"].IsString())
                    strncpy(m_szLogin, doc["login"].GetString(), sizeof(m_szLogin) - 1);

                if (doc.HasMember("password") && doc["password"].IsString())
                    strncpy(m_szPassword, doc["password"].GetString(), sizeof(m_szPassword) - 1);

                if (doc.HasMember("pin") && doc["pin"].IsString())
                    strncpy(m_szPin, doc["pin"].GetString(), sizeof(m_szPin) - 1);

                if (doc.HasMember("slot") && doc["slot"].IsInt())
                    m_characterSlot = doc["slot"].GetInt();

                if (doc.HasMember("save") && doc["save"].IsBool())
                    m_bSaveCredentials = doc["save"].GetBool();

                if (doc.HasMember("auto_reconnect") && doc["auto_reconnect"].IsBool())
                    m_bAutoReconnect = doc["auto_reconnect"].GetBool();
            }
        }
        catch (...)
        {
        }
    }

    void FastBootOverlay::SaveConfig()
    {
        try
        {
            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

            writer.StartObject();
            writer.Key("profile_idx");
            writer.Int(m_selectedProfileIdx);
            writer.Key("login");
            writer.String(m_szLogin);
            writer.Key("password");
            writer.String(m_szPassword);
            writer.Key("pin");
            writer.String(m_szPin);
            writer.Key("slot");
            writer.Int(m_characterSlot);
            writer.Key("save");
            writer.Bool(m_bSaveCredentials);
            writer.Key("auto_reconnect");
            writer.Bool(m_bAutoReconnect);
            writer.EndObject();

            std::ofstream ofs("fast_boot_config.json");
            ofs << sb.GetString();
        }
        catch (...)
        {
        }
    }
}
