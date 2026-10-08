#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <optional>

#include "Client/Core/StrongTypes.h"

#include <d3d9.h>
#include <d3dx9math.h>

// Forward declarations dla struktur DirectX
// D3DXVECTOR3 i D3DXMATRIX sa zdefiniowane w d3dx9math.h

// Deklaracje dla struktur graficznych i silnika
typedef D3DXVECTOR3 TPixelPosition;
struct _D3DCOLOR;
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
