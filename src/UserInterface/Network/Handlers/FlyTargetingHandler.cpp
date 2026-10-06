#include "../../StdAfx.h"
#include "FlyTargetingHandler.h"
#include "../../PythonCharacterManager.h"
#include "../../PythonBackground.h"
#include "../../../GameLib/FlyingObjectManager.h"
#include "../../Packet.h"

// Note: LocalPosition/GlobalPosition conversions and packet headers
// are usually accessible from the client environment.

bool FlyTargetingHandler::HandleReceiveFlyTargeting(std::span<const uint8_t> buffer) {
    if (buffer.size() < sizeof(FlyTargetingPacketServer)) {
        return false;
    }

    auto* packet = reinterpret_cast<const FlyTargetingPacketServer*>(buffer.data());

    int32_t localX = packet->x;
    int32_t localY = packet->y;
    CPythonBackground::Instance().GlobalPositionToLocalPosition(localX, localY);

    auto& charMgr = CPythonCharacterManager::Instance();
    CInstanceBase* shooter = charMgr.GetInstancePtr(packet->shooterId);

    if (!shooter) {
        return true;
    }

    CInstanceBase* target = charMgr.GetInstancePtr(packet->targetId);

    if (packet->targetId && target) {
        shooter->GetGraphicThingInstancePtr()->SetFlyTarget(target->GetGraphicThingInstancePtr());
    } else {
        float h = CPythonBackground::Instance().GetHeight(localX, localY) + 60.0f;
        shooter->GetGraphicThingInstancePtr()->SetFlyTarget(D3DXVECTOR3(localX, localY, h));
    }

    return true;
}

bool FlyTargetingHandler::HandleReceiveAddFlyTargeting(std::span<const uint8_t> buffer) {
    if (buffer.size() < sizeof(FlyTargetingPacketServer)) {
        return false;
    }

    auto* packet = reinterpret_cast<const FlyTargetingPacketServer*>(buffer.data());

    int32_t localX = packet->x;
    int32_t localY = packet->y;
    CPythonBackground::Instance().GlobalPositionToLocalPosition(localX, localY);

    auto& charMgr = CPythonCharacterManager::Instance();
    CInstanceBase* shooter = charMgr.GetInstancePtr(packet->shooterId);

    if (!shooter) {
        return true;
    }

    CInstanceBase* target = charMgr.GetInstancePtr(packet->targetId);

    if (packet->targetId && target) {
        shooter->GetGraphicThingInstancePtr()->AddFlyTarget(target->GetGraphicThingInstancePtr());
    } else {
        float h = CPythonBackground::Instance().GetHeight(localX, localY) + 60.0f;
        shooter->GetGraphicThingInstancePtr()->AddFlyTarget(D3DXVECTOR3(localX, localY, h));
    }

    return true;
}

bool FlyTargetingHandler::HandleReceiveCreateFly(std::span<const uint8_t> buffer) {
    if (buffer.size() < sizeof(CreateFlyPacketServer)) {
        return false;
    }

    auto* packet = reinterpret_cast<const CreateFlyPacketServer*>(buffer.data());

    auto& flyMgr = CFlyingManager::Instance();
    auto& charMgr = CPythonCharacterManager::Instance();

    CInstanceBase* startInst = charMgr.GetInstancePtr(packet->startId);
    CInstanceBase* endInst = charMgr.GetInstancePtr(packet->endId);
    
    if (!startInst || !endInst) {
        return true;
    }

    flyMgr.CreateIndexedFly(packet->type, startInst->GetGraphicThingInstancePtr(), endInst->GetGraphicThingInstancePtr());

    return true;
}

FlyTargetingPacketClient FlyTargetingHandler::BuildSendFlyTargeting(uint32_t targetId, int32_t x, int32_t y) {
    FlyTargetingPacketClient packet{};
    packet.header = 0x0404;
    packet.length = sizeof(FlyTargetingPacketClient);
    packet.targetId = targetId;
    packet.x = x;
    packet.y = y;

    CPythonBackground::Instance().LocalPositionToGlobalPosition(packet.x, packet.y);

    return packet;
}

FlyTargetingPacketClient FlyTargetingHandler::BuildSendAddFlyTargeting(uint32_t targetId, int32_t x, int32_t y) {
    FlyTargetingPacketClient packet{};
    packet.header = 0x0405;
    packet.length = sizeof(FlyTargetingPacketClient);
    packet.targetId = targetId;
    packet.x = x;
    packet.y = y;

    CPythonBackground::Instance().LocalPositionToGlobalPosition(packet.x, packet.y);

    return packet;
}
