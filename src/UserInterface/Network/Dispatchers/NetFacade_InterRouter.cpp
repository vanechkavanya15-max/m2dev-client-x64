#include "../../StdAfx.h"
#include "NetFacade_InterRouter.h"

#include "../../Packet.h"
#include "../PacketDispatcher.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"

// Handlery Shop
#include "../Handlers/ShopHandler.h"

// Handlery Exchange
#include "../Handlers/ExchangeHandler.h"
#include "../Handlers/ExchangeStartHandler.h"
#include "../Handlers/ExchangeMoneyHandler.h"
#include "../Handlers/ExchangeAcceptHandler.h"

// Do zdarzeń z EventBus dla Quest/Script
namespace Network::Dispatchers::Events {
    struct ScriptPacketEvent : public UserInterface::Core::IEvent {
        uint8_t skin;
        uint16_t srcSize;
        ScriptPacketEvent(uint8_t s, uint16_t size) : skin(s), srcSize(size) {}
    };

    struct QuestConfirmEvent : public UserInterface::Core::IEvent {
        std::string msg;
        int32_t timeout;
        uint32_t requestPID;
        QuestConfirmEvent(std::string m, int32_t t, uint32_t p) : msg(std::move(m)), timeout(t), requestPID(p) {}
    };

    struct QuestInfoEvent : public UserInterface::Core::IEvent {
        uint16_t index;
        uint8_t flag;
        QuestInfoEvent(uint16_t idx, uint8_t f) : index(idx), flag(f) {}
    };
}

namespace Network::Dispatchers
{
    void NetFacade_InterRouter::RegisterHandlers()
    {
        auto& dispatcher = Network::PacketDispatcher::Instance();

        dispatcher.RegisterModernHandler(GC::SHOP, &NetFacade_InterRouter::HandleShop);
        dispatcher.RegisterModernHandler(GC::EXCHANGE, &NetFacade_InterRouter::HandleExchange);
        dispatcher.RegisterModernHandler(GC::SCRIPT, &NetFacade_InterRouter::HandleScript);
        dispatcher.RegisterModernHandler(GC::QUEST_CONFIRM, &NetFacade_InterRouter::HandleQuestConfirm);
        dispatcher.RegisterModernHandler(GC::QUEST_INFO, &NetFacade_InterRouter::HandleQuestInfo);

        EterBase::ModernLogger::Info("NetFacade_InterRouter: Zarejestrowano handlery sklepow, wymian i dialogow.");
    }

    EterBase::PacketResult<void> NetFacade_InterRouter::HandleShop(std::span<const uint8_t> payload)
    {
        if (payload.size() < sizeof(TPacketGCShop))
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);

        const auto* packet = reinterpret_cast<const TPacketGCShop*>(payload.data());
        
        switch (packet->subheader)
        {
            case ShopSub::GC::START:
            {
                auto eventResult = ShopHandler::HandleShopStart(payload);
                if (eventResult)
                {
                    UserInterface::Core::EventBus::GetInstance().Publish(std::move(eventResult.value()));
                }
                break;
            }
            case ShopSub::GC::START_EX:
            {
                auto eventResult = ShopHandler::HandleShopStartEx(payload);
                if (eventResult)
                {
                    UserInterface::Core::EventBus::GetInstance().Publish(std::move(eventResult.value()));
                }
                break;
            }
            default:
                // Ignorujemy resztę subheaderów na tym poziomie - pozostają one obsługiwane przez starsze fasady
                break;
        }

        return {};
    }

    EterBase::PacketResult<void> NetFacade_InterRouter::HandleExchange(std::span<const uint8_t> payload)
    {
        if (payload.size() < sizeof(TPacketGCExchange))
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);

        const auto* packet = reinterpret_cast<const TPacketGCExchange*>(payload.data());

        switch (packet->subheader)
        {
            case ExchangeSub::GC::START:
                return Network::Handlers::HandleExchangeStart(payload);
                
            case ExchangeSub::GC::ELK_ADD:
                return UserInterface::Network::Handlers::ExchangeMoneyHandler::HandlePacket(*packet);

            case ExchangeSub::GC::ACCEPT:
                return ExchangeAcceptHandler::HandleAccept(*packet);

            default:
                if (!ExchangeHandler::HandlePacket(*packet)) {
                    EterBase::ModernLogger::Error("NetFacade_InterRouter: Blad przetwarzania pakietu Exchange (fallback).");
                    return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
                }
                break;
        }

        return {};
    }

    EterBase::PacketResult<void> NetFacade_InterRouter::HandleScript(std::span<const uint8_t> payload)
    {
        if (payload.size() < sizeof(TPacketGCScript))
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);

        const auto* packet = reinterpret_cast<const TPacketGCScript*>(payload.data());
        
        EterBase::ModernLogger::Info("NetFacade_InterRouter: Przetworzono pakiet SCRIPT. Skin: {}, Size: {}", packet->skin, packet->src_size);
        
        // Emisja eventu do odsprzegniecia UI
        UserInterface::Core::EventBus::GetInstance().Publish(Events::ScriptPacketEvent(packet->skin, packet->src_size));

        return {};
    }

    EterBase::PacketResult<void> NetFacade_InterRouter::HandleQuestConfirm(std::span<const uint8_t> payload)
    {
        if (payload.size() < sizeof(TPacketGCQuestConfirm))
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);

        const auto* packet = reinterpret_cast<const TPacketGCQuestConfirm*>(payload.data());

        // Bezpieczne parsowanie stringa (std::string_view)
        std::string_view msgView(packet->msg, sizeof(packet->msg));
        size_t nullPos = msgView.find('\0');
        std::string msgStr = std::string(nullPos != std::string_view::npos ? msgView.substr(0, nullPos) : msgView);

        EterBase::ModernLogger::Info("NetFacade_InterRouter: Przetworzono QUEST_CONFIRM.");

        // Emisja eventu
        UserInterface::Core::EventBus::GetInstance().Publish(Events::QuestConfirmEvent(msgStr, packet->timeout, packet->requestPID));

        return {};
    }

    EterBase::PacketResult<void> NetFacade_InterRouter::HandleQuestInfo(std::span<const uint8_t> payload)
    {
        if (payload.size() < sizeof(TPacketGCQuestInfo))
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            
        const auto* packet = reinterpret_cast<const TPacketGCQuestInfo*>(payload.data());

        EterBase::ModernLogger::Info("NetFacade_InterRouter: Przetworzono QUEST_INFO. Index: {}", packet->index);

        // Emisja eventu
        UserInterface::Core::EventBus::GetInstance().Publish(Events::QuestInfoEvent(packet->index, packet->flag));

        return {};
    }

} // namespace Network::Dispatchers
