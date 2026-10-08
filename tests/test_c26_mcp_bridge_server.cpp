#include <iostream>
#include <cassert>
#include <string>
#include "../src/Client/IPC/MCPBridgeServer.h"

// Basic assertion macro
#define ASSERT_TRUE(condition, message) \
    if (!(condition)) { \
        std::cerr << "Assertion failed: " << message << std::endl; \
        assert(condition); \
    }

int main()
{
    Client::IPC::MCPBridgeServer server;

    std::cout << "Testing 'initialize' method..." << std::endl;
    std::string initReq = R"({"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {}})";
    std::string initRes = server.HandleRequest(initReq);
    ASSERT_TRUE(initRes.find(R"("jsonrpc":"2.0")") != std::string::npos, "Missing jsonrpc version");
    ASSERT_TRUE(initRes.find(R"("id":1)") != std::string::npos, "Missing or wrong id");
    ASSERT_TRUE(initRes.find(R"("protocolVersion")") != std::string::npos, "Missing protocolVersion");
    ASSERT_TRUE(initRes.find(R"("serverInfo")") != std::string::npos, "Missing serverInfo");
    ASSERT_TRUE(initRes.find(R"("capabilities")") != std::string::npos, "Missing capabilities");

    std::cout << "Testing 'tools/list' method..." << std::endl;
    std::string listReq = R"({"jsonrpc": "2.0", "id": 2, "method": "tools/list", "params": {}})";
    std::string listRes = server.HandleRequest(listReq);
    ASSERT_TRUE(listRes.find(R"("jsonrpc":"2.0")") != std::string::npos, "Missing jsonrpc version");
    ASSERT_TRUE(listRes.find(R"("id":2)") != std::string::npos, "Missing or wrong id");
    ASSERT_TRUE(listRes.find(R"("tools")") != std::string::npos, "Missing tools array");
    ASSERT_TRUE(listRes.find(R"("m2_goto")") != std::string::npos, "Missing tool m2_goto");
    ASSERT_TRUE(listRes.find(R"("m2_attack_target")") != std::string::npos, "Missing tool m2_attack_target");
    ASSERT_TRUE(listRes.find(R"("m2_pickup_loot")") != std::string::npos, "Missing tool m2_pickup_loot");
    ASSERT_TRUE(listRes.find(R"("m2_use_skill")") != std::string::npos, "Missing tool m2_use_skill");
    ASSERT_TRUE(listRes.find(R"("m2_use_item")") != std::string::npos, "Missing tool m2_use_item");
    ASSERT_TRUE(listRes.find(R"("m2_get_inventory")") != std::string::npos, "Missing tool m2_get_inventory");
    ASSERT_TRUE(listRes.find(R"("m2_inspect_surroundings")") != std::string::npos, "Missing tool m2_inspect_surroundings");
    ASSERT_TRUE(listRes.find(R"("m2_get_player_state")") != std::string::npos, "Missing tool m2_get_player_state");

    std::cout << "Testing 'tools/call' method..." << std::endl;
    std::string callReq = R"({"jsonrpc": "2.0", "id": 3, "method": "tools/call", "params": {"name": "m2_goto", "arguments": {"x": 100, "y": 200}}})";
    std::string callRes = server.HandleRequest(callReq);
    ASSERT_TRUE(callRes.find(R"("jsonrpc":"2.0")") != std::string::npos, "Missing jsonrpc version");
    ASSERT_TRUE(callRes.find(R"("id":3)") != std::string::npos, "Missing or wrong id");
    ASSERT_TRUE(callRes.find(R"("content")") != std::string::npos, "Missing content array");
    ASSERT_TRUE(callRes.find(R"("isError":false)") != std::string::npos, "isError should be false");

    // Invalid JSON
    std::cout << "Testing invalid request..." << std::endl;
    std::string invalidReq = R"({"jsonrpc": "2.0", "id": 4, "method": "non_existent"})";
    std::string invalidRes = server.HandleRequest(invalidReq);
    ASSERT_TRUE(invalidRes.find(R"("error")") != std::string::npos, "Should return an error for unknown method");
    ASSERT_TRUE(invalidRes.find(R"(-32601)") != std::string::npos, "Should return Method not found code");

    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
