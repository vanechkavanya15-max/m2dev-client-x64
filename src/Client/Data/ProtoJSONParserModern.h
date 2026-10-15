#pragma once

#include "EterBase/StdAfx.h"
#include "EterBase/Singleton.h"
#include "UserInterface/StdAfx.h"
#include "GameLib/ItemData.h"
#include "UserInterface/PythonNonPlayer.h"
#include "../../EterBase/Result.h"

#include <vector>
#include <string>
#include <string_view>
#include <expected>
#include <format>

#ifndef ITEM_NAME_MAX_LEN
#define ITEM_NAME_MAX_LEN CItemData::ITEM_NAME_MAX_LEN
#endif
#ifndef ITEM_LIMIT_MAX_NUM
#define ITEM_LIMIT_MAX_NUM CItemData::ITEM_LIMIT_MAX_NUM
#endif
#ifndef ITEM_VALUES_MAX_NUM
#define ITEM_VALUES_MAX_NUM CItemData::ITEM_VALUES_MAX_NUM
#endif
#ifndef ITEM_APPLY_MAX_NUM
#define ITEM_APPLY_MAX_NUM CItemData::ITEM_APPLY_MAX_NUM
#endif
#ifndef ITEM_SOCKET_MAX_NUM
#define ITEM_SOCKET_MAX_NUM CItemData::ITEM_SOCKET_MAX_NUM
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
