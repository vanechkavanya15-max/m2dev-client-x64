//#include "../StdAfx.h"
#include "RadixSort64.h"

void Sort64(RenderQueueEntry* entries, size_t count, RenderQueueEntry* tempBuffer) noexcept {
    RadixSort(entries, count, tempBuffer, [](const RenderQueueEntry& entry) { return entry.key; });
}

