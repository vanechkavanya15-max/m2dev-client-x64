#pragma once

#include <span>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include "EterBase/PacketResult.h"
#include "Client/Core/EventBus.h"
#include "Client/Gameplay/TradeDomain.h"
#include "Client/Network/Protocol/Protocol.h"

namespace Client::Core::Events {

/**
 * @brief Zdarzenie otrzymania informacji o ulepszaniu przedmiotu (Kowal).
 */
struct RefineInformationEvent : public Client::Core::IEvent {
    uint8_t type{0};
    uint16_t pos{0};
    uint32_t srcVnum{0};
    uint32_t resultVnum{0};
    uint32_t cost{0};
    uint8_t prob{0};
    struct Material {
        uint32_t vnum{0};
        uint8_t count{0};
    };
    std::vector<Material> materials;
};

/**
 * @brief Zdarzenie ulepszania smoczych kamieni (Dragon Soul Refine).
 */
struct DragonSoulRefineEvent : public Client::Core::IEvent {
    uint8_t subType{0};
    constexpr explicit DragonSoulRefineEvent(uint8_t st) noexcept : subType(st) {}
};

/**
 * @brief Zdarzenie rozpoczecia handlu bezposredniego z innym graczem.
 */
struct ExchangeStartedEvent : public Client::Core::IEvent {
    uint32_t targetVid{0};
    constexpr explicit ExchangeStartedEvent(uint32_t vid) noexcept : targetVid(vid) {}
};

/**
 * @brief Zdarzenie wlozenia przedmiotu do okna handlu.
 */
struct ExchangeItemAddedEvent : public Client::Core::IEvent {
    uint8_t displayPos{0};
    uint32_t vnum{0};
    uint8_t count{0};
    bool isTargetOwner{false};
};

/**
 * @brief Zdarzenie usuniecia przedmiotu z okna handlu.
 */
struct ExchangeItemRemovedEvent : public Client::Core::IEvent {
    uint8_t pos{0};
    bool isTargetOwner{false};
};

/**
 * @brief Zdarzenie zmiany kwoty Yang w oknie handlu.
 */
struct ExchangeElkAddedEvent : public Client::Core::IEvent {
    uint32_t amount{0};
    bool isTargetOwner{false};
};

/**
 * @brief Zdarzenie zatwierdzenia/akceptacji handlu przez strone.
 */
struct ExchangeAcceptedEvent : public Client::Core::IEvent {
    bool isTargetOwner{false};
};

/**
 * @brief Zdarzenie pomyslnego lub przerwanego zakonczenia transakcji.
 */
struct ExchangeFinishedEvent : public Client::Core::IEvent {
    bool success{true};
    constexpr explicit ExchangeFinishedEvent(bool s) noexcept : success(s) {}
};

} // namespace Client::Core::Events

namespace Client::Network::Handlers {

/**
 * @brief Zwarty handler domenowy SRP dla operacji kowala (Refine) oraz handlu (Exchange).
 * Realizuje pelna walidacje pakietow, obsluge materialow ulepszania oraz synchronizacje
 * okna handlu miedzy graczami.
 */
class RefineExchangeDomainHandler {
public:
    RefineExchangeDomainHandler() = default;
    explicit RefineExchangeDomainHandler(std::shared_ptr<Client::Gameplay::PlayerExchange> exchange);
    ~RefineExchangeDomainHandler() = default;

    void SetPlayerExchange(std::shared_ptr<Client::Gameplay::PlayerExchange> exchange) noexcept {
        m_exchange = std::move(exchange);
    }

    [[nodiscard]] std::shared_ptr<Client::Gameplay::PlayerExchange> GetPlayerExchange() const noexcept {
        return m_exchange;
    }

    // Refine Handlers
    static EterBase::PacketResult<void> HandleRefineInformation(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleRefineInformationNew(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleDragonSoulRefine(std::span<const uint8_t> payload);

    // Exchange Handlers
    static EterBase::PacketResult<void> HandleExchangePacket(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleExchangeStart(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleExchangeItemAdd(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleExchangeItemDel(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleExchangeElkAdd(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleExchangeAccept(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleExchangeEnd(std::span<const uint8_t> payload);

    // Kodery pakietow wychodzacych
    static std::vector<uint8_t> EncodeRefine(uint8_t pos, uint8_t type);
    static inline std::vector<uint8_t> EncodeDragonSoulRefine(uint8_t refineType, const TItemPos* posArray, size_t count) {
        TPacketCGDragonSoulRefine packet{};
        packet.header = CG::DRAGON_SOUL_REFINE;
        packet.length = sizeof(TPacketCGDragonSoulRefine);
        packet.bSubType = refineType;
        if (posArray && count > 0) {
            size_t copyCount = (count < DS_REFINE_WINDOW_MAX_NUM) ? count : DS_REFINE_WINDOW_MAX_NUM;
            std::memcpy(packet.ItemGrid, posArray, sizeof(TItemPos) * copyCount);
        }

        std::vector<uint8_t> buffer(sizeof(packet));
        std::memcpy(buffer.data(), &packet, sizeof(packet));
        return buffer;
    }
    static std::vector<uint8_t> EncodeExchangeStart(uint32_t targetVid);
    static inline std::vector<uint8_t> EncodeExchangeItemAdd(TItemPos pos, uint8_t displayPos) {
        TPacketCGExchange packet{};
        packet.header = CG::EXCHANGE;
        packet.length = sizeof(TPacketCGExchange);
        packet.subheader = ExchangeSub::CG::ITEM_ADD;
        packet.Pos = pos;
        packet.arg2 = displayPos;

        std::vector<uint8_t> buffer(sizeof(packet));
        std::memcpy(buffer.data(), &packet, sizeof(packet));
        return buffer;
    }
    static std::vector<uint8_t> EncodeExchangeItemDel(uint8_t displayPos);
    static std::vector<uint8_t> EncodeExchangeElkAdd(uint32_t elk);
    static std::vector<uint8_t> EncodeExchangeAccept();
    static std::vector<uint8_t> EncodeExchangeCancel();

private:
    std::shared_ptr<Client::Gameplay::PlayerExchange> m_exchange;
};

} // namespace Client::Network::Handlers
