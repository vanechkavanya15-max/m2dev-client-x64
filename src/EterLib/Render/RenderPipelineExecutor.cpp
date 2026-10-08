#include "RenderPipelineExecutor.h"

namespace EterLib::Render {

    void RenderPipelineExecutor::ExecuteQueue(LPDIRECT3DDEVICE9 dev, RenderQueue& queue) noexcept {
        size_t count = queue.GetCount();
        if (count == 0) return;

        RenderEntry* entries = queue.GetEntries();
        if (!entries) return;

        RadixSort64::Sort(entries, count);

        uint8_t currentPass = 0xFF; // Invalid/Unset pass id

        for (size_t i = 0; i < count; ++i) {
            const auto& entry = entries[i];
            
            // Extract pass from key (assuming top 8 bits for pass id as typical in radix 64bit sort rendering)
            uint8_t passId = static_cast<uint8_t>((entry.key >> 56) & 0xFF);
            
            if (passId != currentPass) {
                PassDispatcher::Activate(passId);
                currentPass = passId;
            }

            RenderStateDeduplicator::Apply(dev);

            if (entry.cmd) {
                entry.cmd->Execute(dev);
            }
        }
    }

}

