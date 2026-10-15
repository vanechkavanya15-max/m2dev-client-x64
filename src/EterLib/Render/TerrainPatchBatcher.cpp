#include "TerrainPatchBatcher.h"
#include "RenderQueue.h"
#include "LinearFrameAllocator.h"
#include "DrawIndexedCommand.h"

namespace EterLib::Render
{
    void TerrainPatchBatcher::SubmitPatches(std::span<const uint32_t> visibleNodeIds, RenderQueue& targetQueue, LinearFrameAllocator& allocator)
    {
        for (const auto nodeId : visibleNodeIds)
        {
            auto* cmd = allocator.AllocateObject<DrawIndexedCommand>();
            if (!cmd)
            {
                continue;
            }

            // Populate command with dummy default values suitable for testing/future terrain render payload
            cmd->primitiveType = D3DPT_TRIANGLELIST;
            cmd->baseVertexIndex = 0;
            cmd->minVertexIndex = 0;
            cmd->numVertices = 0; 
            cmd->startIndex = 0;
            cmd->primitiveCount = 0;
            cmd->vertexBuffer = nullptr;
            cmd->indexBuffer = nullptr;
            cmd->stride = 0;

            // Build 64-bit sort key for Terrain
            // Depth(12) | Material(8) | Texture(20) | Shader(12) | Viewport(4) | Pass(8 MSB)
            // Note: Since we are not including RenderSortKey.h directly because RenderQueue.h defines a simple struct,
            // we calculate the `value` manually matching the bitfield layout from RenderSortKey.h:
            // bit 56..63: Pass
            // bit 52..55: Viewport
            // bit 40..51: Shader
            // bit 20..39: Texture
            // bit 12..19: Material
            // bit  0..11: Depth
            
            uint64_t passVal = static_cast<uint64_t>(Pass::Opaque);
            uint64_t shaderVal = static_cast<uint64_t>(TERRAIN_SHADER_ID);
            uint64_t depthVal = static_cast<uint64_t>(nodeId) & 0xFFFULL;

            uint64_t keyValue = (passVal << 56) | (shaderVal << 40) | depthVal;
            
            RenderSortKey sortKey;
            sortKey.value = keyValue;

            targetQueue.Submit(sortKey, cmd, CommandType::Draw);
        }
    }
} // namespace EterLib::Render

