#include "AgentMCPBridge.h"
#include "Client/Network/Protocol/IServerProtocolDriver.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <charconv>

namespace Client::MCP
{
    namespace
    {
        // Bezpieczna funkcja ucieczki ciagow znakow dla JSON (RFC 8259)
        std::string EscapeJson(std::string_view input)
        {
            std::string out;
            out.reserve(input.size() + 8);
            for (char c : input)
            {
                switch (c)
                {
                    case '\"': out += "\\\""; break;
                    case '\\': out += "\\\\"; break;
                    case '\b': out += "\\b"; break;
                    case '\f': out += "\\f"; break;
                    case '\n': out += "\\n"; break;
                    case '\r': out += "\\r"; break;
                    case '\t': out += "\\t"; break;
                    default:
                        if (static_cast<unsigned char>(c) < 0x20)
                        {
                            char buf[8];
                            snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                            out += buf;
                        }
                        else
                        {
                            out += c;
                        }
                        break;
                }
            }
            return out;
        }

        // Zabezpieczenie wartosci zmiennoprzecinkowych przed NaN i Inf (RFC 8259 nie wspiera NaN/Inf)
        float SafeFloat(float val, float fallback = 0.0f) noexcept
        {
            return std::isfinite(val) ? val : fallback;
        }

        // Pomocniczy parser pola "id" z JSON-RPC 2.0
        std::string ExtractRpcId(std::string_view json)
        {
            size_t idPos = json.find("\"id\"");
            if (idPos == std::string_view::npos)
                return "1";

            size_t colonPos = json.find(':', idPos);
            if (colonPos == std::string_view::npos)
                return "1";

            size_t start = json.find_first_not_of(" \t\r\n", colonPos + 1);
            if (start == std::string_view::npos)
                return "1";

            if (json[start] == '\"')
            {
                size_t endQuote = json.find('\"', start + 1);
                if (endQuote != std::string_view::npos)
                    return std::string(json.substr(start, endQuote - start + 1));
            }
            else
            {
                size_t endNum = json.find_first_of(",} \t\r\n", start);
                if (endNum != std::string_view::npos)
                    return std::string(json.substr(start, endNum - start));
                else
                    return std::string(json.substr(start));
            }
            return "1";
        }

        // Pomocniczy parser parametrow liczbowych z pola JSON
        template <typename T>
        T ExtractNumericParam(std::string_view json, std::string_view paramName, T defaultValue)
        {
            size_t pos = json.find(paramName);
            if (pos == std::string_view::npos)
                return defaultValue;

            size_t colonPos = json.find(':', pos + paramName.size());
            if (colonPos == std::string_view::npos)
                return defaultValue;

            size_t start = json.find_first_not_of(" \t\r\n\"", colonPos + 1);
            if (start == std::string_view::npos)
                return defaultValue;

            size_t end = json.find_first_of(",}\" \t\r\n", start);
            std::string_view numStr = (end != std::string_view::npos) ? json.substr(start, end - start) : json.substr(start);

            T result = defaultValue;
            if constexpr (std::is_floating_point_v<T>)
            {
                try {
                    result = static_cast<T>(std::stod(std::string(numStr)));
                } catch (...) {
                    result = defaultValue;
                }
            }
            else
            {
                int64_t val = 0;
                auto [ptr, ec] = std::from_chars(numStr.data(), numStr.data() + numStr.size(), val);
                if (ec == std::errc())
                    result = static_cast<T>(val);
            }
            return result;
        }
    }

    AgentMCPBridge& AgentMCPBridge::Instance() noexcept
    {
        static AgentMCPBridge s_instance;
        return s_instance;
    }

    void AgentMCPBridge::Initialize(
        Contracts::IPlayerDomainContract* pPlayer,
        Contracts::IInventoryDomainContract* pInventory,
        Contracts::ICombatDomainContract* pCombat,
        Contracts::IWorldQueryContract* pWorld,
        const ::Network::Protocol::IServerProtocolDriver* pDriver
    ) noexcept
    {
        m_pPlayer = pPlayer;
        m_pInventory = pInventory;
        m_pCombat = pCombat;
        m_pWorld = pWorld;
        m_pProtocolDriver = pDriver;
        ResetRateLimiter();
    }

    std::string AgentMCPBridge::Tool_GetGameState() const
    {
        if (!m_pPlayer)
        {
            return "{\"error\":\"Player domain contract not initialized\"}";
        }

        auto snap = m_pPlayer->CaptureSnapshot();
        std::ostringstream ss;
        ss << "{"
           << "\"vid\":" << snap.vid << ","
           << "\"name\":\"" << EscapeJson(snap.name) << "\","
           << "\"level\":" << snap.level << ","
           << "\"hp\":" << snap.hp << ","
           << "\"max_hp\":" << snap.maxHp << ","
           << "\"sp\":" << snap.sp << ","
           << "\"max_sp\":" << snap.maxSp << ","
           << "\"yang\":" << snap.yang << ","
           << "\"exp\":" << snap.exp << ","
           << "\"coords\":{"
           << "\"x\":" << SafeFloat(snap.coords.x) << ","
           << "\"y\":" << SafeFloat(snap.coords.y) << ","
           << "\"z\":" << SafeFloat(snap.coords.z) << ","
           << "\"rot\":" << SafeFloat(snap.coords.rotation)
           << "},"
           << "\"target_vid\":" << snap.targetVid << ","
           << "\"in_combat\":" << (snap.isCombatActive ? "true" : "false") << ","
           << "\"map\":\"" << EscapeJson(snap.mapName) << "\""
           << "}";
        return ss.str();
    }

    std::string AgentMCPBridge::Tool_GetInventory(uint8_t windowType) const
    {
        if (!m_pInventory)
        {
            return "{\"error\":\"Inventory domain contract not initialized\"}";
        }

        auto items = m_pInventory->GetAllItems(windowType);
        std::ostringstream ss;
        ss << "{\"window\":" << static_cast<int>(windowType) << ",\"count\":" << items.size() << ",\"items\":[";
        for (size_t i = 0; i < items.size(); ++i)
        {
            if (i > 0) ss << ",";
            const auto& item = items[i];
            ss << "{"
               << "\"cell\":" << item.pos.cellIndex << ","
               << "\"vnum\":" << item.vnum << ","
               << "\"count\":" << item.count << ","
               << "\"name\":\"" << EscapeJson(item.localizedName) << "\""
               << "}";
        }
        ss << "]}";
        return ss.str();
    }

    std::string AgentMCPBridge::Tool_GetNearbyEntities(float radius) const
    {
        if (!m_pWorld)
        {
            return "{\"error\":\"World query contract not initialized\"}";
        }

        radius = SafeFloat(radius, 3000.0f);
        auto entities = m_pWorld->GetEntitiesInRadius(radius);
        std::ostringstream ss;
        ss << "{\"radius\":" << radius << ",\"count\":" << entities.size() << ",\"entities\":[";
        for (size_t i = 0; i < entities.size(); ++i)
        {
            if (i > 0) ss << ",";
            const auto& e = entities[i];
            ss << "{"
               << "\"vid\":" << e.vid << ","
               << "\"type\":" << static_cast<int>(e.type) << ","
               << "\"name\":\"" << EscapeJson(e.name) << "\","
               << "\"distance\":" << SafeFloat(e.distance) << ","
               << "\"hostile\":" << (e.isHostile ? "true" : "false") << ","
               << "\"dead\":" << (e.isDead ? "true" : "false")
               << "}";
        }
        ss << "]}";
        return ss.str();
    }

    std::string AgentMCPBridge::Tool_ActionAttack(uint32_t targetVid)
    {
        if (!m_pCombat)
        {
            return "{\"success\":false,\"error\":\"Combat domain contract not initialized\"}";
        }

        auto verdict = m_pCombat->EvaluateAttack(targetVid);
        if (!verdict.canAttack)
        {
            std::string reasonStr = verdict.reason ? EscapeJson(verdict.reason) : "UNKNOWN";
            std::ostringstream ss;
            ss << "{\"success\":false,\"reason\":\"" << reasonStr
               << "\",\"cooldown\":" << SafeFloat(verdict.remainingCooldown)
               << ",\"target_vid\":" << targetVid << "}";
            return ss.str();
        }

        bool ok = m_pCombat->ExecuteAttack(targetVid);
        std::ostringstream ss;
        ss << "{\"success\":" << (ok ? "true" : "false") << ",\"target_vid\":" << targetVid << "}";
        return ss.str();
    }

    std::string AgentMCPBridge::Tool_ActionUseItem(uint8_t windowType, uint16_t cell)
    {
        if (!m_pInventory)
        {
            return "{\"success\":false,\"error\":\"Inventory domain contract not initialized\"}";
        }

        Contracts::ItemCellAddress addr{windowType, cell};
        bool ok = m_pInventory->RequestUseItem(addr);
        std::ostringstream ss;
        ss << "{\"success\":" << (ok ? "true" : "false")
           << ",\"window\":" << static_cast<int>(windowType)
           << ",\"cell\":" << cell << "}";
        return ss.str();
    }

    uint32_t AgentMCPBridge::GetMinAttackIntervalMs() const noexcept
    {
        if (m_overrideMinAttackIntervalMs.has_value())
        {
            return *m_overrideMinAttackIntervalMs;
        }
        if (m_pProtocolDriver)
        {
            return m_pProtocolDriver->GetMinAttackIntervalMs();
        }
        return 150;
    }

    uint32_t AgentMCPBridge::GetMinActionIntervalMs() const noexcept
    {
        if (m_overrideMinActionIntervalMs.has_value())
        {
            return *m_overrideMinActionIntervalMs;
        }
        if (m_pProtocolDriver)
        {
            return m_pProtocolDriver->GetMinActionIntervalMs();
        }
        return 100;
    }

    bool AgentMCPBridge::CanExecuteAttack() const noexcept
    {
        if (!m_hasLastAttackTime)
        {
            return true;
        }
        auto now = std::chrono::steady_clock::now();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastAttackTime).count();
        return elapsedMs >= static_cast<int64_t>(GetMinAttackIntervalMs());
    }

    bool AgentMCPBridge::CanExecuteAction() const noexcept
    {
        if (!m_hasLastActionTime)
        {
            return true;
        }
        auto now = std::chrono::steady_clock::now();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastActionTime).count();
        return elapsedMs >= static_cast<int64_t>(GetMinActionIntervalMs());
    }

    void AgentMCPBridge::ResetRateLimiter() noexcept
    {
        m_hasLastAttackTime = false;
        m_hasLastActionTime = false;
        m_lastAttackTime = {};
        m_lastActionTime = {};
    }

    std::string AgentMCPBridge::HandleJsonRpcRequest(std::string_view jsonRequest)
    {
        std::string rpcId = ExtractRpcId(jsonRequest);

        // game_ping - lekka metoda liveness
        if (jsonRequest.find("\"game_ping\"") != std::string_view::npos)
        {
            return "{\"jsonrpc\":\"2.0\",\"result\":{\"status\":\"pong\",\"ready\":" +
                   std::string(IsReady() ? "true" : "false") + "},\"id\":" + rpcId + "}";
        }

        // game_get_state
        if (jsonRequest.find("\"game_get_state\"") != std::string_view::npos)
        {
            if (!m_pPlayer)
                return "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32000,\"message\":\"Player contract not initialized\"},\"id\":" + rpcId + "}";
            return "{\"jsonrpc\":\"2.0\",\"result\":" + Tool_GetGameState() + ",\"id\":" + rpcId + "}";
        }

        // game_get_inventory
        if (jsonRequest.find("\"game_get_inventory\"") != std::string_view::npos)
        {
            if (!m_pInventory)
                return "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32000,\"message\":\"Inventory contract not initialized\"},\"id\":" + rpcId + "}";
            uint8_t windowType = ExtractNumericParam<uint8_t>(jsonRequest, "\"window\"", 1);
            if (windowType == 0)
                windowType = ExtractNumericParam<uint8_t>(jsonRequest, "\"windowType\"", 1);
            return "{\"jsonrpc\":\"2.0\",\"result\":" + Tool_GetInventory(windowType) + ",\"id\":" + rpcId + "}";
        }

        // game_get_nearby_entities
        if (jsonRequest.find("\"game_get_nearby_entities\"") != std::string_view::npos)
        {
            if (!m_pWorld)
                return "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32000,\"message\":\"World query contract not initialized\"},\"id\":" + rpcId + "}";
            float radius = ExtractNumericParam<float>(jsonRequest, "\"radius\"", 3000.0f);
            return "{\"jsonrpc\":\"2.0\",\"result\":" + Tool_GetNearbyEntities(radius) + ",\"id\":" + rpcId + "}";
        }

        // game_action_attack
        if (jsonRequest.find("\"game_action_attack\"") != std::string_view::npos)
        {
            if (!m_pCombat)
                return "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32000,\"message\":\"Combat contract not initialized\"},\"id\":" + rpcId + "}";

            if (!CanExecuteAttack())
            {
                return "{\"jsonrpc\":\"2.0\",\"ok\":true,\"id\":" + rpcId + ",\"result\":{\"status\":\"rate_limited_buffered\",\"delayed\":true}}";
            }
            m_lastAttackTime = std::chrono::steady_clock::now();
            m_hasLastAttackTime = true;

            uint32_t targetVid = ExtractNumericParam<uint32_t>(jsonRequest, "\"target_vid\"", 0);
            if (targetVid == 0)
                targetVid = ExtractNumericParam<uint32_t>(jsonRequest, "\"targetVid\"", 0);
            return "{\"jsonrpc\":\"2.0\",\"result\":" + Tool_ActionAttack(targetVid) + ",\"id\":" + rpcId + "}";
        }

        // game_action_use_item
        if (jsonRequest.find("\"game_action_use_item\"") != std::string_view::npos)
        {
            if (!m_pInventory)
                return "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32000,\"message\":\"Inventory contract not initialized\"},\"id\":" + rpcId + "}";

            if (!CanExecuteAction())
            {
                return "{\"jsonrpc\":\"2.0\",\"ok\":true,\"id\":" + rpcId + ",\"result\":{\"status\":\"rate_limited_buffered\",\"delayed\":true}}";
            }
            m_lastActionTime = std::chrono::steady_clock::now();
            m_hasLastActionTime = true;

            uint8_t windowType = ExtractNumericParam<uint8_t>(jsonRequest, "\"window\"", 1);
            if (windowType == 0)
                windowType = ExtractNumericParam<uint8_t>(jsonRequest, "\"windowType\"", 1);
            uint16_t cell = ExtractNumericParam<uint16_t>(jsonRequest, "\"cell\"", 0);
            return "{\"jsonrpc\":\"2.0\",\"result\":" + Tool_ActionUseItem(windowType, cell) + ",\"id\":" + rpcId + "}";
        }

        return "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32601,\"message\":\"Method not found\"},\"id\":" + rpcId + "}";
    }
}
