#include "StdAfx.h"
#include "SendSyncPositionPacket.h"
#include "UserInterface/Packet.h"
#include "UserInterface/PythonNetworkStream.h"
#include "EterBase/LogModern.h"
#include "../../PythonBackground.h"

namespace UserInterface::Network {

    EterBase::PacketResult<void> SendSyncPositionPacket(CPythonNetworkStream& stream, std::span<const SyncPositionTarget> targets)
    {
        if (targets.empty()) {
            EterBase::ModernLogger::Error("SendSyncPositionPacket failed: targets span is empty.");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        // 1. Wyslanie glownego naglowka pakietu (TPacketCGSyncPosition)
        TPacketCGSyncPosition packetHeader;
        packetHeader.header = CG::SYNC_POSITION;
        packetHeader.length = static_cast<uint16_t>(sizeof(TPacketCGSyncPosition) + sizeof(TPacketCGSyncPositionElement) * targets.size());

        if (!stream.Send(sizeof(packetHeader), &packetHeader)) {
            EterBase::ModernLogger::Error("SendSyncPositionPacket failed: stream.Send(header) rejected.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        // 2. Wysylanie poszczegolnych elementow (TPacketCGSyncPositionElement)
        for (const auto& target : targets) {
            TPacketCGSyncPositionElement element;
            element.dwVID = target.victimId.value();
            element.lX = target.x;
            element.lY = target.y;

            CPythonBackground::Instance().LocalPositionToGlobalPosition(element.lX, element.lY);

            if (!stream.Send(sizeof(element), &element)) {
                EterBase::ModernLogger::Error("SendSyncPositionPacket failed: stream.Send(element) rejected for VID {}.", element.dwVID);
                return EterBase::MakeError(EterBase::PacketError::SessionClosed);
            }
        }

        return {};
    }

} // namespace UserInterface::Network
