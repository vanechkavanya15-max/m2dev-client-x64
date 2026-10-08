#pragma once

#include <string>
#include <vector>
#include <expected>
#include <cstdint>
#include "../../EterBase/StrongTypes.h"

namespace Client::Data {

enum class ProtoParseError {
    FileNotFound,
    InvalidHeader,
    MissingColumn,
    ParseError
};

struct ItemProtoRecord {
    EterBase::ItemVnum vnum;
    std::string name;
    uint32_t type{0};
    uint32_t subtype{0};
    uint64_t gold{0};
};

class ProtoCSVParserModern {
public:
    // Parses the content of a CSV file and returns a vector of records or an error
    static std::expected<std::vector<ItemProtoRecord>, ProtoParseError> Parse(const std::string& content);
};

} // namespace Client::Data
