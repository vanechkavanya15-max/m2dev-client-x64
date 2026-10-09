#include "StranglerInstanceFacade.h"
#include "StranglerFacade.h"
#include <string>

namespace Client::Bridge {

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
    uint32_t vid, uint32_t race, uint8_t type, float x, float y, float z, float rotation, std::string_view name) {
    
    auto* ctx = GetWorldContext();
    if (!ctx) {
        return EterBase::MakeError("Brak dostepnego WorldContext w StranglerInstanceFacade");
    }

    Client::World::ActorRecord record{
        .vid = Client::World::EntityVid(vid),
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
    {
        std::unique_lock genLock(m_genMutex);
        if (!m_vidToHandle.contains(vid)) {
            auto handleRes = m_generationalRegistry.Insert(vid);
            if (handleRes) {
                m_vidToHandle[vid] = *handleRes;
            }
        }
    }

    return {};
}

EterBase::VoidResult<> StranglerInstanceFacade::UnregisterInstance(uint32_t vid) {
    auto* ctx = GetWorldContext();
    if (!ctx) {
        return EterBase::MakeError("Brak dostepnego WorldContext w StranglerInstanceFacade");
    }

    // Uniewaznienie powiazania w GenerationalRegistry
    {
        std::unique_lock genLock(m_genMutex);
        auto it = m_vidToHandle.find(vid);
        if (it != m_vidToHandle.end()) {
            (void)m_generationalRegistry.Erase(it->second);
            m_vidToHandle.erase(it);
        }
    }

    if (!ctx->UnregisterActor(Client::Core::EntityVid{vid})) {
        return EterBase::MakeError("Aktor o zadanym VID nie istnieje w ActorRegistry");
    }

    return {};
}

EterBase::VoidResult<> StranglerInstanceFacade::UpdatePosition(uint32_t vid, float x, float y, float z, float rotation) {
    auto* ctx = GetWorldContext();
    if (!ctx) {
        return EterBase::MakeError("Brak dostepnego WorldContext w StranglerInstanceFacade");
    }

    if (!ctx->UpdateActorPosition(Client::Core::EntityVid{vid}, x, y, z, rotation)) {
        return EterBase::MakeError("Aktor o zadanym VID nie istnieje, aktualizacja pozycji niemozliwa");
    }

    return {};
}

EterBase::VoidResult<> StranglerInstanceFacade::SetDead(uint32_t vid, bool isDead) {
    auto* ctx = GetWorldContext();
    if (!ctx) {
        return EterBase::MakeError("Brak dostepnego WorldContext w StranglerInstanceFacade");
    }

    auto record = ctx->actors.GetActor(Client::World::EntityVid(vid));
    if (!record) {
        return EterBase::MakeError("Aktor o zadanym VID nie istnieje, zmiana stanu niemozliwa");
    }

    ctx->actors.SetDead(Client::World::EntityVid(vid), isDead);
    return {};
}

EterBase::VoidResult<> StranglerInstanceFacade::SetMainInstance(
    uint32_t vid, float x, float y, float z, float rotation, std::string_view name) {
    
    auto* ctx = GetWorldContext();
    if (!ctx) {
        return EterBase::MakeError("Brak dostepnego WorldContext w StranglerInstanceFacade");
    }

    ctx->SetLocalPlayer(Client::Core::EntityVid{vid}, x, y, z, rotation, std::string(name));
    return {};
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
