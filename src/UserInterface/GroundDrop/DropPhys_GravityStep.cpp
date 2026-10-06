#include "../StdAfx.h"
#include "IDropPhysicsSimulator.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include <unordered_map>
#include <cmath>
#include <memory>

namespace UserInterface::GroundDrop {

    struct GroundDropSettledEvent : public Core::IEvent {
        EterBase::EntityId dropId;
        float finalX;
        float finalY;
        float finalZ;

        GroundDropSettledEvent(EterBase::EntityId id, float x, float y, float z)
            : dropId(id), finalX(x), finalY(y), finalZ(z) {}
    };

    class DropPhys_GravityStep final : public IDropPhysicsSimulator {
    public:
        void SpawnImpulse(uint32_t virtualId, float originX, float originY, float originZ, float force) override {
            EterBase::EntityId id(virtualId);
            
            DropTrajectory t;
            t.posX = originX;
            t.posY = originY;
            t.posZ = originZ;
            
            // Apply impulse velocity
            t.velX = force * 0.5f;
            t.velY = force * 0.5f;
            t.velZ = force;
            t.isSettled = false;
            
            trajectories_[id] = t;
            
            EterBase::ModernLogger::Debug("DropPhys_GravityStep: Spawned impulse for VID: {} at ({}, {}, {}) with force {}", 
                                            id.value(), originX, originY, originZ, force);
        }

        void StepPhysics(float deltaTime, float groundHeight) override {
            constexpr float GRAVITY = -981.0f; // Standard gravity constant scaled for the engine
            
            for (auto& [id, traj] : trajectories_) {
                if (traj.isSettled) {
                    continue;
                }
                
                // Update velocity with gravity
                traj.velZ += GRAVITY * deltaTime;
                
                // Update position
                traj.posX += traj.velX * deltaTime;
                traj.posY += traj.velY * deltaTime;
                traj.posZ += traj.velZ * deltaTime;
                
                // Check ground collision
                if (traj.posZ <= groundHeight) {
                    traj.posZ = groundHeight;
                    traj.velX = 0.0f;
                    traj.velY = 0.0f;
                    traj.velZ = 0.0f;
                    traj.isSettled = true;
                    
                    // Publish event that drop settled
                    UserInterface::Core::EventBus::GetInstance().Publish(GroundDropSettledEvent{id, traj.posX, traj.posY, traj.posZ});
                }
            }
        }

        DropTrajectory GetTrajectory(uint32_t virtualId) const override {
            EterBase::EntityId id(virtualId);
            if (auto it = trajectories_.find(id); it != trajectories_.end()) {
                return it->second;
            }
            return DropTrajectory{};
        }

        void Clear() override {
            trajectories_.clear();
            EterBase::ModernLogger::Debug("DropPhys_GravityStep: Cleared all trajectories");
        }
        
    private:
        std::unordered_map<EterBase::EntityId, DropTrajectory> trajectories_;
    };

    // Factory method using std::expected
    [[nodiscard]] std::expected<std::unique_ptr<IDropPhysicsSimulator>, EterBase::EntityError> CreateDropPhysicsSimulator() {
        auto simulator = std::make_unique<DropPhys_GravityStep>();
        if (!simulator) {
            return std::unexpected(EterBase::EntityError::NotFound);
        }
        return simulator;
    }
}
