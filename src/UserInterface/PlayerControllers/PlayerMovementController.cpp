#include "../StdAfx.h"
#include "PlayerMovementController.h"

#include "../InstanceBase.h"
#include "../PythonApplication.h"
#include "EterLib/Camera.h"
#include "EterBase/Timer.h"
#include "GameLib/GameUtil.h"
#include "EffectLib/EffectManager.h"
#include "EterPythonLib/PythonWindowManager.h"

#include <dinput.h>

#ifndef DIK_UP
#define DIK_UP 0xC8
#define DIK_DOWN 0xD0
#define DIK_LEFT 0xCB
#define DIK_RIGHT 0xCD
#endif

namespace UserInterface::PlayerControllers
{
    PlayerMovementController::PlayerMovementController(
        Contracts::IActorProvider* pActorProvider,
        Contracts::INetworkService* pNetworkService)
        : m_pActorProvider(pActorProvider)
        , m_pNetworkService(pNetworkService)
        , m_isUp(false)
        , m_isDown(false)
        , m_isLeft(false)
        , m_isRight(false)
        , m_isDirKey(false)
        , m_fMovDirRot(0.0f)
        , m_isCmrRot(false)
        , m_fCmrRotSpd(180.0f)
        , m_isConsumingStamina(FALSE)
        , m_fConsumeStaminaPerSec(0.0f)
        , m_fCurrentStamina(0.0f)
        , m_isDestPosition(FALSE)
        , m_ixDestPos(0)
        , m_iyDestPos(0)
        , m_iLastAlarmTime(0)
        , m_MovingCursorPosition(0.0f, 0.0f, 0.0f)
        , m_fMovingCursorSettingTime(0.0f)
        , m_fMovableGroundDistance(50.0f)
    {
    }

    void PlayerMovementController::SetSingleDIKKeyState(int eDIKKey, bool isPress)
    {
        switch (eDIKKey)
        {
            case DIK_UP:
                SetSingleDirKeyState(DIR_UP, isPress);
                break;
            case DIK_DOWN:
                SetSingleDirKeyState(DIR_DOWN, isPress);
                break;
            case DIK_LEFT:
                SetSingleDirKeyState(DIR_LEFT, isPress);
                break;
            case DIK_RIGHT:
                SetSingleDirKeyState(DIR_RIGHT, isPress);
                break;
            default:
                break;
        }
    }

    void PlayerMovementController::SetSingleDirKeyState(int eDirKey, bool isPress)
    {
        switch (eDirKey)
        {
            case DIR_UP:
                m_isUp = isPress;
                break;
            case DIR_DOWN:
                m_isDown = isPress;
                break;
            case DIR_LEFT:
                m_isLeft = isPress;
                break;
            case DIR_RIGHT:
                m_isRight = isPress;
                break;
            default:
                break;
        }

        m_isDirKey = (m_isUp || m_isDown || m_isLeft || m_isRight);
        SetMultiDirKeyState(m_isLeft, m_isRight, m_isUp, m_isDown);
    }

    void PlayerMovementController::SetMultiDirKeyState(bool isLeft, bool isRight, bool isUp, bool isDown)
    {
        if (!CanMove())
            return;

        bool isAny = (isLeft || isRight || isUp || isDown);

        if (isAny)
        {
            float fDirRot = 0.0f;
            GetMultiKeyDirRotation(isLeft, isRight, isUp, isDown, &fDirRot);

            if (!MoveToDirection(fDirRot))
            {
                Tracen("PlayerMovementController::SetMultiDirKeyState - MoveToDirection ERROR");
                return;
            }
        }
        else
        {
            Stop();
        }
    }

    bool PlayerMovementController::MoveToDirection(float fDirRot)
    {
        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (!pkInstMain)
            return false;

        if (pkInstMain->isLock())
        {
            if (!pkInstMain->IsUsingMovingSkill())
                return true;
        }

        CCamera* pkCmrCur = CCameraManager::Instance().GetCurrentCamera();
        if (pkCmrCur)
        {
            float fCmrCurRot = CameraRotationToCharacterRotation(pkCmrCur->GetRoll());

            if (m_isCmrRot)
            {
                float fSigDirRot = fDirRot;
                if (fSigDirRot > 180.0f)
                    fSigDirRot = fSigDirRot - 360.0f;

                float fRotRat = fSigDirRot;
                if (fRotRat > 90.0f)
                    fRotRat = (180.0f - fRotRat);
                else if (fRotRat < -90.0f)
                    fRotRat = (-180.0f - fRotRat);

                float fElapsedTime = CPythonApplication::Instance().GetGlobalElapsedTime();
                float fRotDeg = -m_fCmrRotSpd * fElapsedTime * fRotRat / 90.0f;
                pkCmrCur->Roll(fRotDeg);
            }

            fDirRot = fmod(360.0f + fCmrCurRot + fDirRot, 360.0f);
        }

        m_fMovDirRot = fDirRot;
        pkInstMain->NEW_MoveToDirection(fDirRot);
        return true;
    }

    bool PlayerMovementController::MoveToDestPixelPositionDirection(const TPixelPosition& c_rkPPosDst)
    {
        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (!pkInstMain)
            return false;

        if (!IsMovableGroundDistance(c_rkPPosDst))
            return false;

        return pkInstMain->NEW_MoveToDestPixelPositionDirection(c_rkPPosDst);
    }

    void PlayerMovementController::Stop()
    {
        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (pkInstMain)
        {
            pkInstMain->NEW_Stop();
        }

        m_isLeft = false;
        m_isRight = false;
        m_isUp = false;
        m_isDown = false;
        m_isDirKey = false;
    }

    float PlayerMovementController::GetDegreeFromDirection(int iUD, int iLR) const
    {
        switch (iUD)
        {
            case KEYBOARD_UD_UP:
                if (KEYBOARD_LR_LEFT == iLR)
                    return +45.0f;
                else if (KEYBOARD_LR_RIGHT == iLR)
                    return -45.0f;
                return 0.0f;

            case KEYBOARD_UD_DOWN:
                if (KEYBOARD_LR_LEFT == iLR)
                    return +135.0f;
                else if (KEYBOARD_LR_RIGHT == iLR)
                    return -135.0f;
                return +180.0f;

            case KEYBOARD_UD_NONE:
                if (KEYBOARD_LR_LEFT == iLR)
                    return +90.0f;
                else if (KEYBOARD_LR_RIGHT == iLR)
                    return -90.0f;
                break;

            default:
                break;
        }

        return 0.0f;
    }

    float PlayerMovementController::GetDegreeFromPosition(int ix, int iy, int iHalfWidth, int iHalfHeight) const
    {
        D3DXVECTOR3 vtDir(float(ix - iHalfWidth), float(iy - iHalfHeight), 0.0f);
        D3DXVec3Normalize(&vtDir, &vtDir);

        D3DXVECTOR3 vtStan(0, -1, 0);
        float ret = D3DXToDegree(acosf(D3DXVec3Dot(&vtDir, &vtStan)));

        if (vtDir.x < 0.0f)
            ret = 360.0f - ret;

        return 360.0f - ret;
    }

    void PlayerMovementController::GetMultiKeyDirRotation(bool isLeft, bool isRight, bool isUp, bool isDown, float* pfDirRot) const
    {
        float fScrX = 0.5f;
        float fScrY = 0.5f;

        if (isLeft)
            fScrX = 0.0f;
        else if (isRight)
            fScrX = 1.0f;

        if (isUp)
            fScrY = 0.0f;
        else if (isDown)
            fScrY = 1.0f;

        GetMouseDirRotation(fScrX, fScrY, pfDirRot);
    }

    void PlayerMovementController::GetMouseDirRotation(float fScrX, float fScrY, float* pfDirRot) const
    {
        long lWidth = UI::CWindowManager::Instance().GetScreenWidth();
        long lHeight = UI::CWindowManager::Instance().GetScreenHeight();
        int nScrPosX = static_cast<int>(lWidth * fScrX);
        int nScrPosY = static_cast<int>(lHeight * fScrY);
        int nScrCenterX = static_cast<int>(lWidth / 2);
        int nScrCenterY = static_cast<int>(lHeight / 2);

        float finputRotation = GetDegreeFromPosition(nScrPosX, nScrPosY, nScrCenterX, nScrCenterY);
        if (pfDirRot)
            *pfDirRot = finputRotation;
    }

    void PlayerMovementController::ResetCameraRotation()
    {
        CCamera* pkCmrCur = CCameraManager::Instance().GetCurrentCamera();
        CPythonApplication& rkApp = CPythonApplication::Instance();

        if (pkCmrCur)
            pkCmrCur->EndDrag();

        rkApp.SetCursorNum(CPythonApplication::NORMAL);
        if (CPythonApplication::CURSOR_MODE_HARDWARE == rkApp.GetCursorMode())
            rkApp.SetCursorVisible(TRUE);
    }

    bool PlayerMovementController::CanMove() const
    {
        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (!pkInstMain)
            return false;

        if (!pkInstMain->CanMove())
        {
            if (!pkInstMain->IsUsingMovingSkill())
                return false;
        }

        return true;
    }

    void PlayerMovementController::SetDungeonDestinationPosition(int ix, int iy)
    {
        m_isDestPosition = TRUE;
        m_ixDestPos = ix;
        m_iyDestPos = iy;

        AlarmHaveToGo();
    }

    void PlayerMovementController::AlarmHaveToGo()
    {
        m_iLastAlarmTime = CTimer::Instance().GetCurrentMillisecond();

        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (!pkInstMain)
            return;

        TPixelPosition PixelPosition;
        pkInstMain->NEW_GetPixelPosition(&PixelPosition);

        float fAngle = GetDegreeFromPosition2(PixelPosition.x, PixelPosition.y, float(m_ixDestPos), float(m_iyDestPos));
        fAngle = fmod(540.0f - fAngle, 360.0f);
        D3DXVECTOR3 v3Rotation(0.0f, 0.0f, fAngle);

        PixelPosition.y *= -1.0f;

        CEffectManager::Instance().RegisterEffect("d:/ymir work/effect/etc/compass/appear_middle.mse");
        CEffectManager::Instance().CreateEffect("d:/ymir work/effect/etc/compass/appear_middle.mse", PixelPosition, v3Rotation);
    }

    void PlayerMovementController::StartStaminaConsume(DWORD dwConsumePerSec, DWORD dwCurrentStamina)
    {
        m_isConsumingStamina = TRUE;
        m_fConsumeStaminaPerSec = float(dwConsumePerSec);
        m_fCurrentStamina = float(dwCurrentStamina);
    }

    void PlayerMovementController::StopStaminaConsume(DWORD dwCurrentStamina)
    {
        m_isConsumingStamina = FALSE;
        m_fConsumeStaminaPerSec = 0.0f;
        m_fCurrentStamina = float(dwCurrentStamina);
    }

    bool PlayerMovementController::IsMovableGroundDistance(const TPixelPosition& c_rkPPosPickedGround) const
    {
        CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
        if (!pkInstMain)
            return false;

        float fDistance = pkInstMain->NEW_GetDistanceFromDestPixelPosition(c_rkPPosPickedGround);
        return (fDistance >= m_fMovableGroundDistance);
    }

    void PlayerMovementController::Update(float fElapsedTime)
    {
        if (m_isDestPosition)
        {
            CInstanceBase* pkInstMain = m_pActorProvider ? m_pActorProvider->GetMainActor() : nullptr;
            if (pkInstMain)
            {
                TPixelPosition PixelPosition;
                pkInstMain->NEW_GetPixelPosition(&PixelPosition);

                if (abs(int(PixelPosition.x) - m_ixDestPos) + abs(int(PixelPosition.y) - m_iyDestPos) < 10000)
                {
                    m_isDestPosition = FALSE;
                }
                else
                {
                    if (CTimer::Instance().GetCurrentMillisecond() - m_iLastAlarmTime > 20000)
                    {
                        AlarmHaveToGo();
                    }
                }
            }
        }

        if (m_isConsumingStamina)
        {
            m_fCurrentStamina -= (fElapsedTime * m_fConsumeStaminaPerSec);
            if (m_fCurrentStamina < 0.0f)
                m_fCurrentStamina = 0.0f;
        }
    }

    bool PlayerMovementController::SendMovePacket(LONG lX, LONG lY, float fRot, DWORD dwTime, BYTE bFunc, BYTE bArg)
    {
        if (m_pNetworkService)
            return m_pNetworkService->SendCharacterMovePacket(lX, lY, fRot, dwTime, bFunc, bArg);

        return false;
    }

    bool PlayerMovementController::SendSyncPosition(DWORD dwVID, LONG lX, LONG lY)
    {
        if (m_pNetworkService)
            return m_pNetworkService->SendSyncPositionPacket(dwVID, lX, lY);

        return false;
    }
}
