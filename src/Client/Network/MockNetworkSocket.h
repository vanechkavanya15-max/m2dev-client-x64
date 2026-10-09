#pragma once

#include <cstdint>
#include <span>
#include <vector>
#include <deque>
#include <string>
#include <string_view>
#include <mutex>
#include <memory>
#include <algorithm>
#include <chrono>

#include "../../EterBase/Result.h"
#include "../../EterBase/PacketResult.h"

namespace Client::Network
{
    enum class SocketError : uint8_t
    {
        None = 0,
        NotConnected,
        AlreadyConnected,
        BufferOverflow,
        BufferUnderflow,
        ConnectionReset,
        Timeout,
        SimulatedError
    };

    constexpr std::string_view ToString(SocketError err) noexcept
    {
        switch (err)
        {
            case SocketError::None: return "None";
            case SocketError::NotConnected: return "NotConnected";
            case SocketError::AlreadyConnected: return "AlreadyConnected";
            case SocketError::BufferOverflow: return "BufferOverflow";
            case SocketError::BufferUnderflow: return "BufferUnderflow";
            case SocketError::ConnectionReset: return "ConnectionReset";
            case SocketError::Timeout: return "Timeout";
            case SocketError::SimulatedError: return "SimulatedError";
        }
        return "UnknownSocketError";
    }

    /**
     * @struct SentPacketRecord
     * @brief Zapis pojedynczego pakietu wyslanego przez klienta przez mock socket.
     */
    struct SentPacketRecord
    {
        std::vector<uint8_t> data;
        std::chrono::steady_clock::time_point timestamp;

        [[nodiscard]] uint8_t GetOpcode() const noexcept
        {
            return data.empty() ? 0 : data[0];
        }

        [[nodiscard]] std::span<const uint8_t> GetPayload() const noexcept
        {
            if (data.size() <= 1) return {};
            return std::span<const uint8_t>(data.data() + 1, data.size() - 1);
        }
    };

    /**
     * @class MockNetworkSocket
     * @brief Wirtualny symulator gniazda sieciowego dla testow jednostkowych i srodowiska headless.
     * 
     * Pozwala wstrzykiwac pakiety przychodzace (symulacja serwera) oraz inspekcjonowac pakiety
     * wychodzace wyslane przez kod klienta bez otwierania fizycznych deskryptorow TCP.
     */
    class MockNetworkSocket
    {
    public:
        MockNetworkSocket() = default;
        ~MockNetworkSocket() = default;

        // Disallow copying, allow moving
        MockNetworkSocket(const MockNetworkSocket&) = delete;
        MockNetworkSocket& operator=(const MockNetworkSocket&) = delete;
        MockNetworkSocket(MockNetworkSocket&& other) noexcept
        {
            std::scoped_lock lock(other.m_mutex);
            m_connected = other.m_connected;
            m_remoteHost = std::move(other.m_remoteHost);
            m_remotePort = other.m_remotePort;
            m_recvQueue = std::move(other.m_recvQueue);
            m_sentPackets = std::move(other.m_sentPackets);
            m_simulateError = other.m_simulateError;
        }
        MockNetworkSocket& operator=(MockNetworkSocket&& other) noexcept
        {
            if (this != &other)
            {
                std::scoped_lock lock(m_mutex, other.m_mutex);
                m_connected = other.m_connected;
                m_remoteHost = std::move(other.m_remoteHost);
                m_remotePort = other.m_remotePort;
                m_recvQueue = std::move(other.m_recvQueue);
                m_sentPackets = std::move(other.m_sentPackets);
                m_simulateError = other.m_simulateError;
            }
            return *this;
        }

        /**
         * @brief Symuluje nawiazanie polaczenia z adresem serwera.
         */
        [[nodiscard]] bool Connect(std::string_view host, uint16_t port) noexcept
        {
            std::scoped_lock lock(m_mutex);
            if (m_simulateError) return false;
            m_remoteHost = host;
            m_remotePort = port;
            m_connected = true;
            return true;
        }

        /**
         * @brief Zamyka wirtualne polaczenie sieciowe.
         */
        void Disconnect() noexcept
        {
            std::scoped_lock lock(m_mutex);
            m_connected = false;
        }

        [[nodiscard]] bool IsConnected() const noexcept
        {
            std::scoped_lock lock(m_mutex);
            return m_connected;
        }

        [[nodiscard]] std::string_view GetRemoteHost() const noexcept
        {
            std::scoped_lock lock(m_mutex);
            return m_remoteHost;
        }

        [[nodiscard]] uint16_t GetRemotePort() const noexcept
        {
            std::scoped_lock lock(m_mutex);
            return m_remotePort;
        }

        /**
         * @brief Symuluje wyslanie danych przez klienta do serwera.
         */
        [[nodiscard]] size_t Send(std::span<const uint8_t> data)
        {
            std::scoped_lock lock(m_mutex);
            if (!m_connected || m_simulateError) return 0;
            if (data.empty()) return 0;

            SentPacketRecord record;
            record.data.assign(data.begin(), data.end());
            record.timestamp = std::chrono::steady_clock::now();
            m_sentPackets.push_back(std::move(record));
            return data.size();
        }

        /**
         * @brief Odczytuje dane z bufora przychodzacego do podanego bufora docelowego.
         */
        [[nodiscard]] size_t Receive(std::span<uint8_t> destBuffer)
        {
            std::scoped_lock lock(m_mutex);
            if (!m_connected || m_simulateError) return 0;
            if (destBuffer.empty() || m_recvQueue.empty()) return 0;

            const size_t bytesToCopy = std::min(destBuffer.size(), m_recvQueue.size());
            for (size_t i = 0; i < bytesToCopy; ++i)
            {
                destBuffer[i] = m_recvQueue.front();
                m_recvQueue.pop_front();
            }
            return bytesToCopy;
        }

        /**
         * @brief Wstrzykuje surowe bajty pakietu serwera do kolejki odbiorczej klienta (Test Harness).
         */
        void InjectBytes(std::span<const uint8_t> rawBytes)
        {
            std::scoped_lock lock(m_mutex);
            m_recvQueue.insert(m_recvQueue.end(), rawBytes.begin(), rawBytes.end());
        }

        /**
         * @brief Wstrzykuje kompletny pakiet (naglowek opcode + dane ciala).
         */
        void InjectPacket(uint8_t opcode, std::span<const uint8_t> payload = {})
        {
            std::scoped_lock lock(m_mutex);
            m_recvQueue.push_back(opcode);
            m_recvQueue.insert(m_recvQueue.end(), payload.begin(), payload.end());
        }

        /**
         * @brief Zwraca liste wszystkich pakietow wyslanych przez klienta.
         */
        [[nodiscard]] std::vector<SentPacketRecord> GetSentPackets() const
        {
            std::scoped_lock lock(m_mutex);
            return m_sentPackets;
        }

        [[nodiscard]] size_t GetSentPacketCount() const noexcept
        {
            std::scoped_lock lock(m_mutex);
            return m_sentPackets.size();
        }

        /**
         * @brief Czysci historie wyslanych pakietow.
         */
        void ClearSentPackets() noexcept
        {
            std::scoped_lock lock(m_mutex);
            m_sentPackets.clear();
        }

        /**
         * @brief Czysci kolejke odbiorcza.
         */
        void ClearRecvQueue() noexcept
        {
            std::scoped_lock lock(m_mutex);
            m_recvQueue.clear();
        }

        /**
         * @brief Resetuje caly stan gniazda.
         */
        void Reset() noexcept
        {
            std::scoped_lock lock(m_mutex);
            m_connected = false;
            m_remoteHost.clear();
            m_remotePort = 0;
            m_recvQueue.clear();
            m_sentPackets.clear();
            m_simulateError = false;
        }

        void SetSimulateError(bool simulate) noexcept
        {
            std::scoped_lock lock(m_mutex);
            m_simulateError = simulate;
        }

        [[nodiscard]] size_t GetAvailableBytes() const noexcept
        {
            std::scoped_lock lock(m_mutex);
            return m_recvQueue.size();
        }

    private:
        mutable std::mutex m_mutex;
        bool m_connected{false};
        std::string m_remoteHost{};
        uint16_t m_remotePort{0};
        std::deque<uint8_t> m_recvQueue{};
        std::vector<SentPacketRecord> m_sentPackets{};
        bool m_simulateError{false};
    };
}
