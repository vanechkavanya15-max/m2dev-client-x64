#include "ActorRegistry.h"
#include <mutex>

namespace Client::World {

bool ActorRegistry::RegisterActor(const ActorRecord& record) {
    std::unique_lock lock(m_mutex);
    if (m_actors.contains(record.vid)) {
        return false;
    }
    m_actors.emplace(record.vid, record);
    return true;
}

bool ActorRegistry::UnregisterActor(EntityVid vid) {
    std::unique_lock lock(m_mutex);
    return m_actors.erase(vid) > 0;
}

std::optional<ActorRecord> ActorRegistry::GetActor(EntityVid vid) const {
    std::shared_lock lock(m_mutex);
    auto it = m_actors.find(vid);
    if (it != m_actors.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool ActorRegistry::UpdatePosition(EntityVid vid, float x, float y, float z, float rotation) {
    std::unique_lock lock(m_mutex);
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
    std::unique_lock lock(m_mutex);
    auto it = m_actors.find(vid);
    if (it != m_actors.end()) {
        it->second.isDead = isDead;
    }
}

size_t ActorRegistry::Count() const {
    std::shared_lock lock(m_mutex);
    return m_actors.size();
}

void ActorRegistry::Clear() {
    std::unique_lock lock(m_mutex);
    m_actors.clear();
}

} // namespace Client::World
