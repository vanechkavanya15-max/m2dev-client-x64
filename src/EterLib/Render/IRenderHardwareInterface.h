#pragma once

#include <cstdint>

namespace EterLib::Render
{
    class IRenderHardwareInterface
    {
    public:
        virtual ~IRenderHardwareInterface() = default;

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;
        virtual void SetViewport(int x, int y, int w, int h) = 0;
        virtual void Clear(uint32_t flags, uint32_t color, float depth) = 0;
    };
}

