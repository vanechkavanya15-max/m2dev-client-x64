#pragma once
#include <gtest/gtest.h>

namespace testing {

template <typename T>
class NiceMock : public T {
public:
    using T::T;
};

struct AnyMatcher {};
inline const AnyMatcher _;

template <typename T>
struct ReturnAction {
    T value;
    ReturnAction(T v) : value(v) {}
};

template <typename T>
inline ReturnAction<T> Return(T v) {
    return ReturnAction<T>(v);
}

struct MockActionCall {
    template <typename... Args>
    MockActionCall& WillOnce(Args&&...) { return *this; }
    template <typename... Args>
    MockActionCall& WillByDefault(Args&&...) { return *this; }
    template <typename... Args>
    MockActionCall& WillRepeatedly(Args&&...) { return *this; }
};

} // namespace testing

#define MOCK_METHOD(ret, name, args, specs) \
    ret name args specs { return ret{}; }

#define ON_CALL(obj, call) ::testing::MockActionCall()
#define EXPECT_CALL(obj, call) ::testing::MockActionCall()
