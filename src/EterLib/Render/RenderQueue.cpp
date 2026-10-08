#include "RenderQueue.h"
#include <algorithm>

namespace EterLib::Render
{

void RenderQueue::Submit(RenderSortKey key, void* cmd, CommandType type)
{
    m_entries.push_back(RenderQueueEntry{key, cmd, type});
}

void RenderQueue::Sort() noexcept
{
    std::sort(m_entries.begin(), m_entries.end());
}

void RenderQueue::Clear() noexcept
{
    m_entries.clear();
}

size_t RenderQueue::GetEntryCount() const noexcept
{
    return m_entries.size();
}

std::span<const RenderQueueEntry> RenderQueue::GetEntries() const noexcept
{
    return m_entries;
}

} // namespace EterLib::Render

