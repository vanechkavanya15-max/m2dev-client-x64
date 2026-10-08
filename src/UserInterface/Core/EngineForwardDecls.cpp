#include "StdAfx.h"
#include "EngineForwardDecls.h"
#include "PythonPlayer.h"
#include "PythonCharacterManager.h"
#include "PythonNetworkStream.h"

namespace UserInterface::Core {

bool EngineForwardDecls::IsPlayerAvailable() noexcept
{
    return CPythonPlayer::InstancePtr() != nullptr;
}

bool EngineForwardDecls::IsCharacterManagerAvailable() noexcept
{
    return CPythonCharacterManager::InstancePtr() != nullptr;
}

bool EngineForwardDecls::IsNetworkAvailable() noexcept
{
    return CPythonNetworkStream::InstancePtr() != nullptr;
}

} // namespace UserInterface::Core
