#pragma once

#include "../Core/EngineForwardDecls.h"
#include "../Contracts/IActorProvider.h"
#include "../Contracts/INetworkService.h"
#include <cstdint>

namespace UserInterface::PlayerControllers
{
    /**
     * @brief Kontroler odpowiedzialny za ruch gracza, rotacje kamery i zuzycie staminy.
     * Zgodny z guardrailami: ZERO MUTEXES, NEVER CACHE POINTERS, INETWORKSERVICE.
     */
    class PlayerMovementController
    {
    public:
        enum EKeyBoard_UD
        {
            KEYBOARD_UD_NONE,
            KEYBOARD_UD_UP,
            KEYBOARD_UD_DOWN,
        };

        enum EKeyBoard_LR
        {
            KEYBOARD_LR_NONE,
            KEYBOARD_LR_LEFT,
            KEYBOARD_LR_RIGHT,
        };

        enum
        {
            DIR_UP,
            DIR_DOWN,
            DIR_LEFT,
            DIR_RIGHT,
        };

    public:
        PlayerMovementController(
            Contracts::IActorProvider* pActorProvider = nullptr,
            Contracts::INetworkService* pNetworkService = nullptr);
        ~PlayerMovementController() = default;

        // Rejestracja dostawcow uslug
        void SetActorProvider(Contracts::IActorProvider* pActorProvider) { m_pActorProvider = pActorProvider; }
        void SetNetworkService(Contracts::INetworkService* pNetworkService) { m_pNetworkService = pNetworkService; }

        // Klawiatura i sterowanie kierunkiem
        void SetSingleDirKeyState(int eDirKey, bool isPress);
        void SetSingleDIKKeyState(int eDIKKey, bool isPress);
        void SetMultiDirKeyState(bool isLeft, bool isRight, bool isUp, bool isDown);
        bool MoveToDirection(float fDirRot);
        bool MoveToDestPixelPositionDirection(const TPixelPosition& c_rkPPosDst);
        void Stop();

        // Matematyka kierunku i rotacji
        float GetDegreeFromDirection(int iUD, int iLR) const;
        float GetDegreeFromPosition(int ix, int iy, int iHalfWidth, int iHalfHeight) const;
        void GetMultiKeyDirRotation(bool isLeft, bool isRight, bool isUp, bool isDown, float* pfDirRot) const;
        void GetMouseDirRotation(float fScrX, float fScrY, float* pfDirRot) const;

        // Rotacja kamery
        void SetAutoCameraRotationSpeed(float fRotSpd) { m_fCmrRotSpd = fRotSpd; }
        void ResetCameraRotation();
        void SetCameraRotationEnabled(bool bEnable) { m_isCmrRot = bEnable; }
        bool IsCameraRotationEnabled() const { return m_isCmrRot; }

        // Weryfikacja mozliwosci poruszania sie
        bool CanMove() const;

        // Pozycja docelowa (Dungeon)
        void SetDungeonDestinationPosition(int ix, int iy);
        void AlarmHaveToGo();
        bool IsDestPosition() const { return m_isDestPosition != FALSE; }
        void ClearDestPosition() { m_isDestPosition = FALSE; }
        int GetDestPosX() const { return m_ixDestPos; }
        int GetDestPosY() const { return m_iyDestPos; }

        // Stamina
        void StartStaminaConsume(DWORD dwConsumePerSec, DWORD dwCurrentStamina);
        void StopStaminaConsume(DWORD dwCurrentStamina);
        float GetCurrentStamina() const { return m_fCurrentStamina; }
        bool IsConsumingStamina() const { return m_isConsumingStamina != FALSE; }

        // Kursor ruchu i dystans
        void SetMovingCursorPosition(const TPixelPosition& c_rkPPos) { m_MovingCursorPosition = c_rkPPos; }
        const TPixelPosition& GetMovingCursorPosition() const { return m_MovingCursorPosition; }
        void SetMovableGroundDistance(float fDistance) { m_fMovableGroundDistance = fDistance; }
        float GetMovableGroundDistance() const { return m_fMovableGroundDistance; }
        bool IsMovableGroundDistance(const TPixelPosition& c_rkPPosPickedGround) const;

        // Aktualizacja klatki (stamina, alarm pozycji docelowej)
        void Update(float fElapsedTime);

        // Wysylanie pakietow sieciowych przez INetworkService
        bool SendMovePacket(LONG lX, LONG lY, float fRot, DWORD dwTime, BYTE bFunc, BYTE bArg);
        bool SendSyncPosition(DWORD dwVID, LONG lX, LONG lY);

    private:
        Contracts::IActorProvider* m_pActorProvider{nullptr};
        Contracts::INetworkService* m_pNetworkService{nullptr};

        // Flagi klawiatury
        bool m_isUp{false};
        bool m_isDown{false};
        bool m_isLeft{false};
        bool m_isRight{false};
        bool m_isDirKey{false};

        // Rotacja kamery i ruchu
        float m_fMovDirRot{0.0f};
        bool m_isCmrRot{false};
        float m_fCmrRotSpd{180.0f};

        // Stamina
        BOOL m_isConsumingStamina{FALSE};
        float m_fConsumeStaminaPerSec{0.0f};
        float m_fCurrentStamina{0.0f};

        // Pozycja docelowa (Dungeon)
        BOOL m_isDestPosition{FALSE};
        int m_ixDestPos{0};
        int m_iyDestPos{0};
        int m_iLastAlarmTime{0};

        // Kursor ruchu
        TPixelPosition m_MovingCursorPosition{0.0f, 0.0f, 0.0f};
        float m_fMovingCursorSettingTime{0.0f};
        float m_fMovableGroundDistance{50.0f};
    };
}
