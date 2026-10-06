#pragma once

#include <cstdint>
#include <string>
#include <span>
#include <cstring>
#include "EventBus.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"

/**
 * @file DialogQuestEvents.h
 * @brief Modern C++23 event structures for NPC Dialogs, Quests, and Script Prompts.
 * 
 * This file defines the event structures used to decouple network packets
 * from the GUI layer via the EventBusHeadless subsystem. All parsing methods
 * use std::expected (EterBase::PacketResult) instead of legacy output parameters.
 */

namespace UserInterface::Core::Events {

    /**
     * @brief Event triggered when an NPC dialog is requested or opened.
     */
    struct NpcDialogOpened : public UserInterface::Core::IEvent {
        EterBase::EntityId npcId; ///< The strong-typed EntityId of the NPC.

        /**
         * @brief Constructs the NPC dialog opened event.
         * @param id The ID of the NPC being interacted with.
         */
        explicit NpcDialogOpened(EterBase::EntityId id) : npcId(id) {}

        /**
         * @brief Parses raw packet buffer into the event safely.
         * @param buffer A span covering the packet payload.
         * @return EterBase::PacketResult<NpcDialogOpened> with the parsed event or an error.
         */
        static EterBase::PacketResult<NpcDialogOpened> FromPacket(std::span<const uint8_t> buffer) {
            if (buffer.size() < sizeof(uint32_t)) {
                EterBase::ModernLogger::Error("Failed to parse NpcDialogOpened: Buffer underflow.");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            uint32_t rawId = 0;
            std::memcpy(&rawId, buffer.data(), sizeof(uint32_t));
            return NpcDialogOpened{EterBase::EntityId{rawId}};
        }
    };

    /**
     * @brief Event triggered when a quest state is advanced or updated.
     */
    struct QuestStateAdvanced : public UserInterface::Core::IEvent {
        uint16_t questIndex; ///< The unique network index of the quest.
        uint8_t stateFlag;   ///< The new state flag or progress marker.

        /**
         * @brief Constructs the quest state advanced event.
         * @param index The quest network index.
         * @param flag The state flag of the quest.
         */
        QuestStateAdvanced(uint16_t index, uint8_t flag) 
            : questIndex(index), stateFlag(flag) {}

        /**
         * @brief Parses raw packet buffer into the event safely.
         * @param buffer A span covering the packet payload.
         * @return EterBase::PacketResult<QuestStateAdvanced> with the parsed event or an error.
         */
        static EterBase::PacketResult<QuestStateAdvanced> FromPacket(std::span<const uint8_t> buffer) {
            constexpr size_t expectedSize = sizeof(uint16_t) + sizeof(uint8_t);
            if (buffer.size() < expectedSize) {
                EterBase::ModernLogger::Error("Failed to parse QuestStateAdvanced: Buffer underflow.");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            uint16_t index = 0;
            std::memcpy(&index, buffer.data(), sizeof(uint16_t));
            
            uint8_t flag = buffer[sizeof(uint16_t)];
            
            return QuestStateAdvanced{index, flag};
        }
    };

    /**
     * @brief Event triggered when a script prompt payload is received from the server.
     */
    struct ScriptPromptReceived : public UserInterface::Core::IEvent {
        uint8_t skin;          ///< The UI skin layout parameter for the prompt.
        std::string content;   ///< The script content string.

        /**
         * @brief Constructs the script prompt received event.
         * @param skin The visual layout parameter.
         * @param content The string payload of the script prompt.
         */
        ScriptPromptReceived(uint8_t skin, std::string content) 
            : skin(skin), content(std::move(content)) {}

        /**
         * @brief Parses raw packet buffer into the event safely.
         * @param buffer A span covering the packet payload.
         * @return EterBase::PacketResult<ScriptPromptReceived> with the parsed event or an error.
         */
        static EterBase::PacketResult<ScriptPromptReceived> FromPacket(std::span<const uint8_t> buffer) {
            if (buffer.empty()) {
                EterBase::ModernLogger::Error("Failed to parse ScriptPromptReceived: Buffer underflow.");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            uint8_t skin = buffer[0];
            std::string content;
            
            if (buffer.size() > sizeof(uint8_t)) {
                content.assign(
                    reinterpret_cast<const char*>(buffer.data() + sizeof(uint8_t)), 
                    buffer.size() - sizeof(uint8_t)
                );
            }

            return ScriptPromptReceived{skin, std::move(content)};
        }
    };

} // namespace UserInterface::Core::Events
