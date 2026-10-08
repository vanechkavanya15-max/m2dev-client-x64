#include <iostream>
#include <span>
#include <vector>
#include <cstdint>
#include <cstring>
#include "../src/EterBase/Result.h"

// Mocks to allow compilation
struct TPacketGCQuestInfo {
    uint16_t header; uint16_t length; uint16_t index; uint8_t flag;
};
#define QUEST_SEND_IS_BEGIN (1 << 0)
#define QUEST_SEND_TITLE (1 << 1)
#define QUEST_SEND_CLOCK_NAME (1 << 2)
#define QUEST_SEND_CLOCK_VALUE (1 << 3)
#define QUEST_SEND_COUNTER_NAME (1 << 4)
#define QUEST_SEND_COUNTER_VALUE (1 << 5)
#define QUEST_SEND_ICON_FILE (1 << 6)

struct TPacketGCScript {
    uint16_t header; uint16_t length; uint8_t skin; uint16_t src_size;
};
struct TPacketGCWarp {
    uint16_t header; uint16_t length; int32_t lX; int32_t lY; int32_t lAddr; uint16_t wPort;
};

namespace UserInterface::Core {
    struct IEvent { virtual ~IEvent() = default; };
    struct EventBus {
        static EventBus& GetInstance() { static EventBus e; return e; }
        template<typename T> void Publish(const T&) {}
    };
}

struct CPythonQuest {
    static CPythonQuest& Instance() { static CPythonQuest q; return q; }
    void DeleteQuestInstance(uint16_t) {}
    bool IsQuest(uint16_t) { return false; }
    void MakeQuest(uint16_t) {}
    void SetQuestTitle(uint16_t, const char*) {}
    void SetQuestClockName(uint16_t, const char*) {}
    void SetQuestCounterName(uint16_t, const char*) {}
    void SetQuestIconFileName(uint16_t, const char*) {}
    void SetQuestClockValue(uint16_t, int) {}
    void SetQuestCounterValue(uint16_t, int) {}
    struct SQuestInstance {
        uint32_t dwIndex; std::string strTitle, strClockName, strCounterName, strIconFileName;
        int iClockValue, iCounterValue;
    };
    void RegisterQuestInstance(SQuestInstance&) {}
};

struct CPythonEventManager {
    static CPythonEventManager& Instance() { static CPythonEventManager e; return e; }
    int RegisterEventSetFromString(const std::string&) { return 1; }
    void SetVisibleLineCount(int, int) {}
};

struct CPythonNetworkStream {
    static CPythonNetworkStream& Instance() { static CPythonNetworkStream s; return s; }
    void OnScriptEventStart(uint8_t, int) {}
};

#define TEST_COMPILATION
#include "../src/Client/Network/Handlers/QuestHandler.h"
#include "../src/Client/Network/Handlers/QuestHandler.cpp"

void testHandleWarp() {
    Network::Handlers::QuestHandler handler;
    std::vector<uint8_t> buffer(sizeof(TPacketGCWarp));
    TPacketGCWarp warp;
    warp.lX = 100; warp.lY = 200; warp.lAddr = 12345; warp.wPort = 1234;
    std::memcpy(buffer.data(), &warp, sizeof(warp));

    auto result = handler.HandleWarp(buffer);
    if (result.has_value()) {
        std::cout << "Test passed (HandleWarp)" << std::endl;
    } else {
        std::cout << "Test failed (HandleWarp)" << std::endl;
    }
}

void testHandleScript() {
    Network::Handlers::QuestHandler handler;
    std::string scriptContent = "def test(): pass";
    std::vector<uint8_t> buffer(sizeof(TPacketGCScript) + scriptContent.size() + 1);
    
    TPacketGCScript sp;
    sp.length = buffer.size();
    sp.skin = 1;
    sp.src_size = scriptContent.size();
    
    std::memcpy(buffer.data(), &sp, sizeof(sp));
    std::memcpy(buffer.data() + sizeof(sp), scriptContent.c_str(), scriptContent.size());
    buffer.back() = '\0';
    
    auto result = handler.HandleScript(buffer);
    if (result.has_value()) {
        std::cout << "Test passed (HandleScript)" << std::endl;
    } else {
        std::cout << "Test failed (HandleScript)" << std::endl;
    }
}

void testHandleQuestInfo() {
    Network::Handlers::QuestHandler handler;
    std::vector<uint8_t> buffer(sizeof(TPacketGCQuestInfo) + sizeof(uint8_t) + 31);
    
    TPacketGCQuestInfo qi;
    qi.flag = QUEST_SEND_IS_BEGIN | QUEST_SEND_TITLE;
    qi.index = 10;
    
    size_t offset = 0;
    std::memcpy(buffer.data(), &qi, sizeof(qi));
    offset += sizeof(qi);
    
    uint8_t isBegin = 1;
    std::memcpy(buffer.data() + offset, &isBegin, sizeof(isBegin));
    offset += sizeof(isBegin);
    
    std::string title = "Test Quest Title";
    std::memcpy(buffer.data() + offset, title.c_str(), title.size());
    buffer[offset + title.size()] = '\0';
    
    auto result = handler.HandleQuestInfo(buffer);
    if (result.has_value()) {
        std::cout << "Test passed (HandleQuestInfo)" << std::endl;
    } else {
        std::cout << "Test failed (HandleQuestInfo)" << std::endl;
    }
}

int main() {
    testHandleWarp();
    testHandleScript();
    testHandleQuestInfo();
    return 0;
}
