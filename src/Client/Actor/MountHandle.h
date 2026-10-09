#pragma once

#include <optional>
#include <string_view>
#include <format>
#include <expected>
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../World/ActorRegistry.h"

namespace Client::Actor {

enum class MountLinkError : uint8_t {
    None = 0,
    InvalidRider,
    InvalidMount,
    InvalidPet,
    RiderNotFound,
    RiderDead,
    MountNotFound,
    MountDead,
    PetNotFound,
    PetDead,
    AlreadyMounted,
    NotMounted,
    AlreadyHasPet,
    NoPet
};

[[nodiscard]] constexpr std::string_view ToString(MountLinkError err) noexcept {
    switch (err) {
        case MountLinkError::None: return "None";
        case MountLinkError::InvalidRider: return "InvalidRider";
        case MountLinkError::InvalidMount: return "InvalidMount";
        case MountLinkError::InvalidPet: return "InvalidPet";
        case MountLinkError::RiderNotFound: return "RiderNotFound";
        case MountLinkError::RiderDead: return "RiderDead";
        case MountLinkError::MountNotFound: return "MountNotFound";
        case MountLinkError::MountDead: return "MountDead";
        case MountLinkError::PetNotFound: return "PetNotFound";
        case MountLinkError::PetDead: return "PetDead";
        case MountLinkError::AlreadyMounted: return "AlreadyMounted";
        case MountLinkError::NotMounted: return "NotMounted";
        case MountLinkError::AlreadyHasPet: return "AlreadyHasPet";
        case MountLinkError::NoPet: return "NoPet";
    }
    return "UnknownMountLinkError";
}

template <typename T = void>
using MountLinkResult = std::expected<T, MountLinkError>;

class MountHandle {
public:
    explicit MountHandle(Client::World::EntityVid riderVid) noexcept : m_riderVid(riderVid) {}

    MountHandle(const MountHandle&) = delete;
    MountHandle& operator=(const MountHandle&) = delete;
    MountHandle(MountHandle&&) noexcept = default;
    MountHandle& operator=(MountHandle&&) noexcept = default;

    [[nodiscard]] Client::World::EntityVid GetRiderVid() const noexcept { return m_riderVid; }
    [[nodiscard]] std::optional<Client::World::EntityVid> GetMountVid() const noexcept { return m_mountVid; }
    [[nodiscard]] std::optional<Client::World::EntityVid> GetPetVid() const noexcept { return m_petVid; }

    MountLinkResult<void> AttachMount(Client::World::EntityVid mountVid) noexcept {
        if (!mountVid) {
            return std::unexpected(MountLinkError::InvalidMount);
        }
        if (m_mountVid.has_value()) {
            return std::unexpected(MountLinkError::AlreadyMounted);
        }
        m_mountVid = mountVid;
        return {};
    }

    MountLinkResult<void> DetachMount() noexcept {
        if (!m_mountVid.has_value()) {
            return std::unexpected(MountLinkError::NotMounted);
        }
        m_mountVid.reset();
        return {};
    }

    MountLinkResult<void> AttachPet(Client::World::EntityVid petVid) noexcept {
        if (!petVid) {
            return std::unexpected(MountLinkError::InvalidPet);
        }
        if (m_petVid.has_value()) {
            return std::unexpected(MountLinkError::AlreadyHasPet);
        }
        m_petVid = petVid;
        return {};
    }

    MountLinkResult<void> DetachPet() noexcept {
        if (!m_petVid.has_value()) {
            return std::unexpected(MountLinkError::NoPet);
        }
        m_petVid.reset();
        return {};
    }

    MountLinkResult<void> ValidateMount(const Client::World::ActorRegistry& registry) const noexcept {
        if (!m_mountVid.has_value()) {
            return std::unexpected(MountLinkError::NotMounted);
        }
        auto mount = registry.GetActor(*m_mountVid);
        if (!mount.has_value()) {
            return std::unexpected(MountLinkError::MountNotFound);
        }
        if (mount->isDead) {
            return std::unexpected(MountLinkError::MountDead);
        }
        return {};
    }

    MountLinkResult<void> ValidatePet(const Client::World::ActorRegistry& registry) const noexcept {
        if (!m_petVid.has_value()) {
            return std::unexpected(MountLinkError::NoPet);
        }
        auto pet = registry.GetActor(*m_petVid);
        if (!pet.has_value()) {
            return std::unexpected(MountLinkError::PetNotFound);
        }
        if (pet->isDead) {
            return std::unexpected(MountLinkError::PetDead);
        }
        return {};
    }

    MountLinkResult<void> SynchronizePositions(Client::World::ActorRegistry& registry) noexcept {
        if (!m_riderVid) {
            return std::unexpected(MountLinkError::InvalidRider);
        }

        auto rider = registry.GetActor(m_riderVid);
        if (!rider.has_value()) {
            return std::unexpected(MountLinkError::RiderNotFound);
        }
        if (rider->isDead) {
            return std::unexpected(MountLinkError::RiderDead);
        }

        if (m_mountVid.has_value()) {
            auto mountRes = ValidateMount(registry);
            if (mountRes.has_value()) {
                registry.UpdatePosition(*m_mountVid, rider->x, rider->y, rider->z, rider->rotation);
            } else {
                // Jesli wierzchowiec stal sie niepoprawny/martwy, odlacz go.
                m_mountVid.reset();
            }
        }

        if (m_petVid.has_value()) {
            auto petRes = ValidatePet(registry);
            if (petRes.has_value()) {
                // Przesun zwierzaka nieco, aby nie nakladal sie dokladnie na gracza
                registry.UpdatePosition(*m_petVid, rider->x + 50.0f, rider->y + 50.0f, rider->z, rider->rotation);
            } else {
                // Jesli zwierzak stal sie niepoprawny/martwy, odlacz go.
                m_petVid.reset();
            }
        }

        return {};
    }

private:
    Client::World::EntityVid m_riderVid;
    std::optional<Client::World::EntityVid> m_mountVid;
    std::optional<Client::World::EntityVid> m_petVid;
};

} // namespace Client::Actor

template <>
struct std::formatter<Client::Actor::MountLinkError> : std::formatter<std::string_view> {
    auto format(Client::Actor::MountLinkError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Actor::ToString(err), ctx);
    }
};
