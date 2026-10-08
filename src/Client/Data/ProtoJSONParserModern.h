#pragma once


#include <vector>
#include <string>
#include <string_view>
#include <expected>
#include <format>
#include "../../EterBase/Result.h"

#include "../../dummy_structs.h"
#ifndef TEST_MOCK_STRUCTS


#endif

namespace Client::Data {

    enum class ProtoParseError : uint8_t {
        None = 0,
        TypeMismatch,
        MissingField,
        InvalidFormat
    };

    [[nodiscard]] constexpr std::string_view ToString(ProtoParseError err) noexcept {
        switch (err) {
            case ProtoParseError::None: return "None";
            case ProtoParseError::TypeMismatch: return "TypeMismatch";
            case ProtoParseError::MissingField: return "MissingField";
            case ProtoParseError::InvalidFormat: return "InvalidFormat";
        }
        return "UnknownProtoParseError";
    }

    class ProtoJSONParserModern {
    public:
        [[nodiscard]] static EterBase::Result<std::vector<CItemData::TItemTable>, ProtoParseError> ParseItemTable(std::string_view jsonContent);
        [[nodiscard]] static EterBase::Result<std::vector<CPythonNonPlayer::TMobTable>, ProtoParseError> ParseMobTable(std::string_view jsonContent);

        [[nodiscard]] static EterBase::Result<std::string, ProtoParseError> ExportItemTableToJSON(const std::vector<CItemData::TItemTable>& items);
        [[nodiscard]] static EterBase::Result<std::string, ProtoParseError> ExportMobTableToJSON(const std::vector<CPythonNonPlayer::TMobTable>& mobs);
    };

} // namespace Client::Data

template <>
struct std::formatter<Client::Data::ProtoParseError> : std::formatter<std::string_view> {
    auto format(Client::Data::ProtoParseError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Data::ToString(err), ctx);
    }
};
