#pragma once

#include "ITextTailService.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include <unordered_map>
#include <string>

namespace UserInterface::TextTail
{
    class TextTailService : public ITextTailService
    {
    public:
        static TextTailService& Instance()
        {
            static TextTailService s_instance;
            return s_instance;
        }

        EterBase::PacketResult<void> RegisterActorTail(const TextTailCreateData& data) override;

        EterBase::PacketResult<void> RegisterItemTail(uint32_t virtualId, std::string_view name) override
        {
            if (name.empty())
            {
                EterBase::ModernLogger::Error("TextTailService::RegisterItemTail - Brak nazwy przedmiotu dla VID {}!", virtualId);
                return std::unexpected(EterBase::PacketError::MalformedPayload);
            }
            EterBase::ModernLogger::Info("TextTailService::RegisterItemTail - Rejestracja przedmiotu VID: {}, nazwa: '{}'", virtualId, name);
            m_tails[virtualId] = ModernTailInfo{ virtualId, std::string(name) };
            return {};
        }

        void RemoveTail(uint32_t virtualId) override
        {
            m_tails.erase(virtualId);
            EterBase::ModernLogger::Debug("TextTailService::RemoveTail - Usunieto etykiete VID: {}", virtualId);
        }

        void UpdateScreenPositions(float viewMatrix[16], float projMatrix[16]) override
        {
            float viewProj[16];
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    viewProj[i * 4 + j] = 0.0f;
                    for (int k = 0; k < 4; ++k)
                    {
                        viewProj[i * 4 + j] += viewMatrix[i * 4 + k] * projMatrix[k * 4 + j];
                    }
                }
            }

            constexpr float SCREEN_WIDTH = 800.0f;
            constexpr float SCREEN_HEIGHT = 600.0f;
            constexpr float HALF_WIDTH = SCREEN_WIDTH * 0.5f;
            constexpr float HALF_HEIGHT = SCREEN_HEIGHT * 0.5f;

            for (auto& [vid, info] : m_tails)
            {
                float w = info.worldX * viewProj[3] + info.worldY * viewProj[7] + info.worldZ * viewProj[11] + viewProj[15];
                if (w > 0.01f)
                {
                    float clipX = info.worldX * viewProj[0] + info.worldY * viewProj[4] + info.worldZ * viewProj[8] + viewProj[12];
                    float clipY = info.worldX * viewProj[1] + info.worldY * viewProj[5] + info.worldZ * viewProj[9] + viewProj[13];
                    float clipZ = info.worldX * viewProj[2] + info.worldY * viewProj[6] + info.worldZ * viewProj[10] + viewProj[14];
                    float invW = 1.0f / w;
                    info.screenX = (clipX * invW + 1.0f) * HALF_WIDTH;
                    info.screenY = (1.0f - clipY * invW) * HALF_HEIGHT;
                    info.screenZ = clipZ * invW;
                    info.isVisible = (info.screenZ >= 0.0f && info.screenZ <= 1.0f);
                }
                else
                {
                    info.isVisible = false;
                }
            }
        }

        void RenderBatch() override
        {
        }

        void ClearAll() override
        {
            m_tails.clear();
            EterBase::ModernLogger::Debug("TextTailService::ClearAll - Wszystkie etykiety wyczyszczone.");
        }

        void SetTailPosition(uint32_t virtualId, float x, float y, float z)
        {
            auto it = m_tails.find(virtualId);
            if (it != m_tails.end())
            {
                it->second.worldX = x;
                it->second.worldY = y;
                it->second.worldZ = z;
            }
            else
            {
                ModernTailInfo info{};
                info.virtualId = virtualId;
                info.worldX = x;
                info.worldY = y;
                info.worldZ = z;
                m_tails[virtualId] = info;
            }
        }

        bool GetTailPosition(uint32_t virtualId, float* px, float* py, float* pz) const
        {
            auto it = m_tails.find(virtualId);
            if (it != m_tails.end())
            {
                if (px) *px = it->second.worldX;
                if (py) *py = it->second.worldY;
                if (pz) *pz = it->second.worldZ;
                return true;
            }
            return false;
        }

    private:
        struct ModernTailInfo
        {
            uint32_t virtualId{0};
            std::string text;
            float worldX{0.0f}, worldY{0.0f}, worldZ{0.0f};
            float screenX{0.0f}, screenY{0.0f}, screenZ{0.0f};
            bool isVisible{false};
        };

        std::unordered_map<uint32_t, ModernTailInfo> m_tails;
    };
}
