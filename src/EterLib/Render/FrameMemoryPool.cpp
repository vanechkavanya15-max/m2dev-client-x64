#include "FrameMemoryPool.h"

namespace EterLib::Render
{
    // Jawne instancjacje FrameMemoryPool dla popularnych rozmiarow wezlow w architekturze RenderQueue
    template class FrameMemoryPool<uint64_t, 1024>;
    template class FrameMemoryPool<uint32_t, 1024>;
} // namespace EterLib::Render
