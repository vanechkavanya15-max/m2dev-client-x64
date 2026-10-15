#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "Client/Core/TypeSanitization.h"
#include <limits>

using namespace Client::Core;

TEST_CASE("TypeSanitization: SafeCast") {
    SUBCASE("Valid casts within range") {
        auto val1 = SafeCast<uint32_t>(static_cast<uint16_t>(500));
        CHECK(val1.has_value());
        CHECK(val1.value() == 500);

        auto val2 = SafeCast<int32_t>(static_cast<uint16_t>(32767));
        CHECK(val2.has_value());
        CHECK(val2.value() == 32767);
    }

    SUBCASE("Invalid casts (overflow/truncation)") {
        // Overflow: uint32_t to uint16_t
        auto overflow_u16 = SafeCast<uint16_t>(static_cast<uint32_t>(70000));
        CHECK_FALSE(overflow_u16.has_value());

        // Underflow: negative int32_t to uint32_t
        auto underflow_u32 = SafeCast<uint32_t>(static_cast<int32_t>(-1));
        CHECK_FALSE(underflow_u32.has_value());

        // Overflow: int64_t to uint32_t
        auto overflow_u32_from_i64 = SafeCast<uint32_t>(static_cast<int64_t>(5000000000LL));
        CHECK_FALSE(overflow_u32_from_i64.has_value());
    }
}

TEST_CASE("TypeSanitization: IsValidVnum") {
    CHECK(IsValidVnum(1));
    CHECK(IsValidVnum(9999999));
    CHECK_FALSE(IsValidVnum(0));
    CHECK_FALSE(IsValidVnum(10000000));
}

TEST_CASE("TypeSanitization: IsValidSlot") {
    CHECK(IsValidSlot(0));
    CHECK(IsValidSlot(179));
    CHECK_FALSE(IsValidSlot(180));
    CHECK_FALSE(IsValidSlot(200));

    // Custom max slots
    CHECK(IsValidSlot(49, 50));
    CHECK_FALSE(IsValidSlot(50, 50));
}

TEST_CASE("TypeSanitization: IsValidVID") {
    CHECK(IsValidVID(1));
    CHECK(IsValidVID(0xFFFFFFFF));
    CHECK_FALSE(IsValidVID(0));
}

TEST_CASE("TypeSanitization: Converters to StrongTypes") {
    SUBCASE("ToSlotIndex") {
        auto validSlot = ToSlotIndex(5);
        CHECK(validSlot.has_value());
        CHECK(validSlot.value().get() == 5);

        auto invalidSlot = ToSlotIndex(180);
        CHECK_FALSE(invalidSlot.has_value());
    }

    SUBCASE("ToItemVnum") {
        auto validVnum = ToItemVnum(12345);
        CHECK(validVnum.has_value());
        CHECK(validVnum.value().get() == 12345);

        auto invalidVnum = ToItemVnum(0);
        CHECK_FALSE(invalidVnum.has_value());
    }

    SUBCASE("ToEntityVid") {
        auto validVid = ToEntityVid(42);
        CHECK(validVid.has_value());
        CHECK(validVid.value().get() == 42);

        auto invalidVid = ToEntityVid(0);
        CHECK_FALSE(invalidVid.has_value());
    }
}
