#pragma once

#include "TransformComponentTable.h"
#include "CombatComponentTable.h"
#include "../Core/EventBus.h"
#include "../Packet.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"

#include <expected>
#include <memory>
#include <cstdint>

namespace UserInterface::ECS
{
    struct EntityRegisteredEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;
        explicit EntityRegisteredEvent(EterBase::EntityId id) : entityId(id) {}
    };

    struct EntityRemovedEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;
        explicit EntityRemovedEvent(EterBase::EntityId id) : entityId(id) {}
    };

    class ECSWorldRegistry final
    {
    public:
        static ECSWorldRegistry& GetInstance()
        {
            static ECSWorldRegistry instance;
            return instance;
        }

        void Reserve(size_t capacity)
        {
            transformTable_.Reserve(capacity);
            combatTable_.Reserve(capacity);
            EterBase::ModernLogger::Info("ECSWorldRegistry reserved capacity for {} entities.", capacity);
        }

        void Clear() noexcept
        {
            transformTable_.Clear();
            combatTable_.Clear();
            EterBase::ModernLogger::Info("ECSWorldRegistry cleared all entities.");
        }

        std::expected<void, EterBase::EntityError> RegisterEntity(
            EterBase::EntityId id, 
            float x, float y, float z, float rot, float speed,
            uint32_t hp = 100, uint32_t maxHp = 100, uint8_t state = 0, uint8_t dead = 0)
        {
            if (id.value() == 0)
            {
                EterBase::ModernLogger::Error("Failed to register entity: Invalid ID 0");
                return std::unexpected(EterBase::EntityError::InvalidType);
            }

            transformTable_.AddOrUpdate(id.value(), x, y, z, rot, speed);
            combatTable_.AddOrUpdate(id.value(), hp, maxHp, state, dead);

            EterBase::ModernLogger::Debug("Entity {} registered in ECS.", id.value());
            Core::EventBus::GetInstance().Publish(EntityRegisteredEvent{id});

            return {};
        }

        std::expected<void, EterBase::EntityError> RemoveEntity(EterBase::EntityId id)
        {
            if (id.value() == 0)
            {
                return std::unexpected(EterBase::EntityError::InvalidType);
            }

            transformTable_.Remove(id.value());
            combatTable_.Remove(id.value());

            EterBase::ModernLogger::Debug("Entity {} removed from ECS.", id.value());
            Core::EventBus::GetInstance().Publish(EntityRemovedEvent{id});

            return {};
        }

        TransformComponentTable& GetTransformTable() { return transformTable_; }
        const TransformComponentTable& GetTransformTable() const { return transformTable_; }
        CombatComponentTable& GetCombatTable() { return combatTable_; }
        const CombatComponentTable& GetCombatTable() const { return combatTable_; }

    private:
        ECSWorldRegistry() = default;
        ~ECSWorldRegistry() = default;

        ECSWorldRegistry(const ECSWorldRegistry&) = delete;
        ECSWorldRegistry& operator=(const ECSWorldRegistry&) = delete;
        ECSWorldRegistry(ECSWorldRegistry&&) = delete;
        ECSWorldRegistry& operator=(ECSWorldRegistry&&) = delete;

        TransformComponentTable transformTable_;
        CombatComponentTable combatTable_;
    };
}
