#include "StdAfx.h"
#include "ChatHandler.h"
#include "../../PythonChat.h"
#include <string>

namespace Network::Handlers
{
    EterBase::PacketResult<void> ProcessChatMessage(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketChatHeader))
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketChatHeader*>(buffer.data());
        size_t messageLen = buffer.size() - sizeof(PacketChatHeader);

        if (messageLen == 0)
        {
            return {};
        }

        const char* textPtr = reinterpret_cast<const char*>(buffer.data() + sizeof(PacketChatHeader));
        std::string message(textPtr, messageLen);

        // Zapis do logiki czatu (odseparowane od okna GUI)
        CPythonChat::Instance().AppendChat(packet->type, message.c_str());

        return {};
    }

    bool HandleChatMessage(std::span<const uint8_t> buffer)
    {
        return ProcessChatMessage(buffer).has_value();
    }
}
