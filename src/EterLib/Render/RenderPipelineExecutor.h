#pragma once

#include <cstdint>
#include <cstddef>

#ifndef _WIN32
struct IDirect3DDevice9 {};
using LPDIRECT3DDEVICE9 = IDirect3DDevice9*;
#else
#include <windows.h>
#include <d3d9.h>
#endif

namespace EterLib::Render {

    struct RenderCommand {
        virtual ~RenderCommand() = default;
        virtual void Execute(LPDIRECT3DDEVICE9 dev) = 0;
    };

    struct RenderEntry {
        uint64_t key;
        RenderCommand* cmd;
    };

    class RenderQueue {
    public:
        virtual ~RenderQueue() = default;
        virtual RenderEntry* GetEntries() = 0;
        virtual size_t GetCount() const = 0;
    };

    class RadixSort64 {
    public:
        static void Sort(RenderEntry* entries, size_t count);
    };

    class PassDispatcher {
    public:
        static void Activate(uint8_t passId);
    };

    class RenderStateDeduplicator {
    public:
        static void Apply(LPDIRECT3DDEVICE9 dev);
    };

    class RenderPipelineExecutor {
    public:
        RenderPipelineExecutor() = default;
        ~RenderPipelineExecutor() = default;

        void ExecuteQueue(LPDIRECT3DDEVICE9 dev, RenderQueue& queue) noexcept;
    };

}

