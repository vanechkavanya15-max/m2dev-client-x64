#include <iostream>
#include <cassert>
#include "../src/Client/Gameplay/ChatFilterEngine.h"
#include "../src/Client/Gameplay/ChatFilterEngine.cpp"

void TestChatFilterEngine() {
    using namespace Client::Gameplay;
    ChatFilterEngine engine;
    engine.AddForbiddenWord("badword");
    engine.AddForbiddenWord("spam");

    // Test 1: Simple forbidden word
    {
        std::string res = engine.SanitizeMessage("This is a badword test.");
        assert(res == "This is a ******* test.");
    }

    // Test 2: Case insensitivity
    {
        std::string res = engine.SanitizeMessage("bAdWoRd and SPAM!");
        assert(res == "******* and ****!");
    }

    // Test 3: Control characters removal
    {
        std::string msg = "Hello\x07\x0A\x0D World";
        std::string res = engine.SanitizeMessage(msg);
        assert(res == "Hello World");
    }

    // Test 4: Color codes are ignored during masking
    {
        std::string res = engine.SanitizeMessage("This is a |cFF00FF00bad|rword test.");
        assert(res == "This is a |cFF00FF00***|r**** test.");
    }

    // Test 5: Invalid color codes are treated as text and masked if matching
    {
        // |cZZZZZZZZ is invalid hex, so it's treated as text. "badword" shouldn't be matched across |c though, wait.
        // Wait, if it's text, it's just characters. Let's test a simple masking with invalid tags.
        engine.AddForbiddenWord("|cZZZZZZZZ");
        std::string res = engine.SanitizeMessage("Invalid |cZZZZZZZZ tag");
        assert(res == "Invalid ********** tag");
    }

    // Test 6: |H ... |h tags
    {
        std::string res = engine.SanitizeMessage("Link: |Hitem:12345|h[Sword of spam]|h");
        assert(res == "Link: |Hitem:12345|h[Sword of ****]|h");
    }

    std::cout << "All tests passed for ChatFilterEngine!" << std::endl;
}

int main() {
    TestChatFilterEngine();
    return 0;
}
