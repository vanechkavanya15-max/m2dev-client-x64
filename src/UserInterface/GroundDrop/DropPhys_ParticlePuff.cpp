#include "../StdAfx.h"
#include "IDropPhysicsSimulator.h"
#include "../../EterBase/ModernLogger.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EffectLib/EffectManager.h"
#include "../Core/EventBus.h"
#include "../Core/Events.h"

#include <unordered_map>
#include <memory>
#include <random>
#include <cmath>
#include <d3dx9.h>

namespace UserInterface::GroundDrop
{
    class ParticlePuffPhysicsSimulator final : public IDropPhysicsSimulator
    {
    public:
        ParticlePuffPhysicsSimulator() = default;
        ~ParticlePuffPhysicsSimulator() override = default;

        void SpawnImpulse(uint32_t virtualId, float originX, float originY, float originZ, float force) override
        {
            EterBase::EntityId entity(virtualId);
            
            DropTrajectory traj;
            traj.posX = originX;
            traj.posY = originY;
            traj.posZ = originZ;
            
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> dis(0.0f, 360.0f);
            
            float angle = dis(gen) * 3.14159265f / 180.0f;
            traj.velX = std::cos(angle) * force;
            traj.velY = std::sin(angle) * force;
            traj.velZ = force * 1.5f;
            
            traj.isSettled = false;
            m_trajectories[entity.value()] = traj;
            
            EterBase::ModernLogger::Debug("Spawned impulse for item {} at ({}, {}, {}) with force {}", 
                                          entity.value(), originX, originY, originZ, force);
        }

        void StepPhysics(float deltaTime, float groundHeight) override
        {
            const float kGravityAccel = -980.0f;
            const float DAMPING = 0.5f;

            for (auto& [vid, traj] : m_trajectories)
            {
                if (traj.isSettled)
                    continue;

                traj.velZ += kGravityAccel * deltaTime;
                traj.posX += traj.velX * deltaTime;
                traj.posY += traj.velY * deltaTime;
                traj.posZ += traj.velZ * deltaTime;

                if (traj.posZ <= groundHeight)
                {
                    traj.posZ = groundHeight;

                    if (traj.velZ < -50.0f) 
                    {
                        auto res = SpawnPuffEffect(traj.posX, traj.posY, traj.posZ);
                        if (!res)
                        {
                            EterBase::ModernLogger::Warning("Could not spawn puff effect: {}", EterBase::ToString(res.error()));
                        }
                        
                        EterBase::EntityId entity(vid);
                        ::Core::Events::ItemDrop dropEvent;
                        dropEvent.id = entity.value();
                        dropEvent.vnum = 0;
                        dropEvent.count = 1;
                        dropEvent.x = static_cast<int32_t>(traj.posX);
                        dropEvent.y = static_cast<int32_t>(traj.posY);
                        dropEvent.z = static_cast<int32_t>(traj.posZ);
                        Core::EventBus::GetInstance().Publish(dropEvent);
                        
                        EterBase::ModernLogger::Info("Item {} hit ground, published ItemDrop event.", entity.value());
                        
                        traj.velZ = -traj.velZ * DAMPING;
                        traj.velX *= DAMPING;
                        traj.velY *= DAMPING;
                    }
                    else
                    {
                        traj.isSettled = true;
                        traj.velX = 0.0f;
                        traj.velY = 0.0f;
                        traj.velZ = 0.0f;
                    }
                }
            }
        }

        DropTrajectory GetTrajectory(uint32_t virtualId) const override
        {
            EterBase::EntityId entity(virtualId);
            auto it = m_trajectories.find(entity.value());
            if (it != m_trajectories.end())
            {
                return it->second;
            }
            return DropTrajectory{};
        }

        void Clear() override
        {
            m_trajectories.clear();
        }

    private:
        std::expected<void, EterBase::EntityError> SpawnPuffEffect(float x, float y, float z)
        {
            DWORD effectID = 0;
            if (CEffectManager::Instance().RegisterEffect2("d:/ymir work/effect/etc/dropitem/dropitem.mse", &effectID))
            {
                D3DXVECTOR3 pos(x, y, z);
                D3DXVECTOR3 rot(0.0f, 0.0f, 0.0f);
                CEffectManager::Instance().CreateEffect(effectID, pos, rot);
                return {};
            }
            return EterBase::MakeError(EterBase::EntityError::NotFound);
        }

    private:
        std::unordered_map<uint32_t, DropTrajectory> m_trajectories;
    };

} // namespace UserInterface::GroundDrop

std::unique_ptr<UserInterface::GroundDrop::IDropPhysicsSimulator> CreateDropPhysicsSimulator_ParticlePuff()
{
    return std::make_unique<UserInterface::GroundDrop::ParticlePuffPhysicsSimulator>();
}
