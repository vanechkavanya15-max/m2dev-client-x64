#include "../StdAfx.h"
#include "ITextTailService.h"
#include "../Core/EventBus.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../ECS/ECSWorldRegistry.h"

#include <cmath>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <memory>

namespace UserInterface::TextTail
{
    struct TextTailInstance
    {
        EterBase::EntityId virtualId{0};
        float offsetY{0.0f};
        bool isVisible{true};
    };

    class TextTailOcclusionCullImpl final : public ITextTailService
    {
    public:
        TextTailOcclusionCullImpl() = default;
        ~TextTailOcclusionCullImpl() override = default;

        EterBase::PacketResult<void> RegisterActorTail(const TextTailCreateData& data) override
        {
            if (m_tails.contains(data.vid.value()))
            {
                EterBase::ModernLogger::Warning("TextTail already exists for actor VID {}", data.vid.value());
                return std::unexpected(EterBase::PacketError::InvalidHeader);
            }

            TextTailInstance inst;
            inst.virtualId = data.vid;
            inst.offsetY = data.offsetY;
            inst.isVisible = true;
            
            m_tails[data.vid.value()] = inst;
            EterBase::ModernLogger::Trace("Registered text tail for actor VID {}", data.vid.value());
            return {};
        }

        EterBase::PacketResult<void> RegisterItemTail(uint32_t virtualId, std::string_view name) override
        {
            if (m_tails.contains(virtualId))
            {
                return std::unexpected(EterBase::PacketError::InvalidHeader); 
            }

            TextTailInstance inst;
            inst.virtualId = EterBase::EntityId(virtualId);
            inst.isVisible = true;
            m_tails[virtualId] = inst;
            
            return {};
        }

        void RemoveTail(uint32_t virtualId) override
        {
            if (m_tails.erase(virtualId) > 0)
            {
                EterBase::ModernLogger::Trace("Removed text tail for VID {}", virtualId);
            }
        }

        void UpdateScreenPositions(float viewMatrix[16], float projMatrix[16]) override
        {
            float vpMatrix[16];
            // Fix Matrix Math Error: Matrix order must be viewMatrix * projMatrix for row-vector math
            MultiplyMatrices(viewMatrix, projMatrix, vpMatrix);

            const auto& transformTable = ECS::ECSWorldRegistry::GetInstance().GetTransformTable();
            const auto& idToIndexMap = transformTable.GetIdToIndexMap();

            for (auto& [id, tail] : m_tails)
            {
                auto it = idToIndexMap.find(id);
                if (it == idToIndexMap.end())
                {
                    // If entity isn't in ECS yet, default to hidden or last known visibility
                    continue;
                }

                size_t ecsIndex = it->second;
                float worldX = transformTable.posX[ecsIndex];
                float worldY = transformTable.posY[ecsIndex];
                float worldZ = transformTable.posZ[ecsIndex] + tail.offsetY; // Apply offset bug fix

                // Projection
                float clipW = worldX * vpMatrix[3] + worldY * vpMatrix[7] + worldZ * vpMatrix[11] + vpMatrix[15];
                bool wasVisible = tail.isVisible;
                bool isNowVisible = true;

                if (clipW < 0.1f)
                {
                    // Behind camera or too close to near plane
                    isNowVisible = false;
                }
                else
                {
                    float clipX = worldX * vpMatrix[0] + worldY * vpMatrix[4] + worldZ * vpMatrix[8] + vpMatrix[12];
                    float clipY = worldX * vpMatrix[1] + worldY * vpMatrix[5] + worldZ * vpMatrix[9] + vpMatrix[13];

                    float ndcX = clipX / clipW;
                    float ndcY = clipY / clipW;

                    // Check if outside screen bounds [-1, 1]
                    if (ndcX < -1.0f || ndcX > 1.0f || ndcY < -1.0f || ndcY > 1.0f)
                    {
                        isNowVisible = false;
                    }
                }

                if (wasVisible != isNowVisible)
                {
                    tail.isVisible = isNowVisible;
                    UserInterface::Core::EventBus::GetInstance().Publish(
                        UserInterface::Core::TextTailVisibilityChangedEvent(tail.virtualId.value(), isNowVisible)
                    );
                }
            }
        }

        void RenderBatch() override
        {
            EterBase::ModernLogger::Trace("RenderBatch called (delegated via Events to UI).");
        }

        void ClearAll() override
        {
            m_tails.clear();
            EterBase::ModernLogger::Info("Cleared all text tails.");
        }

    private:
        std::unordered_map<uint32_t, TextTailInstance> m_tails;

        static void MultiplyMatrices(const float a[16], const float b[16], float result[16])
        {
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    result[i * 4 + j] = a[i * 4 + 0] * b[0 * 4 + j] +
                                        a[i * 4 + 1] * b[1 * 4 + j] +
                                        a[i * 4 + 2] * b[2 * 4 + j] +
                                        a[i * 4 + 3] * b[3 * 4 + j];
                }
            }
        }
    };

    // Factory function to prevent dead code and allow instantiation
    std::unique_ptr<ITextTailService> CreateTextTailOcclusionCuller()
    {
        return std::make_unique<TextTailOcclusionCullImpl>();
    }

} // namespace UserInterface::TextTail
