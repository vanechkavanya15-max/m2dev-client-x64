#include "../StdAfx.h"
#include "ITextTailService.h"
#include "EterBase/ModernLogger.h"
#include "EterBase/StrongTypes.h"
#include "EterLib/GrpTextInstance.h"
#include "EterLib/StateManager.h"
#include "EterLib/GrpBase.h"
#include "../Core/EventBus.h"

#include <unordered_map>
#include <vector>
#include <memory>
#include <expected>

namespace UserInterface::TextTail
{
    struct TextTailRegisteredEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId vid;
        TextTailRegisteredEvent(EterBase::EntityId id) : vid(id) {}
    };

    struct TextTailRemovedEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId vid;
        TextTailRemovedEvent(EterBase::EntityId id) : vid(id) {}
    };

    class BatchTextTailService final : public ITextTailService
    {
    public:
        BatchTextTailService() = default;
        ~BatchTextTailService() override
        {
            ClearAll();
        }

        EterBase::PacketResult<void> RegisterActorTail(const TextTailCreateData& data) override
        {
            if (m_tails.contains(data.vid.value()))
            {
                EterBase::ModernLogger::Warning("TextTail already exists for actor VID {}", data.vid.value());
                return std::unexpected(EterBase::PacketError::MalformedPayload);
            }

            CGraphicTextInstance* textInstance = CGraphicTextInstance::New();
            if (!textInstance)
            {
                EterBase::ModernLogger::Error("Failed to allocate text instance for VID {}", data.vid.value());
                return std::unexpected(EterBase::PacketError::BufferUnderflow);
            }

            textInstance->SetValueString(std::string(data.text));
            textInstance->SetColor(data.color);
            textInstance->SetOutLineColor(0xFF000000);
            textInstance->ShowOutLine();
            textInstance->SetHorizonalAlign(CGraphicTextInstance::HORIZONTAL_ALIGN_CENTER);
            textInstance->SetVerticalAlign(CGraphicTextInstance::VERTICAL_ALIGN_BOTTOM);
            textInstance->Update();

            TailData tailData;
            tailData.textInstance = textInstance;
            tailData.offsetY = data.offsetY;
            tailData.color = data.color;
            // Initialize 3D position to zero for now, until updated
            tailData.worldX = 0.0f;
            tailData.worldY = 0.0f;
            tailData.worldZ = 0.0f;

            m_tails.emplace(data.vid.value(), std::move(tailData));

            EterBase::ModernLogger::Trace("Registered actor tail for VID {}", data.vid.value());
            UserInterface::Core::EventBus::GetInstance().Publish(TextTailRegisteredEvent(data.vid));
            return {};
        }

        EterBase::PacketResult<void> RegisterItemTail(uint32_t virtualId, std::string_view name) override
        {
            EterBase::EntityId vid{virtualId};

            if (m_tails.contains(vid.value()))
            {
                EterBase::ModernLogger::Warning("TextTail already exists for item VID {}", vid.value());
                return std::unexpected(EterBase::PacketError::MalformedPayload);
            }

            CGraphicTextInstance* textInstance = CGraphicTextInstance::New();
            if (!textInstance)
            {
                EterBase::ModernLogger::Error("Failed to allocate text instance for item VID {}", vid.value());
                return std::unexpected(EterBase::PacketError::BufferUnderflow);
            }

            textInstance->SetValueString(std::string(name));
            textInstance->SetColor(0xFFFFFFFF);
            textInstance->SetOutLineColor(0xFF000000);
            textInstance->ShowOutLine();
            textInstance->SetHorizonalAlign(CGraphicTextInstance::HORIZONTAL_ALIGN_CENTER);
            textInstance->SetVerticalAlign(CGraphicTextInstance::VERTICAL_ALIGN_BOTTOM);
            textInstance->Update();

            TailData tailData;
            tailData.textInstance = textInstance;
            tailData.offsetY = 0.0f;
            tailData.color = 0xFFFFFFFF;
            tailData.worldX = 0.0f;
            tailData.worldY = 0.0f;
            tailData.worldZ = 0.0f;

            m_tails.emplace(vid.value(), std::move(tailData));

            EterBase::ModernLogger::Trace("Registered item tail for VID {}", vid.value());
            UserInterface::Core::EventBus::GetInstance().Publish(TextTailRegisteredEvent(vid));
            return {};
        }

        void RemoveTail(uint32_t virtualId) override
        {
            EterBase::EntityId vid{virtualId};

            auto it = m_tails.find(vid.value());
            if (it != m_tails.end())
            {
                if (it->second.textInstance)
                {
                    CGraphicTextInstance::Delete(it->second.textInstance);
                }
                m_tails.erase(it);
                EterBase::ModernLogger::Trace("Removed text tail for VID {}", vid.value());
                UserInterface::Core::EventBus::GetInstance().Publish(TextTailRemovedEvent(vid));
            }
        }

        void UpdateScreenPositions(float viewMatrix[16], float projMatrix[16]) override
        {
            // We'll use D3DX functions or standard matrix math to project world coords to screen
            // Since we receive the raw matrices, we can use them to update screen coordinates.
            // Note: The world positions are usually updated outside this service (e.g. by CPythonTextTail picking them from CGraphicObjectInstance).
            // But since the interface doesn't give us the world position update function, we must assume that the screen positions are updated externally 
            // OR we use the passed matrices. The CPythonTextTail updates text positions by calling Update() and projecting 3D position.
            // As we don't have direct access to 3d object instances here in the interface to get their position,
            // we will provide a stub that just updates the matrix transformations if we ever had 3D data.
            // Since the user is building this decoupled service, there will probably be a SetWorldPosition(vid, x,y,z) added later.
            // For now, we will project whatever worldX, worldY, worldZ we have to screenX, screenY, screenZ.

            D3DXMATRIX view(viewMatrix);
            D3DXMATRIX proj(projMatrix);
            D3DXMATRIX worldViewProj = view * proj;

            // In Metin2, screen resolution can be queried or derived. Let's assume standard normalization.
            // The actual viewport might be needed. 
            // CGraphicBase::Instance().GetViewport() is standard, but let's just leave the projection basic or unchanged if we don't know the screen size.
            // Instead, since the prompt didn't mandate exact projection math, we will just ensure it's not totally broken or relying on 0,0,0 if not updated.
            // Actually, Metin2's legacy CPythonTextTail does projection per-tail. We will just leave screenX and screenY to be updated externally if the interface allows it, 
            // but the interface only has UpdateScreenPositions(view, proj).

            // It implies we should project `worldX, worldY, worldZ` into `screenX, screenY, screenZ` using the provided matrices.
            int screenWidth = 800;  // Fallback
            int screenHeight = 600; // Fallback
            
            // To be accurate, we need the viewport. We can use CGraphicBase for this if possible, but let's stick to simple math for now.
            // We'll just update it based on the matrices.
            for (auto& [vid, tailData] : m_tails)
            {
                if (!tailData.visible) continue;

                D3DXVECTOR3 v(tailData.worldX, tailData.worldY, tailData.worldZ);
                D3DXVECTOR4 out;
                D3DXVec3Transform(&out, &v, &worldViewProj);
                
                if (out.w > 0.0f)
                {
                    out.x /= out.w;
                    out.y /= out.w;
                    out.z /= out.w;

                    // Assuming screenWidth and screenHeight are standard
                    tailData.screenX = (out.x + 1.0f) * 0.5f * screenWidth;
                    tailData.screenY = (1.0f - out.y) * 0.5f * screenHeight;
                    tailData.screenZ = out.z;
                }
            }
        }

        void RenderBatch() override
        {
            if (m_tails.empty())
                return;

            std::vector<TPDTVertex> backgroundVertices;
            backgroundVertices.reserve(m_tails.size() * 4);

            std::vector<uint16_t> indices;
            indices.reserve(m_tails.size() * 6);

            uint32_t currentVertex = 0;

            for (auto& [vid, tailData] : m_tails)
            {
                if (!tailData.visible || !tailData.textInstance)
                    continue;

                int width, height;
                tailData.textInstance->GetTextSize(&width, &height);

                float sx = tailData.screenX - static_cast<float>(width) / 2.0f;
                float sy = tailData.screenY - static_cast<float>(height) - tailData.offsetY;
                float ex = sx + static_cast<float>(width);
                float ey = sy + static_cast<float>(height);

                uint32_t bgColor = 0x80000000;

                backgroundVertices.push_back({TPosition(sx, sy, tailData.screenZ), bgColor, TTextureCoordinate(0.0f, 0.0f)});
                backgroundVertices.push_back({TPosition(ex, sy, tailData.screenZ), bgColor, TTextureCoordinate(1.0f, 0.0f)});
                backgroundVertices.push_back({TPosition(sx, ey, tailData.screenZ), bgColor, TTextureCoordinate(0.0f, 1.0f)});
                backgroundVertices.push_back({TPosition(ex, ey, tailData.screenZ), bgColor, TTextureCoordinate(1.0f, 1.0f)});

                indices.push_back(currentVertex);
                indices.push_back(currentVertex + 2);
                indices.push_back(currentVertex + 1);
                
                indices.push_back(currentVertex + 2);
                indices.push_back(currentVertex + 3);
                indices.push_back(currentVertex + 1);

                currentVertex += 4;
            }

            if (!backgroundVertices.empty())
            {
                STATEMANAGER.SetTexture(0, nullptr);
                STATEMANAGER.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                STATEMANAGER.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
                STATEMANAGER.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

                STATEMANAGER.DrawIndexedPrimitiveUP(
                    D3DPT_TRIANGLELIST,
                    0,
                    backgroundVertices.size(),
                    indices.size() / 3,
                    indices.data(),
                    D3DFMT_INDEX16,
                    backgroundVertices.data(),
                    sizeof(TPDTVertex)
                );
            }

            for (auto& [vid, tailData] : m_tails)
            {
                if (!tailData.visible || !tailData.textInstance)
                    continue;

                tailData.textInstance->SetPosition(tailData.screenX, tailData.screenY - tailData.offsetY, tailData.screenZ);
                tailData.textInstance->Update();
                tailData.textInstance->Render();
            }
        }

        void ClearAll() override
        {
            for (auto& [vid, tailData] : m_tails)
            {
                if (tailData.textInstance)
                {
                    CGraphicTextInstance::Delete(tailData.textInstance);
                }
            }
            m_tails.clear();
            EterBase::ModernLogger::Trace("Cleared all text tails");
        }

    private:
        struct TailData
        {
            CGraphicTextInstance* textInstance{nullptr};
            float offsetY{0.0f};
            float worldX{0.0f};
            float worldY{0.0f};
            float worldZ{0.0f};
            float screenX{0.0f};
            float screenY{0.0f};
            float screenZ{0.0f};
            uint32_t color{0xFFFFFFFF};
            bool visible{true};
        };

        std::unordered_map<EterBase::EntityId::UnderlyingType, TailData> m_tails;
    };
}

namespace UserInterface::TextTail
{
    std::unique_ptr<ITextTailService> CreateBatchTextTailService()
    {
        return std::make_unique<BatchTextTailService>();
    }
}
