#include "../StdAfx.h"
#include "IInstanceMountHorseController.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"
#include <cmath>

namespace UserInterface::Core {
    struct MountCombatStateEvent : public IEvent {
        float attackRange;
        float rotationAngle;
        
        MountCombatStateEvent(float attackRange, float rotationAngle)
            : attackRange(attackRange), rotationAngle(rotationAngle) {}
    };
}

namespace UserInterface::InstanceControllers {

    class InstanceMountCombatController : public IInstanceMountHorseController {
    public:
        InstanceMountCombatController() = default;
        ~InstanceMountCombatController() override = default;

        EterBase::PacketResult<void> Mount(EterBase::EntityId mountVid, uint32_t mountVnum) override {
            EterBase::ModernLogger::Error("Mount method stubbed in combat controller.");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        EterBase::PacketResult<void> Dismount() override {
            EterBase::ModernLogger::Error("Dismount method stubbed in combat controller.");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        bool IsMounted() const override {
            return isMounted;
        }

        EterBase::EntityId GetMountVID() const override {
            return currentMountVid;
        }

        uint32_t GetMountVnum() const override {
            return currentMountVnum;
        }

        void UpdateMountTransform(float x, float y, float z, float rot) override {
            if (!isMounted) return;

            auto rangeResult = CalculateCombatAttackRange(x, y, z);
            auto rotationResult = CalculateCombatRotation(rot);

            float attackRange = rangeResult.value_or(0.0f);
            float rotationAngle = rotationResult.value_or(0.0f);

            UserInterface::Core::EventBus::GetInstance().Publish(
                UserInterface::Core::MountCombatStateEvent(attackRange, rotationAngle)
            );
        }

        void Clear() override {
            EterBase::ModernLogger::Error("Clear method stubbed in combat controller.");
        }

    private:
        bool isMounted{false};
        EterBase::EntityId currentMountVid;
        uint32_t currentMountVnum{0};

        std::expected<float, std::string_view> CalculateCombatAttackRange(float x, float y, float z) const {
            // Placeholder logic for range calculation.
            float range = std::sqrt(x * x + y * y + z * z);
            return range;
        }

        std::expected<float, std::string_view> CalculateCombatRotation(float rot) const {
             // Placeholder logic for rotation calculation.
            float newRot = std::fmod(rot + 15.0f, 360.0f);
            return newRot;
        }
    };

}
