#pragma once

#include <string>
#include "rapidjson/document.h"

namespace Client::IPC
{
    class MCPBridgeServer
    {
    public:
        MCPBridgeServer() = default;
        ~MCPBridgeServer() = default;

        std::string HandleRequest(const std::string& requestJson);

    private:
        std::string HandleInitialize(const rapidjson::Value& id);
        std::string HandleToolsList(const rapidjson::Value& id);
        std::string HandleToolsCall(const rapidjson::Value& id, const std::string& name, const std::string& argumentsStr);
        std::string CreateErrorResponse(const rapidjson::Value& id, int code, const std::string& message);
    };
} // namespace Client::IPC
