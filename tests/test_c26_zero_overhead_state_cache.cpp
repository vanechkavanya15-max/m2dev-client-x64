#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "Client/Graphics/ZeroOverheadStateCache.h"
#include <cstdint>

using namespace Client::Graphics;

TEST_CASE("ZeroOverheadStateCache - Test 1: Filtracja powtorzen i telemetria")
{
    ZeroOverheadStateCache cache;
    cache.ResetToDefaults();

    uint64_t hardwareDispatches = 0;
    StateDispatchCallbacks callbacks{};
    callbacks.userData = &hardwareDispatches;
    callbacks.onSetRenderState = [](uint32_t /*stateType*/, uint32_t /*value*/, void* userData) {
        auto* counter = static_cast<uint64_t*>(userData);
        ++(*counter);
    };
    cache.SetCallbacks(callbacks);

    const uint32_t testStateType = 7; // np. D3DRS_ZENABLE
    const uint32_t testValue = 1;

    // Pierwsze wywolanie zmienia stan z domyslnego 0 na 1
    const bool firstCallResult = cache.SetRenderState(testStateType, testValue);
    CHECK(firstCallResult == true);
    CHECK(cache.GetRenderState(testStateType) == testValue);
    CHECK(cache.GetForwardedCalls() == 1);
    CHECK(cache.GetFilteredCalls() == 0);
    CHECK(hardwareDispatches == 1);

    // Kolejne 9 999 wywolan z ta sama wartoscia
    for (int i = 0; i < 9999; ++i)
    {
        const bool redundantCall = cache.SetRenderState(testStateType, testValue);
        CHECK_FALSE(redundantCall);
    }

    // Dokladnie 1 przeslane, 9 999 odfiltrowanych
    CHECK(cache.GetForwardedCalls() == 1);
    CHECK(cache.GetFilteredCalls() == 9999);
    CHECK(hardwareDispatches == 1);

    // Wspolczynnik efektywnosci > 99.9% (dokladnie 99.99%)
    const double efficiency = cache.GetEfficiencyRatio();
    CHECK(efficiency > 0.999);
    CHECK(efficiency == doctest::Approx(0.9999));
}

TEST_CASE("ZeroOverheadStateCache - Test 2: Poprawnosc stosu Push/Pop dla zagniezdzonego UI")
{
    ZeroOverheadStateCache cache;
    cache.ResetToDefaults();

    const uint32_t rsBlend = 27;     // D3DRS_ALPHABLENDENABLE
    const uint32_t rsSrcBlend = 19;  // D3DRS_SRCBLEND
    const uint32_t rsDestBlend = 20; // D3DRS_DESTBLEND

    // Stan poczatkowy
    cache.SetRenderState(rsBlend, 0);
    cache.SetRenderState(rsSrcBlend, 1);
    cache.SetRenderState(rsDestBlend, 2);

    CHECK(cache.GetRenderState(rsBlend) == 0);
    CHECK(cache.GetRenderState(rsSrcBlend) == 1);
    CHECK(cache.GetRenderState(rsDestBlend) == 2);

    // Poziom 1: Glowne okno wlacza Alpha Blending
    cache.PushRenderState(rsBlend, 1);
    CHECK(cache.GetRenderState(rsBlend) == 1);

    // Poziom 2: Panel potomny zmienia tryb mieszania
    cache.PushRenderState(rsSrcBlend, 5);  // D3DBLEND_SRCALPHA
    cache.PushRenderState(rsDestBlend, 6); // D3DBLEND_INVSRCALPHA
    CHECK(cache.GetRenderState(rsSrcBlend) == 5);
    CHECK(cache.GetRenderState(rsDestBlend) == 6);

    // Poziom 3: Wewnetrzna kontrolka (nieprzezroczysta bitmapa) wylacza blend
    cache.PushRenderState(rsBlend, 0);
    CHECK(cache.GetRenderState(rsBlend) == 0);

    // Opuszczenie Poziomu 3 - przywrocenie blend = 1
    cache.PopRenderState(rsBlend);
    CHECK(cache.GetRenderState(rsBlend) == 1);

    // Opuszczenie Poziomu 2 - przywrocenie trybow mieszania
    cache.PopRenderState(rsDestBlend);
    CHECK(cache.GetRenderState(rsDestBlend) == 2);
    cache.PopRenderState(rsSrcBlend);
    CHECK(cache.GetRenderState(rsSrcBlend) == 1);

    // Opuszczenie Poziomu 1 - powrot do poczatkowego blend = 0
    cache.PopRenderState(rsBlend);
    CHECK(cache.GetRenderState(rsBlend) == 0);

    // Bezposrednia weryfikacja szablonu FixedStack
    FixedStack<uint32_t, 16> stack;
    CHECK(stack.IsEmpty());
    CHECK_FALSE(stack.IsFull());
    CHECK(stack.Size() == 0);

    for (uint32_t i = 1; i <= 16; ++i)
    {
        CHECK(stack.Push(i * 10) == true);
    }
    CHECK(stack.IsFull());
    CHECK(stack.Size() == 16);
    CHECK_FALSE(stack.Push(999)); // Odrzucenie przepelnienia

    for (uint32_t i = 16; i >= 1; --i)
    {
        uint32_t val = 0;
        CHECK(stack.Pop(val) == true);
        CHECK(val == i * 10);
    }
    CHECK(stack.IsEmpty());
    CHECK_FALSE(stack.Pop());
}

TEST_CASE("ZeroOverheadStateCache - Test 3: Weryfikacja stanow textur i samplerow na 8 etapach")
{
    ZeroOverheadStateCache cache;
    cache.ResetToDefaults();

    // Test dla 8 etapow tekstur (0..7) oraz 64 stanow
    for (uint32_t stage = 0; stage < 8; ++stage)
    {
        const uint32_t stateType = 4; // D3DTSS_COLOROP
        const uint32_t val = 100 + stage;

        CHECK(cache.SetTextureStageState(stage, stateType, val) == true);
        CHECK(cache.GetTextureStageState(stage, stateType) == val);

        // Powtorne wywolanie musi zostac odfiltrowane
        CHECK(cache.SetTextureStageState(stage, stateType, val) == false);
    }

    // Weryfikacja izolacji miedzy etapami
    for (uint32_t stage = 0; stage < 8; ++stage)
    {
        CHECK(cache.GetTextureStageState(stage, 4) == 100 + stage);
    }

    // Test dla 8 samplerow (0..7) oraz 16 stanow
    for (uint32_t stage = 0; stage < 8; ++stage)
    {
        const uint32_t samplerState = 1; // D3DSAMP_ADDRESSU
        const uint32_t val = 200 + stage;

        CHECK(cache.SetSamplerState(stage, samplerState, val) == true);
        CHECK(cache.GetSamplerState(stage, samplerState) == val);

        // Powtorne wywolanie musi zostac odfiltrowane
        CHECK(cache.SetSamplerState(stage, samplerState, val) == false);
    }

    // Weryfikacja granic (odrzucenie niepoprawnych indeksow)
    CHECK_FALSE(cache.SetTextureStageState(8, 0, 1));
    CHECK_FALSE(cache.SetTextureStageState(0, 64, 1));
    CHECK_FALSE(cache.SetSamplerState(8, 0, 1));
    CHECK_FALSE(cache.SetSamplerState(0, 16, 1));
    CHECK(cache.GetTextureStageState(8, 0) == 0);
    CHECK(cache.GetSamplerState(8, 0) == 0);
}

TEST_CASE("ZeroOverheadStateCache - Test 4: Poprawnosc masek brudnych bitow (dirty bits)")
{
    ZeroOverheadStateCache cache;
    cache.ResetToDefaults();
    cache.ClearAllDirty();

    // Po wyczyszczeniu - wszystkie 4 bloki musza miec wartosc 0
    CHECK_FALSE(cache.HasAnyDirtyStates());
    CHECK(cache.GetDirtyBlock(0) == 0ULL);
    CHECK(cache.GetDirtyBlock(1) == 0ULL);
    CHECK(cache.GetDirtyBlock(2) == 0ULL);
    CHECK(cache.GetDirtyBlock(3) == 0ULL);

    // 1. Zmiana w Bloku 0: stan 10 (bit 10)
    CHECK(cache.SetRenderState(10, 123) == true);
    CHECK(cache.IsRenderStateDirty(10) == true);
    CHECK(cache.GetDirtyBlock(0) == (1ULL << 10));
    CHECK(cache.GetDirtyBlock(1) == 0ULL);
    CHECK(cache.GetDirtyBlock(2) == 0ULL);
    CHECK(cache.GetDirtyBlock(3) == 0ULL);

    // 2. Zmiana w Bloku 1: stan 70 (64 + 6 -> bit 6)
    CHECK(cache.SetRenderState(70, 456) == true);
    CHECK(cache.IsRenderStateDirty(70) == true);
    CHECK(cache.GetDirtyBlock(1) == (1ULL << 6));

    // 3. Zmiana w Bloku 2: stan 130 (128 + 2 -> bit 2)
    CHECK(cache.SetRenderState(130, 789) == true);
    CHECK(cache.IsRenderStateDirty(130) == true);
    CHECK(cache.GetDirtyBlock(2) == (1ULL << 2));

    // 4. Zmiana w Bloku 3: stan 200 (192 + 8 -> bit 8)
    CHECK(cache.SetRenderState(200, 999) == true);
    CHECK(cache.IsRenderStateDirty(200) == true);
    CHECK(cache.GetDirtyBlock(3) == (1ULL << 8));

    CHECK(cache.HasAnyDirtyStates() == true);

    // Powtorne wywolanie z ta sama wartoscia nie zmienia maski
    CHECK_FALSE(cache.SetRenderState(10, 123));
    CHECK(cache.GetDirtyBlock(0) == (1ULL << 10));

    // Czyszczenie pojedynczego stanu
    cache.ClearDirtyRenderState(10);
    CHECK_FALSE(cache.IsRenderStateDirty(10));
    CHECK(cache.GetDirtyBlock(0) == 0ULL);
    CHECK(cache.GetDirtyBlock(1) == (1ULL << 6));
    CHECK(cache.GetDirtyBlock(2) == (1ULL << 2));
    CHECK(cache.GetDirtyBlock(3) == (1ULL << 8));

    // MarkAllDirty
    cache.MarkAllDirty();
    CHECK(cache.GetDirtyBlock(0) == ~0ULL);
    CHECK(cache.GetDirtyBlock(1) == ~0ULL);
    CHECK(cache.GetDirtyBlock(2) == ~0ULL);
    CHECK(cache.GetDirtyBlock(3) == ~0ULL);

    // InvalidateAll - uniewaznia wszystkie stany i oznacza je jako brudne
    cache.InvalidateAll();
    CHECK(cache.GetDirtyBlock(0) == ~0ULL);

    // Po InvalidateAll kolejne wywolanie z ta sama wartoscia 123 musi przejsc (nie byc odfiltrowane)
    CHECK(cache.SetRenderState(10, 123) == true);
}
