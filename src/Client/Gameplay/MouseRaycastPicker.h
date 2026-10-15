#pragma once

#include <expected>
#include <span>
#include <optional>
#include <cstdint>
#include <cmath>

// Zaleznosci zewnetrzne - makiety (mocks), by komponent byl testowalny i niezalezny
namespace Client::Graphics {
    struct Vector3 {
        float x, y, z;
    };
    struct Ray {
        Vector3 origin;
        Vector3 direction;
    };
} // namespace Client::Graphics

namespace Client::Game {
    struct Actor {
        uint32_t id;
        Client::Graphics::Vector3 position;
        float radius;
        float height;
    };
} // namespace Client::Game

namespace Client::Gameplay {

    enum class RaycastError {
        InvalidRay,
        ActorListEmpty,
        NoIntersection
    };

    struct RaycastHit {
        uint32_t actorId;
        float distance;
        Client::Graphics::Vector3 hitPoint;
    };

    class MouseRaycastPicker {
    public:
        // Glowna funkcja: Znajduje najblizszego aktora trafionego przez promien.
        // Uzywa przeciecia walca (promien i wysokosc) dla prostoty i wydajnosci.
        [[nodiscard]] static std::expected<RaycastHit, RaycastError> 
        PickClosestActor(
            const Client::Graphics::Ray& ray, 
            std::span<const Client::Game::Actor> actors) noexcept;

    private:
        // Metoda pomocnicza: Oblicza punkt przeciecia miedzy promieniem a walcem.
        [[nodiscard]] static std::optional<float> 
        IntersectCylinder(
            const Client::Graphics::Ray& ray, 
            const Client::Game::Actor& actor) noexcept;
    };

    // --- Implementacja ---

    inline std::optional<float> 
    MouseRaycastPicker::IntersectCylinder(
        const Client::Graphics::Ray& ray, 
        const Client::Game::Actor& actor) noexcept 
    {
        // Sprawdzenie odleglosci w 2D na plaszczyznie XZ
        float dx = ray.direction.x;
        float dz = ray.direction.z;
        float ox = ray.origin.x - actor.position.x;
        float oz = ray.origin.z - actor.position.z;

        float a = dx * dx + dz * dz;
        if (a < 1e-8f) { // Promien jest pionowy, sprawdz czy poczatek jest w srodku profilu walca
            if (ox * ox + oz * oz <= actor.radius * actor.radius) {
                // Sprawdz granice w osi Y jesli promien wskazuje w gore lub w dol
                if (ray.direction.y < 0) { // w dol
                   float hitY = actor.position.y + actor.height;
                   if (ray.origin.y > hitY) {
                       return (hitY - ray.origin.y) / ray.direction.y;
                   }
                } else if (ray.direction.y > 0) { // w gore
                   float hitY = actor.position.y;
                   if (ray.origin.y < hitY) {
                        return (hitY - ray.origin.y) / ray.direction.y;
                   }
                }
            }
            return std::nullopt; 
        }

        float b = 2.0f * (ox * dx + oz * dz);
        float c = (ox * ox + oz * oz) - (actor.radius * actor.radius);

        float discriminant = b * b - 4.0f * a * c;
        if (discriminant < 0.0f) {
            return std::nullopt; // Brak przeciecia z walcem o nieskonczonej dlugosci
        }

        // Dwa punkty przeciecia z nieskonczonym walcem
        float t1 = (-b - std::sqrt(discriminant)) / (2.0f * a);
        float t2 = (-b + std::sqrt(discriminant)) / (2.0f * a);

        // Chcemy najmniejsza wartosc dodatnia t
        float t = (t1 >= 0.0f) ? t1 : t2;
        if (t < 0.0f) {
            return std::nullopt; // Walec jest za poczatkiem promienia
        }

        // Sprawdz czy punkt przeciecia znajduje sie w obrebie wysokosci walca (oś Y)
        float hitY = ray.origin.y + t * ray.direction.y;
        if (hitY >= actor.position.y && hitY <= actor.position.y + actor.height) {
            return t;
        }
        
        // Sprawdz przeciecia z koncowkami (gora i dol walca)
        if (ray.direction.y != 0.0f) {
            float t_bottom = (actor.position.y - ray.origin.y) / ray.direction.y;
            float t_top = (actor.position.y + actor.height - ray.origin.y) / ray.direction.y;
            
            float min_t_cap = -1.0f;
            
            // Sprawdz dol walca
            if (t_bottom >= 0.0f) {
                float px = ray.origin.x + t_bottom * ray.direction.x;
                float pz = ray.origin.z + t_bottom * ray.direction.z;
                if ((px - actor.position.x) * (px - actor.position.x) + (pz - actor.position.z) * (pz - actor.position.z) <= actor.radius * actor.radius) {
                    min_t_cap = t_bottom;
                }
            }
            
            // Sprawdz gore walca
            if (t_top >= 0.0f) {
                float px = ray.origin.x + t_top * ray.direction.x;
                float pz = ray.origin.z + t_top * ray.direction.z;
                if ((px - actor.position.x) * (px - actor.position.x) + (pz - actor.position.z) * (pz - actor.position.z) <= actor.radius * actor.radius) {
                    if (min_t_cap < 0.0f || t_top < min_t_cap) {
                        min_t_cap = t_top;
                    }
                }
            }
            
            if (min_t_cap >= 0.0f) {
                return min_t_cap;
            }
        }

        return std::nullopt;
    }

    inline std::expected<RaycastHit, RaycastError> 
    MouseRaycastPicker::PickClosestActor(
        const Client::Graphics::Ray& ray, 
        std::span<const Client::Game::Actor> actors) noexcept 
    {
        // Sprawdzenie poprawnosci promienia (wektor kierunkowy nie powinien byc zerowy)
        if (ray.direction.x == 0.0f && ray.direction.y == 0.0f && ray.direction.z == 0.0f) {
            return std::unexpected(RaycastError::InvalidRay);
        }

        if (actors.empty()) {
            return std::unexpected(RaycastError::ActorListEmpty);
        }

        std::optional<RaycastHit> closestHit;

        for (const auto& actor : actors) {
            auto hitT = IntersectCylinder(ray, actor);
            if (hitT) {
                float t = *hitT;
                if (!closestHit || t < closestHit->distance) {
                    closestHit = RaycastHit{
                        .actorId = actor.id,
                        .distance = t,
                        .hitPoint = {
                            ray.origin.x + t * ray.direction.x,
                            ray.origin.y + t * ray.direction.y,
                            ray.origin.z + t * ray.direction.z
                        }
                    };
                }
            }
        }

        if (closestHit) {
            return *closestHit;
        }

        return std::unexpected(RaycastError::NoIntersection);
    }

} // namespace Client::Gameplay
