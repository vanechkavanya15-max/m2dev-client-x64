#ifdef _WIN32
#include "EterBase/StdAfx.h"
#endif

#include "RefineDialogPacketHandler.h"
#include <cstring>

namespace Client::Network::Handlers {

EterBase::PacketResult<RefineDialogInfo> RefineDialogPacketHandler::HandleRefineInformationPacket(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCRefineInformation)) {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCRefineInformation packet;
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCRefineInformation));

    RefineDialogInfo info;
    info.type = packet.type;
    info.pos = packet.pos;
    info.src_vnum = packet.refine_table.src_vnum;
    info.result_vnum = packet.refine_table.result_vnum;
    info.cost = packet.refine_table.cost;
    info.prob = packet.refine_table.prob;

    // Pobieranie materialow potrzebnych do ulepszenia
    for (int i = 0; i < packet.refine_table.material_count && i < REFINE_MATERIAL_MAX_NUM; ++i) {
        info.materials.push_back({
            packet.refine_table.materials[i].vnum,
            packet.refine_table.materials[i].count
        });
    }

    return info;
}

EterBase::PacketResult<RefineDialogInfo> RefineDialogPacketHandler::HandleRefineInformationNewPacket(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCRefineInformationNew)) {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCRefineInformationNew packet;
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCRefineInformationNew));

    RefineDialogInfo info;
    info.type = packet.type;
    info.pos = packet.pos;
    info.src_vnum = packet.refine_table.src_vnum;
    info.result_vnum = packet.refine_table.result_vnum;
    info.cost = packet.refine_table.cost;
    info.prob = packet.refine_table.prob;

    // Pobieranie materialow potrzebnych do ulepszenia
    for (int i = 0; i < packet.refine_table.material_count && i < REFINE_MATERIAL_MAX_NUM; ++i) {
        info.materials.push_back({
            packet.refine_table.materials[i].vnum,
            packet.refine_table.materials[i].count
        });
    }

    return info;
}

} // namespace Client::Network::Handlers
