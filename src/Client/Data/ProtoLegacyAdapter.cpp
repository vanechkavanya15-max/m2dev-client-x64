#include "StdAfx.h"
#include "ProtoLegacyAdapter.h"
#include <algorithm>
#include <cstring>

namespace metin2::gamelib {

ItemProtoRecord ProtoLegacyAdapter::FromLegacyTable(const TItemTable& legacy) {
    ItemProtoRecord record{};
    
    record.vnum = legacy.dwVnum;
    record.vnumRange = legacy.dwVnumRange;
    
    // SAFE COPY per code review: Use std::copy_n or assign.
    // std::array handles string data differently, we use std::copy_n to be safe and set null-term.
    size_t nameLen = strnlen(legacy.szName, ITEM_NAME_MAX_LEN);
    std::copy_n(legacy.szName, nameLen, record.name.data());
    record.name[nameLen] = '\0';
    
    size_t localeNameLen = strnlen(legacy.szLocaleName, ITEM_NAME_MAX_LEN);
    std::copy_n(legacy.szLocaleName, localeNameLen, record.localeName.data());
    record.localeName[localeNameLen] = '\0';
    
    record.type = legacy.bType;
    record.subType = legacy.bSubType;
    record.weight = legacy.bWeight;
    record.size = legacy.bSize;
    
    record.antiFlags = legacy.dwAntiFlags;
    record.flags = legacy.dwFlags;
    record.wearFlags = legacy.dwWearFlags;
    record.immuneFlag = legacy.dwImmuneFlag;
    
    record.buyPrice = legacy.dwIBuyItemPrice;
    record.sellPrice = legacy.dwISellItemPrice;
    
    for (size_t i = 0; i < ITEM_LIMIT_MAX_NUM; ++i) {
        record.limits[i].type = legacy.aLimits[i].bType;
        record.limits[i].value = legacy.aLimits[i].lValue;
    }
    
    for (size_t i = 0; i < ITEM_APPLY_MAX_NUM; ++i) {
        record.applies[i].type = legacy.aApplies[i].bType;
        record.applies[i].value = legacy.aApplies[i].lValue;
    }
    
    for (size_t i = 0; i < ITEM_VALUES_MAX_NUM; ++i) {
        record.values[i] = legacy.alValues[i];
    }
    
    for (size_t i = 0; i < ITEM_SOCKET_MAX_NUM; ++i) {
        record.sockets[i] = legacy.alSockets[i];
    }
    
    record.refinedVnum = legacy.dwRefinedVnum;
    record.refineSet = legacy.wRefineSet;
    record.alterToMagicItemPct = legacy.bAlterToMagicItemPct;
    record.specular = legacy.bSpecular;
    record.gainSocketPct = legacy.bGainSocketPct;
    
    return record;
}

TItemTable ProtoLegacyAdapter::ToLegacyTable(const ItemProtoRecord& record) {
    TItemTable legacy{};
    
    legacy.dwVnum = record.vnum;
    legacy.dwVnumRange = record.vnumRange;
    
    size_t nameLen = strnlen(record.name.data(), ITEM_NAME_MAX_LEN);
    std::copy_n(record.name.data(), nameLen, legacy.szName);
    legacy.szName[nameLen] = '\0';
    
    size_t localeNameLen = strnlen(record.localeName.data(), ITEM_NAME_MAX_LEN);
    std::copy_n(record.localeName.data(), localeNameLen, legacy.szLocaleName);
    legacy.szLocaleName[localeNameLen] = '\0';
    
    legacy.bType = record.type;
    legacy.bSubType = record.subType;
    legacy.bWeight = record.weight;
    legacy.bSize = record.size;
    
    legacy.dwAntiFlags = record.antiFlags;
    legacy.dwFlags = record.flags;
    legacy.dwWearFlags = record.wearFlags;
    legacy.dwImmuneFlag = record.immuneFlag;
    
    legacy.dwIBuyItemPrice = record.buyPrice;
    legacy.dwISellItemPrice = record.sellPrice;
    
    for (size_t i = 0; i < ITEM_LIMIT_MAX_NUM; ++i) {
        legacy.aLimits[i].bType = record.limits[i].type;
        legacy.aLimits[i].lValue = record.limits[i].value;
    }
    
    for (size_t i = 0; i < ITEM_APPLY_MAX_NUM; ++i) {
        legacy.aApplies[i].bType = record.applies[i].type;
        legacy.aApplies[i].lValue = record.applies[i].value;
    }
    
    for (size_t i = 0; i < ITEM_VALUES_MAX_NUM; ++i) {
        legacy.alValues[i] = record.values[i];
    }
    
    for (size_t i = 0; i < ITEM_SOCKET_MAX_NUM; ++i) {
        legacy.alSockets[i] = record.sockets[i];
    }
    
    legacy.dwRefinedVnum = record.refinedVnum;
    legacy.wRefineSet = record.refineSet;
    legacy.bAlterToMagicItemPct = record.alterToMagicItemPct;
    legacy.bSpecular = record.specular;
    legacy.bGainSocketPct = record.gainSocketPct;
    
    return legacy;
}

} // namespace metin2::gamelib
