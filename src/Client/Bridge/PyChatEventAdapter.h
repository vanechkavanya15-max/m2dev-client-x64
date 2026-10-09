#pragma once

#include <string>
#include <cstdint>
#include <string_view>
#include "UserInterface/Core/EventBus.h"
#include "UserInterface/PythonChat.h"
#include "Client/Gameplay/ChatCommandHandler.h"

namespace Client::Bridge {

// Zdarzenie transportujace pojedyncza linie czatu
struct ChatLineEvent : public Core::IEvent {
    Client::Gameplay::ChatType type;
    std::string message;

    ChatLineEvent(Client::Gameplay::ChatType t, std::string_view msg)
        : type(t), message(msg) {}
};

// Adapter wzorca Bridge izolujacy UI od logiki gry
class PyChatEventAdapter {
public:
    PyChatEventAdapter() {
        m_subscriptionId = Core::EventBus::Instance().Subscribe<ChatLineEvent>(
            [this](const ChatLineEvent& event) {
                this->OnChatLine(event);
            });
    }

    ~PyChatEventAdapter() {
        Core::EventBus::Instance().Unsubscribe<ChatLineEvent>(m_subscriptionId);
    }

    // Blokada kopiowania i przenoszenia (RAII / zapobieganie double-unsubscribe)
    PyChatEventAdapter(const PyChatEventAdapter&) = delete;
    PyChatEventAdapter& operator=(const PyChatEventAdapter&) = delete;
    PyChatEventAdapter(PyChatEventAdapter&&) = delete;
    PyChatEventAdapter& operator=(PyChatEventAdapter&&) = delete;

private:
    void OnChatLine(const ChatLineEvent& event) {
        if (event.message.empty()) {
            return;
        }

        int legacyType = MapChatTypeToLegacy(event.type);
        CPythonChat::Instance().AppendChat(legacyType, event.message.c_str());
    }

    constexpr int MapChatTypeToLegacy(Client::Gameplay::ChatType type) const noexcept {
        switch (type) {
            case Client::Gameplay::ChatType::Normal:
                return CHAT_TYPE_TALKING;
            case Client::Gameplay::ChatType::Shout:
                return CHAT_TYPE_SHOUT;
            case Client::Gameplay::ChatType::Group:
                return CHAT_TYPE_PARTY;
            case Client::Gameplay::ChatType::Guild:
                return CHAT_TYPE_GUILD;
            default:
                return CHAT_TYPE_TALKING;
        }
    }

    uint32_t m_subscriptionId{0};
};

} // namespace Client::Bridge
