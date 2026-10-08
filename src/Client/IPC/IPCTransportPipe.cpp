#include "IPCTransportPipe.h"
#include <EterBase/ModernLogger.h>
#include <iostream>
#include <cstring>
#include <limits>

#ifdef _WIN32
#include <windows.h>
#else
// Fallback mocks if somehow needed, though we include the mock windows.h during testing.
#ifndef ERROR_IO_INCOMPLETE
#define ERROR_IO_INCOMPLETE 996
#endif
#endif

namespace Client::IPC {

    IPCTransportPipe::IPCTransportPipe(const std::string& pipeName)
        : hPipe_(INVALID_HANDLE_VALUE),
          isConnected_(false),
          isConnecting_(false),
          isReading_(false),
          readBuffer_(65536),
          tempReadBuffer_(4096)
    {
        if (pipeName.empty()) {
            DWORD pid = GetCurrentProcessId();
            pipeName_ = "\\\\.\\pipe\\m2_client_ipc_" + std::to_string(pid);
        } else {
            pipeName_ = pipeName;
        }

        connectOverlapped_ = std::make_unique<OVERLAPPED>();
        std::memset(connectOverlapped_.get(), 0, sizeof(OVERLAPPED));
        connectOverlapped_->hEvent = CreateEventA(nullptr, TRUE, TRUE, nullptr);

        readOverlapped_ = std::make_unique<OVERLAPPED>();
        std::memset(readOverlapped_.get(), 0, sizeof(OVERLAPPED));
        readOverlapped_->hEvent = CreateEventA(nullptr, TRUE, TRUE, nullptr);
    }

    IPCTransportPipe::~IPCTransportPipe() {
        StopServer();
        if (connectOverlapped_->hEvent) {
            CloseHandle(connectOverlapped_->hEvent);
        }
        if (readOverlapped_->hEvent) {
            CloseHandle(readOverlapped_->hEvent);
        }
    }

    bool IPCTransportPipe::StartServer() {
        hPipe_ = CreateNamedPipeA(
            pipeName_.c_str(),
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1, // Max instances
            65536, // Out buffer size
            65536, // In buffer size
            0, // Default timeout
            nullptr // Security attributes
        );

        if (hPipe_ == INVALID_HANDLE_VALUE) {
            EterBase::ModernLogger::Error("Failed to create named pipe: {}", pipeName_);
            return false;
        }

        ListenForClient();
        return true;
    }

    void IPCTransportPipe::StopServer() {
        if (hPipe_ != INVALID_HANDLE_VALUE) {
            DisconnectNamedPipe(hPipe_);
            CloseHandle(hPipe_);
            hPipe_ = INVALID_HANDLE_VALUE;
        }
        isConnected_ = false;
        isConnecting_ = false;
        isReading_ = false;
    }

    void IPCTransportPipe::ListenForClient() {
        isConnecting_ = true;
        BOOL result = ConnectNamedPipe(hPipe_, connectOverlapped_.get());
        if (result) {
            // Highly unlikely but connected synchronously
            isConnecting_ = false;
            isConnected_ = true;
            EterBase::ModernLogger::Info("Client connected to named pipe (sync)");
        } else {
            DWORD error = GetLastError();
            if (error == ERROR_IO_PENDING) {
                // Pending async connection
            } else if (error == ERROR_PIPE_CONNECTED) {
                // Already connected
                isConnecting_ = false;
                isConnected_ = true;
                EterBase::ModernLogger::Info("Client connected to named pipe (already connected)");
            } else {
                EterBase::ModernLogger::Error("Failed to connect named pipe, error: {}", error);
                isConnecting_ = false;
            }
        }
    }

    void IPCTransportPipe::DisconnectClient() {
        if (isConnected_) {
            DisconnectNamedPipe(hPipe_);
            isConnected_ = false;
            isReading_ = false;
            readBuffer_.Clear();
            EterBase::ModernLogger::Info("Client disconnected from named pipe, waiting for new client");
            ListenForClient();
        }
    }

    void IPCTransportPipe::PollEvents() {
        if (hPipe_ == INVALID_HANDLE_VALUE) {
            return;
        }

        if (isConnecting_) {
            DWORD bytesTransferred = 0;
            if (GetOverlappedResult(hPipe_, connectOverlapped_.get(), &bytesTransferred, FALSE)) {
                isConnecting_ = false;
                isConnected_ = true;
                EterBase::ModernLogger::Info("Client connected to named pipe (async)");
            } else {
                DWORD error = GetLastError();
                if (error != ERROR_IO_INCOMPLETE && error != ERROR_IO_PENDING) {
                    // An error occurred during connecting
                    EterBase::ModernLogger::Error("Error during wait for client connection: {}", error);
                    DisconnectNamedPipe(hPipe_);
                    isConnecting_ = false;
                    ListenForClient();
                }
            }
        }

        if (isConnected_) {
            if (!isReading_) {
                DWORD bytesRead = 0;
                BOOL result = ReadFile(hPipe_, tempReadBuffer_.data(), tempReadBuffer_.size(), &bytesRead, readOverlapped_.get());
                if (result) {
                    // Synchronous read completed immediately
                    if (bytesRead > 0) {
                        readBuffer_.Write(std::span<const uint8_t>(tempReadBuffer_.data(), bytesRead));
                        ProcessReadData();
                    } else {
                        // EOF
                        DisconnectClient();
                    }
                } else {
                    DWORD error = GetLastError();
                    if (error == ERROR_IO_PENDING) {
                        isReading_ = true;
                    } else if (error == ERROR_BROKEN_PIPE) {
                        DisconnectClient();
                    } else {
                        EterBase::ModernLogger::Error("ReadFile failed with error: {}", error);
                        DisconnectClient();
                    }
                }
            } else {
                DWORD bytesTransferred = 0;
                if (GetOverlappedResult(hPipe_, readOverlapped_.get(), &bytesTransferred, FALSE)) {
                    isReading_ = false;
                    if (bytesTransferred > 0) {
                        readBuffer_.Write(std::span<const uint8_t>(tempReadBuffer_.data(), bytesTransferred));
                        ProcessReadData();
                    } else {
                        DisconnectClient();
                    }
                } else {
                    DWORD error = GetLastError();
                    if (error == ERROR_BROKEN_PIPE) {
                        DisconnectClient();
                    } else if (error != ERROR_IO_INCOMPLETE && error != ERROR_IO_PENDING) {
                        EterBase::ModernLogger::Error("GetOverlappedResult for ReadFile failed with error: {}", error);
                        DisconnectClient();
                    }
                }
            }
        }
    }

    void IPCTransportPipe::ProcessReadData() {
        while (readBuffer_.GetSize() >= kHeaderSize) {
            uint32_t frameSize = 0;
            readBuffer_.Peek(std::span<uint8_t>(reinterpret_cast<uint8_t*>(&frameSize), kHeaderSize));
            
            // Protect against integer overflow when adding kHeaderSize and frameSize
            if (frameSize > std::numeric_limits<size_t>::max() - kHeaderSize) {
                // If frameSize is suspiciously large, we might consider closing the connection
                // For safety, clear buffer and disconnect client
                EterBase::ModernLogger::Error("Malicious or malformed frame size detected, disconnecting.");
                DisconnectClient();
                return;
            }

            if (readBuffer_.GetSize() >= kHeaderSize + frameSize) {
                // Limit maximum allocation to prevent DOS (optional, here we cap at something reasonable like 64MB)
                if (frameSize > 64 * 1024 * 1024) {
                    EterBase::ModernLogger::Error("Frame size exceeds arbitrary max cap, disconnecting.");
                    DisconnectClient();
                    return;
                }

                std::vector<uint8_t> frameData(frameSize);
                readBuffer_.Skip(kHeaderSize);
                readBuffer_.Read(std::span<uint8_t>(frameData.data(), frameSize));
                
                if (onFrameReceived_) {
                    onFrameReceived_(frameData);
                }
            } else {
                // Not enough data for full frame yet
                break;
            }
        }
    }

    bool IPCTransportPipe::SendResponse(std::span<const uint8_t> bytes) {
        if (!isConnected_ || hPipe_ == INVALID_HANDLE_VALUE) {
            return false;
        }

        // We prefix with frame size (4 bytes)
        uint32_t frameSize = static_cast<uint32_t>(bytes.size());
        
        std::vector<uint8_t> packet(kHeaderSize + bytes.size());
        std::memcpy(packet.data(), &frameSize, kHeaderSize);
        std::memcpy(packet.data() + kHeaderSize, bytes.data(), bytes.size());

        // For simplicity and matching requirements, we can use a blocking WriteFile or synchronous OVERLAPPED write
        OVERLAPPED writeOverlapped = {0};
        writeOverlapped.hEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
        
        DWORD bytesWritten = 0;
        BOOL result = WriteFile(hPipe_, packet.data(), packet.size(), &bytesWritten, &writeOverlapped);
        if (!result) {
            DWORD error = GetLastError();
            if (error == ERROR_IO_PENDING) {
                // Wait for completion
                if (!GetOverlappedResult(hPipe_, &writeOverlapped, &bytesWritten, TRUE)) {
                    CloseHandle(writeOverlapped.hEvent);
                    DisconnectClient();
                    return false;
                }
            } else {
                CloseHandle(writeOverlapped.hEvent);
                DisconnectClient();
                return false;
            }
        }
        
        CloseHandle(writeOverlapped.hEvent);
        return true;
    }

} // namespace Client::IPC
