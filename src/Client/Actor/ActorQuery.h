#pragma once

#include <vector>
#include <cmath>
#include <span>
#include <expected>
#include "../World/ActorRegistry.h"

namespace Client::Actor {

enum class ActorQueryError {
    InvalidRadius,
    EmptyInput,
    InvalidCenter
};

class ActorQuery {
public:
    static constexpr std::expected<std::vector<World::ActorRecord>, ActorQueryError> 
    GetAliveActorsInRadius(std::span<const World::ActorRecord> actors, float centerX, float centerY, float radius) {
        if (radius < 0.0f) {
            return std::unexpected(ActorQueryError::InvalidRadius);
        }

        std::vector<World::ActorRecord> result;
        const float radiusSq = radius * radius;

        for (const auto& actor : actors) {
            if (actor.isDead) {
                continue;
            }

            const float dx = actor.x - centerX;
            const float dy = actor.y - centerY;
            if ((dx * dx + dy * dy) <= radiusSq) {
                result.push_back(actor);
            }
        }

        return result;
    }

    static constexpr std::expected<std::vector<World::ActorRecord>, ActorQueryError> 
    GetAliveActorsByTypeInRadius(std::span<const World::ActorRecord> actors, float centerX, float centerY, float radius, uint8_t actorType) {
        if (radius < 0.0f) {
            return std::unexpected(ActorQueryError::InvalidRadius);
        }

        std::vector<World::ActorRecord> result;
        const float radiusSq = radius * radius;

        for (const auto& actor : actors) {
            if (actor.isDead || actor.type != actorType) {
                continue;
            }

            const float dx = actor.x - centerX;
            const float dy = actor.y - centerY;
            if ((dx * dx + dy * dy) <= radiusSq) {
                result.push_back(actor);
            }
        }

        return result;
    }
};

} // namespace Client::Actor
