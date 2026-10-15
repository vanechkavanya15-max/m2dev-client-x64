#pragma once

#include "../StdAfx.h"
#include <vector>
#include <functional>

namespace EterLib::Render {

struct AlphaBlendItem {
    float depth;
    std::function<void()> renderCommand;
};

class AlphaBlendPassDispatcher {
public:
    AlphaBlendPassDispatcher() noexcept = default;
    ~AlphaBlendPassDispatcher() noexcept = default;

    void BeginPass(LPDIRECT3DDEVICE9 dev) noexcept;
    void EndPass(LPDIRECT3DDEVICE9 dev) noexcept;

    void Dispatch(float depth, std::function<void()> renderCommand);
    void Flush();

private:
    DWORD m_savedAlphaBlendEnable{0};
    DWORD m_savedSrcBlend{0};
    DWORD m_savedDestBlend{0};
    DWORD m_savedZWriteEnable{0};

    std::vector<AlphaBlendItem> m_items;
};

} // namespace EterLib::Render

