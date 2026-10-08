#pragma once

#include <string>
#include <vector>
#include <span>
#include <functional>
#include <memory>
#include <EterBase/RingBuffer.h>

#ifdef _WIN32
#include <windows.h>
#else
// Forward declarations for mocked types if we want to compile on Linux without #include <windows.h> everywhere
typedef void* HANDLE;
struct OVERLAPPED;
#endif

namespace Client::IPC {

    class IPCTransportPipe {
    public:
        using FrameReceivedCallback = std::function<void(std::span<const uint8_t>)>;

        explicit IPCTransportPipe(const std::string& pipeName = "");
        ~IPCTransportPipe();

        bool StartServer();
        void StopServer();
        void PollEvents();
        bool SendResponse(std::span<const uint8_t> bytes);

        void SetOnFrameReceived(FrameReceivedCallback callback) {
            onFrameReceived_ = std::move(callback);
        }

    private:
        void DisconnectClient();
        void ListenForClient();
        void ProcessReadData();

        std::string pipeName_;
        HANDLE hPipe_;
        std::unique_ptr<OVERLAPPED> connectOverlapped_;
        std::unique_ptr<OVERLAPPED> readOverlapped_;
        
        bool isConnected_;
        bool isConnecting_;
        bool isReading_;

        EterBase::RingBuffer readBuffer_;
        std::vector<uint8_t> tempReadBuffer_;
        FrameReceivedCallback onFrameReceived_;

        // We assume 4 bytes length header for frames
        static constexpr size_t kHeaderSize = 4;
    };

} // namespace Client::IPC
