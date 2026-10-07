#include "StrongTypes.h"
#include <cmath>

namespace Client::Core {

float MapCoords::Length() const noexcept {
    return std::sqrt(x * x + y * y + z * z);
}

float MapCoords::Distance(const MapCoords& other) const noexcept {
    return (*this - other).Length();
}

MapCoords MapCoords::Normalize() const noexcept {
    const float len = Length();
    if (len > 0.0f) {
        return *this / len;
    }
    return *this;
}

} // namespace Client::Core

std::size_t std::hash<Client::Core::MapCoords>::operator()(const Client::Core::MapCoords& obj) const noexcept {
    std::size_t h1 = std::hash<float>{}(obj.x);
    std::size_t h2 = std::hash<float>{}(obj.y);
    std::size_t h3 = std::hash<float>{}(obj.z);
    
    // Simple hash combine
    std::size_t hash = h1;
    hash ^= h2 + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= h3 + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    
    return hash;
}
