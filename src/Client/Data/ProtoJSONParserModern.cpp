#ifndef TEST_MOCK_STRUCTS

#endif
#include "ProtoJSONParserModern.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include <cstring>
#include <iostream>

namespace Client::Data {

#define PARSE_INT_FIELD(obj, fieldName, target) \
    if (!obj.HasMember(fieldName)) { return std::unexpected(ProtoParseError::MissingField); } \
    if (!obj[fieldName].IsInt()) { return std::unexpected(ProtoParseError::TypeMismatch); } \
    target = obj[fieldName].GetInt();

#define PARSE_UINT_FIELD(obj, fieldName, target) \
    if (!obj.HasMember(fieldName)) { return std::unexpected(ProtoParseError::MissingField); } \
    if (!obj[fieldName].IsUint()) { return std::unexpected(ProtoParseError::TypeMismatch); } \
    target = obj[fieldName].GetUint();

#define PARSE_STRING_FIELD(obj, fieldName, target, maxLen) \
    if (!obj.HasMember(fieldName)) { return std::unexpected(ProtoParseError::MissingField); } \
    if (!obj[fieldName].IsString()) { return std::unexpected(ProtoParseError::TypeMismatch); } \
    std::strncpy(target, obj[fieldName].GetString(), maxLen); \
    target[maxLen] = '\0';

    EterBase::Result<std::vector<CItemData::TItemTable>, ProtoParseError> ProtoJSONParserModern::ParseItemTable(std::string_view jsonContent) {
        rapidjson::Document doc;
        doc.Parse(jsonContent.data(), jsonContent.size());

        if (doc.HasParseError() || !doc.IsArray()) {
            return std::unexpected(ProtoParseError::InvalidFormat);
        }

        std::vector<CItemData::TItemTable> items;
        items.reserve(doc.Size());

        for (const auto& val : doc.GetArray()) {
            if (!val.IsObject()) {
                return std::unexpected(ProtoParseError::TypeMismatch);
            }

            CItemData::TItemTable item = {};

            PARSE_UINT_FIELD(val, "dwVnum", item.dwVnum)
            PARSE_UINT_FIELD(val, "dwVnumRange", item.dwVnumRange)
            PARSE_STRING_FIELD(val, "szName", item.szName, ITEM_NAME_MAX_LEN)
            PARSE_STRING_FIELD(val, "szLocaleName", item.szLocaleName, ITEM_NAME_MAX_LEN)
            
            uint32_t bType; PARSE_UINT_FIELD(val, "bType", bType); item.bType = static_cast<BYTE>(bType);
            uint32_t bSubType; PARSE_UINT_FIELD(val, "bSubType", bSubType); item.bSubType = static_cast<BYTE>(bSubType);
            uint32_t bWeight; PARSE_UINT_FIELD(val, "bWeight", bWeight); item.bWeight = static_cast<BYTE>(bWeight);
            uint32_t bSize; PARSE_UINT_FIELD(val, "bSize", bSize); item.bSize = static_cast<BYTE>(bSize);

            PARSE_UINT_FIELD(val, "dwAntiFlags", item.dwAntiFlags)
            PARSE_UINT_FIELD(val, "dwFlags", item.dwFlags)
            PARSE_UINT_FIELD(val, "dwWearFlags", item.dwWearFlags)
            PARSE_UINT_FIELD(val, "dwImmuneFlag", item.dwImmuneFlag)

            PARSE_UINT_FIELD(val, "dwIBuyItemPrice", item.dwIBuyItemPrice)
            PARSE_UINT_FIELD(val, "dwISellItemPrice", item.dwISellItemPrice)

            if (val.HasMember("aLimits") && val["aLimits"].IsArray()) {
                const auto& limits = val["aLimits"].GetArray();
                for (rapidjson::SizeType i = 0; i < limits.Size() && i < ITEM_LIMIT_MAX_NUM; ++i) {
                    if (limits[i].IsObject()) {
                        uint32_t bTypeLim; PARSE_UINT_FIELD(limits[i], "bType", bTypeLim); item.aLimits[i].bType = static_cast<BYTE>(bTypeLim);
                        PARSE_INT_FIELD(limits[i], "lValue", item.aLimits[i].lValue)
                    }
                }
            }
            if (val.HasMember("aApplies") && val["aApplies"].IsArray()) {
                const auto& applies = val["aApplies"].GetArray();
                for (rapidjson::SizeType i = 0; i < applies.Size() && i < ITEM_APPLY_MAX_NUM; ++i) {
                    if (applies[i].IsObject()) {
                         uint32_t bTypeApp; PARSE_UINT_FIELD(applies[i], "bType", bTypeApp); item.aApplies[i].bType = static_cast<BYTE>(bTypeApp);
                         PARSE_INT_FIELD(applies[i], "lValue", item.aApplies[i].lValue)
                    }
                }
            }
            if (val.HasMember("alValues") && val["alValues"].IsArray()) {
                const auto& values = val["alValues"].GetArray();
                for (rapidjson::SizeType i = 0; i < values.Size() && i < ITEM_VALUES_MAX_NUM; ++i) {
                    if (values[i].IsInt()) {
                        item.alValues[i] = values[i].GetInt();
                    }
                }
            }
            if (val.HasMember("alSockets") && val["alSockets"].IsArray()) {
                const auto& sockets = val["alSockets"].GetArray();
                for (rapidjson::SizeType i = 0; i < sockets.Size() && i < ITEM_SOCKET_MAX_NUM; ++i) {
                    if (sockets[i].IsInt()) {
                        item.alSockets[i] = sockets[i].GetInt();
                    }
                }
            }

            PARSE_UINT_FIELD(val, "dwRefinedVnum", item.dwRefinedVnum)
            
            uint32_t wRefineSet; PARSE_UINT_FIELD(val, "wRefineSet", wRefineSet); item.wRefineSet = static_cast<WORD>(wRefineSet);
            uint32_t bAlterToMagicItemPct; PARSE_UINT_FIELD(val, "bAlterToMagicItemPct", bAlterToMagicItemPct); item.bAlterToMagicItemPct = static_cast<BYTE>(bAlterToMagicItemPct);
            uint32_t bSpecular; PARSE_UINT_FIELD(val, "bSpecular", bSpecular); item.bSpecular = static_cast<BYTE>(bSpecular);
            uint32_t bGainSocketPct; PARSE_UINT_FIELD(val, "bGainSocketPct", bGainSocketPct); item.bGainSocketPct = static_cast<BYTE>(bGainSocketPct);

            items.push_back(item);
        }

        return items;
    }

    EterBase::Result<std::vector<CPythonNonPlayer::TMobTable>, ProtoParseError> ProtoJSONParserModern::ParseMobTable(std::string_view jsonContent) {
        rapidjson::Document doc;
        doc.Parse(jsonContent.data(), jsonContent.size());

        if (doc.HasParseError() || !doc.IsArray()) {
            return std::unexpected(ProtoParseError::InvalidFormat);
        }

        std::vector<CPythonNonPlayer::TMobTable> mobs;
        mobs.reserve(doc.Size());

        for (const auto& val : doc.GetArray()) {
            if (!val.IsObject()) {
                return std::unexpected(ProtoParseError::TypeMismatch);
            }

            CPythonNonPlayer::TMobTable mob = {};

            PARSE_UINT_FIELD(val, "dwVnum", mob.dwVnum)
            PARSE_STRING_FIELD(val, "szName", mob.szName, CHARACTER_NAME_MAX_LEN)
            PARSE_STRING_FIELD(val, "szLocaleName", mob.szLocaleName, CHARACTER_NAME_MAX_LEN)

            uint32_t bType; PARSE_UINT_FIELD(val, "bType", bType); mob.bType = static_cast<BYTE>(bType);
            uint32_t bRank; PARSE_UINT_FIELD(val, "bRank", bRank); mob.bRank = static_cast<BYTE>(bRank);
            uint32_t bBattleType; PARSE_UINT_FIELD(val, "bBattleType", bBattleType); mob.bBattleType = static_cast<BYTE>(bBattleType);
            uint32_t bLevel; PARSE_UINT_FIELD(val, "bLevel", bLevel); mob.bLevel = static_cast<BYTE>(bLevel);
            uint32_t bSize; PARSE_UINT_FIELD(val, "bSize", bSize); mob.bSize = static_cast<BYTE>(bSize);

            PARSE_UINT_FIELD(val, "dwGoldMin", mob.dwGoldMin)
            PARSE_UINT_FIELD(val, "dwGoldMax", mob.dwGoldMax)
            PARSE_UINT_FIELD(val, "dwExp", mob.dwExp)
            PARSE_UINT_FIELD(val, "dwMaxHP", mob.dwMaxHP)
            
            uint32_t bRegenCycle; PARSE_UINT_FIELD(val, "bRegenCycle", bRegenCycle); mob.bRegenCycle = static_cast<BYTE>(bRegenCycle);
            uint32_t bRegenPercent; PARSE_UINT_FIELD(val, "bRegenPercent", bRegenPercent); mob.bRegenPercent = static_cast<BYTE>(bRegenPercent);
            uint32_t wDef; PARSE_UINT_FIELD(val, "wDef", wDef); mob.wDef = static_cast<WORD>(wDef);

            PARSE_UINT_FIELD(val, "dwAIFlag", mob.dwAIFlag)
            PARSE_UINT_FIELD(val, "dwRaceFlag", mob.dwRaceFlag)
            PARSE_UINT_FIELD(val, "dwImmuneFlag", mob.dwImmuneFlag)

            uint32_t bStr; PARSE_UINT_FIELD(val, "bStr", bStr); mob.bStr = static_cast<BYTE>(bStr);
            uint32_t bDex; PARSE_UINT_FIELD(val, "bDex", bDex); mob.bDex = static_cast<BYTE>(bDex);
            uint32_t bCon; PARSE_UINT_FIELD(val, "bCon", bCon); mob.bCon = static_cast<BYTE>(bCon);
            uint32_t bInt; PARSE_UINT_FIELD(val, "bInt", bInt); mob.bInt = static_cast<BYTE>(bInt);
            
            if (val.HasMember("dwDamageRange") && val["dwDamageRange"].IsArray()) {
                const auto& dmg = val["dwDamageRange"].GetArray();
                if (dmg.Size() >= 2 && dmg[0].IsUint() && dmg[1].IsUint()) {
                    mob.dwDamageRange[0] = dmg[0].GetUint();
                    mob.dwDamageRange[1] = dmg[1].GetUint();
                } else {
                    return std::unexpected(ProtoParseError::TypeMismatch);
                }
            } else {
                 return std::unexpected(ProtoParseError::MissingField);
            }

            int sAttackSpeed; PARSE_INT_FIELD(val, "sAttackSpeed", sAttackSpeed); mob.sAttackSpeed = static_cast<short>(sAttackSpeed);
            int sMovingSpeed; PARSE_INT_FIELD(val, "sMovingSpeed", sMovingSpeed); mob.sMovingSpeed = static_cast<short>(sMovingSpeed);
            
            uint32_t bAggresiveHPPct; PARSE_UINT_FIELD(val, "bAggresiveHPPct", bAggresiveHPPct); mob.bAggresiveHPPct = static_cast<BYTE>(bAggresiveHPPct);
            uint32_t wAggressiveSight; PARSE_UINT_FIELD(val, "wAggressiveSight", wAggressiveSight); mob.wAggressiveSight = static_cast<WORD>(wAggressiveSight);
            uint32_t wAttackRange; PARSE_UINT_FIELD(val, "wAttackRange", wAttackRange); mob.wAttackRange = static_cast<WORD>(wAttackRange);

            if (val.HasMember("cEnchants") && val["cEnchants"].IsArray()) {
                const auto& arr = val["cEnchants"].GetArray();
                // 6 is MOB_ENCHANTS_MAX_NUM
                for (rapidjson::SizeType i = 0; i < arr.Size() && i < 6; ++i) {
                     if (arr[i].IsInt()) mob.cEnchants[i] = static_cast<char>(arr[i].GetInt());
                }
            }
            if (val.HasMember("cResists") && val["cResists"].IsArray()) {
                const auto& arr = val["cResists"].GetArray();
                // 11 is MOB_RESISTS_MAX_NUM
                for (rapidjson::SizeType i = 0; i < arr.Size() && i < 11; ++i) {
                     if (arr[i].IsInt()) mob.cResists[i] = static_cast<char>(arr[i].GetInt());
                }
            }

            PARSE_UINT_FIELD(val, "dwResurrectionVnum", mob.dwResurrectionVnum)
            PARSE_UINT_FIELD(val, "dwDropItemVnum", mob.dwDropItemVnum)

            uint32_t bMountCapacity; PARSE_UINT_FIELD(val, "bMountCapacity", bMountCapacity); mob.bMountCapacity = static_cast<BYTE>(bMountCapacity);
            uint32_t bOnClickType; PARSE_UINT_FIELD(val, "bOnClickType", bOnClickType); mob.bOnClickType = static_cast<BYTE>(bOnClickType);
            uint32_t bEmpire; PARSE_UINT_FIELD(val, "bEmpire", bEmpire); mob.bEmpire = static_cast<BYTE>(bEmpire);
            
            PARSE_STRING_FIELD(val, "szFolder", mob.szFolder, 64)

            if (!val.HasMember("fDamMultiply")) { return std::unexpected(ProtoParseError::MissingField); }
            if (val["fDamMultiply"].IsNumber()) { mob.fDamMultiply = val["fDamMultiply"].GetFloat(); }
            else { return std::unexpected(ProtoParseError::TypeMismatch); }

            PARSE_UINT_FIELD(val, "dwSummonVnum", mob.dwSummonVnum)
            PARSE_UINT_FIELD(val, "dwDrainSP", mob.dwDrainSP)
            PARSE_UINT_FIELD(val, "dwMonsterColor", mob.dwMonsterColor)
            PARSE_UINT_FIELD(val, "dwPolymorphItemVnum", mob.dwPolymorphItemVnum)
            
            if (val.HasMember("Skills") && val["Skills"].IsArray()) {
                const auto& arr = val["Skills"].GetArray();
                // 5 is MOB_SKILL_MAX_NUM
                for (rapidjson::SizeType i = 0; i < arr.Size() && i < 5; ++i) {
                     if (arr[i].IsObject()) {
                         PARSE_UINT_FIELD(arr[i], "dwVnum", mob.Skills[i].dwVnum)
                         uint32_t bLevelSkill; PARSE_UINT_FIELD(arr[i], "bLevel", bLevelSkill); mob.Skills[i].bLevel = static_cast<BYTE>(bLevelSkill);
                     }
                }
            }

            uint32_t bBerserkPoint; PARSE_UINT_FIELD(val, "bBerserkPoint", bBerserkPoint); mob.bBerserkPoint = static_cast<BYTE>(bBerserkPoint);
            uint32_t bStoneSkinPoint; PARSE_UINT_FIELD(val, "bStoneSkinPoint", bStoneSkinPoint); mob.bStoneSkinPoint = static_cast<BYTE>(bStoneSkinPoint);
            uint32_t bGodSpeedPoint; PARSE_UINT_FIELD(val, "bGodSpeedPoint", bGodSpeedPoint); mob.bGodSpeedPoint = static_cast<BYTE>(bGodSpeedPoint);
            uint32_t bDeathBlowPoint; PARSE_UINT_FIELD(val, "bDeathBlowPoint", bDeathBlowPoint); mob.bDeathBlowPoint = static_cast<BYTE>(bDeathBlowPoint);
            uint32_t bRevivePoint; PARSE_UINT_FIELD(val, "bRevivePoint", bRevivePoint); mob.bRevivePoint = static_cast<BYTE>(bRevivePoint);

            mobs.push_back(mob);
        }

        return mobs;
    }

    EterBase::Result<std::string, ProtoParseError> ProtoJSONParserModern::ExportItemTableToJSON(const std::vector<CItemData::TItemTable>& items) {
        rapidjson::StringBuffer sb;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);

        writer.StartArray();
        for (const auto& item : items) {
            writer.StartObject();
            writer.Key("dwVnum"); writer.Uint(item.dwVnum);
            writer.Key("dwVnumRange"); writer.Uint(item.dwVnumRange);
            writer.Key("szName"); writer.String(item.szName);
            writer.Key("szLocaleName"); writer.String(item.szLocaleName);
            
            writer.Key("bType"); writer.Uint(item.bType);
            writer.Key("bSubType"); writer.Uint(item.bSubType);
            writer.Key("bWeight"); writer.Uint(item.bWeight);
            writer.Key("bSize"); writer.Uint(item.bSize);

            writer.Key("dwAntiFlags"); writer.Uint(item.dwAntiFlags);
            writer.Key("dwFlags"); writer.Uint(item.dwFlags);
            writer.Key("dwWearFlags"); writer.Uint(item.dwWearFlags);
            writer.Key("dwImmuneFlag"); writer.Uint(item.dwImmuneFlag);
            
            writer.Key("dwIBuyItemPrice"); writer.Uint(item.dwIBuyItemPrice);
            writer.Key("dwISellItemPrice"); writer.Uint(item.dwISellItemPrice);

            writer.Key("aLimits");
            writer.StartArray();
            for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i) {
                writer.StartObject();
                writer.Key("bType"); writer.Uint(item.aLimits[i].bType);
                writer.Key("lValue"); writer.Int(item.aLimits[i].lValue);
                writer.EndObject();
            }
            writer.EndArray();

            writer.Key("aApplies");
            writer.StartArray();
            for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i) {
                writer.StartObject();
                writer.Key("bType"); writer.Uint(item.aApplies[i].bType);
                writer.Key("lValue"); writer.Int(item.aApplies[i].lValue);
                writer.EndObject();
            }
            writer.EndArray();

            writer.Key("alValues");
            writer.StartArray();
            for (int i = 0; i < ITEM_VALUES_MAX_NUM; ++i) writer.Int(item.alValues[i]);
            writer.EndArray();

            writer.Key("alSockets");
            writer.StartArray();
            for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i) writer.Int(item.alSockets[i]);
            writer.EndArray();

            writer.Key("dwRefinedVnum"); writer.Uint(item.dwRefinedVnum);
            writer.Key("wRefineSet"); writer.Uint(item.wRefineSet);
            writer.Key("bAlterToMagicItemPct"); writer.Uint(item.bAlterToMagicItemPct);
            writer.Key("bSpecular"); writer.Uint(item.bSpecular);
            writer.Key("bGainSocketPct"); writer.Uint(item.bGainSocketPct);

            writer.EndObject();
        }
        writer.EndArray();

        return sb.GetString();
    }

    EterBase::Result<std::string, ProtoParseError> ProtoJSONParserModern::ExportMobTableToJSON(const std::vector<CPythonNonPlayer::TMobTable>& mobs) {
        rapidjson::StringBuffer sb;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);

        writer.StartArray();
        for (const auto& mob : mobs) {
            writer.StartObject();
            
            writer.Key("dwVnum"); writer.Uint(mob.dwVnum);
            writer.Key("szName"); writer.String(mob.szName);
            writer.Key("szLocaleName"); writer.String(mob.szLocaleName);

            writer.Key("bType"); writer.Uint(mob.bType);
            writer.Key("bRank"); writer.Uint(mob.bRank);
            writer.Key("bBattleType"); writer.Uint(mob.bBattleType);
            writer.Key("bLevel"); writer.Uint(mob.bLevel);
            writer.Key("bSize"); writer.Uint(mob.bSize);

            writer.Key("dwGoldMin"); writer.Uint(mob.dwGoldMin);
            writer.Key("dwGoldMax"); writer.Uint(mob.dwGoldMax);
            writer.Key("dwExp"); writer.Uint(mob.dwExp);
            writer.Key("dwMaxHP"); writer.Uint(mob.dwMaxHP);
            
            writer.Key("bRegenCycle"); writer.Uint(mob.bRegenCycle);
            writer.Key("bRegenPercent"); writer.Uint(mob.bRegenPercent);
            writer.Key("wDef"); writer.Uint(mob.wDef);

            writer.Key("dwAIFlag"); writer.Uint(mob.dwAIFlag);
            writer.Key("dwRaceFlag"); writer.Uint(mob.dwRaceFlag);
            writer.Key("dwImmuneFlag"); writer.Uint(mob.dwImmuneFlag);

            writer.Key("bStr"); writer.Uint(mob.bStr);
            writer.Key("bDex"); writer.Uint(mob.bDex);
            writer.Key("bCon"); writer.Uint(mob.bCon);
            writer.Key("bInt"); writer.Uint(mob.bInt);
            
            writer.Key("dwDamageRange");
            writer.StartArray();
            writer.Uint(mob.dwDamageRange[0]);
            writer.Uint(mob.dwDamageRange[1]);
            writer.EndArray();

            writer.Key("sAttackSpeed"); writer.Int(mob.sAttackSpeed);
            writer.Key("sMovingSpeed"); writer.Int(mob.sMovingSpeed);
            
            writer.Key("bAggresiveHPPct"); writer.Uint(mob.bAggresiveHPPct);
            writer.Key("wAggressiveSight"); writer.Uint(mob.wAggressiveSight);
            writer.Key("wAttackRange"); writer.Uint(mob.wAttackRange);

            writer.Key("cEnchants");
            writer.StartArray();
            // 6 is MOB_ENCHANTS_MAX_NUM
            for (int i = 0; i < 6; ++i) writer.Int(mob.cEnchants[i]);
            writer.EndArray();

            writer.Key("cResists");
            writer.StartArray();
            // 11 is MOB_RESISTS_MAX_NUM
            for (int i = 0; i < 11; ++i) writer.Int(mob.cResists[i]);
            writer.EndArray();

            writer.Key("dwResurrectionVnum"); writer.Uint(mob.dwResurrectionVnum);
            writer.Key("dwDropItemVnum"); writer.Uint(mob.dwDropItemVnum);

            writer.Key("bMountCapacity"); writer.Uint(mob.bMountCapacity);
            writer.Key("bOnClickType"); writer.Uint(mob.bOnClickType);
            writer.Key("bEmpire"); writer.Uint(mob.bEmpire);
            
            writer.Key("szFolder"); writer.String(mob.szFolder);

            writer.Key("fDamMultiply"); writer.Double(mob.fDamMultiply);

            writer.Key("dwSummonVnum"); writer.Uint(mob.dwSummonVnum);
            writer.Key("dwDrainSP"); writer.Uint(mob.dwDrainSP);
            writer.Key("dwMonsterColor"); writer.Uint(mob.dwMonsterColor);
            writer.Key("dwPolymorphItemVnum"); writer.Uint(mob.dwPolymorphItemVnum);

            writer.Key("Skills");
            writer.StartArray();
            // 5 is MOB_SKILL_MAX_NUM
            for (int i = 0; i < 5; ++i) {
                writer.StartObject();
                writer.Key("dwVnum"); writer.Uint(mob.Skills[i].dwVnum);
                writer.Key("bLevel"); writer.Uint(mob.Skills[i].bLevel);
                writer.EndObject();
            }
            writer.EndArray();
            
            writer.Key("bBerserkPoint"); writer.Uint(mob.bBerserkPoint);
            writer.Key("bStoneSkinPoint"); writer.Uint(mob.bStoneSkinPoint);
            writer.Key("bGodSpeedPoint"); writer.Uint(mob.bGodSpeedPoint);
            writer.Key("bDeathBlowPoint"); writer.Uint(mob.bDeathBlowPoint);
            writer.Key("bRevivePoint"); writer.Uint(mob.bRevivePoint);

            writer.EndObject();
        }
        writer.EndArray();

        return sb.GetString();
    }

} // namespace Client::Data
