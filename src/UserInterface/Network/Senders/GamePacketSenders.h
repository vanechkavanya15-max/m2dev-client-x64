#pragma once

#include "../../StdAfx.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../GameType.h"
#include "../../Core/EventBus.h"
#include "../../Packets/Packet_Chat.h"

#include <cstdint>
#include <string_view>
#include <string>
#include <optional>
#include <span>
#include <functional>

// Forward declarations
class CNetworkStream;
class CPythonNetworkStream;

// Zdarzenia domenowe zwizane z pakietami wychodzcymi
namespace UserInterface::Core {

    /**
     * @brief Zdarzenie publikowane przy dodaniu przedmiotu do okna wymiany handlowej.
     */
    struct ExchangeItemOfferEvent : public IEvent {
        TItemPos itemPos;
        EterBase::ItemSlot displayPos;
        ExchangeItemOfferEvent(const TItemPos& item, EterBase::ItemSlot disp)
            : itemPos(item), displayPos(disp) {}
    };

    /**
     * @brief Zdarzenie publikowane przy wyslaniu tekstu questa do serwera.
     */
    struct QuestInputSentEvent : public IEvent {
        std::string input;
        explicit QuestInputSentEvent(std::string input) : input(std::move(input)) {}
    };

    /**
     * @brief Zdarzenie publikowane przy kliknieciu przycisku skryptu questa.
     */
    struct ScriptButtonSentEvent : public IEvent {
        uint32_t buttonIndex;
        explicit ScriptButtonSentEvent(uint32_t buttonIndex) : buttonIndex(buttonIndex) {}
    };

    /**
     * @brief Zdarzenie publikowane przy wyslaniu zadania podniesienia przedmiotu z ziemi.
     */
    struct ItemPickUpEvent : public IEvent {
        uint32_t dropVid{0};
        ItemPickUpEvent() = default;
        explicit ItemPickUpEvent(uint32_t vid) : dropVid(vid) {}
    };

} // namespace UserInterface::Core

namespace Core {
    using UserInterface::Core::ExchangeItemOfferEvent;
    using UserInterface::Core::QuestInputSentEvent;
    using UserInterface::Core::ScriptButtonSentEvent;
    using UserInterface::Core::ItemPickUpEvent;
} // namespace Core

namespace Network::Senders {

    // --- ZDARZENIA SPECYFICZNE DLA NADAJNIKOW PAKIETOW ---

    /**
     * @brief Zdarzenie publikowane po pomyslnym wyslaniu pakietu ataku.
     */
    struct NetworkAttackSentEvent : public UserInterface::Core::IEvent {
        EterBase::EntityId victimId;
        uint8_t attackType;
        NetworkAttackSentEvent(EterBase::EntityId victimId, uint8_t attackType)
            : victimId(victimId), attackType(attackType) {}
    };

    /**
     * @brief Zdarzenie publikowane po wyslaniu pakietu upuszczenia przedmiotu.
     */
    struct ItemDropPacketSentEvent : public UserInterface::Core::IEvent {
        uint8_t windowType;
        EterBase::ItemSlot slot;
        uint32_t gold;
        uint8_t count;
        ItemDropPacketSentEvent(uint8_t windowType, EterBase::ItemSlot slot, uint32_t gold, uint8_t count)
            : windowType(windowType), slot(slot), gold(gold), count(count) {}
    };

    /**
     * @brief Zdarzenie publikowane po pomyslnym wyslaniu odpowiedzi dialogowej NPC.
     */
    struct ScriptAnswerSentEvent : public UserInterface::Core::IEvent {
        uint8_t answer;
        explicit ScriptAnswerSentEvent(uint8_t answer) : answer(answer) {}
    };

    /**
     * @brief Zdarzenie publikowane przy zgloszeniu uzycia umiejetnosci.
     */
    struct SkillUseRequestedEvent : public UserInterface::Core::IEvent {
        EterBase::SkillId skillId;
        std::optional<EterBase::EntityId> targetId;
        explicit SkillUseRequestedEvent(EterBase::SkillId skillId, std::optional<EterBase::EntityId> targetId = std::nullopt)
            : skillId(skillId), targetId(targetId) {}
    };

    // --- STRUKTURY POMOCNICZE PAKIETOW ---

    /**
     * @brief Element docelowy pakietu synchronizacji pozycji zbiorczej.
     */
    struct SyncPositionTarget {
        EterBase::EntityId victimId;
        int32_t x;
        int32_t y;
    };

    // --- 1. ATAK ---

    class AttackSender {
    public:
        static EterBase::PacketDispatchResult<void> Send(EterBase::EntityId victimId, uint8_t attackType, CNetworkStream* networkStream);
    };

    inline EterBase::PacketDispatchResult<void> SendAttackPacket(EterBase::EntityId victimId, uint8_t attackType, CNetworkStream* networkStream) {
        return AttackSender::Send(victimId, attackType, networkStream);
    }

    // --- 2. CZAT ---

    EterBase::PacketResult<void> SendChatPacket(
        std::string_view text, 
        Network::Packets::ChatType type, 
        const std::function<bool(std::span<const uint8_t>)>& sendCallback);

    // --- 3. AKCEPTACJA WYMIANY (EXCHANGE) ---

    EterBase::PacketResult<void> SendExchangeAcceptPacket(CPythonNetworkStream& stream);

    // --- 4. DODANIE PRZEDMIOTU DO WYMIANY ---

    EterBase::PacketResult<void> SendExchangeItemAddPacket(
        const TItemPos& itemPos, 
        EterBase::ItemSlot displayPos, 
        const std::function<bool(std::span<const uint8_t>)>& sendCallback);

    // --- 5. ROZPOCZECIE WYMIANY HANDLOWEJ ---

    class SendExchangeStartPacketHandler {
    public:
        static EterBase::PacketResult<void> SendExchangeStart(EterBase::EntityId targetId, CNetworkStream* networkStream);
    };

    inline EterBase::PacketResult<void> SendExchangeStartPacket(EterBase::EntityId targetId, CNetworkStream* networkStream) {
        return SendExchangeStartPacketHandler::SendExchangeStart(targetId, networkStream);
    }

    inline EterBase::PacketResult<void> SendExchangeStart(EterBase::EntityId targetId, CNetworkStream* networkStream) {
        return SendExchangeStartPacketHandler::SendExchangeStart(targetId, networkStream);
    }

    // --- 6. NAMIAR POCISKU (FLY TARGETING) ---

    EterBase::PacketResult<void> SendFlyTargetingPacket(
        CPythonNetworkStream& stream, 
        EterBase::EntityId targetId, 
        int32_t x, 
        int32_t y
    );

    // --- 7. UPUSZCZENIE PRZEDMIOTU (ITEM DROP) ---

    class SendItemDropPacket {
    public:
        static EterBase::PacketResult<void> SendItemDrop(uint8_t windowType, EterBase::ItemSlot itemSlot, uint32_t elk, CNetworkStream* networkStream);
        static EterBase::PacketResult<void> SendItemDropNew(uint8_t windowType, EterBase::ItemSlot itemSlot, uint32_t gold, uint8_t count, CNetworkStream* networkStream);
    };

    inline EterBase::PacketResult<void> SendItemDrop(uint8_t windowType, EterBase::ItemSlot itemSlot, uint32_t elk, CNetworkStream* networkStream) {
        return SendItemDropPacket::SendItemDrop(windowType, itemSlot, elk, networkStream);
    }

    inline EterBase::PacketResult<void> SendItemDropNew(uint8_t windowType, EterBase::ItemSlot itemSlot, uint32_t gold, uint8_t count, CNetworkStream* networkStream) {
        return SendItemDropPacket::SendItemDropNew(windowType, itemSlot, gold, count, networkStream);
    }

    // --- 8. PRZESUNIECIE PRZEDMIOTU (ITEM MOVE) ---

    EterBase::PacketResult<void> SendItemMovePacket(
        const TItemPos& sourcePos, 
        const TItemPos& targetPos, 
        uint8_t count, 
        const std::function<bool(std::span<const uint8_t>)>& sendCallback);

    // --- 9. PODNIESIENIE PRZEDMIOTU (ITEM PICKUP) ---

    class SendItemPickUpHandler {
    public:
        static EterBase::PacketResult<void> SendItemPickUp(EterBase::EntityId dropVid, CNetworkStream* networkStream);
    };

    inline EterBase::PacketResult<void> SendItemPickUp(EterBase::EntityId dropVid, CNetworkStream* networkStream) {
        return SendItemPickUpHandler::SendItemPickUp(dropVid, networkStream);
    }

    inline EterBase::PacketResult<void> SendItemPickUpPacket(EterBase::EntityId dropVid, CNetworkStream* networkStream) {
        return SendItemPickUpHandler::SendItemPickUp(dropVid, networkStream);
    }

    // --- 10. UZYCIE PRZEDMIOTU (ITEM USE) ---

    EterBase::PacketResult<void> SendItemUsePacket(EterBase::ItemSlot slot);

    // --- 11. RUCH POSTACI (MOVE) ---

    EterBase::PacketResult<void> SendMovePacket(const TPixelPosition& position, float rotation, uint8_t func, uint8_t arg);

    // --- 12. KLIKNIECIE W CEL (ON CLICK) ---

    EterBase::PacketResult<void> SendOnClickPacket(CPythonNetworkStream& stream, EterBase::EntityId targetId);

    // --- 13. ZAPROSZENIE DO GRUPY (PARTY INVITE) ---

    EterBase::PacketResult<void> SendPartyInvitePacket(EterBase::EntityId targetId);

    // --- 14. ODPOWIEDZ PONG (KEEP-ALIVE) ---

    class PongSender {
    public:
        static EterBase::PacketResult<void> Send(CNetworkStream* networkStream);
    };

    inline EterBase::PacketResult<void> SendPongPacket(CNetworkStream* networkStream) {
        return PongSender::Send(networkStream);
    }

    // --- 15. INPUT TEKSTOWY QUESTA ---

    EterBase::PacketResult<void> SendQuestInputPacket(
        std::string_view inputText,
        const std::function<bool(std::span<const uint8_t>)>& sendCallback);

    // --- 16. WLOZENIE DO DEPOZYTU (SAFEBOX CHECKIN) ---

    class SendSafeboxCheckinPacketHandler {
    public:
        static EterBase::PacketResult<void> Send(CNetworkStream* networkStream, uint8_t safeboxSlot, EterBase::ItemSlot inventorySlot);
    };

    inline EterBase::PacketResult<void> SendSafeboxCheckin(CNetworkStream* networkStream, uint8_t safeboxSlot, EterBase::ItemSlot inventorySlot) {
        return SendSafeboxCheckinPacketHandler::Send(networkStream, safeboxSlot, inventorySlot);
    }

    // --- 17. WYJECIE Z DEPOZYTU (SAFEBOX CHECKOUT) ---

    class SendSafeboxCheckoutHandler {
    public:
        static EterBase::PacketResult<void> SendSafeboxCheckout(EterBase::ItemSlot safeBoxSlot, const TItemPos& inventoryPos, CNetworkStream* networkStream);
    };

    inline EterBase::PacketResult<void> SendSafeboxCheckout(EterBase::ItemSlot safeBoxSlot, const TItemPos& inventoryPos, CNetworkStream* networkStream) {
        return SendSafeboxCheckoutHandler::SendSafeboxCheckout(safeBoxSlot, inventoryPos, networkStream);
    }

    // --- 18. ODPOWIEDZ DIALOGU QUESTA ---

    EterBase::PacketResult<void> SendScriptAnswer(std::optional<CPythonNetworkStream*> stream, uint8_t answer);

    // --- 19. PRZYCISK SKRYPTU QUESTA ---

    EterBase::PacketResult<void> SendScriptButtonPacket(uint32_t buttonIndex, CNetworkStream* networkStream);

    // --- 20. STRZAL / POCISK UMIEJETNOSCI (SHOOT) ---

    EterBase::PacketResult<void> SendShootPacket(CPythonNetworkStream& stream, EterBase::SkillId skillId);
    EterBase::PacketResult<void> SendShootPacket(EterBase::SkillId skillId);

    // --- 21. SKLEP (SHOP) ---

    EterBase::PacketResult<void> SendShopEnd();
    EterBase::PacketResult<void> SendShopBuy(uint8_t count, EterBase::ItemSlot position);
    EterBase::PacketResult<void> SendShopSell(EterBase::ItemSlot slot);
    EterBase::PacketResult<void> SendShopSellNew(EterBase::ItemSlot slot, uint8_t count);

    // --- 22. SYNCHRONIZACJA POZYCJI (SYNC POSITION) ---

    EterBase::PacketResult<void> SendSyncPositionPacket(CPythonNetworkStream& stream, std::span<const SyncPositionTarget> targets);

    // --- 23. ZAZNACZENIE CELU (TARGET) ---

    class SendTargetPacket {
    public:
        static EterBase::PacketResult<void> Execute(CNetworkStream& stream, EterBase::EntityId targetId);
    };

    inline EterBase::PacketResult<void> SendTarget(CNetworkStream& stream, EterBase::EntityId targetId) {
        return SendTargetPacket::Execute(stream, targetId);
    }

    // --- 24. UZYCIE UMIEJETNOSCI (USE SKILL) ---

    class SendUseSkillHandler {
    public:
        static EterBase::PacketDispatchResult<void> SendUseSkill(
            EterBase::SkillId skillId, 
            std::optional<EterBase::EntityId> targetId, 
            CNetworkStream* networkStream);
    };

    inline EterBase::PacketDispatchResult<void> SendUseSkill(
        EterBase::SkillId skillId, 
        std::optional<EterBase::EntityId> targetId, 
        CNetworkStream* networkStream) {
        return SendUseSkillHandler::SendUseSkill(skillId, targetId, networkStream);
    }

    // --- 25. WIADOMOSC PRYWATNA / SZEPT (WHISPER) ---

    EterBase::PacketResult<void> SendWhisperPacket(
        std::string_view targetName,
        std::string_view message,
        const std::function<bool(std::span<const uint8_t>)>& sendCallback);

} // namespace Network::Senders

// Aliasy przestrzeni nazw dla pelnej wstecznej kompatybilnosci
namespace UserInterface::Network::Senders {
    using namespace ::Network::Senders;
}

namespace UserInterface::Network {
    using namespace ::Network::Senders;
}

namespace Network {
    using namespace ::Network::Senders;
}

// Aliasy globalne dla klas wczesniej zdefiniowanych poza namespace
using SendExchangeStartPacketHandler = Network::Senders::SendExchangeStartPacketHandler;
using SendSafeboxCheckinPacketHandler = Network::Senders::SendSafeboxCheckinPacketHandler;
