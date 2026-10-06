#include "../../StdAfx.h"
#include "ItemSetHandler.h"
#include "../../PythonNetworkStream.h"
#include "../../AbstractPlayer.h"
#include "../../Packet.h"

namespace Network
{
namespace Handlers
{

/**
 * @brief Handles the item set packet (TPacketGCItemSet) from the server.
 * 
 * Reads the TPacketGCItemSet struct from the network stream, translates the data into
 * a TItemData structure, and updates the player's inventory or equipment slot using
 * the IAbstractPlayer singleton. This function decouples network reception from the
 * Python UI, delegating the visual refresh to the network stream or another event system.
 * 
 * @param stream The network stream instance.
 * @return true if the packet was successfully parsed and handled, false otherwise.
 */
bool HandleItemSet(CPythonNetworkStream& stream)
{
	TPacketGCItemSet packet;
	if (!stream.Recv(sizeof(packet), &packet))
	{
		return false;
	}

	TItemData itemData{};
	itemData.vnum = packet.vnum;
	itemData.count = packet.count;
	itemData.flags = packet.flags;
	itemData.anti_flags = packet.anti_flags;

	for (size_t i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i)
	{
		itemData.alSockets[i] = packet.alSockets[i];
	}

	for (size_t i = 0; i < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++i)
	{
		itemData.aAttr[i] = packet.aAttr[i];
	}

	IAbstractPlayer& player = IAbstractPlayer::GetSingleton();
	player.SetItemData(packet.pos, itemData);

	// The Python UI update should be decoupled, meaning the GUI refresh logic should
	// happen outside this function. Currently we do not call the UI directly here.
	
	return true;
}

} // namespace Handlers
} // namespace Network
