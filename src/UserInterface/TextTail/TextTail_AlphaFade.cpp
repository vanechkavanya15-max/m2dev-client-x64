#include "../StdAfx.h"
#include "ITextTailService.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <string>

namespace UserInterface::TextTail
{
    struct TextTailAlphaUpdatedEvent : public Core::IEvent
    {
        EterBase::EntityId virtualId;
        uint32_t color;
        float alpha;

        TextTailAlphaUpdatedEvent(EterBase::EntityId vid, uint32_t c, float a)
            : virtualId(vid), color(c), alpha(a) {}
    };

    class AlphaFadeTextTailService final : public ITextTailService
    {
    public:
        AlphaFadeTextTailService() = default;
        ~AlphaFadeTextTailService() override = default;

        EterBase::PacketResult<void> RegisterActorTail(const TextTailCreateData& data) override
        {
            if (!static_cast<bool>(data.vid))
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);

            if (m_tails.contains(data.vid))
            {
                EterBase::ModernLogger::Warn("ActorTail already registered for VID: {}", data.vid.value());
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            TailEntry entry{
                .text = std::string(data.text),
                .baseColor = data.color,
                .currentColor = data.color,
                .offsetY = data.offsetY,
                .isItem = false,
                .x = 0.0f,
                .y = 0.0f,
                .z = 0.0f
            };
            m_tails.emplace(data.vid, std::move(entry));
            EterBase::ModernLogger::Info("Registered ActorTail for VID: {}", data.vid.value());
            return {};
        }

        EterBase::PacketResult<void> RegisterItemTail(uint32_t virtualId, std::string_view name) override
        {
            EterBase::EntityId itemVid{virtualId};

            if (m_tails.contains(itemVid))
            {
                EterBase::ModernLogger::Warn("ItemTail already registered for ID: {}", virtualId);
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            TailEntry entry{
                .text = std::string(name),
                .baseColor = 0xFFFFFFFF,
                .currentColor = 0xFFFFFFFF,
                .offsetY = 0.0f,
                .isItem = true,
                .x = 0.0f,
                .y = 0.0f,
                .z = 0.0f
            };
            m_tails.emplace(itemVid, std::move(entry));
            EterBase::ModernLogger::Info("Registered ItemTail for ID: {}", virtualId);
            return {};
        }

        void RemoveTail(uint32_t virtualId) override
        {
            EterBase::EntityId idToRemove{virtualId};
            if (m_tails.erase(idToRemove))
            {
                EterBase::ModernLogger::Info("Removed Tail for ID: {}", virtualId);
            }
        }

        void UpdateScreenPositions(float viewMatrix[16], float projMatrix[16]) override
        {
            // Calculate distance based on matrix translation components.
            // Since we don't have access to global player coordinates directly in this service,
            // we approximate the distance based on distance from the camera (view matrix).
            // This is standard for screen-space text tails.

            // The camera position can be extracted from the inverted view matrix,
            // but for simplicity, we assume distance to camera Z plane or similar.
            // Realistically, distance should be calculated using the entity's 3D position
            // stored in m_tails, transformed by viewMatrix.

            for (auto& [vid, tail] : m_tails)
            {
                // Transform the 3D coordinate (x,y,z) by the view matrix to get view-space Z
                // This gives us the distance from the camera.
                // Assuming viewMatrix is column-major:
                // z_view = x * m02 + y * m12 + z * m22 + m32
                float z_view = tail.x * viewMatrix[2] + tail.y * viewMatrix[6] + tail.z * viewMatrix[10] + viewMatrix[14];
                
                float distance = std::abs(z_view);

                float alpha = CalculateAlpha(distance);
                uint32_t updatedColor = ApplyAlpha(tail.baseColor, alpha);

                if (updatedColor != tail.currentColor)
                {
                    tail.currentColor = updatedColor;
                    Core::EventBus::GetInstance().Publish(TextTailAlphaUpdatedEvent(vid, tail.currentColor, alpha));
                }
            }
        }

        void RenderBatch() override
        {
            // Render logic placeholder
        }

        void ClearAll() override
        {
            m_tails.clear();
            EterBase::ModernLogger::Info("Cleared all TextTails");
        }

        void SetEntityPosition(EterBase::EntityId vid, float x, float y, float z)
        {
            auto it = m_tails.find(vid);
            if (it != m_tails.end())
            {
                it->second.x = x;
                it->second.y = y;
                it->second.z = z;
            }
        }

    private:
        struct TailEntry
        {
            std::string text;
            uint32_t baseColor;
            uint32_t currentColor;
            float offsetY;
            bool isItem;
            float x, y, z;
        };

        std::unordered_map<EterBase::EntityId, TailEntry> m_tails;

        static constexpr float FADE_START_DIST = 2000.0f;
        static constexpr float FADE_END_DIST = 4000.0f;

        float CalculateAlpha(float distance) const
        {
            if (distance <= FADE_START_DIST) return 1.0f;
            if (distance >= FADE_END_DIST) return 0.0f;
            return 1.0f - ((distance - FADE_START_DIST) / (FADE_END_DIST - FADE_START_DIST));
        }

        uint32_t ApplyAlpha(uint32_t color, float alpha) const
        {
            alpha = std::clamp(alpha, 0.0f, 1.0f);
            uint8_t a = static_cast<uint8_t>(((color >> 24) & 0xFF) * alpha);
            uint8_t r = (color >> 16) & 0xFF;
            uint8_t g = (color >> 8) & 0xFF;
            uint8_t b = color & 0xFF;
            return (a << 24) | (r << 16) | (g << 8) | b;
        }
    };

    // Factory method to allow instantiation without exposing the class definition
    std::unique_ptr<ITextTailService> CreateAlphaFadeTextTailService()
    {
        return std::make_unique<AlphaFadeTextTailService>();
    }
}
