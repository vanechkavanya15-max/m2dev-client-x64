#include "StranglerInstanceFacade.h"
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
    return m_context;
}

EterBase::VoidResult<> StranglerInstanceFacade::RegisterInstance(
    uint32_t vid, uint32_t race, uint8_t type, float x, float y, float z, float rotation, std::string_view name) {
    
    if (!m_context) {
        return EterBase::MakeError("Brak ustawionego WorldContext w StranglerInstanceFacade");
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

    if (!m_context->RegisterActor(record)) {
        return EterBase::MakeError("Nie udalo sie zarejestrowac aktora w ActorRegistry");
    }

    return {};
}

EterBase::VoidResult<> StranglerInstanceFacade::UnregisterInstance(uint32_t vid) {
    if (!m_context) {
        return EterBase::MakeError("Brak ustawionego WorldContext w StranglerInstanceFacade");
    }

    if (!m_context->UnregisterActor(Client::World::EntityVid(vid))) {
        return EterBase::MakeError("Aktor o zadanym VID nie istnieje w ActorRegistry");
    }

    return {};
}

EterBase::VoidResult<> StranglerInstanceFacade::UpdatePosition(uint32_t vid, float x, float y, float z, float rotation) {
    if (!m_context) {
        return EterBase::MakeError("Brak ustawionego WorldContext w StranglerInstanceFacade");
    }

    if (!m_context->UpdateActorPosition(Client::World::EntityVid(vid), x, y, z, rotation)) {
        return EterBase::MakeError("Aktor o zadanym VID nie istnieje, aktualizacja pozycji niemozliwa");
    }

    return {};
}

EterBase::VoidResult<> StranglerInstanceFacade::SetDead(uint32_t vid, bool isDead) {
    if (!m_context) {
        return EterBase::MakeError("Brak ustawionego WorldContext w StranglerInstanceFacade");
    }

    auto record = m_context->actors.GetActor(Client::World::EntityVid(vid));
    if (!record) {
        return EterBase::MakeError("Aktor o zadanym VID nie istnieje, zmiana stanu niemozliwa");
    }

    m_context->actors.SetDead(Client::World::EntityVid(vid), isDead);
    return {};
}

EterBase::VoidResult<> StranglerInstanceFacade::SetMainInstance(
    uint32_t vid, float x, float y, float z, float rotation, std::string_view name) {
    
    if (!m_context) {
        return EterBase::MakeError("Brak ustawionego WorldContext w StranglerInstanceFacade");
    }

    m_context->SetLocalPlayer(Client::World::EntityVid(vid), x, y, z, rotation, std::string(name));
    return {};
}

} // namespace Client::Bridge
