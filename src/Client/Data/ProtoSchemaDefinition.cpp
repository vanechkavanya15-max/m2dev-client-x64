#include "../../EterBase/StdAfx.h"
#include "ProtoSchemaDefinition.h"

namespace Client::Data {

EterBase::VoidResult<std::string_view> ItemProtoRecord::Invariants() const {
    if (vnum.value() == 0) {
        return EterBase::MakeError("Item VNUM cannot be 0");
    }
    if (sellPrice > buyPrice) {
        return EterBase::MakeError("Sell price cannot be greater than buy price");
    }
    return {};
}

EterBase::VoidResult<std::string_view> MobProtoRecord::Invariants() const {
    if (vnum.value() == 0) {
        return EterBase::MakeError("Mob VNUM cannot be 0");
    }
    if (goldMin > goldMax) {
        return EterBase::MakeError("Min gold cannot be greater than max gold");
    }
    return {};
}

uint64_t ProtoSchemaDefinition::GetItemProtoSchemaHash() noexcept {
    std::string_view schemaStr = "ItemProtoRecord:vnum,vnumRange,name,localeName,type,subType,weight,size,antiFlags,flags,wearFlags,immuneFlags,buyPrice,sellPrice,limits,applies,values,sockets,refinedVnum,refineSet";
    auto hashRes = EterBase::XXHash64::Hash(schemaStr);
    return hashRes.value_or(0);
}

uint64_t ProtoSchemaDefinition::GetMobProtoSchemaHash() noexcept {
    std::string_view schemaStr = "MobProtoRecord:vnum,name,localeName,type,rank,battleType,level,size,goldMin,goldMax,exp,maxHp,regenCycle,regenPercent,defense";
    auto hashRes = EterBase::XXHash64::Hash(schemaStr);
    return hashRes.value_or(0);
}

} // namespace Client::Data
