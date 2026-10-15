#include "../src/Client/Data/ProtoCSVParserModern.h"
#include <cassert>
#include <iostream>

using namespace Client::Data;
using namespace EterBase;

void TestValidCSV() {
    std::string csv = R"(
# This is a comment
VNUM,NAME,TYPE,SUBTYPE,GOLD
10,Sword,1,2,500
20,Shield,3,4,1000
    )";
    auto result = ProtoCSVParserModern::Parse(csv);
    assert(result.has_value());
    assert(result->size() == 2);
    assert(result.value()[0].vnum == ItemVnum(10));
    assert(result.value()[0].name == "Sword");
    assert(result.value()[0].type == 1);
    assert(result.value()[0].subtype == 2);
    assert(result.value()[0].gold == 500);

    assert(result.value()[1].vnum == ItemVnum(20));
    assert(result.value()[1].name == "Shield");
    assert(result.value()[1].type == 3);
    assert(result.value()[1].subtype == 4);
    assert(result.value()[1].gold == 1000);
    std::cout << "TestValidCSV passed.\n";
}

void TestPermutedColumns() {
    std::string csv = R"(
TYPE,GOLD,NAME,VNUM,SUBTYPE
1,500,Sword,10,2
3,1000,Shield,20,4
    )";
    auto result = ProtoCSVParserModern::Parse(csv);
    assert(result.has_value());
    assert(result->size() == 2);
    assert(result.value()[0].vnum == ItemVnum(10));
    assert(result.value()[0].name == "Sword");
    assert(result.value()[0].type == 1);
    assert(result.value()[0].subtype == 2);
    assert(result.value()[0].gold == 500);
    std::cout << "TestPermutedColumns passed.\n";
}

void TestMissingOptionalColumns() {
    std::string csv = R"(
VNUM,NAME,TYPE
10,Sword,1
    )";
    auto result = ProtoCSVParserModern::Parse(csv);
    assert(result.has_value());
    assert(result->size() == 1);
    assert(result.value()[0].vnum == ItemVnum(10));
    assert(result.value()[0].name == "Sword");
    assert(result.value()[0].type == 1);
    assert(result.value()[0].subtype == 0); // Default
    assert(result.value()[0].gold == 0); // Default
    std::cout << "TestMissingOptionalColumns passed.\n";
}

void TestMissingRequiredColumns() {
    std::string csv = R"(
NAME,TYPE
Sword,1
    )";
    auto result = ProtoCSVParserModern::Parse(csv);
    assert(!result.has_value());
    assert(result.error() == ProtoParseError::InvalidHeader);
    std::cout << "TestMissingRequiredColumns passed.\n";
}

void TestEmptyLinesAndComments() {
    std::string csv = R"(
# Header comment
VNUM,NAME,TYPE,SUBTYPE,GOLD

// another comment
10,Sword,1,2,500


# middle comment
20,Shield,3,4,1000
    )";
    auto result = ProtoCSVParserModern::Parse(csv);
    assert(result.has_value());
    assert(result->size() == 2);
    assert(result.value()[0].vnum == ItemVnum(10));
    assert(result.value()[1].vnum == ItemVnum(20));
    std::cout << "TestEmptyLinesAndComments passed.\n";
}

void TestInvalidData() {
    std::string csv = R"(
VNUM,NAME,TYPE,SUBTYPE,GOLD
invalid,Sword,1,2,500
    )";
    auto result = ProtoCSVParserModern::Parse(csv);
    assert(!result.has_value());
    assert(result.error() == ProtoParseError::ParseError);
    std::cout << "TestInvalidData passed.\n";
}

void TestTabSeparated() {
    std::string csv = "VNUM\tNAME\tTYPE\tSUBTYPE\tGOLD\n"
                      "10\tSword\t1\t2\t500\n";
    auto result = ProtoCSVParserModern::Parse(csv);
    assert(result.has_value());
    assert(result->size() == 1);
    assert(result.value()[0].vnum == ItemVnum(10));
    assert(result.value()[0].name == "Sword");
    std::cout << "TestTabSeparated passed.\n";
}

void TestQuotes() {
    std::string csv = R"(
"VNUM","NAME","TYPE","SUBTYPE","GOLD"
"10","Sword","1","2","500"
    )";
    auto result = ProtoCSVParserModern::Parse(csv);
    assert(result.has_value());
    assert(result->size() == 1);
    assert(result.value()[0].vnum == ItemVnum(10));
    assert(result.value()[0].name == "Sword");
    assert(result.value()[0].gold == 500);
    std::cout << "TestQuotes passed.\n";
}

void TestBitFlagsAndHex() {
    std::string csv = R"(
VNUM,NAME,TYPE,SUBTYPE,GOLD
0x1A,SwordHex,0x1,0x2,0x1F4
    )";
    auto result = ProtoCSVParserModern::Parse(csv);
    assert(result.has_value());
    assert(result->size() == 1);
    assert(result.value()[0].vnum == ItemVnum(0x1A));
    assert(result.value()[0].name == "SwordHex");
    assert(result.value()[0].gold == 500); // 0x1F4 == 500
    std::cout << "TestBitFlagsAndHex passed.\n";
}

int main() {
    TestValidCSV();
    TestPermutedColumns();
    TestMissingOptionalColumns();
    TestMissingRequiredColumns();
    TestEmptyLinesAndComments();
    TestInvalidData();
    TestTabSeparated();
    TestQuotes();
    TestBitFlagsAndHex();
    std::cout << "All tests passed.\n";
    return 0;
}
