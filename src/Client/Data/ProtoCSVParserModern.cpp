#include "ProtoCSVParserModern.h"
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <cctype>

namespace Client::Data {

namespace {

std::string Trim(const std::string& str) {
    if (str.empty()) return str;

    auto start = str.begin();
    while (start != str.end() && std::isspace(static_cast<unsigned char>(*start))) {
        start++;
    }

    auto end = str.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(static_cast<unsigned char>(*end)));

    return std::string(start, end + 1);
}

std::string ToUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c){ return std::toupper(c); });
    return result;
}

std::vector<std::string> SplitCSVLine(const std::string& line) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    
    // Simplistic split by tab or comma. 
    // We assume the proto might be tab separated or comma separated. Let's handle both.
    // In metin2, CSVs often use \t or , as separators. 
    // A standard csv parses by comma. Let's do comma for now, with handling of quotes if needed, 
    // or just simple split by ','
    // Let's split by \t if it exists, otherwise comma.
    char delimiter = ',';
    if (line.find('\t') != std::string::npos) {
        delimiter = '\t';
    }

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(Trim(token));
    }
    
    // Add empty token if line ends with delimiter
    if (!line.empty() && line.back() == delimiter) {
        tokens.push_back("");
    }

    return tokens;
}

}

std::expected<std::vector<ItemProtoRecord>, ProtoParseError> ProtoCSVParserModern::Parse(const std::string& content) {
    std::vector<ItemProtoRecord> records;
    std::stringstream ss(content);
    std::string line;

    bool headerParsed = false;
    std::unordered_map<std::string, size_t> columnIndices;

    while (std::getline(ss, line)) {
        // Handle CRLF
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::string trimmedLine = Trim(line);

        // Ignore empty lines and comments
        if (trimmedLine.empty() || trimmedLine.starts_with("#") || trimmedLine.starts_with("//")) {
            continue;
        }

        std::vector<std::string> tokens = SplitCSVLine(trimmedLine);

        if (!headerParsed) {
            for (size_t i = 0; i < tokens.size(); ++i) {
                // Remove quotes from header columns
                std::string headerCol = tokens[i];
                if (headerCol.size() >= 2 && headerCol.front() == '"' && headerCol.back() == '"') {
                    headerCol = headerCol.substr(1, headerCol.size() - 2);
                }
                columnIndices[ToUpper(headerCol)] = i;
            }

            // Verify required columns
            if (columnIndices.find("VNUM") == columnIndices.end() ||
                columnIndices.find("NAME") == columnIndices.end()) {
                return std::unexpected(ProtoParseError::InvalidHeader);
            }

            headerParsed = true;
            continue;
        }

        // Parse data row
        ItemProtoRecord record;

        try {
            auto getCol = [&](const std::string& colName) -> std::string {
                auto it = columnIndices.find(colName);
                if (it != columnIndices.end() && it->second < tokens.size()) {
                    std::string val = tokens[it->second];
                    if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
                        val = val.substr(1, val.size() - 2);
                    }
                    return val;
                }
                return "";
            };

            std::string vnumStr = getCol("VNUM");
            if (vnumStr.empty()) {
                return std::unexpected(ProtoParseError::MissingColumn);
            }
            record.vnum = EterBase::ItemVnum(std::stoul(vnumStr, nullptr, 0));
            record.name = getCol("NAME");

            std::string typeStr = getCol("TYPE");
            if (!typeStr.empty()) record.type = std::stoul(typeStr, nullptr, 0);

            std::string subtypeStr = getCol("SUBTYPE");
            if (!subtypeStr.empty()) record.subtype = std::stoul(subtypeStr, nullptr, 0);

            std::string goldStr = getCol("GOLD");
            if (!goldStr.empty()) record.gold = std::stoull(goldStr, nullptr, 0);

            records.push_back(record);
        } catch (const std::exception&) {
            return std::unexpected(ProtoParseError::ParseError);
        }
    }

    return records;
}

} // namespace Client::Data
