#pragma once

#include <vector>
#include <unordered_map>
#include <expected>
#include <optional>
#include <format>
#include <string>
#include <algorithm>
#include <span>

#include "../EterBase/StrongTypes.h"
#include "../EterBase/LogModern.h"
#include "../UserInterface/Core/EventBus.h"

namespace GameLib {

/**
 * @brief Enum defining possible errors when manipulating the PortalWarpGraph.
 */
enum class PortalGraphError {
    ConnectionAlreadyExists,
    MapIndexInvalid,
    PathNotFound
};

/**
 * @brief Event published when a new portal connection is added to the graph.
 */
struct PortalConnectionAddedEvent : public UserInterface::Core::IEvent {
    EterBase::MapIndex source;
    EterBase::MapIndex destination;

    /**
     * @brief Constructs the event.
     * @param source The origin map index.
     * @param destination The target map index.
     */
    PortalConnectionAddedEvent(EterBase::MapIndex source, EterBase::MapIndex destination)
        : source(source), destination(destination) {}
};

/**
 * @brief Manages the graph of portal connections between different maps.
 * 
 * Uses C++23 features such as std::expected and monadic std::optional 
 * for safety and clarity, adhering strictly to the Zero-Hungarian notation rule.
 */
class PortalWarpGraph {
public:
    /**
     * @brief Represents a connection (edge) to another map.
     */
    struct Connection {
        EterBase::MapIndex destination;
    };

    /**
     * @brief Default constructor.
     */
    PortalWarpGraph() = default;

    /**
     * @brief Adds a new portal connection from a source map to a destination map.
     * @param source The starting map index.
     * @param destination The ending map index.
     * @return std::expected<void, PortalGraphError> Result of the operation.
     */
    std::expected<void, PortalGraphError> AddConnection(EterBase::MapIndex source, EterBase::MapIndex destination) {
        if (!source || !destination) {
            EterBase::ModernLogger::Error("Failed to add portal connection: Invalid MapIndex");
            return std::unexpected(PortalGraphError::MapIndexInvalid);
        }

        auto& connections = graph[source];
        
        // Check if connection already exists
        for (const auto& conn : connections) {
            if (conn.destination == destination) {
                EterBase::ModernLogger::Warn("Portal connection from {} to {} already exists", source.value(), destination.value());
                return std::unexpected(PortalGraphError::ConnectionAlreadyExists);
            }
        }

        connections.push_back(Connection{destination});
        EterBase::ModernLogger::Info("Added portal connection from {} to {}", source.value(), destination.value());

        // Publish event
        UserInterface::Core::EventBus::GetInstance().Publish(PortalConnectionAddedEvent{source, destination});

        return {};
    }

    /**
     * @brief Gets all outgoing connections for a given map.
     * @param source The starting map index.
     * @return std::optional containing a span of connections, or std::nullopt if none exist.
     */
    std::optional<std::span<const Connection>> GetConnections(EterBase::MapIndex source) const {
        if (auto it = graph.find(source); it != graph.end()) {
            return std::span<const Connection>(it->second);
        }
        return std::nullopt;
    }

    /**
     * @brief Checks if a direct connection exists using monadic operations.
     * @param source The starting map index.
     * @param destination The ending map index.
     * @return true if a direct connection exists, false otherwise.
     */
    bool HasDirectConnection(EterBase::MapIndex source, EterBase::MapIndex destination) const {
        return GetConnections(source)
            .transform([destination](std::span<const Connection> connections) {
                for (const auto& conn : connections) {
                    if (conn.destination == destination) {
                        return true;
                    }
                }
                return false;
            })
            .value_or(false);
    }

    /**
     * @brief Finds a path between two maps using breadth-first search.
     * @param source The starting map index.
     * @param destination The ending map index.
     * @return std::expected<std::vector<EterBase::MapIndex>, PortalGraphError> The path of maps, or an error.
     */
    std::expected<std::vector<EterBase::MapIndex>, PortalGraphError> FindPath(EterBase::MapIndex source, EterBase::MapIndex destination) const {
        if (!source || !destination) {
            EterBase::ModernLogger::Error("Failed to find path: Invalid MapIndex");
            return std::unexpected(PortalGraphError::MapIndexInvalid);
        }

        if (source == destination) {
            return std::vector<EterBase::MapIndex>{source};
        }

        std::unordered_map<EterBase::MapIndex, EterBase::MapIndex> cameFrom;
        std::vector<EterBase::MapIndex> queue;
        queue.push_back(source);
        
        size_t head = 0;
        bool found = false;

        while (head < queue.size()) {
            auto current = queue[head++];
            
            if (current == destination) {
                found = true;
                break;
            }

            auto connsOpt = GetConnections(current);
            if (connsOpt.has_value()) {
                for (const auto& conn : connsOpt.value()) {
                    if (cameFrom.find(conn.destination) == cameFrom.end() && conn.destination != source) {
                        cameFrom[conn.destination] = current;
                        queue.push_back(conn.destination);
                    }
                }
            }
        }

        if (!found) {
            EterBase::ModernLogger::Warn("Path from {} to {} not found", source.value(), destination.value());
            return std::unexpected(PortalGraphError::PathNotFound);
        }

        std::vector<EterBase::MapIndex> path;
        auto current = destination;
        while (current != source) {
            path.push_back(current);
            current = cameFrom[current];
        }
        path.push_back(source);
        
        std::reverse(path.begin(), path.end());
        return path;
    }

private:
    std::unordered_map<EterBase::MapIndex, std::vector<Connection>> graph;
};

} // namespace GameLib
