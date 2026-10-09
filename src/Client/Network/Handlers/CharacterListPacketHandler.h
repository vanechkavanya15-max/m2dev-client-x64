#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include <functional>
#include <string_view>
#include <cstring>

#include "../../../EterBase/Result.h"
#include "../../../EterBase/PacketResult.h"
#include "../ModernPacketDispatcher.h"

namespace Client::Network::Handlers
{
    /**
     * @struct CharacterSummary
     * @brief Silnie typowana reprezentacja pojedynczej postaci na liscie wyboru konta.
     */
    struct CharacterSummary
    {
        uint32_t id{0};
        std::string name{};
        uint8_t job{0};
        uint8_t level{0};
        uint32_t playMinutes{0};
        uint8_t st{0};
        uint8_t ht{0};
        uint8_t dx{0};
        uint8_t iq{0};
        uint16_t mainPart{0};
        uint16_t hairPart{0};
        int32_t x{0};
        int32_t y{0};
        uint32_t guildId{0};
        std::string guildName{};
        uint8_t changeName{0};
        uint8_t skillGroup{0};
    };

    /**
     * @struct RawCharacterSlotRecord
     * @brief Binarne odwzorowanie pojedynczego rekordu postaci w protokole sieciowym Metin2.
     */
    #pragma pack(push, 1)
    struct RawCharacterSlotRecord
    {
        uint32_t id;
        char name[24 + 1];
        uint8_t job;
        uint8_t level;
        uint32_t playMinutes;
        uint8_t st;
        uint8_t ht;
        uint8_t dx;
        uint8_t iq;
        uint16_t mainPart;
        uint8_t changeName;
        uint16_t hairPart;
        uint8_t dummy[2]; // padding / align
        int32_t x;
        int32_t y;
        uint32_t addr;
        uint16_t port;
        uint8_t skillGroup;
        uint32_t guildId;
        char guildName[12 + 1];
    };
    #pragma pack(pop)

    /**
     * @class CharacterListPacketHandler
     * @brief Handler przetwarzajacy pakiet listy postaci konta (HEADER_GC_CHARACTER_ADD / HEADER_GC_CHARACTER_LIST).
     * 
     * Implementuje zero-copy parsing z std::span, bezpieczna ekstrakcje stringow bez ryzyka buffer overflow,
     * oraz powiadamia zarejestrowany callback o zaktualizowanej liscie postaci.
     */
    class CharacterListPacketHandler final : public IPacketHandler
    {
    public:
        using CallbackType = std::function<void(const std::vector<CharacterSummary>&)>;

        explicit CharacterListPacketHandler(CallbackType callback = nullptr) noexcept
            : m_callback(std::move(callback))
        {
        }

        ~CharacterListPacketHandler() override = default;

        // Disallow copy and move for thread/session safety
        CharacterListPacketHandler(const CharacterListPacketHandler&) = delete;
        CharacterListPacketHandler& operator=(const CharacterListPacketHandler&) = delete;
        CharacterListPacketHandler(CharacterListPacketHandler&&) = delete;
        CharacterListPacketHandler& operator=(CharacterListPacketHandler&&) = delete;

        void SetCallback(CallbackType callback) noexcept
        {
            m_callback = std::move(callback);
        }

        [[nodiscard]] const std::vector<CharacterSummary>& GetCharacters() const noexcept
        {
            return m_characters;
        }

        [[nodiscard]] EterBase::PacketResult<void> Handle(std::span<const uint8_t> payload) override
        {
            m_characters.clear();

            if (payload.empty())
            {
                return EterBase::PacketResult<void>::Error(EterBase::PacketError::Underflow);
            }

            // Sprawdzamy czy pakiet ma naglowek z liczba postaci czy jest lista rekordow
            size_t offset = 0;
            uint8_t characterCount = 0;

            // Pierwszy bajt moze zawierac liczbe postaci lub opcode byl juz sciety przez dyspozytor
            if (payload.size() >= sizeof(uint8_t) && (payload.size() % sizeof(RawCharacterSlotRecord) == sizeof(uint8_t)))
            {
                characterCount = payload[0];
                offset = 1;
            }
            else
            {
                characterCount = static_cast<uint8_t>(payload.size() / sizeof(RawCharacterSlotRecord));
            }

            const size_t recordsSize = payload.size() - offset;
            const size_t expectedBytes = static_cast<size_t>(characterCount) * sizeof(RawCharacterSlotRecord);

            if (recordsSize < expectedBytes)
            {
                return EterBase::PacketResult<void>::Error(EterBase::PacketError::Underflow);
            }

            m_characters.reserve(characterCount);

            for (uint8_t i = 0; i < characterCount; ++i)
            {
                const size_t recordOffset = offset + (i * sizeof(RawCharacterSlotRecord));
                const auto* rawRecord = reinterpret_cast<const RawCharacterSlotRecord*>(payload.data() + recordOffset);

                // Bezpieczne czytanie stringow null-terminated
                std::string charName;
                charName.assign(rawRecord->name, strnlen(rawRecord->name, sizeof(rawRecord->name)));

                std::string guildName;
                guildName.assign(rawRecord->guildName, strnlen(rawRecord->guildName, sizeof(rawRecord->guildName)));

                CharacterSummary summary;
                summary.id = rawRecord->id;
                summary.name = std::move(charName);
                summary.job = rawRecord->job;
                summary.level = rawRecord->level;
                summary.playMinutes = rawRecord->playMinutes;
                summary.st = rawRecord->st;
                summary.ht = rawRecord->ht;
                summary.dx = rawRecord->dx;
                summary.iq = rawRecord->iq;
                summary.mainPart = rawRecord->mainPart;
                summary.hairPart = rawRecord->hairPart;
                summary.x = rawRecord->x;
                summary.y = rawRecord->y;
                summary.guildId = rawRecord->guildId;
                summary.guildName = std::move(guildName);
                summary.changeName = rawRecord->changeName;
                summary.skillGroup = rawRecord->skillGroup;

                m_characters.push_back(std::move(summary));
            }

            if (m_callback)
            {
                m_callback(m_characters);
            }

            return EterBase::PacketResult<void>::Success();
        }

        [[nodiscard]] uint16_t GetExpectedSize() const override
        {
            return 0; // Dynamic size packet
        }

        [[nodiscard]] bool IsDynamicSize() const override
        {
            return true;
        }

    private:
        CallbackType m_callback;
        std::vector<CharacterSummary> m_characters;
    };
}
