#include "MCPBridgeServer.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

namespace Client::IPC
{
    std::string MCPBridgeServer::HandleRequest(const std::string& requestJson)
    {
        rapidjson::Document doc;
        if (doc.Parse(requestJson.c_str()).HasParseError())
        {
            return CreateErrorResponse(rapidjson::Value(), -32700, "Parse error");
        }

        if (!doc.IsObject())
        {
            return CreateErrorResponse(rapidjson::Value(), -32600, "Invalid Request");
        }

        if (!doc.HasMember("jsonrpc") || !doc["jsonrpc"].IsString() || std::string(doc["jsonrpc"].GetString()) != "2.0")
        {
            return CreateErrorResponse(rapidjson::Value(), -32600, "Invalid Request: missing or invalid jsonrpc");
        }

        rapidjson::Value id;
        if (doc.HasMember("id"))
        {
            id.CopyFrom(doc["id"], doc.GetAllocator());
        }

        if (!doc.HasMember("method") || !doc["method"].IsString())
        {
            return CreateErrorResponse(id, -32600, "Invalid Request: missing or invalid method");
        }

        std::string method = doc["method"].GetString();

        if (method == "initialize")
        {
            return HandleInitialize(id);
        }
        else if (method == "tools/list")
        {
            return HandleToolsList(id);
        }
        else if (method == "tools/call")
        {
            if (!doc.HasMember("params") || !doc["params"].IsObject())
            {
                return CreateErrorResponse(id, -32602, "Invalid params");
            }
            const auto& params = doc["params"];
            if (!params.HasMember("name") || !params["name"].IsString())
            {
                return CreateErrorResponse(id, -32602, "Invalid params: missing tool name");
            }
            std::string name = params["name"].GetString();
            std::string argumentsStr = "{}";
            
            if (params.HasMember("arguments") && params["arguments"].IsObject())
            {
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                params["arguments"].Accept(writer);
                argumentsStr = buffer.GetString();
            }
            
            return HandleToolsCall(id, name, argumentsStr);
        }

        return CreateErrorResponse(id, -32601, "Method not found");
    }

    std::string MCPBridgeServer::HandleInitialize(const rapidjson::Value& id)
    {
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        doc.AddMember("jsonrpc", "2.0", allocator);
        rapidjson::Value idCopy;
        idCopy.CopyFrom(id, allocator);
        doc.AddMember("id", idCopy, allocator);

        rapidjson::Value result(rapidjson::kObjectType);
        
        rapidjson::Value capabilities(rapidjson::kObjectType);
        rapidjson::Value tools(rapidjson::kObjectType);
        capabilities.AddMember("tools", tools, allocator);
        result.AddMember("capabilities", capabilities, allocator);
        
        rapidjson::Value serverInfo(rapidjson::kObjectType);
        serverInfo.AddMember("name", "MCPBridgeServer", allocator);
        serverInfo.AddMember("version", "1.0.0", allocator);
        result.AddMember("serverInfo", serverInfo, allocator);
        
        // MCP protocol version
        result.AddMember("protocolVersion", "2024-11-05", allocator);

        doc.AddMember("result", result, allocator);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        return buffer.GetString();
    }

    std::string MCPBridgeServer::HandleToolsList(const rapidjson::Value& id)
    {
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        doc.AddMember("jsonrpc", "2.0", allocator);
        rapidjson::Value idCopy;
        idCopy.CopyFrom(id, allocator);
        doc.AddMember("id", idCopy, allocator);

        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value tools(rapidjson::kArrayType);

        // Define tools
        const char* toolNames[] = {
            "m2_goto", "m2_attack_target", "m2_pickup_loot", "m2_use_skill",
            "m2_use_item", "m2_get_inventory", "m2_inspect_surroundings", "m2_get_player_state"
        };
        const char* toolDescs[] = {
            "Go to a specific location", "Attack a target", "Pickup loot around the player", "Use a skill",
            "Use an item", "Get inventory contents", "Inspect surroundings", "Get player state"
        };

        for (size_t i = 0; i < 8; ++i)
        {
            rapidjson::Value tool(rapidjson::kObjectType);
            tool.AddMember("name", rapidjson::StringRef(toolNames[i]), allocator);
            tool.AddMember("description", rapidjson::StringRef(toolDescs[i]), allocator);
            
            rapidjson::Value inputSchema(rapidjson::kObjectType);
            inputSchema.AddMember("type", "object", allocator);
            rapidjson::Value properties(rapidjson::kObjectType);
            // Simple placeholder properties for schema
            inputSchema.AddMember("properties", properties, allocator);
            
            tool.AddMember("inputSchema", inputSchema, allocator);
            tools.PushBack(tool, allocator);
        }

        result.AddMember("tools", tools, allocator);
        doc.AddMember("result", result, allocator);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        return buffer.GetString();
    }

    std::string MCPBridgeServer::HandleToolsCall(const rapidjson::Value& id, const std::string& name, const std::string& argumentsStr)
    {
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        doc.AddMember("jsonrpc", "2.0", allocator);
        rapidjson::Value idCopy;
        idCopy.CopyFrom(id, allocator);
        doc.AddMember("id", idCopy, allocator);

        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value content(rapidjson::kArrayType);
        
        rapidjson::Value contentItem(rapidjson::kObjectType);
        contentItem.AddMember("type", "text", allocator);
        
        std::string textResult = "Called " + name + " with args: " + argumentsStr;
        contentItem.AddMember("text", rapidjson::Value(textResult.c_str(), allocator).Move(), allocator);
        
        content.PushBack(contentItem, allocator);
        result.AddMember("content", content, allocator);
        result.AddMember("isError", false, allocator);

        doc.AddMember("result", result, allocator);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        return buffer.GetString();
    }

    std::string MCPBridgeServer::CreateErrorResponse(const rapidjson::Value& id, int code, const std::string& message)
    {
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        doc.AddMember("jsonrpc", "2.0", allocator);
        rapidjson::Value idCopy;
        idCopy.CopyFrom(id, allocator);
        doc.AddMember("id", idCopy, allocator);

        rapidjson::Value error(rapidjson::kObjectType);
        error.AddMember("code", code, allocator);
        error.AddMember("message", rapidjson::Value(message.c_str(), allocator).Move(), allocator);

        doc.AddMember("error", error, allocator);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        return buffer.GetString();
    }
} // namespace Client::IPC
