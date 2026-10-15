#include "StranglerInstanceFacade.h"
#include "StranglerFacade.h"
#include <string>

namespace Client::Bridge {

using EntityVid = Client::World::EntityVid;
using EntityHandle = ::Client::Actor::EntityHandle;

StranglerInstanceFacade& StranglerInstanceFacade::Instance() noexcept {
    static StranglerInstanceFacade s_instance;
    return s_instance;
}

void StranglerInstanceFacade::SetWorldContext(Client::Core::WorldContext* context) noexcept {
    m_context = context;
}

void StranglerInstanceFacade::ClearWorldContext() noexcept {
    m_context = nullptr;
}

Client::Core::WorldContext* StranglerInstanceFacade::GetWorldContext() const noexcept {
    if (m_context) {
        return m_context;
    }
    return &StranglerFacade::Instance().GetWorldContext();
}

EterBase::VoidResult<> StranglerInstanceFacade::RegisterInstance(
    EntityVid vid, uint32_t race, uint8_t type, float x, float y, float z, float rotation, std::string_view name) {
    
    auto* ctx = GetWorldContext();
    if (!ctx) {
        return EterBase::MakeError("Brak dostepnego WorldContext w StranglerInstanceFacade");
    }

    Client::World::ActorRecord record{
        .vid = vid,
        .race = race,
        .type = type,
        .x = x,
        .y = y,
        .z = z,
        .rotation = rotation,
        .name = std::string(name),
        .guildId = 0,
        .empire = 0,
        .isDead = false
    };

    if (!ctx->RegisterActor(record)) {
        return EterBase::MakeError("Nie udalo sie zarejestrowac aktora w ActorRegistry");
    }

    // Rejestracja powiazania w GenerationalRegistry
    const uint32_t rawVid = vid.get();
    {
        std::unique_lock genLock(m_genMutex);
        if (!m_vidToHandle.contains(rawVid)) {
            auto handleRes = m_generationalRegistry.Insert(rawVid);
            if (handleRes) {
                m_vidToHandle[rawVid] = *handleRes;
            }
        }
    }

    return {};
}

EterBase::VoidResult<> StranglerInstanceFacade::RegisterInstance(
    uint32_t vid, uint32_t race, uint8_t type, float x, float y, float z, float rotation, std::string_view name) {
    return RegisterInstance(EntityVid(vid), race, type, x, y, z, rotation, name);
}

EterBase::VoidResult<> StranglerInstanceFacade::UnregisterInstance(EntityVid vid) {
    auto* ctx = GetWorldContext();
    if (!ctx) {
        return EterBase::MakeError("Brak dostepnego WorldContext w StranglerInstanceFacade");
    }

    const uint32_t rawVid = vid.get();
    // Uniewaznienie powiazania w GenerationalRegistry
    {
        std::unique_lock genLock(m_genMutex);
        auto it = m_vidToHandle.find(rawVid);
        if (it != m_vidToHandle.end()) {
            (void)m_generationalRegistry.Erase(it->second);
            m_vidToHandle.erase(it);
        }
    }

    if (!ctx->UnregisterActor(Client::Core::EntityVid{rawVid})) {
        return EterBase::MakeError("Aktor o zadanym VID nie istnieje w ActorRegistry");
    }

    return {};
}

EterBase::VoidResult<> StranglerInstanceFacade::UnregisterInstance(uint32_t vid) {
    return UnregisterInstance(EntityVid(vid));
}

EterBase::VoidResult<> StranglerInstanceFacade::UpdatePosition(EntityVid vid, float x, float y, float z, float rotation) {
    auto* ctx = GetWorldContext();
    if (!ctx) {
        return EterBase::MakeError("Brak dostepnego WorldContext w StranglerInstanceFacade");
    }

    if (!ctx->UpdateActorPosition(Client::Core::EntityVid{vid.get()}, x, y, z, rotation)) {
        return EterBase::MakeError("Aktor o zadanym VID nie istnieje, aktualizacja pozycji niemozliwa");
    }

    return {};
}

EterBase::VoidResult<> StranglerInstanceFacade::UpdatePosition(uint32_t vid, float x, float y, float z, float rotation) {
    return UpdatePosition(EntityVid(vid), x, y, z, rotation);
}

EterBase::VoidResult<> StranglerInstanceFacade::SetDead(EntityVid vid, bool isDead) {
    auto* ctx = GetWorldContext();
    if (!ctx) {
        return EterBase::MakeError("Brak dostepnego WorldContext w StranglerInstanceFacade");
    }

    auto record = ctx->actors.GetActor(vid);
    if (!record) {
        return EterBase::MakeError("Aktor o zadanym VID nie istnieje, zmiana stanu niemozliwa");
    }

    ctx->actors.SetDead(vid, isDead);
    return {};
}

EterBase::VoidResult<> StranglerInstanceFacade::SetDead(uint32_t vid, bool isDead) {
    return SetDead(EntityVid(vid), isDead);
}

EterBase::VoidResult<> StranglerInstanceFacade::SetMainInstance(
    EntityVid vid, float x, float y, float z, float rotation, std::string_view name) {
    
    auto* ctx = GetWorldContext();
    if (!ctx) {
        return EterBase::MakeError("Brak dostepnego WorldContext w StranglerInstanceFacade");
    }

    ctx->SetLocalPlayer(Client::Core::EntityVid{vid.get()}, x, y, z, rotation, std::string(name));
    return {};
}

EterBase::VoidResult<> StranglerInstanceFacade::SetMainInstance(
    uint32_t vid, float x, float y, float z, float rotation, std::string_view name) {
    return SetMainInstance(EntityVid(vid), x, y, z, rotation, name);
}

EntityVid StranglerInstanceFacade::GetMainActorVid() const noexcept {
    auto* ctx = GetWorldContext();
    if (!ctx) {
        return EntityVid{0};
    }
    return EntityVid{ctx->localPlayerVid.get()};
}

std::optional<Client::Actor::EntityHandle> StranglerInstanceFacade::GetMainActorHandle() const noexcept {
    const auto vid = GetMainActorVid();
    if (vid.get() == 0) {
        return std::nullopt;
    }
    return GetGenerationalHandle(vid);
}

bool StranglerInstanceFacade::IsActorAlive(EntityVid vid) const noexcept {
    auto* ctx = GetWorldContext();
    if (!ctx) {
        return false;
    }
    return ctx->actors.IsAlive(vid);
}

bool StranglerInstanceFacade::IsActorAlive(uint32_t vid) const noexcept {
    return IsActorAlive(EntityVid(vid));
}

bool StranglerInstanceFacade::IsActorDead(EntityVid vid) const noexcept {
    auto* ctx = GetWorldContext();
    if (!ctx) {
        return false;
    }
    return ctx->actors.IsDead(vid);
}

bool StranglerInstanceFacade::IsActorDead(uint32_t vid) const noexcept {
    return IsActorDead(EntityVid(vid));
}

bool StranglerInstanceFacade::HasActor(EntityVid vid) const noexcept {
    auto* ctx = GetWorldContext();
    if (!ctx) {
        return false;
    }
    return ctx->actors.HasActor(vid);
}

bool StranglerInstanceFacade::HasActor(uint32_t vid) const noexcept {
    return HasActor(EntityVid(vid));
}

std::optional<Client::World::ActorRecord> StranglerInstanceFacade::GetActor(EntityVid vid) const {
    auto* ctx = GetWorldContext();
    if (!ctx) {
        return std::nullopt;
    }
    return ctx->actors.GetActor(vid);
}

std::optional<Client::World::ActorRecord> StranglerInstanceFacade::GetActor(uint32_t vid) const {
    return GetActor(EntityVid(vid));
}

size_t StranglerInstanceFacade::GetActorCount() const noexcept {
    auto* ctx = GetWorldContext();
    if (!ctx) {
        return 0;
    }
    return ctx->actors.Count();
}

EterBase::Result<Client::Actor::EntityHandle, EterBase::EntityError> StranglerInstanceFacade::RegisterGenerational(EntityVid vid) {
    return RegisterGenerational(vid.get());
}

EterBase::Result<Client::Actor::EntityHandle, EterBase::EntityError> StranglerInstanceFacade::RegisterGenerational(uint32_t vid) {
    std::unique_lock lock(m_genMutex);
    auto it = m_vidToHandle.find(vid);
    if (it != m_vidToHandle.end()) {
        return it->second;
    }
    auto handleRes = m_generationalRegistry.Insert(vid);
    if (handleRes) {
        m_vidToHandle[vid] = *handleRes;
    }
    return handleRes;
}

EterBase::Result<void, EterBase::EntityError> StranglerInstanceFacade::UnregisterGenerational(Client::Actor::EntityHandle handle) {
    std::unique_lock lock(m_genMutex);
    auto vidRes = m_generationalRegistry.Get(handle);
    if (vidRes) {
        m_vidToHandle.erase(*vidRes.value());
    }
    return m_generationalRegistry.Erase(handle);
}

std::optional<uint32_t> StranglerInstanceFacade::ResolveGenerational(Client::Actor::EntityHandle handle) const {
    std::shared_lock lock(m_genMutex);
    auto res = m_generationalRegistry.Get(handle);
    if (res) {
        return *res.value();
    }
    return std::nullopt;
}

std::optional<EntityVid> StranglerInstanceFacade::ResolveGenerationalVid(Client::Actor::EntityHandle handle) const {
    auto raw = ResolveGenerational(handle);
    if (raw) {
        return EntityVid(*raw);
    }
    return std::nullopt;
}

std::optional<Client::Actor::EntityHandle> StranglerInstanceFacade::GetGenerationalHandle(EntityVid vid) const {
    return GetGenerationalHandle(vid.get());
}

std::optional<Client::Actor::EntityHandle> StranglerInstanceFacade::GetGenerationalHandle(uint32_t vid) const {
    std::shared_lock lock(m_genMutex);
    auto it = m_vidToHandle.find(vid);
    if (it != m_vidToHandle.end()) {
        return it->second;
    }
    return std::nullopt;
}

void StranglerInstanceFacade::ClearGenerational() noexcept {
    std::unique_lock lock(m_genMutex);
    m_generationalRegistry.Clear();
    m_vidToHandle.clear();
}

} // namespace Client::Bridge
