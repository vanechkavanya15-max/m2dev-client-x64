
#include "ItemPacketCodec.h"
#include "../../EterBase/ModernLogger.h"

#include <cstring>
#include <format>

namespace Client::Network {

// --- TPacketCGItemUse ---

std::expected<std::vector<uint8_t>, std::string> ItemPacketCodec::EncodeItemUse(const TPacketCGItemUse& packet) {
    std::vector<uint8_t> buffer(sizeof(TPacketCGItemUse));
    std::memcpy(buffer.data(), &packet, sizeof(TPacketCGItemUse));
    return buffer;
}

std::expected<TPacketCGItemUse, std::string> ItemPacketCodec::DecodeItemUse(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(TPacketCGItemUse)) {
        std::string err = std::format("DecodeItemUse failed: buffer size {} < {}", buffer.size(), sizeof(TPacketCGItemUse));
        EterBase::ModernLogger::Error("{}", err);
        return std::unexpected(err);
    }
    TPacketCGItemUse packet;
    std::memcpy(&packet, buffer.data(), sizeof(TPacketCGItemUse));
    return packet;
}

// --- TPacketCGItemMove ---

std::expected<std::vector<uint8_t>, std::string> ItemPacketCodec::EncodeItemMove(const TPacketCGItemMove& packet) {
    std::vector<uint8_t> buffer(sizeof(TPacketCGItemMove));
    std::memcpy(buffer.data(), &packet, sizeof(TPacketCGItemMove));
    return buffer;
}

std::expected<TPacketCGItemMove, std::string> ItemPacketCodec::DecodeItemMove(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(TPacketCGItemMove)) {
        std::string err = std::format("DecodeItemMove failed: buffer size {} < {}", buffer.size(), sizeof(TPacketCGItemMove));
        EterBase::ModernLogger::Error("{}", err);
        return std::unexpected(err);
    }
    TPacketCGItemMove packet;
    std::memcpy(&packet, buffer.data(), sizeof(TPacketCGItemMove));
    return packet;
}

// --- TPacketCGItemDrop ---

std::expected<std::vector<uint8_t>, std::string> ItemPacketCodec::EncodeItemDrop(const TPacketCGItemDrop& packet) {
    std::vector<uint8_t> buffer(sizeof(TPacketCGItemDrop));
    std::memcpy(buffer.data(), &packet, sizeof(TPacketCGItemDrop));
    return buffer;
}

std::expected<TPacketCGItemDrop, std::string> ItemPacketCodec::DecodeItemDrop(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(TPacketCGItemDrop)) {
        std::string err = std::format("DecodeItemDrop failed: buffer size {} < {}", buffer.size(), sizeof(TPacketCGItemDrop));
        EterBase::ModernLogger::Error("{}", err);
        return std::unexpected(err);
    }
    TPacketCGItemDrop packet;
    std::memcpy(&packet, buffer.data(), sizeof(TPacketCGItemDrop));
    return packet;
}

// --- TPacketGCItemSet ---

std::expected<std::vector<uint8_t>, std::string> ItemPacketCodec::EncodeItemSet(const TPacketGCItemSet& packet) {
    std::vector<uint8_t> buffer(sizeof(TPacketGCItemSet));
    std::memcpy(buffer.data(), &packet, sizeof(TPacketGCItemSet));
    return buffer;
}

std::expected<TPacketGCItemSet, std::string> ItemPacketCodec::DecodeItemSet(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(TPacketGCItemSet)) {
        std::string err = std::format("DecodeItemSet failed: buffer size {} < {}", buffer.size(), sizeof(TPacketGCItemSet));
        EterBase::ModernLogger::Error("{}", err);
        return std::unexpected(err);
    }
    TPacketGCItemSet packet;
    std::memcpy(&packet, buffer.data(), sizeof(TPacketGCItemSet));
    return packet;
}

// --- TPacketGCItemDel ---

std::expected<std::vector<uint8_t>, std::string> ItemPacketCodec::EncodeItemDel(const TPacketGCItemDel& packet) {
    std::vector<uint8_t> buffer(sizeof(TPacketGCItemDel));
    std::memcpy(buffer.data(), &packet, sizeof(TPacketGCItemDel));
    return buffer;
}

std::expected<TPacketGCItemDel, std::string> ItemPacketCodec::DecodeItemDel(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(TPacketGCItemDel)) {
        std::string err = std::format("DecodeItemDel failed: buffer size {} < {}", buffer.size(), sizeof(TPacketGCItemDel));
        EterBase::ModernLogger::Error("{}", err);
        return std::unexpected(err);
    }
    TPacketGCItemDel packet;
    std::memcpy(&packet, buffer.data(), sizeof(TPacketGCItemDel));
    return packet;
}

// --- TPacketGCItemGroundAdd ---

std::expected<std::vector<uint8_t>, std::string> ItemPacketCodec::EncodeItemGroundAdd(const TPacketGCItemGroundAdd& packet) {
    std::vector<uint8_t> buffer(sizeof(TPacketGCItemGroundAdd));
    std::memcpy(buffer.data(), &packet, sizeof(TPacketGCItemGroundAdd));
    return buffer;
}

std::expected<TPacketGCItemGroundAdd, std::string> ItemPacketCodec::DecodeItemGroundAdd(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(TPacketGCItemGroundAdd)) {
        std::string err = std::format("DecodeItemGroundAdd failed: buffer size {} < {}", buffer.size(), sizeof(TPacketGCItemGroundAdd));
        EterBase::ModernLogger::Error("{}", err);
        return std::unexpected(err);
    }
    TPacketGCItemGroundAdd packet;
    std::memcpy(&packet, buffer.data(), sizeof(TPacketGCItemGroundAdd));
    return packet;
}

} // namespace Client::Network
