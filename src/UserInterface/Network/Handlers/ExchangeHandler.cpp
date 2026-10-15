#include "StdAfx.h"
/**
 * @file ExchangeHandler.cpp
 * @brief Implementation of the modern C++20 ExchangeHandler.
 */

#include "ExchangeHandler.h"
#include "../../Packet.h"
#include "../../PythonExchange.h"
#include "../../PythonCharacterManager.h"
#include "../../PythonPlayer.h"
#include <unordered_map>

/**
 * @brief Main entry point for processing an exchange packet.
 * @param packet The incoming GC exchange packet.
 * @return true if the packet subheader is recognized and handled, false otherwise.
 */
bool ExchangeHandler::HandlePacket(const TPacketGCExchange& packet)
{
    switch (packet.subheader)
    {
        case ExchangeSub::GC::START:
            HandleStart(packet);
            break;
        case ExchangeSub::GC::ITEM_ADD:
            HandleItemAdd(packet);
            break;
        case ExchangeSub::GC::ITEM_DEL:
            HandleItemDel(packet);
            break;
        case ExchangeSub::GC::ELK_ADD:
            HandleElkAdd(packet);
            break;
        case ExchangeSub::GC::ACCEPT:
            HandleAccept(packet);
            break;
        case ExchangeSub::GC::END:
            HandleEnd();
            break;
        case ExchangeSub::GC::ALREADY:
            HandleAlready();
            break;
        case ExchangeSub::GC::LESS_ELK:
            HandleLessElk();
            break;
        default:
            TraceError("ExchangeHandler::HandlePacket: unknown subheader %d", packet.subheader);
            return false;
    }

    return true;
}

/**
 * @brief Handles the START exchange event.
 * @param packet The incoming GC exchange packet containing target ID.
 */
void ExchangeHandler::HandleStart(const TPacketGCExchange& packet)
{
    CPythonExchange& exchange = CPythonExchange::Instance();
    exchange.Clear();
    exchange.Start();
    exchange.SetSelfName(CPythonPlayer::Instance().GetName());

    uint32_t targetId = packet.arg1;
    CInstanceBase* targetInstance = CPythonCharacterManager::Instance().GetInstancePtr(targetId);

    if (targetInstance)
    {
        exchange.SetTargetName(targetInstance->GetNameString());
    }
}

/**
 * @brief Handles adding an item to the exchange window.
 * @param packet The incoming GC exchange packet containing item details.
 */
void ExchangeHandler::HandleItemAdd(const TPacketGCExchange& packet)
{
    CPythonExchange& exchange = CPythonExchange::Instance();
    uint8_t slotIndex = packet.arg2.cell;
    uint32_t itemVnum = packet.arg1;
    uint8_t itemCount = static_cast<uint8_t>(packet.arg3);

    if (packet.is_me)
    {
        exchange.SetItemToSelf(slotIndex, itemVnum, itemCount);
        for (int i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i)
        {
            exchange.SetItemMetinSocketToSelf(slotIndex, i, packet.alValues[i]);
        }
        for (int j = 0; j < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++j)
        {
            exchange.SetItemAttributeToSelf(slotIndex, j, packet.aAttr[j].bType, packet.aAttr[j].sValue);
        }
    }
    else
    {
        exchange.SetItemToTarget(slotIndex, itemVnum, itemCount);
        for (int i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i)
        {
            exchange.SetItemMetinSocketToTarget(slotIndex, i, packet.alValues[i]);
        }
        for (int j = 0; j < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++j)
        {
            exchange.SetItemAttributeToTarget(slotIndex, j, packet.aAttr[j].bType, packet.aAttr[j].sValue);
        }
    }
}

/**
 * @brief Handles removing an item from the exchange window.
 * @param packet The incoming GC exchange packet containing the slot index.
 */
void ExchangeHandler::HandleItemDel(const TPacketGCExchange& packet)
{
    CPythonExchange& exchange = CPythonExchange::Instance();
    uint8_t slotIndex = static_cast<uint8_t>(packet.arg1);

    if (packet.is_me)
    {
        exchange.DelItemOfSelf(slotIndex);
    }
    else
    {
        exchange.DelItemOfTarget(slotIndex);
    }
}

/**
 * @brief Handles adding gold/elk to the exchange window.
 * @param packet The incoming GC exchange packet containing the elk amount.
 */
void ExchangeHandler::HandleElkAdd(const TPacketGCExchange& packet)
{
    CPythonExchange& exchange = CPythonExchange::Instance();
    uint32_t elkAmount = packet.arg1;

    if (packet.is_me)
    {
        exchange.SetElkToSelf(elkAmount);
    }
    else
    {
        exchange.SetElkToTarget(elkAmount);
    }
}

/**
 * @brief Handles the trade accept event.
 * @param packet The incoming GC exchange packet containing the accept flag.
 */
void ExchangeHandler::HandleAccept(const TPacketGCExchange& packet)
{
    CPythonExchange& exchange = CPythonExchange::Instance();
    uint8_t isAccepted = static_cast<uint8_t>(packet.arg1);

    if (packet.is_me)
    {
        exchange.SetAcceptToSelf(isAccepted);
    }
    else
    {
        exchange.SetAcceptToTarget(isAccepted);
    }
}

/**
 * @brief Handles the end of the exchange event.
 */
void ExchangeHandler::HandleEnd()
{
    CPythonExchange::Instance().End();
}

/**
 * @brief Handles the already trading error event.
 */
void ExchangeHandler::HandleAlready()
{
    Tracef("trade_already");
}

/**
 * @brief Handles the not enough elk error event.
 */
void ExchangeHandler::HandleLessElk()
{
    Tracef("trade_less_elk");
}
