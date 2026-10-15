#include "StdAfx.h"
#include "GamePacketSenders.h"

#include "../../Packet.h"
#include "../../PythonNetworkStream.h"
#include "../../PythonBackground.h"
#include "../../../EterLib/NetStream.h"
#include "../../../EterLib/ControlPackets.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/Timer.h"
#include "../../Core/Events/SafeboxEvents.h"

#include <cmath>
#include <cstring>
#include <algorithm>
#include <vector>
#include <span>
#include <optional>
#include <format>

// --- LOKALNE STRUKTURY PROXY PAKIETOW O SCISLYM 1-BAJTOWYM WYROWNIANIU ---

#pragma pack(push, 1)

/**
 * @brief Struktura proxy dla CG::ATTACK (0x0401).
 * Odpowiada ukladowi legacy TPacketCGAttack (11 bajtow).
 */
struct ProxyPacketCGAttack
{
    uint16_t header;         ///< Identyfikator naglowka (CG::ATTACK = 0x0401)
    uint16_t length;         ///< Dlugosc pakietu
    uint8_t  attackType;     ///< Typ / indeks animacji ataku
    uint32_t victimId;       ///< VID atakowanego celu
    uint8_t  crcProcPiece;   ///< Suma kontrolna procesora
    uint8_t  crcFilePiece;   ///< Suma kontrolna pliku
};
static_assert(sizeof(ProxyPacketCGAttack) == 11, "ProxyPacketCGAttack musi miec dokladnie 11 bajtow");

/**
 * @brief Struktura proxy dla inicjalizacji handlu (CG::EXCHANGE, subheader = START).
 */
struct ProxyPacketCGExchangeStart
{
    uint16_t header;       ///< Identyfikator naglowka (CG::EXCHANGE)
    uint16_t length;       ///< Calkowita dlugosc pakietu
    uint8_t  subheader;    ///< Akcja wymiany (ExchangeSub::CG::START)
    uint32_t targetVid;    ///< VID gracza docelowego
    uint8_t  padding1;     ///< Wyrownanie arg2
    TItemPos paddingPos;   ///< Wyrownanie pozycji przedmiotu
};
static_assert(sizeof(ProxyPacketCGExchangeStart) == 13, "ProxyPacketCGExchangeStart musi miec 13 bajtow");

/**
 * @brief Struktura proxy dla upuszczenia przedmiotu bez ilosci (CG::ITEM_DROP).
 */
struct ProxyPacketCGItemDrop
{
    uint16_t header;    ///< Identyfikator naglowka
    uint16_t length;    ///< Dlugosc pakietu
    TItemPos pos;       ///< Pozycja upuszczanego przedmiotu
    uint32_t elk;       ///< Ilosc upuszczanego zlota / yang
};

/**
 * @brief Struktura proxy dla upuszczenia przedmiotu z iloscia (CG::ITEM_DROP2).
 */
struct ProxyPacketCGItemDrop2
{
    uint16_t header;    ///< Identyfikator naglowka
    uint16_t length;    ///< Dlugosc pakietu
    TItemPos pos;       ///< Pozycja upuszczanego przedmiotu
    uint32_t gold;      ///< Ilosc zlota
    uint8_t  count;     ///< Liczba sztuk przedmiotu
};

/**
 * @brief Struktura proxy dla podniesienia przedmiotu z ziemi (CG::ITEM_PICKUP).
 */
struct ProxyPacketCGItemPickUp
{
    uint8_t  header; ///< Identyfikator naglowka
    uint32_t vid;    ///< VID podnoszonego przedmiotu
};
static_assert(sizeof(ProxyPacketCGItemPickUp) == 5, "ProxyPacketCGItemPickUp musi miec 5 bajtow");

/**
 * @brief Struktura proxy dla wprowadzania tekstu w skrypcie questa (HEADER_CG_QUEST_INPUT_STRING).
 */
struct PacketQuestInputString
{
    uint8_t header;
    char text[64 + 1]; // QUEST_INPUT_STRING_MAX_NUM = 64, +1 na null-terminator
};

/**
 * @brief Struktura proxy dla uzycia umiejetnosci (0x0402).
 */
struct ProxyPacketCGUseSkill
{
    uint16_t header;       ///< Naglowek (0x0402)
    uint16_t length;       ///< Dlugosc struktury
    uint32_t skillId;      ///< VNUM umiejetnosci
    uint32_t targetId;     ///< VID celu
};
static_assert(sizeof(ProxyPacketCGUseSkill) == 12, "ProxyPacketCGUseSkill musi miec 12 bajtow");

#pragma pack(pop)

namespace {
    constexpr size_t LOCAL_QUEST_INPUT_STRING_MAX_NUM = 64;
    constexpr uint8_t HEADER_CG_QUEST_INPUT_STRING = 30;
}

namespace Network::Senders {

    // =========================================================================
    // 1. ATAK (AttackSender)
    // =========================================================================

    EterBase::PacketDispatchResult<void> AttackSender::Send(EterBase::EntityId victimId, uint8_t attackType, CNetworkStream* networkStream)
    {
        if (!networkStream)
        {
            return EterBase::MakeError(EterBase::PacketDispatchError::Disconnected);
        }

        ProxyPacketCGAttack attackPacket;
        std::memset(&attackPacket, 0, sizeof(attackPacket));
        
        attackPacket.header = 0x0401; // CG::ATTACK
        attackPacket.length = sizeof(ProxyPacketCGAttack);
        attackPacket.attackType = attackType;
        attackPacket.victimId = victimId.get();
        attackPacket.crcProcPiece = 0;
        attackPacket.crcFilePiece = 0;

        std::span<const uint8_t> packetSpan(reinterpret_cast<const uint8_t*>(&attackPacket), sizeof(attackPacket));
        
        if (!networkStream->Send(static_cast<int>(packetSpan.size()), packetSpan.data()))
        {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "Failed to send CG::ATTACK for VictimId: {}", victimId.get());
            return EterBase::MakeError(EterBase::PacketDispatchError::QueueFull);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(NetworkAttackSentEvent(victimId, attackType));

        EterBase::ModernLogger::Log(EterBase::LogLevel::Info, "Successfully queued CG::ATTACK for VictimId: {}", victimId.get());
        
        return {};
    }

    // =========================================================================
    // 2. CZAT (SendChatPacket)
    // =========================================================================

    EterBase::PacketResult<void> SendChatPacket(
        std::string_view text, 
        Network::Packets::ChatType type, 
        const std::function<bool(std::span<const uint8_t>)>& sendCallback)
    {
        if (text.empty())
        {
            EterBase::ModernLogger::Warn("SendChatPacket: Attempted to send an empty chat message.");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        if (type >= Network::Packets::ChatType::MaxNum)
        {
            EterBase::ModernLogger::Warn("SendChatPacket: Invalid chat type provided.");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        const size_t structSize = sizeof(Network::Packets::ChatCS);
        const size_t totalLength = structSize + text.size();

        if (totalLength > UINT16_MAX)
        {
            EterBase::ModernLogger::Warn("SendChatPacket: Chat message exceeds maximum allowed packet length.");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        std::vector<uint8_t> packetBuffer(totalLength);
        
        auto* packetHeader = reinterpret_cast<Network::Packets::ChatCS*>(packetBuffer.data());
        packetHeader->header = static_cast<uint16_t>(Network::Packets::ChatHeader::CG_CHAT);
        packetHeader->length = static_cast<uint16_t>(totalLength);
        packetHeader->type = static_cast<uint8_t>(type);

        std::memcpy(packetBuffer.data() + structSize, text.data(), text.size());

        std::span<const uint8_t> packetSpan(packetBuffer);

        if (!sendCallback(packetSpan))
        {
            EterBase::ModernLogger::Error("SendChatPacket: Network stream failed to send chat packet (type: {}).", static_cast<uint8_t>(type));
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Debug("SendChatPacket: Successfully sent chat message of length {} (type: {}).", text.size(), static_cast<uint8_t>(type));
        return {};
    }

    // =========================================================================
    // 3. AKCEPTACJA WYMIANY (SendExchangeAcceptPacket)
    // =========================================================================

    EterBase::PacketResult<void> SendExchangeAcceptPacket(CPythonNetworkStream& stream)
    {
        TPacketCGExchange packet{};
        packet.header = CG::EXCHANGE;
        packet.length = static_cast<uint16_t>(sizeof(packet));
        packet.subheader = ExchangeSub::CG::ACCEPT;
        
        std::span<const uint8_t> packetSpan{ reinterpret_cast<const uint8_t*>(&packet), sizeof(packet) };

        if (!stream.Send(static_cast<int>(packetSpan.size()), packetSpan.data()))
        {
            EterBase::ModernLogger::Error("SendExchangeAcceptPacket Error: Failed to send packet");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        return {};
    }

    // =========================================================================
    // 4. DODANIE PRZEDMIOTU DO WYMIANY (SendExchangeItemAddPacket)
    // =========================================================================

    EterBase::PacketResult<void> SendExchangeItemAddPacket(
        const TItemPos& itemPos, 
        EterBase::ItemSlot displayPos, 
        const std::function<bool(std::span<const uint8_t>)>& sendCallback)
    {
        if (!sendCallback)
        {
            EterBase::ModernLogger::Error("[Network] SendExchangeItemAddPacket: callback is null.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        if (!const_cast<TItemPos*>(&itemPos)->IsValidCell())
        {
            EterBase::ModernLogger::Warn("[Network] SendExchangeItemAddPacket: Invalid item cell specified. Window: {}, Cell: {}", 
                itemPos.window_type, itemPos.cell);
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        TPacketCGExchange packet;
        std::memset(&packet, 0, sizeof(packet));

        packet.header = CG::EXCHANGE;
        packet.length = sizeof(packet);
        packet.subheader = ExchangeSub::CG::ITEM_ADD;
        packet.arg1 = 0;
        packet.arg2 = static_cast<uint8_t>(displayPos.value());
        packet.Pos = itemPos;

        std::span<const uint8_t> packetSpan(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));

        if (!sendCallback(packetSpan))
        {
            EterBase::ModernLogger::Error("[Network] SendExchangeItemAddPacket: failed to send packet payload.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Debug("[Network] Sent Exchange Item Add (Window: {}, Cell: {}, DisplaySlot: {})",
            itemPos.window_type, itemPos.cell, displayPos.value());

        UserInterface::Core::ExchangeItemOfferEvent eventData{itemPos, displayPos};
        UserInterface::Core::EventBus::GetInstance().Publish(eventData);

        return {};
    }

    // =========================================================================
    // 5. ROZPOCZECIE WYMIANY HANDLOWEJ (SendExchangeStartPacketHandler)
    // =========================================================================

    EterBase::PacketResult<void> SendExchangeStartPacketHandler::SendExchangeStart(EterBase::EntityId targetId, CNetworkStream* networkStream)
    {
        if (!networkStream)
        {
            EterBase::ModernLogger::Error("SendExchangeStartPacketHandler::SendExchangeStart: networkStream is null.");
            return std::unexpected(EterBase::PacketError::SessionClosed);
        }

        if (!targetId)
        {
            EterBase::ModernLogger::Error("SendExchangeStartPacketHandler::SendExchangeStart: invalid targetId.");
            return std::unexpected(EterBase::PacketError::MalformedPayload);
        }

        ProxyPacketCGExchangeStart packet;
        std::memset(&packet, 0, sizeof(packet));

        packet.header = CG::EXCHANGE;
        packet.length = static_cast<uint16_t>(sizeof(packet));
        packet.subheader = ExchangeSub::CG::START;
        packet.targetVid = targetId.get();

        std::span<const uint8_t> buffer(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));

        if (!networkStream->Send(static_cast<int>(buffer.size()), buffer.data()))
        {
            EterBase::ModernLogger::Error("SendExchangeStartPacketHandler::SendExchangeStart: failed to send packet for target {}", targetId.get());
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }

        EterBase::ModernLogger::Debug("SendExchangeStartPacketHandler::SendExchangeStart: successfully sent for target {}", targetId.get());

        return {};
    }

    // =========================================================================
    // 6. NAMIAR POCISKU (SendFlyTargetingPacket)
    // =========================================================================

    EterBase::PacketResult<void> SendFlyTargetingPacket(
        CPythonNetworkStream& stream, 
        EterBase::EntityId targetId, 
        int32_t x, 
        int32_t y
    ) {
        if (targetId.value() == 0) {
            EterBase::ModernLogger::Warn("SendFlyTargetingPacket: Invalid Target ID: {}", targetId.value());
        }

        TPacketCGFlyTargeting packet{};
        packet.header = CG::FLY_TARGETING;
        packet.length = sizeof(packet);
        packet.dwTargetVID = targetId.value();
        packet.lX = x;
        packet.lY = y;

        CPythonBackground::Instance().LocalPositionToGlobalPosition(packet.lX, packet.lY);

        if (!stream.Send(sizeof(packet), &packet)) {
            EterBase::ModernLogger::Error("SendFlyTargetingPacket: stream.Send failed for Target VID {}.", packet.dwTargetVID);
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Debug("SendFlyTargetingPacket: Successfully sent for Target VID {} at ({}, {})", packet.dwTargetVID, packet.lX, packet.lY);
        return {};
    }

    // =========================================================================
    // 7. UPUSZCZENIE PRZEDMIOTU (SendItemDropPacket)
    // =========================================================================

    EterBase::PacketResult<void> SendItemDropPacket::SendItemDrop(uint8_t windowType, EterBase::ItemSlot itemSlot, uint32_t elk, CNetworkStream* networkStream)
    {
        if (!networkStream)
        {
            EterBase::ModernLogger::Error("SendItemDropPacket::SendItemDrop: networkStream is null.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        ProxyPacketCGItemDrop dropPacket{};
        dropPacket.header = CG::ITEM_DROP;
        dropPacket.length = sizeof(dropPacket);
        dropPacket.pos = TItemPos(windowType, itemSlot.value());
        dropPacket.elk = elk;

        std::span<const uint8_t> buffer(reinterpret_cast<const uint8_t*>(&dropPacket), sizeof(dropPacket));
        if (!networkStream->Send(static_cast<int>(buffer.size()), buffer.data()))
        {
            EterBase::ModernLogger::Error("SendItemDropPacket::SendItemDrop: Failed to send item drop packet.");
            return EterBase::MakeError(EterBase::PacketError::SequenceMismatch);
        }

        EterBase::ModernLogger::Debug("SendItemDropPacket::SendItemDrop: Dropped item from window={}, cell={}, elk={}", 
            windowType, itemSlot.value(), elk);

        UserInterface::Core::EventBus::GetInstance().Publish(ItemDropPacketSentEvent(windowType, itemSlot, elk, 0));

        return {};
    }

    EterBase::PacketResult<void> SendItemDropPacket::SendItemDropNew(uint8_t windowType, EterBase::ItemSlot itemSlot, uint32_t gold, uint8_t count, CNetworkStream* networkStream)
    {
        if (!networkStream)
        {
            EterBase::ModernLogger::Error("SendItemDropPacket::SendItemDropNew: networkStream is null.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        ProxyPacketCGItemDrop2 dropPacket{};
        dropPacket.header = CG::ITEM_DROP2;
        dropPacket.length = sizeof(dropPacket);
        dropPacket.pos = TItemPos(windowType, itemSlot.value());
        dropPacket.gold = gold;
        dropPacket.count = count;

        std::span<const uint8_t> buffer(reinterpret_cast<const uint8_t*>(&dropPacket), sizeof(dropPacket));
        if (!networkStream->Send(static_cast<int>(buffer.size()), buffer.data()))
        {
            EterBase::ModernLogger::Error("SendItemDropPacket::SendItemDropNew: Failed to send item drop packet (count).");
            return EterBase::MakeError(EterBase::PacketError::SequenceMismatch);
        }

        EterBase::ModernLogger::Debug("SendItemDropPacket::SendItemDropNew: Dropped item from window={}, cell={}, gold={}, count={}", 
            windowType, itemSlot.value(), gold, count);

        UserInterface::Core::EventBus::GetInstance().Publish(ItemDropPacketSentEvent(windowType, itemSlot, gold, count));

        return {};
    }

    // =========================================================================
    // 8. PRZESUNIECIE PRZEDMIOTU (SendItemMovePacket)
    // =========================================================================

    EterBase::PacketResult<void> SendItemMovePacket(
        const TItemPos& sourcePos, 
        const TItemPos& targetPos, 
        uint8_t count, 
        const std::function<bool(std::span<const uint8_t>)>& sendCallback)
    {
#pragma pack(push, 1)
        TPacketCGItemMove movePacket;
#pragma pack(pop)
        
        std::memset(&movePacket, 0, sizeof(movePacket));
        
        movePacket.header = CG::ITEM_MOVE;
        movePacket.length = sizeof(movePacket);
        movePacket.pos = sourcePos;
        movePacket.change_pos = targetPos;
        movePacket.num = count;

        std::span<const uint8_t> packetSpan(reinterpret_cast<const uint8_t*>(&movePacket), sizeof(movePacket));

        if (!sendCallback(packetSpan))
        {
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Core::NetworkPacketReceivedEvent(CG::ITEM_MOVE, packetSpan));

        return {};
    }

    // =========================================================================
    // 9. PODNIESIENIE PRZEDMIOTU (SendItemPickUpHandler)
    // =========================================================================

    EterBase::PacketResult<void> SendItemPickUpHandler::SendItemPickUp(EterBase::EntityId dropVid, CNetworkStream* networkStream)
    {
        if (!networkStream)
        {
            EterBase::ModernLogger::Error("Failed to send item pickup packet: networkStream is null.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        if (dropVid.value() == 0)
        {
            EterBase::ModernLogger::Error("Failed to send item pickup packet: dropVid is invalid (0).");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        ProxyPacketCGItemPickUp pickUpPacket;
        std::memset(&pickUpPacket, 0, sizeof(pickUpPacket));
        pickUpPacket.header = static_cast<uint8_t>(::CG::ITEM_PICKUP);
        pickUpPacket.vid = dropVid.value();

        std::span<const uint8_t> pickUpBuffer(reinterpret_cast<const uint8_t*>(&pickUpPacket), sizeof(pickUpPacket));
        
        if (!networkStream->Send(static_cast<int>(pickUpBuffer.size()), pickUpBuffer.data()))
        {
            EterBase::ModernLogger::Error("Failed to send item pickup packet: Send() returned false.");
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Core::ItemPickUpEvent(dropVid.value()));

        EterBase::ModernLogger::Info("Item pickup packet sent successfully for VID: {}", dropVid.value());

        return {};
    }

    // =========================================================================
    // 10. UZYCIE PRZEDMIOTU (SendItemUsePacket)
    // =========================================================================

    EterBase::PacketResult<void> SendItemUsePacket(EterBase::ItemSlot slot)
    {
        TItemPos pos(INVENTORY, static_cast<uint16_t>(slot.value()));

        if (!pos.IsValidCell())
        {
            EterBase::ModernLogger::Log(
                EterBase::LogLevel::Warning,
                "Attempted to send ITEM_USE for invalid cell - window: {}, cell: {}",
                pos.window_type, pos.cell
            );
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        TPacketCGItemUse itemUsePacket{};
        itemUsePacket.header = CG::ITEM_USE;
        itemUsePacket.length = sizeof(itemUsePacket);
        itemUsePacket.pos = pos;

        if (!CPythonNetworkStream::Instance().Send(sizeof(TPacketCGItemUse), &itemUsePacket))
        {
            EterBase::ModernLogger::Log(
                EterBase::LogLevel::Error,
                "Failed to transmit ITEM_USE packet for item at window: {}, cell: {}. Session might be closed.",
                pos.window_type, pos.cell
            );
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Log(
            EterBase::LogLevel::Debug,
            "Successfully dispatched ITEM_USE packet - window: {}, cell: {}",
            pos.window_type, pos.cell
        );

        return {};
    }

    // =========================================================================
    // 11. RUCH POSTACI (SendMovePacket)
    // =========================================================================

    EterBase::PacketResult<void> SendMovePacket(const TPixelPosition& position, float rotation, uint8_t func, uint8_t arg)
    {
        if (rotation < 0.0f) {
            rotation = 360.0f + std::fmod(rotation, 360.0f);
        } else if (rotation >= 360.0f) {
            rotation = std::fmod(rotation, 360.0f);
        }

        int32_t x = static_cast<int32_t>(position.x);
        int32_t y = static_cast<int32_t>(position.y);

        CPythonBackground::Instance().LocalPositionToGlobalPosition(x, y);

        TPacketCGMove movePacket{};
        movePacket.header = CG::MOVE;
        movePacket.length = sizeof(TPacketCGMove);
        movePacket.bFunc  = func;
        movePacket.bArg   = arg;
        movePacket.bRot   = static_cast<uint8_t>(rotation / 5.0f);
        movePacket.lX     = x;
        movePacket.lY     = y;
        movePacket.dwTime = ELTimer_GetServerMSec();

        const auto* rawData = reinterpret_cast<const uint8_t*>(&movePacket);
        std::span<const uint8_t> payload(rawData, sizeof(movePacket));

        try {
            UserInterface::Core::EventBus::GetInstance().Publish(Core::NetworkPacketReceivedEvent(static_cast<uint8_t>(CG::MOVE), payload));
        } catch (const std::exception& e) {
            EterBase::ModernLogger::Error("Failed to publish move packet event: {}", e.what());
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Debug("SendMovePacket: Dispatched MOVE event (X: {}, Y: {}, Rot: {})", x, y, rotation);

        return {};
    }

    // =========================================================================
    // 12. KLIKNIECIE W CEL (SendOnClickPacket)
    // =========================================================================

    EterBase::PacketResult<void> SendOnClickPacket(CPythonNetworkStream& stream, EterBase::EntityId targetId)
    {
        TPacketCGOnClick packet{};
        packet.header = CG::ON_CLICK;
        packet.length = sizeof(packet);
        packet.vid    = targetId.value();

        if (!stream.Send(sizeof(packet), &packet))
        {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "Failed to send SendOnClickPacket for targetId: {}", targetId.value());
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Log(EterBase::LogLevel::Trace, "SendOnClickPacket sent successfully for targetId: {}", targetId.value());
        return {};
    }

    // =========================================================================
    // 13. ZAPROSZENIE DO GRUPY (SendPartyInvitePacket)
    // =========================================================================

    EterBase::PacketResult<void> SendPartyInvitePacket(EterBase::EntityId targetId)
    {
        TPacketCGPartyInvite packet{};
        packet.header = CG::PARTY_INVITE;
        packet.length = sizeof(packet);
        packet.vid = targetId.get();

        if (!CPythonNetworkStream::Instance().Send(sizeof(packet), &packet))
        {
            EterBase::ModernLogger::Error("SendPartyInvitePacket [{}] - PACKET SEND ERROR", targetId.get());
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Debug(" << SendPartyInvitePacket : {}", targetId.get());
        return {};
    }

    // =========================================================================
    // 14. ODPOWIEDZ PONG (PongSender)
    // =========================================================================

    EterBase::PacketResult<void> PongSender::Send(CNetworkStream* networkStream)
    {
        if (!networkStream) {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "PongSender::Send failed: networkStream is null.");
            return std::unexpected(EterBase::PacketError::SessionClosed);
        }

        TPacketCGPong pongPacket;
        std::memset(&pongPacket, 0, sizeof(pongPacket));
        pongPacket.header = CG::PONG;
        pongPacket.length = sizeof(pongPacket);

        if (!networkStream->Send(sizeof(pongPacket), &pongPacket)) {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "PongSender::Send failed: could not send packet via networkStream.");
            return std::unexpected(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "PongSender::Send: PONG packet sent successfully.");
        return {};
    }

    // =========================================================================
    // 15. INPUT TEKSTOWY QUESTA (SendQuestInputPacket)
    // =========================================================================

    EterBase::PacketResult<void> SendQuestInputPacket(
        std::string_view inputText,
        const std::function<bool(std::span<const uint8_t>)>& sendCallback)
    {
        if (inputText.length() > LOCAL_QUEST_INPUT_STRING_MAX_NUM) {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Warning, "[NETWORK] SendQuestInputPacket: input text length {} exceeds maximum allowed length of {}", inputText.length(), LOCAL_QUEST_INPUT_STRING_MAX_NUM);
            return std::unexpected(EterBase::PacketError::MalformedPayload);
        }

        PacketQuestInputString packet;
        std::memset(&packet, 0, sizeof(packet));

        packet.header = HEADER_CG_QUEST_INPUT_STRING;
        
        if (!inputText.empty()) {
            std::copy_n(inputText.begin(), inputText.length(), packet.text);
        }

        std::span<const uint8_t> packetSpan(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));

        if (!sendCallback(packetSpan)) {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "[NETWORK] SendQuestInputPacket: Failed to send packet through callback.");
            return std::unexpected(EterBase::PacketError::SessionClosed);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Core::QuestInputSentEvent{std::string(inputText)});

        return {};
    }

    // =========================================================================
    // 16. WLOZENIE DO DEPOZYTU (SendSafeboxCheckinPacketHandler)
    // =========================================================================

    EterBase::PacketResult<void> SendSafeboxCheckinPacketHandler::Send(CNetworkStream* networkStream, uint8_t safeboxSlot, EterBase::ItemSlot inventorySlot)
    {
        if (!networkStream)
        {
            EterBase::ModernLogger::Error("SendSafeboxCheckinPacketHandler::Send - Network stream is null.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        TPacketCGSafeboxCheckin packet{};
        packet.header = CG::SAFEBOX_CHECKIN;
        packet.length = static_cast<uint16_t>(sizeof(packet));
        packet.bSafePos = safeboxSlot;
        packet.ItemPos = TItemPos(INVENTORY, inventorySlot.value());

        std::span<const uint8_t> packetSpan{reinterpret_cast<const uint8_t*>(&packet), sizeof(packet)};

        if (!networkStream->Send(static_cast<int>(packetSpan.size()), packetSpan.data()))
        {
            EterBase::ModernLogger::Error("SendSafeboxCheckinPacketHandler::Send - Failed to send payload (safeboxSlot: {}, inventorySlot: {}).", 
                                          safeboxSlot, inventorySlot.value());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        EterBase::ModernLogger::Info("SendSafeboxCheckinPacketHandler::Send - Checkin successful (safeboxSlot: {}, inventorySlot: {}).", 
                                     safeboxSlot, inventorySlot.value());
                                     
        UserInterface::Core::EventBus::GetInstance().Publish(Core::Events::SafeboxCheckinSentEvent{safeboxSlot, inventorySlot.value()});
                                     
        return {};
    }

    // =========================================================================
    // 17. WYJECIE Z DEPOZYTU (SendSafeboxCheckoutHandler)
    // =========================================================================

    EterBase::PacketResult<void> SendSafeboxCheckoutHandler::SendSafeboxCheckout(EterBase::ItemSlot safeBoxSlot, const TItemPos& inventoryPos, CNetworkStream* networkStream)
    {
        if (!networkStream)
        {
            EterBase::ModernLogger::Error("SendSafeboxCheckout failed: null network stream");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        TPacketCGSafeboxCheckout packet;
        std::memset(&packet, 0, sizeof(packet));
        packet.header = CG::SAFEBOX_CHECKOUT;
        packet.length = sizeof(packet);
        packet.bSafePos = static_cast<uint8_t>(safeBoxSlot.get());
        packet.ItemPos = inventoryPos;

        std::span<const uint8_t> buffer(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
        if (!networkStream->Send(static_cast<int>(buffer.size()), buffer.data()))
        {
            EterBase::ModernLogger::Error("SendSafeboxCheckout failed to send packet data to stream");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Info("SendSafeboxCheckout success: safeboxSlot={}, inventoryPos.window={}, inventoryPos.cell={}", 
            safeBoxSlot.get(), inventoryPos.window_type, inventoryPos.cell);
            
        return {};
    }

    // =========================================================================
    // 18. ODPOWIEDZ DIALOGU QUESTA (SendScriptAnswer)
    // =========================================================================

    EterBase::PacketResult<void> SendScriptAnswer(std::optional<CPythonNetworkStream*> stream, uint8_t answer)
    {
        if (!stream.has_value() || *stream == nullptr)
        {
            EterBase::ModernLogger::Error("SendScriptAnswer failed: Network stream is null.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        CPythonNetworkStream* validStream = *stream;

        TPacketCGScriptAnswer packet{};
        packet.header = CG::SCRIPT_ANSWER;
        packet.length = sizeof(packet);
        packet.answer = answer;

        if (!validStream->Send(sizeof(packet), &packet))
        {
            EterBase::ModernLogger::Error("SendScriptAnswer failed: Stream Send returned false.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Info("SendScriptAnswerPacket sent successfully (answer: {})", answer);
        
        UserInterface::Core::EventBus::GetInstance().Publish(ScriptAnswerSentEvent{answer});
        
        return {};
    }

    // =========================================================================
    // 19. PRZYCISK SKRYPTU QUESTA (SendScriptButtonPacket)
    // =========================================================================

    EterBase::PacketResult<void> SendScriptButtonPacket(uint32_t buttonIndex, CNetworkStream* networkStream)
    {
        if (!networkStream)
        {
            EterBase::ModernLogger::Error("SendScriptButtonPacket failed: networkStream is null.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        TPacketCGScriptButton packet;
        std::memset(&packet, 0, sizeof(packet));

        packet.header = CG::SCRIPT_BUTTON;
        packet.length = static_cast<uint16_t>(sizeof(TPacketCGScriptButton));
        packet.idx = buttonIndex;

        std::span<const uint8_t> buffer(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));

        if (!networkStream->Send(static_cast<int>(buffer.size()), buffer.data()))
        {
            EterBase::ModernLogger::Error("SendScriptButtonPacket failed: network send error.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Debug("Successfully sent SCRIPT_BUTTON packet for buttonIndex={}.", buttonIndex);

        UserInterface::Core::EventBus::GetInstance().Publish(
            UserInterface::Core::ScriptButtonSentEvent(buttonIndex)
        );

        return {};
    }

    // =========================================================================
    // 20. STRZAL / POCISK UMIEJETNOSCI (SendShootPacket)
    // =========================================================================

    EterBase::PacketResult<void> SendShootPacket(CPythonNetworkStream& stream, EterBase::SkillId skillId)
    {
        TPacketCGShoot packet;
        packet.header = CG::SHOOT;
        packet.length = sizeof(packet);
        packet.bType = static_cast<uint8_t>(skillId.value());

        if (!stream.Send(sizeof(packet), &packet))
        {
            EterBase::ModernLogger::Error("Failed to send shoot packet for skill ID {}", skillId.value());
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Debug("Successfully sent shoot packet for skill ID {}", skillId.value());

        return {};
    }

    EterBase::PacketResult<void> SendShootPacket(EterBase::SkillId skillId)
    {
        return SendShootPacket(CPythonNetworkStream::Instance(), skillId);
    }

    // =========================================================================
    // 21. SKLEP (SendShop*)
    // =========================================================================

    EterBase::PacketResult<void> SendShopEnd()
    {
        auto& stream = CPythonNetworkStream::Instance();

        TPacketCGShop packetShop;
        packetShop.header = CG::SHOP;
        packetShop.length = sizeof(packetShop);
        packetShop.subheader = ShopSub::CG::END;

        if (!stream.Send(sizeof(packetShop), &packetShop))
        {
            EterBase::ModernLogger::Error("SendShopEnd: Failed to send TPacketCGShop.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        return {};
    }

    EterBase::PacketResult<void> SendShopBuy(uint8_t count, EterBase::ItemSlot position)
    {
        auto& stream = CPythonNetworkStream::Instance();

        TPacketCGShop packetShop;
        packetShop.header = CG::SHOP;
        packetShop.length = sizeof(packetShop) + sizeof(uint8_t) + sizeof(uint8_t);
        packetShop.subheader = ShopSub::CG::BUY;

        if (!stream.Send(sizeof(packetShop), &packetShop))
        {
            EterBase::ModernLogger::Error("SendShopBuy: Failed to send TPacketCGShop header.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        if (!stream.Send(sizeof(uint8_t), &count))
        {
            EterBase::ModernLogger::Error("SendShopBuy: Failed to send count.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        uint8_t rawPosition = static_cast<uint8_t>(position.value());
        if (!stream.Send(sizeof(uint8_t), &rawPosition))
        {
            EterBase::ModernLogger::Error("SendShopBuy: Failed to send position.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        return {};
    }

    EterBase::PacketResult<void> SendShopSell(EterBase::ItemSlot slot)
    {
        auto& stream = CPythonNetworkStream::Instance();

        TPacketCGShop packetShop;
        packetShop.header = CG::SHOP;
        packetShop.length = sizeof(packetShop) + sizeof(uint8_t);
        packetShop.subheader = ShopSub::CG::SELL;

        if (!stream.Send(sizeof(packetShop), &packetShop))
        {
            EterBase::ModernLogger::Error("SendShopSell: Failed to send TPacketCGShop header.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        uint8_t rawSlot = static_cast<uint8_t>(slot.value());
        if (!stream.Send(sizeof(uint8_t), &rawSlot))
        {
            EterBase::ModernLogger::Error("SendShopSell: Failed to send slot.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        return {};
    }

    EterBase::PacketResult<void> SendShopSellNew(EterBase::ItemSlot slot, uint8_t count)
    {
        auto& stream = CPythonNetworkStream::Instance();

        TPacketCGShop packetShop;
        packetShop.header = CG::SHOP;
        packetShop.length = sizeof(packetShop) + sizeof(uint8_t) + sizeof(uint8_t);
        packetShop.subheader = ShopSub::CG::SELL2;

        if (!stream.Send(sizeof(packetShop), &packetShop))
        {
            EterBase::ModernLogger::Error("SendShopSellNew: Failed to send TPacketCGShop header.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        uint8_t rawSlot = static_cast<uint8_t>(slot.value());
        if (!stream.Send(sizeof(uint8_t), &rawSlot))
        {
            EterBase::ModernLogger::Error("SendShopSellNew: Failed to send slot.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        if (!stream.Send(sizeof(uint8_t), &count))
        {
            EterBase::ModernLogger::Error("SendShopSellNew: Failed to send count.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Debug("SendShopSellNew(slot={}, count={})", rawSlot, count);

        return {};
    }

    // =========================================================================
    // 22. SYNCHRONIZACJA POZYCJI (SendSyncPositionPacket)
    // =========================================================================

    EterBase::PacketResult<void> SendSyncPositionPacket(CPythonNetworkStream& stream, std::span<const SyncPositionTarget> targets)
    {
        if (targets.empty()) {
            EterBase::ModernLogger::Error("SendSyncPositionPacket failed: targets span is empty.");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        TPacketCGSyncPosition packetHeader;
        packetHeader.header = CG::SYNC_POSITION;
        packetHeader.length = static_cast<uint16_t>(sizeof(TPacketCGSyncPosition) + sizeof(TPacketCGSyncPositionElement) * targets.size());

        if (!stream.Send(sizeof(packetHeader), &packetHeader)) {
            EterBase::ModernLogger::Error("SendSyncPositionPacket failed: stream.Send(header) rejected.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

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

    // =========================================================================
    // 23. ZAZNACZENIE CELU (SendTargetPacket)
    // =========================================================================

    EterBase::PacketResult<void> SendTargetPacket::Execute(CNetworkStream& stream, EterBase::EntityId targetId)
    {
        TPacketCGTarget packet{};
        packet.header = CG::TARGET;
        packet.length = sizeof(packet);
        packet.dwVID = targetId.value();

        if (!stream.Send(sizeof(packet), &packet))
        {
            EterBase::ModernLogger::Error("Failed to send CG::TARGET packet. EntityId: {}", targetId.value());
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Debug("Sent CG::TARGET packet successfully. EntityId: {}", targetId.value());
        
        UserInterface::Core::EventBus::GetInstance().Publish(
            UserInterface::Core::TargetBoardRefreshEvent(targetId.value())
        );

        return {};
    }

    // =========================================================================
    // 24. UZYCIE UMIEJETNOSCI (SendUseSkillHandler)
    // =========================================================================

    EterBase::PacketDispatchResult<void> SendUseSkillHandler::SendUseSkill(
        EterBase::SkillId skillId, 
        std::optional<EterBase::EntityId> targetId, 
        CNetworkStream* networkStream)
    {
        if (!networkStream)
        {
            EterBase::ModernLogger::Error("SendUseSkill failed: NetworkStream is null (skill: {}, target: {})", 
                                          skillId.value(), 
                                          targetId.transform([](auto id) { return id.value(); }).value_or(0));
            return EterBase::MakeError(EterBase::PacketDispatchError::Disconnected);
        }

        ProxyPacketCGUseSkill packet{};
        packet.header = 0x0402; // CG::USE_SKILL
        packet.length = static_cast<uint16_t>(sizeof(packet));
        packet.skillId = skillId.value();
        packet.targetId = targetId.transform([](auto id) { return id.value(); }).value_or(0);

        std::span<const uint8_t> buffer(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));

        if (!networkStream->Send(static_cast<int>(buffer.size()), buffer.data()))
        {
            EterBase::ModernLogger::Error("SendUseSkill failed to send buffer to network stream (skill: {}, target: {})", 
                                          skillId.value(), packet.targetId);
            return EterBase::MakeError(EterBase::PacketDispatchError::QueueFull);
        }

        EterBase::ModernLogger::Debug("SendUseSkill succeeded (skill: {}, target: {})", 
                                      skillId.value(), packet.targetId);

        UserInterface::Core::EventBus::GetInstance().Publish(SkillUseRequestedEvent{skillId, targetId});

        return {};
    }

    // =========================================================================
    // 25. WIADOMOSC PRYWATNA / SZEPT (SendWhisperPacket)
    // =========================================================================

    EterBase::PacketResult<void> SendWhisperPacket(
        std::string_view targetName,
        std::string_view message,
        const std::function<bool(std::span<const uint8_t>)>& sendCallback)
    {
        if (message.length() >= 255)
        {
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        if (targetName.empty() || targetName.length() > CHARACTER_NAME_MAX_LEN)
        {
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        const uint16_t messageLength = static_cast<uint16_t>(message.length() + 1);

        TPacketCGWhisper whisperPacket;
        std::memset(&whisperPacket, 0, sizeof(whisperPacket));
        whisperPacket.header = CG::WHISPER;
        whisperPacket.length = static_cast<uint16_t>(sizeof(whisperPacket) + messageLength);

        std::memcpy(whisperPacket.szNameTo, targetName.data(), targetName.length());

        std::vector<uint8_t> buffer;
        buffer.reserve(whisperPacket.length);

        const uint8_t* packetPtr = reinterpret_cast<const uint8_t*>(&whisperPacket);
        buffer.insert(buffer.end(), packetPtr, packetPtr + sizeof(whisperPacket));
        
        const uint8_t* messagePtr = reinterpret_cast<const uint8_t*>(message.data());
        buffer.insert(buffer.end(), messagePtr, messagePtr + message.length());
        buffer.push_back('\0');

        std::span<const uint8_t> packetSpan(buffer.data(), buffer.size());

        if (!sendCallback(packetSpan))
        {
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        return {};
    }

} // namespace Network::Senders
