// FastBootOverlay.h
// Nowoczesny kontroler Fast-Boot / Direct-to-World (Dear ImGui DX9)
// Umozliwia natychmiastowe logowanie, wybor profilu serwera i wejscie do gry
// bez posrednictwa archaicznych faz Pythona (introLogin, introSelect)

#pragma once

#include <windows.h>
#include <d3d9.h>
#include <string>
#include <vector>

namespace UserInterface::FastBoot
{
    struct ServerProfile
    {
        std::string name;
        std::string driverName;
        std::string serverIp;
        int authPort;
        int gamePort;
    };

    class FastBootOverlay
    {
    public:
        static FastBootOverlay& Instance();

        bool Initialize(HWND hWnd, IDirect3DDevice9* pDevice);
        void Shutdown();
        void OnLostDevice();
        void OnResetDevice();

        void Render();
        bool ProcessWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        void SetVisible(bool visible) noexcept { m_bVisible = visible; }
        bool IsVisible() const noexcept { return m_bVisible; }
        void ToggleVisible() noexcept { m_bVisible = !m_bVisible; }

        void ConnectCurrentProfile();
        void ConnectLocalDev();

        void SetLoginFailure(const char* szStatus);
        void OnRemoteDisconnect();

        void SetAutoReconnect(bool bEnable, DWORD delayMs = 3000, int maxAttempts = 10) noexcept;
        bool IsAutoReconnectEnabled() const noexcept { return m_bAutoReconnect; }
        bool IsPendingReconnect() const noexcept { return m_bPendingReconnect; }
        int GetReconnectAttempts() const noexcept { return m_reconnectAttempts; }
        DWORD GetReconnectDelayMs() const noexcept { return m_reconnectDelayMs; }
        void TriggerAutoReconnect();
        void CancelAutoReconnect();

        void LoadConfig();
        void SaveConfig();

    private:
        FastBootOverlay();
        ~FastBootOverlay();

        FastBootOverlay(const FastBootOverlay&) = delete;
        FastBootOverlay& operator=(const FastBootOverlay&) = delete;

        void ApplyStyle();
        void UpdateNetworkStatus();

        bool m_bInitialized = false;
        bool m_bVisible = true;
        HWND m_hWnd = nullptr;
        IDirect3DDevice9* m_pDevice = nullptr;

        std::vector<ServerProfile> m_profiles;
        int m_selectedProfileIdx = 0;

        char m_szLogin[64] = "loginek60";
        char m_szPassword[64] = "!a123123";
        char m_szPin[16] = "1544";
        int m_characterSlot = 0;
        bool m_bSaveCredentials = true;
        bool m_bConnecting = false;
        std::string m_statusMessage = "Gotowy do polaczenia";
        unsigned int m_statusColor = 0xFFCCCCCC; // ARGB
        DWORD m_enterGameTime = 0;
        DWORD m_connectStartTime = 0;
        bool m_bAutoHidden = false;

        // Auto-reconnect po zerwaniu sesji
        bool m_bAutoReconnect = false;
        DWORD m_reconnectDelayMs = 3000;
        bool m_bPendingReconnect = false;
        DWORD m_reconnectTriggerTime = 0;
        int m_reconnectAttempts = 0;
        int m_maxReconnectAttempts = 10;
        bool m_bWasInGame = false;

        // Kreator postaci (Zero-Python)
        bool m_bNeedCreateCharacter = false;
        char m_szNewCharName[32] = "NowaPostac";
        int m_newCharJob = 0;    // 0=Warrior, 1=Assassin, 2=Sura, 3=Shaman
        int m_newCharGender = 0; // 0=Male, 1=Female
        int m_newCharShape = 0;

        void CreateCharacterAndEnter();
    };
}

using FastBootOverlay = UserInterface::FastBoot::FastBootOverlay;
