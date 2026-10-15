#include "Client/Data/ProtoLegacyAdapter.h"
#include <iostream>
#include <cassert>
#include <cstring>
#include <algorithm>

using namespace metin2::gamelib;

void TestRoundTrip() {
    TItemTable legacyOriginal{};
    legacyOriginal.dwVnum = 12345;
    legacyOriginal.dwVnumRange = 0;
    
    // Fill string without null terminator to test safe copying
    std::memset(legacyOriginal.szName, 'A', ITEM_NAME_MAX_LEN);
    legacyOriginal.szName[ITEM_NAME_MAX_LEN] = '\0';
    
    std::memset(legacyOriginal.szLocaleName, 'B', ITEM_NAME_MAX_LEN);
    legacyOriginal.szLocaleName[ITEM_NAME_MAX_LEN] = '\0';
    
    legacyOriginal.bType = 1;
    legacyOriginal.bSubType = 2;
    legacyOriginal.bWeight = 10;
    legacyOriginal.bSize = 1;
    
    legacyOriginal.dwAntiFlags = 0xFFFFFFFF;
    legacyOriginal.dwFlags = 0x12345678;
    legacyOriginal.dwWearFlags = 0x87654321;
    legacyOriginal.dwImmuneFlag = 0x0F0F0F0F;
    
    legacyOriginal.dwIBuyItemPrice = 1000;
    legacyOriginal.dwISellItemPrice = 500;
    
    for (size_t i = 0; i < ITEM_LIMIT_MAX_NUM; ++i) {
        legacyOriginal.aLimits[i].bType = i;
        legacyOriginal.aLimits[i].lValue = i * 100;
    }
    
    for (size_t i = 0; i < ITEM_APPLY_MAX_NUM; ++i) {
        legacyOriginal.aApplies[i].bType = i + 10;
        legacyOriginal.aApplies[i].lValue = i * 200;
    }
    
    for (size_t i = 0; i < ITEM_VALUES_MAX_NUM; ++i) {
        legacyOriginal.alValues[i] = i * 300;
    }
    
    for (size_t i = 0; i < ITEM_SOCKET_MAX_NUM; ++i) {
        legacyOriginal.alSockets[i] = i * 400;
    }
    
    legacyOriginal.dwRefinedVnum = 12346;
    legacyOriginal.wRefineSet = 99;
    legacyOriginal.bAlterToMagicItemPct = 50;
    legacyOriginal.bSpecular = 100;
    legacyOriginal.bGainSocketPct = 25;

    // legacy -> modern
    ItemProtoRecord modern = ProtoLegacyAdapter::FromLegacyTable(legacyOriginal);
    
    // modern -> legacy
    TItemTable legacyConverted = ProtoLegacyAdapter::ToLegacyTable(modern);

    // Verify all fields
    assert(legacyConverted.dwVnum == legacyOriginal.dwVnum);
    assert(legacyConverted.dwVnumRange == legacyOriginal.dwVnumRange);
    assert(std::strncmp(legacyConverted.szName, legacyOriginal.szName, ITEM_NAME_MAX_LEN) == 0);
    assert(std::strncmp(legacyConverted.szLocaleName, legacyOriginal.szLocaleName, ITEM_NAME_MAX_LEN) == 0);
    assert(legacyConverted.bType == legacyOriginal.bType);
    assert(legacyConverted.bSubType == legacyOriginal.bSubType);
    assert(legacyConverted.bWeight == legacyOriginal.bWeight);
    assert(legacyConverted.bSize == legacyOriginal.bSize);
    assert(legacyConverted.dwAntiFlags == legacyOriginal.dwAntiFlags);
    assert(legacyConverted.dwFlags == legacyOriginal.dwFlags);
    assert(legacyConverted.dwWearFlags == legacyOriginal.dwWearFlags);
    assert(legacyConverted.dwImmuneFlag == legacyOriginal.dwImmuneFlag);
    assert(legacyConverted.dwIBuyItemPrice == legacyOriginal.dwIBuyItemPrice);
    assert(legacyConverted.dwISellItemPrice == legacyOriginal.dwISellItemPrice);
    
    for (size_t i = 0; i < ITEM_LIMIT_MAX_NUM; ++i) {
        assert(legacyConverted.aLimits[i].bType == legacyOriginal.aLimits[i].bType);
        assert(legacyConverted.aLimits[i].lValue == legacyOriginal.aLimits[i].lValue);
    }
    
    for (size_t i = 0; i < ITEM_APPLY_MAX_NUM; ++i) {
        assert(legacyConverted.aApplies[i].bType == legacyOriginal.aApplies[i].bType);
        assert(legacyConverted.aApplies[i].lValue == legacyOriginal.aApplies[i].lValue);
    }
    
    for (size_t i = 0; i < ITEM_VALUES_MAX_NUM; ++i) {
        assert(legacyConverted.alValues[i] == legacyOriginal.alValues[i]);
    }
    
    for (size_t i = 0; i < ITEM_SOCKET_MAX_NUM; ++i) {
        assert(legacyConverted.alSockets[i] == legacyOriginal.alSockets[i]);
    }
    
    assert(legacyConverted.dwRefinedVnum == legacyOriginal.dwRefinedVnum);
    assert(legacyConverted.wRefineSet == legacyOriginal.wRefineSet);
    assert(legacyConverted.bAlterToMagicItemPct == legacyOriginal.bAlterToMagicItemPct);
    assert(legacyConverted.bSpecular == legacyOriginal.bSpecular);
    assert(legacyConverted.bGainSocketPct == legacyOriginal.bGainSocketPct);

    std::cout << "[SUCCESS] Round-trip conversion legacy -> modern -> legacy is lossless and correct.\n";
}

int main() {
    TestRoundTrip();
    return 0;
}
