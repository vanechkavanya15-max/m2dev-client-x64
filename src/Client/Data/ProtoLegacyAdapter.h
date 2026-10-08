#pragma once

#include "GameLib/ItemDataRegistry.h"
#include "GameLib/ItemData.h"

namespace metin2::gamelib {

using ItemProtoRecord = metin2::gamelib::ItemDataEntry;

class ProtoLegacyAdapter {
public:
    static ItemProtoRecord FromLegacyTable(const TItemTable& legacy);
    static TItemTable ToLegacyTable(const ItemProtoRecord& record);
};

} // namespace metin2::gamelib
