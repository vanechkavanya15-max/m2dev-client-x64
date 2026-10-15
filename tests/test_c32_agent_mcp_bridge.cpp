#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
#include <string>

#include "Client/Contracts/IPlayerDomainContract.h"
#include "Client/Contracts/IInventoryDomainContract.h"
#include "Client/Contracts/ICombatDomainContract.h"
#include "Client/Contracts/IWorldQueryContract.h"
#include "Client/MCP/AgentMCPBridge.h"

using namespace Client::Contracts;
using namespace Client::MCP;

// Mock implementacje kontraktow domenowych dla testu integracyjnego MCP
class MockPlayerContract : public IPlayerDomainContract
{
public:
    uint32_t GetPlayerVID() const noexcept override { return 10001; }
    std::string_view GetPlayerName() const noexcept override { return "TestHero"; }
    int GetPlayerLevel() const noexcept override { return 75; }

    int64_t GetCurrentHP() const noexcept override { return 14500; }
    int64_t GetMaxHP() const noexcept override { return 15000; }
    int64_t GetCurrentSP() const noexcept override { return 3000; }
    int64_t GetMaxSP() const noexcept override { return 3000; }
    int64_t GetYang() const noexcept override { return 25000000; }
    int64_t GetExp() const noexcept override { return 120000; }

    PlayerCoordinates GetCoordinates() const noexcept override { return {150.0f, 250.0f, 0.0f, 90.0f}; }
    uint32_t GetTargetVID() const noexcept override { return 5002; }
    bool IsInCombat() const noexcept override { return true; }

    PlayerStatusSnapshot CaptureSnapshot() const override
    {
        return PlayerStatusSnapshot{
            .vid = 10001,
            .name = "TestHero",
            .level = 75,
            .hp = 14500,
            .maxHp = 15000,
            .sp = 3000,
            .maxSp = 3000,
            .yang = 25000000,
            .exp = 120000,
            .coords = {150.0f, 250.0f, 0.0f, 90.0f},
            .targetVid = 5002,
            .isCombatActive = true,
            .mapName = "metin2_map_spiderdungeon_02"
        };
    }
};

class MockInventoryContract : public IInventoryDomainContract
{
public:
    std::optional<ItemDataSnapshot> GetItem(const ItemCellAddress& pos) const override
    {
        if (pos.cellIndex == 0)
        {
            return ItemDataSnapshot{
                .pos = pos,
                .vnum = 27003,
                .count = 200,
                .localizedName = "Czerwona Mikstura(D)"
            };
        }
        return std::nullopt;
    }

    std::vector<ItemDataSnapshot> GetAllItems(uint8_t windowType) const override
    {
        std::vector<ItemDataSnapshot> list;
        list.push_back(ItemDataSnapshot{
            .pos = {windowType, 0},
            .vnum = 27003,
            .count = 200,
            .localizedName = "Czerwona Mikstura(D)"
        });
        list.push_back(ItemDataSnapshot{
            .pos = {windowType, 1},
            .vnum = 189,
            .count = 1,
            .localizedName = "Zatruty Miecz+9"
        });
        return list;
    }

    bool IsCellEmpty(const ItemCellAddress& pos) const noexcept override { return pos.cellIndex > 1; }
    size_t GetFreeSlotCount(uint8_t windowType) const noexcept override { return 43; }

    bool RequestUseItem(const ItemCellAddress& pos) override { return pos.cellIndex == 0; }
    bool RequestMoveItem(const ItemCellAddress& src, const ItemCellAddress& dst, uint32_t count) override { return true; }
    bool RequestDropItem(const ItemCellAddress& pos, uint32_t count) override { return true; }
};

class MockCombatContract : public ICombatDomainContract
{
public:
    CombatVerdict EvaluateAttack(uint32_t targetVid) const override
    {
        if (targetVid == 0) return {false, 0.0f, 0.0f, 0, "Brak celu"};
        if (targetVid == 5002) return {true, 0.0f, 250.0f, 95, "Cel w zasiegu"};
        return {false, 0.25f, 5000.0f, 0, "Cel poza zasiegiem"};
    }

    float GetAttackCooldown() const noexcept override { return 0.25f; }
    uint32_t GetLastTargetVID() const noexcept override { return 5002; }

    bool ExecuteAttack(uint32_t targetVid) override { return targetVid == 5002; }
    void UpdateCombatState(float deltaTime) override {}
};

class MockWorldContract : public IWorldQueryContract
{
public:
    std::vector<EntitySnapshot> GetEntitiesInRadius(float radius) const override
    {
        std::vector<EntitySnapshot> res;
        res.push_back(EntitySnapshot{
            .vid = 5002,
            .type = EntityType::Monster,
            .name = "Szponowaty Pajak",
            .distance = 250.0f,
            .currentHpPercent = 45,
            .isHostile = true,
            .isDead = false
        });
        return res;
    }

    std::optional<EntitySnapshot> GetEntityByVID(uint32_t vid) const override
    {
        if (vid == 5002)
        {
            return EntitySnapshot{
                .vid = 5002,
                .type = EntityType::Monster,
                .name = "Szponowaty Pajak",
                .distance = 250.0f,
                .currentHpPercent = 45,
                .isHostile = true,
                .isDead = false
            };
        }
        return std::nullopt;
    }

    std::optional<EntitySnapshot> GetNearestHostile() const override { return GetEntityByVID(5002); }
    std::optional<EntitySnapshot> GetNearestItemGround() const override { return std::nullopt; }
};

void TestAgentMCPBridge()
{
    std::cout << "[RUN] TestAgentMCPBridge..." << std::endl;

    MockPlayerContract mockPlayer;
    MockInventoryContract mockInventory;
    MockCombatContract mockCombat;
    MockWorldContract mockWorld;

    AgentMCPBridge& bridge = AgentMCPBridge::Instance();
    bridge.Initialize(&mockPlayer, &mockInventory, &mockCombat, &mockWorld);
    assert(bridge.IsReady());

    // 1. Test pobierania stanu gry
    std::string stateJson = bridge.Tool_GetGameState();
    assert(stateJson.find("\"vid\":10001") != std::string::npos);
    assert(stateJson.find("\"hp\":14500") != std::string::npos);
    assert(stateJson.find("\"yang\":25000000") != std::string::npos);

    // 2. Test pobierania inwentarza
    std::string invJson = bridge.Tool_GetInventory(1);
    assert(invJson.find("Zatruty Miecz+9") != std::string::npos);
    assert(invJson.find("Czerwona Mikstura(D)") != std::string::npos);

    // 3. Test percepcji otoczenia
    std::string entitiesJson = bridge.Tool_GetNearbyEntities(1000.0f);
    assert(entitiesJson.find("Szponowaty Pajak") != std::string::npos);
    assert(entitiesJson.find("\"vid\":5002") != std::string::npos);

    // 4. Test akcji ataku
    std::string attackResult = bridge.Tool_ActionAttack(5002);
    assert(attackResult.find("\"success\":true") != std::string::npos);

    // 5. Test JSON-RPC 2.0 Dispatcher - stan gry z dynamicznym id tekstowym
    std::string rpcRequest = "{\"jsonrpc\": \"2.0\", \"method\": \"game_get_state\", \"id\": \"req-42\"}";
    std::string rpcResponse = bridge.HandleJsonRpcRequest(rpcRequest);
    assert(rpcResponse.find("\"jsonrpc\":\"2.0\"") != std::string::npos);
    assert(rpcResponse.find("\"name\":\"TestHero\"") != std::string::npos);
    assert(rpcResponse.find("\"id\":\"req-42\"") != std::string::npos);

    // 6. Test JSON-RPC ping
    std::string pingReq = "{\"jsonrpc\":\"2.0\",\"method\":\"game_ping\",\"id\":77}";
    std::string pingResp = bridge.HandleJsonRpcRequest(pingReq);
    assert(pingResp.find("\"status\":\"pong\"") != std::string::npos);
    assert(pingResp.find("\"id\":77") != std::string::npos);

    // 7. Test JSON-RPC game_action_attack z parametrami
    std::string attackReq = "{\"jsonrpc\":\"2.0\",\"method\":\"game_action_attack\",\"params\":{\"target_vid\":5002},\"id\":88}";
    std::string attackResp = bridge.HandleJsonRpcRequest(attackReq);
    assert(attackResp.find("\"success\":true") != std::string::npos);
    assert(attackResp.find("\"target_vid\":5002") != std::string::npos);
    assert(attackResp.find("\"id\":88") != std::string::npos);

    // 7b. Test Rate Limiter dla ataku (kolejny atak zbyt szybko)
    std::string fastAttackReq = "{\"jsonrpc\":\"2.0\",\"method\":\"game_action_attack\",\"params\":{\"target_vid\":5002},\"id\":89}";
    std::string fastAttackResp = bridge.HandleJsonRpcRequest(fastAttackReq);
    assert(fastAttackResp.find("\"ok\":true") != std::string::npos);
    assert(fastAttackResp.find("\"id\":89") != std::string::npos);
    assert(fastAttackResp.find("rate_limited_buffered") != std::string::npos);
    assert(fastAttackResp.find("\"delayed\":true") != std::string::npos);

    // 8. Test JSON-RPC game_action_use_item z parametrami (uzycie mikstury w slocie 0)
    std::string useReq = "{\"jsonrpc\":\"2.0\",\"method\":\"game_action_use_item\",\"params\":{\"window\":1,\"cell\":0},\"id\":99}";
    std::string useResp = bridge.HandleJsonRpcRequest(useReq);
    assert(useResp.find("\"success\":true") != std::string::npos);
    assert(useResp.find("\"cell\":0") != std::string::npos);
    assert(useResp.find("\"id\":99") != std::string::npos);

    // 8b. Test Rate Limiter dla use_item (kolejne uzycie przedmiotu zbyt szybko)
    std::string fastUseReq = "{\"jsonrpc\":\"2.0\",\"method\":\"game_action_use_item\",\"params\":{\"window\":1,\"cell\":0},\"id\":100}";
    std::string fastUseResp = bridge.HandleJsonRpcRequest(fastUseReq);
    assert(fastUseResp.find("\"ok\":true") != std::string::npos);
    assert(fastUseResp.find("\"id\":100") != std::string::npos);
    assert(fastUseResp.find("rate_limited_buffered") != std::string::npos);
    assert(fastUseResp.find("\"delayed\":true") != std::string::npos);

    // 8c. Test odblokowania po resecie rate limitera
    bridge.ResetRateLimiter();
    assert(bridge.CanExecuteAttack());
    assert(bridge.CanExecuteAction());
    std::string unblockedAttack = bridge.HandleJsonRpcRequest(attackReq);
    assert(unblockedAttack.find("\"success\":true") != std::string::npos);

    // 9. Test nieistniejacej metody (Error RFC -32601)
    std::string invalidReq = "{\"jsonrpc\":\"2.0\",\"method\":\"game_unknown_call\",\"id\":123}";
    std::string invalidResp = bridge.HandleJsonRpcRequest(invalidReq);
    assert(invalidResp.find("\"code\":-32601") != std::string::npos);
    assert(invalidResp.find("\"id\":123") != std::string::npos);

    std::cout << "[PASS] TestAgentMCPBridge zakonczony pelnym sukcesem!" << std::endl;
}

int main()
{
    std::cout << "======================================================" << std::endl;
    std::cout << " START TESTU C32: AGENT MODEL CONTEXT PROTOCOL BRIDGE " << std::endl;
    std::cout << "======================================================" << std::endl;

    TestAgentMCPBridge();

    std::cout << "======================================================" << std::endl;
    std::cout << " WSZYSTKIE TESTY AGENT MCP BRIDGE ZAKONCZONE SUKCESEM " << std::endl;
    std::cout << "======================================================" << std::endl;
    return 0;
}
