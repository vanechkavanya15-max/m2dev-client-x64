#include "RefineExchangeDomainHandler.h"
#include "EterBase/LogModern.h"
#include <cstring>
#include <algorithm>

namespace Client::Network::Handlers {

RefineExchangeDomainHandler::RefineExchangeDomainHandler(std::shared_ptr<Client::Gameplay::PlayerExchange> exchange)
    : m_exchange(std::move(exchange)) {
}

EterBase::PacketResult<void> RefineExchangeDomainHandler::HandleRefineInformation(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCRefineInformation)) {
        EterBase::ModernLogger::Error("RefineExchangeDomainHandler: Bufor TPacketGCRefineInformation za krotki ({} < {})",
            payload.size(), sizeof(TPacketGCRefineInformation));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCRefineInformation pack{};
    std::memcpy(&pack, payload.data(), sizeof(TPacketGCRefineInformation));

    Client::Core::Events::RefineInformationEvent event{};
    event.type = pack.type;
    event.pos = pack.pos;
    event.srcVnum = pack.refine_table.src_vnum;
    event.resultVnum = pack.refine_table.result_vnum;
    event.cost = pack.refine_table.cost;
    event.prob = pack.refine_table.prob;

    for (int i = 0; i < pack.refine_table.material_count && i < REFINE_MATERIAL_MAX_NUM; ++i) {
        event.materials.push_back({ pack.refine_table.materials[i].vnum, static_cast<uint8_t>(pack.refine_table.materials[i].count) });
    }

    EterBase::ModernLogger::Info("RefineExchangeDomainHandler: Kowal - koszt: {}, szansa: {}%, materialy: {}",
        pack.refine_table.cost, pack.refine_table.prob, event.materials.size());

    Client::Core::EventBus::GetInstance().Publish(event);
    return {};
}

EterBase::PacketResult<void> RefineExchangeDomainHandler::HandleRefineInformationNew(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCRefineInformationNew)) {
        EterBase::ModernLogger::Error("RefineExchangeDomainHandler: Bufor TPacketGCRefineInformationNew za krotki ({} < {})",
            payload.size(), sizeof(TPacketGCRefineInformationNew));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCRefineInformationNew pack{};
    std::memcpy(&pack, payload.data(), sizeof(TPacketGCRefineInformationNew));

    Client::Core::Events::RefineInformationEvent event{};
    event.type = pack.type;
    event.pos = pack.pos;
    event.srcVnum = pack.refine_table.src_vnum;
    event.resultVnum = pack.refine_table.result_vnum;
    event.cost = pack.refine_table.cost;
    event.prob = pack.refine_table.prob;

    for (int i = 0; i < pack.refine_table.material_count && i < REFINE_MATERIAL_MAX_NUM; ++i) {
        event.materials.push_back({ pack.refine_table.materials[i].vnum, static_cast<uint8_t>(pack.refine_table.materials[i].count) });
    }

    EterBase::ModernLogger::Info("RefineExchangeDomainHandler: Kowal (New) - koszt: {}, szansa: {}%, materialy: {}",
        pack.refine_table.cost, pack.refine_table.prob, event.materials.size());

    Client::Core::EventBus::GetInstance().Publish(event);
    return {};
}

EterBase::PacketResult<void> RefineExchangeDomainHandler::HandleDragonSoulRefine(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCDragonSoulRefine)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCDragonSoulRefine pack{};
    std::memcpy(&pack, payload.data(), sizeof(TPacketGCDragonSoulRefine));

    EterBase::ModernLogger::Info("RefineExchangeDomainHandler: Dragon Soul Refine podtyp: 0x{:02X}", pack.bSubType);

    Client::Core::Events::DragonSoulRefineEvent event(pack.bSubType);
    Client::Core::EventBus::GetInstance().Publish(event);
    return {};
}

EterBase::PacketResult<void> RefineExchangeDomainHandler::HandleExchangePacket(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCExchange)) {
        EterBase::ModernLogger::Error("RefineExchangeDomainHandler: Bufor TPacketGCExchange za krotki ({} < {})",
            payload.size(), sizeof(TPacketGCExchange));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* pack = reinterpret_cast<const TPacketGCExchange*>(payload.data());
    std::span<const uint8_t> subPayload = payload.subspan(sizeof(TPacketGCExchange));

    switch (pack->subheader) {
        case ExchangeSub::GC::START:
            return HandleExchangeStart(payload);
        case ExchangeSub::GC::ITEM_ADD:
            return HandleExchangeItemAdd(payload);
        case ExchangeSub::GC::ITEM_DEL:
            return HandleExchangeItemDel(payload);
        case ExchangeSub::GC::ELK_ADD:
            return HandleExchangeElkAdd(payload);
        case ExchangeSub::GC::ACCEPT:
            return HandleExchangeAccept(payload);
        case ExchangeSub::GC::END:
            return HandleExchangeEnd(payload);
        case ExchangeSub::GC::ALREADY:
            EterBase::ModernLogger::Debug("RefineExchangeDomainHandler: Handel - oferta juz zaakceptowana");
            return {};
        case ExchangeSub::GC::LESS_ELK:
            EterBase::ModernLogger::Warning("RefineExchangeDomainHandler: Handel - niewystarczajaca ilosc Yang");
            return {};
        default:
            EterBase::ModernLogger::Warning("RefineExchangeDomainHandler: Nieznany subheader handlu: 0x{:02X}", pack->subheader);
            return {};
    }
}

EterBase::PacketResult<void> RefineExchangeDomainHandler::HandleExchangeStart(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCExchange)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    const auto* pack = reinterpret_cast<const TPacketGCExchange*>(payload.data());

    EterBase::ModernLogger::Info("RefineExchangeDomainHandler: Rozpoczeto handel z graczem VID: {}", pack->arg1);
    Client::Core::Events::ExchangeStartedEvent event(pack->arg1);
    Client::Core::EventBus::GetInstance().Publish(event);
    return {};
}

EterBase::PacketResult<void> RefineExchangeDomainHandler::HandleExchangeItemAdd(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCExchange)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    const auto* pack = reinterpret_cast<const TPacketGCExchange*>(payload.data());

    Client::Core::Events::ExchangeItemAddedEvent event{};
    event.displayPos = static_cast<uint8_t>(pack->arg2.cell);
    event.vnum = pack->arg1;
    event.count = static_cast<uint8_t>(pack->arg3);
    event.isTargetOwner = (pack->is_me == 0);

    EterBase::ModernLogger::Debug("RefineExchangeDomainHandler: Handel przedmiot dodany: vnum {}, pozycja {}, target={}",
        event.vnum, event.displayPos, event.isTargetOwner);

    Client::Core::EventBus::GetInstance().Publish(event);
    return {};
}

EterBase::PacketResult<void> RefineExchangeDomainHandler::HandleExchangeItemDel(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCExchange)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    const auto* pack = reinterpret_cast<const TPacketGCExchange*>(payload.data());

    Client::Core::Events::ExchangeItemRemovedEvent event{};
    event.pos = static_cast<uint8_t>(pack->arg1);
    event.isTargetOwner = (pack->is_me == 0);

    Client::Core::EventBus::GetInstance().Publish(event);
    return {};
}

EterBase::PacketResult<void> RefineExchangeDomainHandler::HandleExchangeElkAdd(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCExchange)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    const auto* pack = reinterpret_cast<const TPacketGCExchange*>(payload.data());

    Client::Core::Events::ExchangeElkAddedEvent event{};
    event.amount = pack->arg1;
    event.isTargetOwner = (pack->is_me == 0);

    EterBase::ModernLogger::Debug("RefineExchangeDomainHandler: Handel Yang dodany: {}, target={}",
        event.amount, event.isTargetOwner);

    Client::Core::EventBus::GetInstance().Publish(event);
    return {};
}

EterBase::PacketResult<void> RefineExchangeDomainHandler::HandleExchangeAccept(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCExchange)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    const auto* pack = reinterpret_cast<const TPacketGCExchange*>(payload.data());

    Client::Core::Events::ExchangeAcceptedEvent event{};
    event.isTargetOwner = (pack->is_me == 0);

    EterBase::ModernLogger::Info("RefineExchangeDomainHandler: Handel zaakceptowany, target={}", event.isTargetOwner);
    Client::Core::EventBus::GetInstance().Publish(event);
    return {};
}

EterBase::PacketResult<void> RefineExchangeDomainHandler::HandleExchangeEnd(std::span<const uint8_t> payload) {
    EterBase::ModernLogger::Info("RefineExchangeDomainHandler: Handel zakonczony pomyslnie");
    Client::Core::Events::ExchangeFinishedEvent event(true);
    Client::Core::EventBus::GetInstance().Publish(event);
    return {};
}

// Outbound Encoders
std::vector<uint8_t> RefineExchangeDomainHandler::EncodeRefine(uint8_t pos, uint8_t type) {
    TPacketCGRefine packet{};
    packet.header = CG::REFINE;
    packet.length = sizeof(TPacketCGRefine);
    packet.pos = pos;
    packet.type = type;

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

std::vector<uint8_t> RefineExchangeDomainHandler::EncodeExchangeStart(uint32_t targetVid) {
    TPacketCGExchange packet{};
    packet.header = CG::EXCHANGE;
    packet.length = sizeof(TPacketCGExchange);
    packet.subheader = ExchangeSub::CG::START;
    packet.arg1 = targetVid;

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

std::vector<uint8_t> RefineExchangeDomainHandler::EncodeExchangeItemDel(uint8_t displayPos) {
    TPacketCGExchange packet{};
    packet.header = CG::EXCHANGE;
    packet.length = sizeof(TPacketCGExchange);
    packet.subheader = ExchangeSub::CG::ITEM_DEL;
    packet.arg1 = displayPos;

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

std::vector<uint8_t> RefineExchangeDomainHandler::EncodeExchangeElkAdd(uint32_t elk) {
    TPacketCGExchange packet{};
    packet.header = CG::EXCHANGE;
    packet.length = sizeof(TPacketCGExchange);
    packet.subheader = ExchangeSub::CG::ELK_ADD;
    packet.arg1 = elk;

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

std::vector<uint8_t> RefineExchangeDomainHandler::EncodeExchangeAccept() {
    TPacketCGExchange packet{};
    packet.header = CG::EXCHANGE;
    packet.length = sizeof(TPacketCGExchange);
    packet.subheader = ExchangeSub::CG::ACCEPT;

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

std::vector<uint8_t> RefineExchangeDomainHandler::EncodeExchangeCancel() {
    TPacketCGExchange packet{};
    packet.header = CG::EXCHANGE;
    packet.length = sizeof(TPacketCGExchange);
    packet.subheader = ExchangeSub::CG::CANCEL;

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

} // namespace Client::Network::Handlers
