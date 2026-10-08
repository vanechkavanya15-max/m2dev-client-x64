#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <optional>

#include "Client/Core/StrongTypes.h"

// Forward declarations dla struktur DirectX
struct D3DXVECTOR3;
struct D3DXMATRIX;
struct _D3DCOLOR;

// Forward declarations dla struktur graficznych i silnika
struct TPixelPosition;
class CGraphicThingInstance;
class CActorInstance;
class CItemData;
class CRaceData;

// Forward declarations dla menedzerow klienta
class CInstanceBase;
class CPythonPlayer;
class CPythonCharacterManager;
class CPythonNetworkStream;
class CPythonTextTail;
class CPythonItem;

namespace UserInterface::Core {

// Silne typy domenowe w przestrzeni interfejsu
using EntityVid = Client::Core::EntityVid;
using ItemVnum = Client::Core::ItemVnum;
using SlotIndex = Client::Core::SlotIndex;
using SkillIndex = Client::Core::SkillIndex;
using Money64 = Client::Core::Money64;

// Aliasy inteligentnych wskaznikow dla zdeklarowanych typow
using InstanceBaseRaw = CInstanceBase*;
using ActorInstanceRaw = CActorInstance*;

/**
 * @brief Lekka fabryka dostepu do glownych fasad klienta gry.
 * 
 * Pozwala komponentom na odpytywanie menedzerow bez koniecznosci
 * wciagania megabajtow naglowkow ze StdAfx.h.
 */
class EngineForwardDecls {
public:
    static bool IsPlayerAvailable() noexcept;
    static bool IsCharacterManagerAvailable() noexcept;
    static bool IsNetworkAvailable() noexcept;
};

} // namespace UserInterface::Core
