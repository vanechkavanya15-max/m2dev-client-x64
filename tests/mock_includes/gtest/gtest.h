#pragma once

#ifndef DOCTEST_SINGLE_MAIN_ALLOWED
#undef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#undef DOCTEST_CONFIG_IMPLEMENT
#endif

#include "../../doctest.h"
#include <string>
#include <cmath>

namespace testing {

class Test {
public:
    virtual ~Test() = default;
    virtual void SetUp() {}
    virtual void TearDown() {}
};

inline void InitGoogleTest(int* = nullptr, char** = nullptr) {}
inline void InitGoogleTest(int* = nullptr, char*[] = nullptr) {}
inline void InitGoogleTest(int* = nullptr, wchar_t** = nullptr) {}

namespace internal {
    struct AssertionProxy {
        AssertionProxy(bool passed, doctest::assertType::Enum at, const char* file, int line, const char* expr) {
            doctest::detail::ResultBuilder rb(at, file, line, expr);
            rb.setResult(doctest::detail::Result(passed));
            rb.log();
            if (!passed) {
                rb.react();
            }
        }
        template <typename T>
        const AssertionProxy& operator<<(const T&) const noexcept {
            return *this;
        }
    };
} // namespace internal

} // namespace testing

// Makra glowne definiowania testow
#define TEST(test_case_name, test_name) \
    DOCTEST_TEST_CASE(#test_case_name "." #test_name)

#define TEST_F(fixture, test_name) \
    namespace { \
    struct DOCTEST_ANONYMOUS(DOCTEST_ANON_CLASS_) : public fixture { \
        void f(); \
    }; \
    static inline void DOCTEST_ANONYMOUS(DOCTEST_ANON_FUNC_)() { \
        DOCTEST_ANONYMOUS(DOCTEST_ANON_CLASS_) v; \
        v.SetUp(); \
        struct Guard { \
            fixture& ref; \
            ~Guard() { ref.TearDown(); } \
        } guard{v}; \
        v.f(); \
    } \
    DOCTEST_REGISTER_FUNCTION(DOCTEST_EMPTY, DOCTEST_ANONYMOUS(DOCTEST_ANON_FUNC_), #fixture "." #test_name) \
    } \
    inline void DOCTEST_ANONYMOUS(DOCTEST_ANON_CLASS_)::f()

// Asercje EXPECT
#define EXPECT_TRUE(cond) \
    for (int _g_i = 0; _g_i < 1; ++_g_i) \
        ::testing::internal::AssertionProxy(static_cast<bool>(cond), doctest::assertType::DT_CHECK, __FILE__, __LINE__, #cond)

#define EXPECT_FALSE(cond) \
    for (int _g_i = 0; _g_i < 1; ++_g_i) \
        ::testing::internal::AssertionProxy(!static_cast<bool>(cond), doctest::assertType::DT_CHECK_FALSE, __FILE__, __LINE__, "!(" #cond ")")

#define EXPECT_EQ(val1, val2) CHECK_EQ(val1, val2)
#define EXPECT_NE(val1, val2) CHECK_NE(val1, val2)
#define EXPECT_GT(val1, val2) CHECK_GT(val1, val2)
#define EXPECT_GE(val1, val2) CHECK_GE(val1, val2)
#define EXPECT_LT(val1, val2) CHECK_LT(val1, val2)
#define EXPECT_LE(val1, val2) CHECK_LE(val1, val2)
#define EXPECT_NEAR(val1, val2, eps) CHECK(std::abs((val1) - (val2)) <= (eps))
#define EXPECT_STREQ(s1, s2) CHECK(std::string(s1) == std::string(s2))

// Asercje ASSERT
#define ASSERT_TRUE(cond) \
    for (int _g_i = 0; _g_i < 1; ++_g_i) \
        ::testing::internal::AssertionProxy(static_cast<bool>(cond), doctest::assertType::DT_REQUIRE, __FILE__, __LINE__, #cond)

#define ASSERT_FALSE(cond) \
    for (int _g_i = 0; _g_i < 1; ++_g_i) \
        ::testing::internal::AssertionProxy(!static_cast<bool>(cond), doctest::assertType::DT_REQUIRE_FALSE, __FILE__, __LINE__, "!(" #cond ")")

#define ASSERT_EQ(val1, val2) REQUIRE_EQ(val1, val2)
#define ASSERT_NE(val1, val2) REQUIRE_NE(val1, val2)
#define ASSERT_GT(val1, val2) REQUIRE_GT(val1, val2)
#define ASSERT_GE(val1, val2) REQUIRE_GE(val1, val2)
#define ASSERT_LT(val1, val2) REQUIRE_LT(val1, val2)
#define ASSERT_LE(val1, val2) REQUIRE_LE(val1, val2)
#define ASSERT_FLOAT_EQ(val1, val2) REQUIRE(doctest::Approx(static_cast<double>(val1)) == static_cast<double>(val2))

#define RUN_ALL_TESTS() 0

// Neutralizacja lokalnych funkcji main() w plikach testowych
#define GTEST_CONCAT_IMPL(a, b) a##b
#define GTEST_CONCAT(a, b) GTEST_CONCAT_IMPL(a, b)
#define main GTEST_CONCAT(dummy_test_main_unused_, __LINE__)
