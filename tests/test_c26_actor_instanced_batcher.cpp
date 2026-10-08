
#include "src/EterLib/Render/ActorInstancedBatcher.h"
#include "src/EterLib/Render/RenderQueue.h"
#include "src/EterLib/Render/LinearFrameAllocator.h"
#include "src/EterBase/LogModern.h"
#include <iostream>

using namespace EterLib::Render;

int main()
{
    EterBase::ModernLogger::Info("Starting ActorInstancedBatcher tests...");

    ActorInstancedBatcher batcher;
    RenderQueue queue;
    LinearFrameAllocator allocator(LinearFrameAllocator::DEFAULT_CAPACITY);

    // Group actors: 50 for modelId 101, 20 for modelId 102
    D3DMATRIX dummyWorld = {};
    for(int i = 0; i < 50; ++i)
        batcher.AddActorInstance(101, dummyWorld, 0xFFFFFFFF);
    
    for(int i = 0; i < 20; ++i)
        batcher.AddActorInstance(102, dummyWorld, 0xFF000000);

    // Flush should group them into exactly 2 commands
    batcher.Flush(queue, allocator);

    size_t count = queue.GetEntryCount();
    if (count != 2)
    {
        EterBase::ModernLogger::Error("Test failed! Expected 2 queue entries, got {}", count);
        return 1;
    }

    // Verify clearing works
    batcher.Clear();
    batcher.Flush(queue, allocator);
    // Queue is not cleared by batcher, but batcher shouldn't submit anything new
    if (queue.GetEntryCount() != 2)
    {
        EterBase::ModernLogger::Error("Test failed! Expected queue count to remain 2 after clearing batcher, got {}", queue.GetEntryCount());
        return 1;
    }

    EterBase::ModernLogger::Info("ActorInstancedBatcher tests passed successfully!");
    return 0;
}

