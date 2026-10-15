#pragma once

#include <string>
#include <vector>
#include <string_view>

namespace Client::Gameplay {

class ChatFilterEngine {
public:
    ChatFilterEngine() = default;
    ~ChatFilterEngine() = default;

    void AddForbiddenWord(std::string_view word);
    [[nodiscard]] std::string SanitizeMessage(std::string_view message) const;

private:
    std::vector<std::string> m_forbiddenWords;
};

} // namespace Client::Gameplay
