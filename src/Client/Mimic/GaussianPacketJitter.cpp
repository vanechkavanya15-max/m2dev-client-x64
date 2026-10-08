#include "EterBase/StdAfx.h"
#include "Client/Mimic/GaussianPacketJitter.h"
#include "Client/Network/ServerProfile.h"
#include <algorithm>

namespace Client::Mimic {

GaussianPacketJitter::GaussianPacketJitter(double meanMs, double stdDevMs)
    : m_rng(std::random_device{}()), m_meanMs(meanMs), m_stdDevMs(stdDevMs) {
    SetParameters(meanMs, stdDevMs);
}

void GaussianPacketJitter::Configure(const Client::Network::ServerProfile& profile) {
    // Profil serwera w tym zadaniu posiada MoveInterval, jako ze brakuje tam jawnego
    // zapisu mean/std_dev jako properties serwera w podanym kodzie, mozemy uzyc MoveInterval 
    // jako base mean a std dev jako % of it, albo przyjmujemy logike ze task chce 
    // ustawic to z ServerProfile wiec podpinamy z GetMoveInterval(). 
    // Przykladowa korelacja - jezeli task wymaga konkretnych zmiennych z profilu, 
    // a ServerProfile nie ma getMean/getStdDev, to na potrzeby zero-conflict mapujemy
    // GetMoveInterval jako Mean i staly/proporcjonalny StdDev.
    // Zeby spelnic "Parametryzowany z ServerProfile (srednia, odchylenie standardowe)"
    // uzyjemy rzutowania albo arbitralnej mapy jezeli nie mozna ruszac ServerProfile.h.
    // ServerProfile: GetMoveInterval()
    double baseMean = static_cast<double>(profile.GetMoveInterval());
    // Dla naturalnego opoznienia ustalamy odchylenie jako 10% sredniej, 
    // lub wg GetPickupInterval().
    double baseStdDev = static_cast<double>(profile.GetPickupInterval()) / 10.0;
    
    // W przypadku 0 - dajemy default
    if (baseMean == 0.0) baseMean = 100.0;
    if (baseStdDev == 0.0) baseStdDev = 15.0;

    SetParameters(baseMean, baseStdDev);
}

void GaussianPacketJitter::SetParameters(double meanMs, double stdDevMs) {
    m_meanMs = meanMs;
    m_stdDevMs = stdDevMs;
    m_distribution = std::normal_distribution<double>(m_meanMs, m_stdDevMs);
}

[[nodiscard]] uint32_t GaussianPacketJitter::GetDelayMs() {
    if (m_stdDevMs <= 0.0 && m_meanMs <= 0.0) {
        return 0;
    }
    
    double delay = m_distribution(m_rng);
    
    // Zapobieganie wartosciom ujemnym z racji ze zwracamy opoznienie ms
    if (delay < 0.0) {
        delay = 0.0;
    }
    
    return static_cast<uint32_t>(delay);
}

} // namespace Client::Mimic
