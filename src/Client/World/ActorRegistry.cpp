#include "ActorRegistry.h"

namespace Client::World {

bool ActorRegistry::RegisterActor(const ActorRecord& record) {
    if (m_actors.contains(record.vid)) {
        return false;
    }
    m_actors.emplace(record.vid, record);
    return true;
}

bool ActorRegistry::UnregisterActor(EntityVid vid) {
    return m_actors.erase(vid) > 0;
}

std::optional<ActorRecord> ActorRegistry::GetActor(EntityVid vid) const {
    auto it = m_actors.find(vid);
    if (it != m_actors.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool ActorRegistry::UpdatePosition(EntityVid vid, float x, float y, float z, float rotation) {
    auto it = m_actors.find(vid);
    if (it != m_actors.end()) {
        it->second.x = x;
        it->second.y = y;
        it->second.z = z;
        it->second.rotation = rotation;
        return true;
    }
    return false;
}

void ActorRegistry::SetDead(EntityVid vid, bool isDead) {
    auto it = m_actors.find(vid);
    if (it != m_actors.end()) {
        it->second.isDead = isDead;
    }
}

size_t ActorRegistry::Count() const {
    return m_actors.size();
}

void ActorRegistry::Clear() {
    m_actors.clear();
}

} // namespace Client::World
