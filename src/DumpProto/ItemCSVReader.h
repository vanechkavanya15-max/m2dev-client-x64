#ifndef __Item_CSV_READER_H__
#define __Item_CSV_READER_H__

#include <iostream>

//csv \xc6\xc4\xc0\xcf\xc0\xbb \xc0о\xee\xbfͼ\xad \xbe\xc6\xc0\xcc\xc5\xdb \xc5\xd7\xc0̺\xed\xbf\xa1 \xb3־\xee\xc1ش\xd9.
void putItemIntoTable(); //(\xc5\xd7\xc0̺\xed, \xc5׽\xbaƮ\xbf\xa9\xba\xce)

int get_Item_Type_Value(std::string inputString);
int get_Item_SubType_Value(int type_value, std::string inputString);
int get_Item_AntiFlag_Value(std::string inputString);
int get_Item_Flag_Value(std::string inputString);
int get_Item_WearFlag_Value(std::string inputString);
int get_Item_Immune_Value(std::string inputString);
int get_Item_LimitType_Value(std::string inputString);
int get_Item_ApplyType_Value(std::string inputString);


//\xb8\xf3\xbd\xba\xc5\xcd \xc7\xc1\xb7\xce\xc5䵵 \xc0\xd0\xc0\xbb \xbc\xf6 \xc0ִ\xd9.
int get_Mob_Rank_Value(std::string inputString);
int get_Mob_Type_Value(std::string inputString);
int get_Mob_BattleType_Value(std::string inputString);

int get_Mob_Size_Value(std::string inputString);
int get_Mob_AIFlag_Value(std::string inputString);
int get_Mob_RaceFlag_Value(std::string inputString);
int get_Mob_ImmuneFlag_Value(std::string inputString);

#endif
