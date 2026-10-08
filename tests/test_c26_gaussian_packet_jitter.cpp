#include <gtest/gtest.h>
#include "Client/Mimic/GaussianPacketJitter.h"
#include "Client/Network/ServerProfile.h"
#include <vector>
#include <numeric>
#include <cmath>

TEST(GaussianPacketJitterTest, ClampedToNonNegative) {
    // Utworz dystrybucje z duzym odchyleniem i ujemna srednia, 
    // aby wymusic generowanie ujemnych wartosci pierwotnych.
    Client::Mimic::GaussianPacketJitter jitter(-50.0, 100.0);
    
    for (int i = 0; i < 1000; ++i) {
        uint32_t delay = jitter.GetDelayMs();
        // W C++ uint32_t jest zawsze >= 0. Wartosci ujemne beda 0 jesli poprawnie obcinane.
        // Jednak gdyby obciecie nie dzialalo i rzutowanie int ujemnego na uint, 
        // to daloby to bardzo duze wartosci dodatnie (underflow/wrap-around).
        // Mozemy testowac ze jest z normalnego rzedu wielkosci (< np. 1000).
        EXPECT_GE(delay, 0u);
        EXPECT_LT(delay, 1000u); // Jezeli delay zostal przekonwertowany z <0 do (uint32_t)-1, to bedzie duzo wieksze
    }
}

TEST(GaussianPacketJitterTest, StatisticalProperties) {
    const double expectedMean = 100.0;
    const double expectedStdDev = 20.0;
    
    Client::Mimic::GaussianPacketJitter jitter(expectedMean, expectedStdDev);
    
    const int sampleSize = 10000;
    std::vector<uint32_t> samples;
    samples.reserve(sampleSize);
    
    for (int i = 0; i < sampleSize; ++i) {
        samples.push_back(jitter.GetDelayMs());
    }
    
    double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
    double mean = sum / sampleSize;
    
    double sqSum = 0.0;
    for (uint32_t val : samples) {
        sqSum += (val - mean) * (val - mean);
    }
    double stdDev = std::sqrt(sqSum / sampleSize);
    
    // Test that the mean is within 5% of expected
    EXPECT_NEAR(mean, expectedMean, expectedMean * 0.05);
    
    // Test that std dev is within 10% of expected
    EXPECT_NEAR(stdDev, expectedStdDev, expectedStdDev * 0.1);
}

TEST(GaussianPacketJitterTest, ProfileConfiguration) {
    Client::Network::ServerProfile profile;
    profile.SetMoveInterval(150);   // To powinno trafic do mean
    profile.SetPickupInterval(200); // baseStdDev to pickup / 10 = 20.0
    
    Client::Mimic::GaussianPacketJitter jitter;
    jitter.Configure(profile);
    
    const int sampleSize = 10000;
    std::vector<uint32_t> samples;
    samples.reserve(sampleSize);
    
    for (int i = 0; i < sampleSize; ++i) {
        samples.push_back(jitter.GetDelayMs());
    }
    
    double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
    double mean = sum / sampleSize;
    
    EXPECT_NEAR(mean, 150.0, 150.0 * 0.05);
}
