#include "../StdAfx.h"
#include "ITextTailService.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "UserInterface/Core/EventBus.h"
#include <unordered_map>
#include <string>
#include <vector>

namespace UserInterface::TextTail
{
    struct TextTailPositionUpdateEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId vid;
        float screenX;
        float screenY;
        float screenZ;
        bool visible;
        
        TextTailPositionUpdateEvent(EterBase::EntityId v, float sx, float sy, float sz, bool vis)
            : vid(v), screenX(sx), screenY(sy), screenZ(sz), visible(vis) {}
    };

    struct ScreenProjectionState
    {
        float x{0.0f};
        float y{0.0f};
        float z{0.0f};
        float sx{0.0f};
        float sy{0.0f};
        float sz{0.0f};
        bool visible{false};
    };

    class TextTailScreenProjection final : public ITextTailService
    {
    public:
        TextTailScreenProjection() = default;
        ~TextTailScreenProjection() override = default;

        EterBase::PacketResult<void> RegisterActorTail(const TextTailCreateData& data) override
        {
            if (m_tails.contains(data.vid)) {
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }
            m_tails[data.vid] = ScreenProjectionState{};
            return {};
        }

        EterBase::PacketResult<void> RegisterItemTail(uint32_t virtualId, std::string_view name) override
        {
            EterBase::EntityId vid(virtualId);
            if (m_tails.contains(vid)) {
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }
            m_tails[vid] = ScreenProjectionState{};
            return {};
        }

        void RemoveTail(uint32_t virtualId) override
        {
            m_tails.erase(EterBase::EntityId(virtualId));
        }

        void UpdateScreenPositions(float viewMatrix[16], float projMatrix[16]) override
        {
            float viewProj[16];
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    viewProj[i * 4 + j] = 0.0f;
                    for (int k = 0; k < 4; ++k) {
                        viewProj[i * 4 + j] += viewMatrix[i * 4 + k] * projMatrix[k * 4 + j];
                    }
                }
            }

            constexpr float SCREEN_WIDTH = 800.0f;
            constexpr float SCREEN_HEIGHT = 600.0f;
            constexpr float HALF_WIDTH = SCREEN_WIDTH * 0.5f;
            constexpr float HALF_HEIGHT = SCREEN_HEIGHT * 0.5f;

            for (auto& [id, state] : m_tails) {
                float w = state.x * viewProj[3] + state.y * viewProj[7] + state.z * viewProj[11] + viewProj[15];

                if (w > 0.01f) {
                    float clipX = state.x * viewProj[0] + state.y * viewProj[4] + state.z * viewProj[8] + viewProj[12];
                    float clipY = state.x * viewProj[1] + state.y * viewProj[5] + state.z * viewProj[9] + viewProj[13];
                    float clipZ = state.x * viewProj[2] + state.y * viewProj[6] + state.z * viewProj[10] + viewProj[14];

                    float invW = 1.0f / w;
                    state.sx = (clipX * invW + 1.0f) * HALF_WIDTH;
                    state.sy = (1.0f - clipY * invW) * HALF_HEIGHT;
                    state.sz = clipZ * invW;
                    state.visible = (state.sz >= 0.0f && state.sz <= 1.0f);
                } else {
                    state.visible = false;
                }

                UserInterface::Core::EventBus::GetInstance().Publish(
                    TextTailPositionUpdateEvent(id, state.sx, state.sy, state.sz, state.visible)
                );
            }
        }

        void RenderBatch() override
        {
            EterBase::ModernLogger::Info("RenderBatch - tails count: {}", m_tails.size());
        }

        void ClearAll() override
        {
            m_tails.clear();
        }

    private:
        std::unordered_map<EterBase::EntityId, ScreenProjectionState> m_tails;
    };
}
