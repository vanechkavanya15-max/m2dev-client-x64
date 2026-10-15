#pragma once

#include <random>
#include <cstdint>

namespace Client::Network {
    class ServerProfile;
}

namespace Client::Mimic {

class GaussianPacketJitter {
public:
    GaussianPacketJitter(double meanMs = 0.0, double stdDevMs = 0.0);

    // Konfiguruje parametry z profilu serwera
    void Configure(const Client::Network::ServerProfile& profile);
    
    void SetParameters(double meanMs, double stdDevMs);

    // Zwraca wygenerowane opoznienie (w milisekundach), co najmniej 0
    [[nodiscard]] uint32_t GetDelayMs();

private:
    std::mt19937_64 m_rng;
    std::normal_distribution<double> m_distribution;
    
    double m_meanMs;
    double m_stdDevMs;
};

} // namespace Client::Mimic
