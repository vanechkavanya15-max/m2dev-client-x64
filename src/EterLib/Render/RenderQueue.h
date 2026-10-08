#pragma once

#include <cstdint>
#include <span>
#include <vector>
#include "RenderSortKey.h"

namespace EterLib::Render
{

enum class Pass : uint8_t
{
    DepthPrepass,
    Opaque,
    AlphaTest,
    Transparent,
    Additive,
    UI
};

enum class CommandType : uint8_t
{
    Draw,
    StateChange,
    Compute
};

struct RenderQueueEntry
{
    RenderSortKey sortKey;
    void* commandPtr;
    CommandType type;

    constexpr bool operator<(const RenderQueueEntry& other) const noexcept
    {
        return sortKey < other.sortKey;
    }
};

class RenderQueue
{
public:
    RenderQueue() = default;
    ~RenderQueue() = default;

    RenderQueue(const RenderQueue&) = delete;
    RenderQueue& operator=(const RenderQueue&) = delete;
    RenderQueue(RenderQueue&&) = default;
    RenderQueue& operator=(RenderQueue&&) = default;

    void Submit(RenderSortKey key, void* cmd, CommandType type);
    void Sort() noexcept;
    void Clear() noexcept;

    size_t GetEntryCount() const noexcept;
    std::span<const RenderQueueEntry> GetEntries() const noexcept;

private:
    std::vector<RenderQueueEntry> m_entries;
};

} // namespace EterLib::Render
