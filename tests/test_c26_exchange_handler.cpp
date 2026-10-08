#include <iostream>
#include <vector>
#include <cassert>
#include "Client/Network/Handlers/ExchangeHandler.h"
#include "UserInterface/Packets/Packet_Exchange.h"
#include "UserInterface/Packet.h"

using namespace Client::Network::Handlers;
using namespace Client::Gameplay;
using namespace EterBase;

int main() {
    EntityId selfId{100};
    EntityId targetId{200};

    PlayerExchange exchange(selfId, targetId);

    // Test START
    {
        TPacketGCExchange pack{};
        pack.header = 0; // GC::EXCHANGE not needed for handler
        pack.subheader = ExchangeSub::GC::START;
        
        auto span = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
        auto result = ExchangeHandler::HandleExchangePacket(span, &exchange, selfId, targetId);
        assert(result.has_value());
    }

    // Test ITEM_ADD
    {
        TPacketGCExchange pack{};
        pack.subheader = ExchangeSub::GC::ITEM_ADD;
        pack.isInitiator = 1; // targets self
        pack.itemPos.cell = 5;
        pack.value1 = 5001; // vnum
        
        auto span = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
        auto result = ExchangeHandler::HandleExchangePacket(span, &exchange, selfId, targetId);
        assert(result.has_value());
        
        const auto* participant = exchange.GetParticipant(selfId);
        assert(participant->items.size() == 1);
        assert(participant->items[0].first.get() == 5);
        assert(participant->items[0].second.get() == 5001);
    }

    // Test ELK_ADD
    {
        TPacketGCExchange pack{};
        pack.subheader = ExchangeSub::GC::ELK_ADD;
        pack.isInitiator = 0; // targets other
        pack.value1 = 10000;
        
        auto span = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
        auto result = ExchangeHandler::HandleExchangePacket(span, &exchange, selfId, targetId);
        assert(result.has_value());
        
        const auto* participant = exchange.GetParticipant(targetId);
        assert(participant->gold.get() == 10000);
    }

    // Test ACCEPT
    {
        TPacketGCExchange pack{};
        pack.subheader = ExchangeSub::GC::ACCEPT;
        pack.isInitiator = 1; // targets self
        pack.value1 = 1; // true
        
        auto span = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
        auto result = ExchangeHandler::HandleExchangePacket(span, &exchange, selfId, targetId);
        assert(result.has_value());
        
        const auto* participant = exchange.GetParticipant(selfId);
        assert(participant->is_locked == true);
        assert(participant->is_accepted == true);
    }

    // Test END
    {
        TPacketGCExchange pack{};
        pack.subheader = ExchangeSub::GC::END;
        
        auto span = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
        auto result = ExchangeHandler::HandleExchangePacket(span, &exchange, selfId, targetId);
        assert(result.has_value());
        
        assert(exchange.GetState() == ExchangeState::Cancel);
    }
    
    // Test BufferUnderflow
    {
        std::vector<uint8_t> smallBuffer(5);
        auto span = std::span<const uint8_t>(smallBuffer.data(), smallBuffer.size());
        auto result = ExchangeHandler::HandleExchangePacket(span, &exchange, selfId, targetId);
        assert(!result.has_value());
        assert(result.error() == PacketError::BufferUnderflow);
    }

    std::cout << "All tests passed successfully.\n";
    return 0;
}
