#include "ChatFilterEngine.h"
#include <algorithm>
#include <cctype>

namespace Client::Gameplay {

void ChatFilterEngine::AddForbiddenWord(std::string_view word) {
    if (word.empty()) return;
    std::string lowerWord{word};
    std::transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    m_forbiddenWords.push_back(lowerWord);
}

std::string ChatFilterEngine::SanitizeMessage(std::string_view message) const {
    std::string noControls;
    noControls.reserve(message.size());
    for (char c : message) {
        // Remove non-printable ASCII control characters (0-31 and 127)
        if ((c >= 0 && c < 32) || c == 127) {
            continue;
        }
        noControls.push_back(c);
    }

    struct Element {
        bool isTag;
        std::string text;
    };
    std::vector<Element> elements;
    
    size_t i = 0;
    while (i < noControls.size()) {
        if (noControls[i] == '|') {
            bool validTag = false;
            size_t tagLen = 0;
            if (i + 1 < noControls.size()) {
                char next = noControls[i + 1];
                if (next == 'c' || next == 'C') {
                    if (i + 9 < noControls.size()) {
                        bool isHex = true;
                        for (size_t j = 2; j < 10; ++j) {
                            if (!std::isxdigit(static_cast<unsigned char>(noControls[i + j]))) {
                                isHex = false;
                                break;
                            }
                        }
                        if (isHex) {
                            validTag = true;
                            tagLen = 10;
                        }
                    }
                } else if (next == 'h' || next == 'H' || next == 'r' || next == 'R') {
                    // Check if it's a |H tag with a matching |h
                    if (next == 'H') {
                        size_t endH = noControls.find("|h", i + 2);
                        if (endH != std::string::npos) {
                            validTag = true;
                            tagLen = (endH + 2) - i;
                        } else {
                            validTag = true;
                            tagLen = 2;
                        }
                    } else {
                        validTag = true;
                        tagLen = 2;
                    }
                }
            }
            if (validTag) {
                elements.push_back({true, noControls.substr(i, tagLen)});
                i += tagLen;
                continue;
            }
        }
        
        std::string textBlock;
        while (i < noControls.size()) {
            if (noControls[i] == '|') {
                bool nextIsTag = false;
                if (i + 1 < noControls.size()) {
                    char next = noControls[i + 1];
                    if (next == 'c' || next == 'C') {
                        if (i + 9 < noControls.size()) {
                            bool isHex = true;
                            for (size_t j = 2; j < 10; ++j) {
                                if (!std::isxdigit(static_cast<unsigned char>(noControls[i + j]))) {
                                    isHex = false;
                                    break;
                                }
                            }
                            if (isHex) nextIsTag = true;
                        }
                    } else if (next == 'h' || next == 'H' || next == 'r' || next == 'R') {
                        nextIsTag = true;
                    }
                }
                if (nextIsTag) break;
            }
            textBlock.push_back(noControls[i]);
            i++;
        }
        elements.push_back({false, textBlock});
    }

    std::string plainText;
    std::vector<size_t> plainToElementMap;
    std::vector<size_t> plainToCharMap;
    
    for (size_t e = 0; e < elements.size(); ++e) {
        if (!elements[e].isTag) {
            for (size_t c = 0; c < elements[e].text.size(); ++c) {
                plainText.push_back(elements[e].text[c]);
                plainToElementMap.push_back(e);
                plainToCharMap.push_back(c);
            }
        }
    }
    
    std::string lowerPlainText = plainText;
    std::transform(lowerPlainText.begin(), lowerPlainText.end(), lowerPlainText.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
                   
    for (const auto& fw : m_forbiddenWords) {
        size_t pos = 0;
        while ((pos = lowerPlainText.find(fw, pos)) != std::string::npos) {
            for (size_t k = 0; k < fw.size(); ++k) {
                lowerPlainText[pos + k] = '*';
                size_t e = plainToElementMap[pos + k];
                size_t c = plainToCharMap[pos + k];
                elements[e].text[c] = '*';
            }
            pos += fw.size();
        }
    }
    
    std::string result;
    for (const auto& el : elements) {
        result += el.text;
    }
    return result;
}

} // namespace Client::Gameplay
