#include "../StdAfx.h"
#include "ITextTailService.h"
#include <algorithm>

namespace UserInterface::TextTail
{
    struct ScreenBounds
    {
        float minX{10.0f};
        float minY{20.0f};
        float maxX{1910.0f};
        float maxY{1060.0f};
    };

    void ClampTextTailPosition(float& screenX, float& screenY, const ScreenBounds& bounds) noexcept
    {
        screenX = std::clamp(screenX, bounds.minX, bounds.maxX);
        screenY = std::clamp(screenY, bounds.minY, bounds.maxY);
    }
}
