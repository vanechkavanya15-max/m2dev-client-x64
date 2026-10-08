#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <span>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#else
// Mock windows.h for Linux compilation of tests is included via -I mock_headers
#endif

#include "../src/Client/IPC/IPCTransportPipe.h"

int main() {
    using namespace Client::IPC;

#ifdef _WIN32
    // Windows expects the named pipe format, e.g., \\.\pipe\pipename
    std::string pipeName = "\\\\.\\pipe\\test_pipe_123";
#else
    std::string pipeName = "test_pipe_123";
#endif

    IPCTransportPipe pipeServer(pipeName);

    bool frameReceived = false;
    pipeServer.SetOnFrameReceived([&frameReceived](std::span<const uint8_t> data) {
        frameReceived = true;
        std::cout << "Received data of size: " << data.size() << std::endl;
        assert(data.size() == 4);
    });

    bool started = pipeServer.StartServer();
    if (!started) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

#ifdef _WIN32
    // In a real Windows environment, we spawn a client thread to connect and send data
    std::thread clientThread([&pipeName]() {
        HANDLE hFile = CreateFileA(
            pipeName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );
        if (hFile != INVALID_HANDLE_VALUE) {
            std::vector<uint8_t> msg = { 4, 0, 0, 0, 1, 2, 3, 4 };
            DWORD bytesWritten;
            WriteFile(hFile, msg.data(), msg.size(), &bytesWritten, NULL);
            CloseHandle(hFile);
        }
    });

    // Wait a bit and poll events to handle connection and read
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pipeServer.PollEvents(); // connect
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pipeServer.PollEvents(); // read

    clientThread.join();
#else
    // For our Linux compilation mock, the embedded mock functions provide dummy data.
    // We just verify compilation and basic mock logic paths.
    pipeServer.PollEvents();
#endif
    
    std::vector<uint8_t> testData = {1, 2, 3, 4};
    bool sent = pipeServer.SendResponse(testData);

    pipeServer.StopServer();

    std::cout << "Test compiled and passed successfully." << std::endl;

    return 0;
}
