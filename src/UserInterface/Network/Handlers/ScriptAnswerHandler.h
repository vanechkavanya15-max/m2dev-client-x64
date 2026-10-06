#pragma once

#include <cstdint>

class CPythonNetworkStream;

namespace Network::Handlers
{
    /**
     * @brief Handler class responsible for sending script answers to the server.
     */
    class ScriptAnswerHandler
    {
    public:
        /**
         * @brief Sends a script answer packet over the provided network stream.
         * 
         * @param networkStream Pointer to the active network stream.
         * @param answer The zero-based index of the chosen answer.
         * @return true if the packet was successfully queued for sending.
         * @return false if the network stream is null or sending failed.
         */
        static bool Send(CPythonNetworkStream* networkStream, uint8_t answer);
    };
}
