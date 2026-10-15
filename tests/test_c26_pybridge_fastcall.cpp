#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include <python/python.h>
#include "Client/UI/PyBridgeFastCall.h"
#include <limits>
#include <string>
#include <string_view>
#include <cmath>

TEST_CASE("PyBridgeFastCall::GetArg - int32_t") {
    // 1. Valid values
    PyObject* pyPos = PyLong_FromLong(123456);
    auto resPos = PyBridgeFastCall::GetArg<int32_t>(pyPos);
    CHECK(resPos.has_value());
    CHECK(resPos.value() == 123456);
    Py_DECREF(pyPos);

    PyObject* pyNeg = PyLong_FromLong(-123456);
    auto resNeg = PyBridgeFastCall::GetArg<int32_t>(pyNeg);
    CHECK(resNeg.has_value());
    CHECK(resNeg.value() == -123456);
    Py_DECREF(pyNeg);

    PyObject* pyMax = PyLong_FromLong(std::numeric_limits<int32_t>::max());
    auto resMax = PyBridgeFastCall::GetArg<int32_t>(pyMax);
    CHECK(resMax.has_value());
    CHECK(resMax.value() == std::numeric_limits<int32_t>::max());
    Py_DECREF(pyMax);

    PyObject* pyMin = PyLong_FromLong(std::numeric_limits<int32_t>::min());
    auto resMin = PyBridgeFastCall::GetArg<int32_t>(pyMin);
    CHECK(resMin.has_value());
    CHECK(resMin.value() == std::numeric_limits<int32_t>::min());
    Py_DECREF(pyMin);

    // 2. Overflow & Underflow -> std::nullopt
    PyObject* pyOverflow = PyLong_FromLongLong(static_cast<long long>(std::numeric_limits<int32_t>::max()) + 1LL);
    auto resOverflow = PyBridgeFastCall::GetArg<int32_t>(pyOverflow);
    CHECK_FALSE(resOverflow.has_value());
    Py_DECREF(pyOverflow);

    PyObject* pyUnderflow = PyLong_FromLongLong(static_cast<long long>(std::numeric_limits<int32_t>::min()) - 1LL);
    auto resUnderflow = PyBridgeFastCall::GetArg<int32_t>(pyUnderflow);
    CHECK_FALSE(resUnderflow.has_value());
    Py_DECREF(pyUnderflow);

    // 3. Invalid types -> std::nullopt
    PyObject* pyFloat = PyFloat_FromDouble(12.34);
    auto resFloatInvalid = PyBridgeFastCall::GetArg<int32_t>(pyFloat);
    CHECK_FALSE(resFloatInvalid.has_value());
    Py_DECREF(pyFloat);

    PyObject* pyStr = PyUnicode_FromString("123456");
    auto resStrInvalid = PyBridgeFastCall::GetArg<int32_t>(pyStr);
    CHECK_FALSE(resStrInvalid.has_value());
    Py_DECREF(pyStr);

    // 4. Bool behavior: True/False must NOT be accepted as int32_t!
    auto resTrueAsInt = PyBridgeFastCall::GetArg<int32_t>(Py_True);
    CHECK_FALSE(resTrueAsInt.has_value());

    auto resFalseAsInt = PyBridgeFastCall::GetArg<int32_t>(Py_False);
    CHECK_FALSE(resFalseAsInt.has_value());

    // 5. Null pointer
    auto resNull = PyBridgeFastCall::GetArg<int32_t>(nullptr);
    CHECK_FALSE(resNull.has_value());
}

TEST_CASE("PyBridgeFastCall::GetArg - uint32_t") {
    // 1. Valid values
    PyObject* pyZero = PyLong_FromUnsignedLong(0U);
    auto resZero = PyBridgeFastCall::GetArg<uint32_t>(pyZero);
    CHECK(resZero.has_value());
    CHECK(resZero.value() == 0U);
    Py_DECREF(pyZero);

    PyObject* pyVal = PyLong_FromUnsignedLong(3000000000U);
    auto resVal = PyBridgeFastCall::GetArg<uint32_t>(pyVal);
    CHECK(resVal.has_value());
    CHECK(resVal.value() == 3000000000U);
    Py_DECREF(pyVal);

    PyObject* pyMax = PyLong_FromUnsignedLong(std::numeric_limits<uint32_t>::max());
    auto resMax = PyBridgeFastCall::GetArg<uint32_t>(pyMax);
    CHECK(resMax.has_value());
    CHECK(resMax.value() == std::numeric_limits<uint32_t>::max());
    Py_DECREF(pyMax);

    // 2. Negative value (underflow/negative for unsigned) -> std::nullopt
    PyObject* pyNeg = PyLong_FromLong(-1);
    auto resNeg = PyBridgeFastCall::GetArg<uint32_t>(pyNeg);
    CHECK_FALSE(resNeg.has_value());
    Py_DECREF(pyNeg);

    // 3. Overflow above UINT32_MAX -> std::nullopt
    PyObject* pyOverflow = PyLong_FromLongLong(static_cast<long long>(std::numeric_limits<uint32_t>::max()) + 1LL);
    auto resOverflow = PyBridgeFastCall::GetArg<uint32_t>(pyOverflow);
    CHECK_FALSE(resOverflow.has_value());
    Py_DECREF(pyOverflow);

    // 4. Invalid types & bool behavior
    PyObject* pyFloat = PyFloat_FromDouble(100.0);
    auto resFloatInvalid = PyBridgeFastCall::GetArg<uint32_t>(pyFloat);
    CHECK_FALSE(resFloatInvalid.has_value());
    Py_DECREF(pyFloat);

    auto resTrueAsUint = PyBridgeFastCall::GetArg<uint32_t>(Py_True);
    CHECK_FALSE(resTrueAsUint.has_value());

    auto resFalseAsUint = PyBridgeFastCall::GetArg<uint32_t>(Py_False);
    CHECK_FALSE(resFalseAsUint.has_value());
}

TEST_CASE("PyBridgeFastCall::GetArg - int64_t") {
    // 1. Valid values
    int64_t expectedMax = std::numeric_limits<int64_t>::max();
    PyObject* pyMax = PyLong_FromLongLong(expectedMax);
    auto resMax = PyBridgeFastCall::GetArg<int64_t>(pyMax);
    CHECK(resMax.has_value());
    CHECK(resMax.value() == expectedMax);
    Py_DECREF(pyMax);

    int64_t expectedMin = std::numeric_limits<int64_t>::min();
    PyObject* pyMin = PyLong_FromLongLong(expectedMin);
    auto resMin = PyBridgeFastCall::GetArg<int64_t>(pyMin);
    CHECK(resMin.has_value());
    CHECK(resMin.value() == expectedMin);
    Py_DECREF(pyMin);

    // 2. Huge overflow (exceeding 64 bits) -> std::nullopt
    PyObject* pyHuge = PyLong_FromString("999999999999999999999999999999999999999999999", nullptr, 10);
    if (pyHuge) {
        auto resHuge = PyBridgeFastCall::GetArg<int64_t>(pyHuge);
        CHECK_FALSE(resHuge.has_value());
        Py_DECREF(pyHuge);
    }

    // 3. Invalid types & bool behavior
    PyObject* pyStr = PyUnicode_FromString("1234567890123");
    auto resStrInvalid = PyBridgeFastCall::GetArg<int64_t>(pyStr);
    CHECK_FALSE(resStrInvalid.has_value());
    Py_DECREF(pyStr);

    auto resTrueAsInt64 = PyBridgeFastCall::GetArg<int64_t>(Py_True);
    CHECK_FALSE(resTrueAsInt64.has_value());

    auto resFalseAsInt64 = PyBridgeFastCall::GetArg<int64_t>(Py_False);
    CHECK_FALSE(resFalseAsInt64.has_value());
}

TEST_CASE("PyBridgeFastCall::GetArg - float") {
    // 1. Valid values
    PyObject* pyFloat = PyFloat_FromDouble(3.14159265);
    auto resFloat = PyBridgeFastCall::GetArg<float>(pyFloat);
    CHECK(resFloat.has_value());
    CHECK(doctest::Approx(resFloat.value()).epsilon(0.0001f) == 3.14159265f);
    Py_DECREF(pyFloat);

    // 2. Overflow (exceeding float max ~3.4e38) -> std::nullopt
    PyObject* pyOverflow = PyFloat_FromDouble(1e39);
    auto resOverflow = PyBridgeFastCall::GetArg<float>(pyOverflow);
    CHECK_FALSE(resOverflow.has_value());
    Py_DECREF(pyOverflow);

    PyObject* pyUnderflow = PyFloat_FromDouble(-1e39);
    auto resUnderflow = PyBridgeFastCall::GetArg<float>(pyUnderflow);
    CHECK_FALSE(resUnderflow.has_value());
    Py_DECREF(pyUnderflow);

    // 3. Invalid types
    PyObject* pyInt = PyLong_FromLong(42);
    auto resIntInvalid = PyBridgeFastCall::GetArg<float>(pyInt);
    CHECK_FALSE(resIntInvalid.has_value());
    Py_DECREF(pyInt);

    PyObject* pyStr = PyUnicode_FromString("3.14");
    auto resStrInvalid = PyBridgeFastCall::GetArg<float>(pyStr);
    CHECK_FALSE(resStrInvalid.has_value());
    Py_DECREF(pyStr);

    auto resTrueAsFloat = PyBridgeFastCall::GetArg<float>(Py_True);
    CHECK_FALSE(resTrueAsFloat.has_value());
}

TEST_CASE("PyBridgeFastCall::GetArg - double") {
    // 1. Valid values
    double expected = 123456789.987654321;
    PyObject* pyDouble = PyFloat_FromDouble(expected);
    auto resDouble = PyBridgeFastCall::GetArg<double>(pyDouble);
    CHECK(resDouble.has_value());
    CHECK(doctest::Approx(resDouble.value()).epsilon(0.000000001) == expected);
    Py_DECREF(pyDouble);

    // 2. Invalid types
    PyObject* pyStr = PyUnicode_FromString("123.456");
    auto resStrInvalid = PyBridgeFastCall::GetArg<double>(pyStr);
    CHECK_FALSE(resStrInvalid.has_value());
    Py_DECREF(pyStr);

    auto resTrueAsDouble = PyBridgeFastCall::GetArg<double>(Py_True);
    CHECK_FALSE(resTrueAsDouble.has_value());
}

TEST_CASE("PyBridgeFastCall::GetArg - bool behavior") {
    // 1. Valid bool values
    auto resTrue = PyBridgeFastCall::GetArg<bool>(Py_True);
    CHECK(resTrue.has_value());
    CHECK(resTrue.value() == true);

    auto resFalse = PyBridgeFastCall::GetArg<bool>(Py_False);
    CHECK(resFalse.has_value());
    CHECK(resFalse.value() == false);

    // 2. Strict bool check: integers 1 and 0 are NOT accepted as bool!
    PyObject* pyOne = PyLong_FromLong(1);
    auto resOne = PyBridgeFastCall::GetArg<bool>(pyOne);
    CHECK_FALSE(resOne.has_value());
    Py_DECREF(pyOne);

    PyObject* pyZero = PyLong_FromLong(0);
    auto resZero = PyBridgeFastCall::GetArg<bool>(pyZero);
    CHECK_FALSE(resZero.has_value());
    Py_DECREF(pyZero);

    // 3. Strings and other types are NOT bool
    PyObject* pyStr = PyUnicode_FromString("True");
    auto resStr = PyBridgeFastCall::GetArg<bool>(pyStr);
    CHECK_FALSE(resStr.has_value());
    Py_DECREF(pyStr);
}

TEST_CASE("PyBridgeFastCall::GetArg - std::string and std::string_view") {
    const std::string text = "Metin2Client2026_FastCall";
    PyObject* pyStr = PyUnicode_FromString(text.c_str());
    REQUIRE(pyStr != nullptr);

    // 1. std::string
    auto resStr = PyBridgeFastCall::GetArg<std::string>(pyStr);
    CHECK(resStr.has_value());
    CHECK(resStr.value() == text);

    // 2. std::string_view
    auto resSv = PyBridgeFastCall::GetArg<std::string_view>(pyStr);
    CHECK(resSv.has_value());
    CHECK(resSv.value() == text);

    // 3. Empty string
    PyObject* pyEmpty = PyUnicode_FromString("");
    auto resEmpty = PyBridgeFastCall::GetArg<std::string>(pyEmpty);
    CHECK(resEmpty.has_value());
    CHECK(resEmpty.value().empty());
    Py_DECREF(pyEmpty);

    // 4. Invalid types
    PyObject* pyInt = PyLong_FromLong(999);
    auto resIntStr = PyBridgeFastCall::GetArg<std::string>(pyInt);
    CHECK_FALSE(resIntStr.has_value());

    auto resIntSv = PyBridgeFastCall::GetArg<std::string_view>(pyInt);
    CHECK_FALSE(resIntSv.has_value());
    Py_DECREF(pyInt);

    auto resTrueStr = PyBridgeFastCall::GetArg<std::string>(Py_True);
    CHECK_FALSE(resTrueStr.has_value());

    auto resTrueSv = PyBridgeFastCall::GetArg<std::string_view>(Py_True);
    CHECK_FALSE(resTrueSv.has_value());

    Py_DECREF(pyStr);
}

TEST_CASE("PyBridgeFastCall::GetArg - Tuple indexing and edge cases") {
    PyObject* tuple = PyTuple_New(4);
    REQUIRE(tuple != nullptr);

    PyTuple_SetItem(tuple, 0, PyLong_FromLong(42));
    PyTuple_SetItem(tuple, 1, PyUnicode_FromString("Alpha"));
    Py_INCREF(Py_True);
    PyTuple_SetItem(tuple, 2, Py_True);
    PyTuple_SetItem(tuple, 3, PyFloat_FromDouble(2.71828));

    // Valid indexing
    auto arg0 = PyBridgeFastCall::GetArg<int32_t>(tuple, 0);
    CHECK(arg0.has_value());
    CHECK(arg0.value() == 42);

    auto arg1 = PyBridgeFastCall::GetArg<std::string>(tuple, 1);
    CHECK(arg1.has_value());
    CHECK(arg1.value() == "Alpha");

    auto arg2 = PyBridgeFastCall::GetArg<bool>(tuple, 2);
    CHECK(arg2.has_value());
    CHECK(arg2.value() == true);

    auto arg3 = PyBridgeFastCall::GetArg<double>(tuple, 3);
    CHECK(arg3.has_value());
    CHECK(doctest::Approx(arg3.value()).epsilon(0.0001) == 2.71828);

    // Out of bounds
    auto outOfBounds1 = PyBridgeFastCall::GetArg<int32_t>(tuple, 4);
    CHECK_FALSE(outOfBounds1.has_value());

    auto outOfBounds2 = PyBridgeFastCall::GetArg<int32_t>(tuple, -1);
    CHECK_FALSE(outOfBounds2.has_value());

    auto outOfBounds3 = PyBridgeFastCall::GetArg<int32_t>(tuple, 100);
    CHECK_FALSE(outOfBounds3.has_value());

    // Type mismatch on valid index
    auto mismatch1 = PyBridgeFastCall::GetArg<int32_t>(tuple, 1); // index 1 is string
    CHECK_FALSE(mismatch1.has_value());

    auto mismatch2 = PyBridgeFastCall::GetArg<std::string>(tuple, 0); // index 0 is int
    CHECK_FALSE(mismatch2.has_value());

    // Null tuple
    auto nullTupleRes = PyBridgeFastCall::GetArg<int32_t>(nullptr, 0);
    CHECK_FALSE(nullTupleRes.has_value());

    // Non-tuple object passed as tuple
    PyObject* notATuple = PyLong_FromLong(123);
    auto notTupleRes = PyBridgeFastCall::GetArg<int32_t>(notATuple, 0);
    CHECK_FALSE(notTupleRes.has_value());
    Py_DECREF(notATuple);

    Py_DECREF(tuple);
}

TEST_CASE("PyBridgeFastCall::CheckArgs - Vectorcall arguments and offset flag") {
    // 1. Zwykle zliczanie argumentow
    CHECK(Client::UI::PyFastCall::CheckArgs(0, 0));
    CHECK(Client::UI::PyFastCall::CheckArgs(2, 2));
    CHECK_FALSE(Client::UI::PyFastCall::CheckArgs(2, 3));
    CHECK_FALSE(Client::UI::PyFastCall::CheckArgs(3, 2));

    // 2. Obsluga flagi PY_VECTORCALL_ARGUMENTS_OFFSET
#if defined(PY_VECTORCALL_ARGUMENTS_OFFSET)
    size_t nargsfWithFlag = 2 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    CHECK(Client::UI::PyFastCall::VectorcallArgCount(nargsfWithFlag) == 2);
    CHECK(Client::UI::PyFastCall::CheckArgs(nargsfWithFlag, 2));
    CHECK_FALSE(Client::UI::PyFastCall::CheckArgs(nargsfWithFlag, 1));
    CHECK_FALSE(Client::UI::PyFastCall::CheckArgs(nargsfWithFlag, 3));

    size_t zeroWithFlag = 0 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    CHECK(Client::UI::PyFastCall::VectorcallArgCount(zeroWithFlag) == 0);
    CHECK(Client::UI::PyFastCall::CheckArgs(zeroWithFlag, 0));
#endif

    // 3. Sprawdzenie zakresu [minCount, maxCount]
    CHECK(Client::UI::PyFastCall::CheckArgs(2, 1, 3));
    CHECK(Client::UI::PyFastCall::CheckArgs(1, 1, 3));
    CHECK(Client::UI::PyFastCall::CheckArgs(3, 1, 3));
    CHECK_FALSE(Client::UI::PyFastCall::CheckArgs(0, 1, 3));
    CHECK_FALSE(Client::UI::PyFastCall::CheckArgs(4, 1, 3));

    // 4. Odrzucanie kwnames jesli funkcja ich nie oczekuje
    PyObject* emptyKw = PyTuple_New(0);
    CHECK(Client::UI::PyFastCall::CheckArgs(2, 2, emptyKw));
    Py_DECREF(emptyKw);

    PyObject* nonKwTuple = PyTuple_New(1);
    PyTuple_SetItem(nonKwTuple, 0, PyUnicode_FromString("test"));
    CHECK_FALSE(Client::UI::PyFastCall::CheckArgs(2, 2, nonKwTuple));
    Py_DECREF(nonKwTuple);
}

TEST_CASE("PyBridgeFastCall::CheckArgs - Extraction of arguments") {
    PyObject* pyInt = PyLong_FromLong(12345);
    PyObject* pyStr = PyUnicode_FromString("HelloVectorcall");
    PyObject* pyBool = Py_True;
    Py_INCREF(pyBool);

    PyObject* args[3] = { pyInt, pyStr, pyBool };
    size_t nargsf = 3;
#if defined(PY_VECTORCALL_ARGUMENTS_OFFSET)
    nargsf |= PY_VECTORCALL_ARGUMENTS_OFFSET;
#endif

    int32_t outInt = 0;
    std::string_view outStr;
    bool outBool = false;

    bool checkOk = Client::UI::PyFastCall::CheckArgs(args, nargsf, nullptr, outInt, outStr, outBool);
    CHECK(checkOk);
    CHECK(outInt == 12345);
    CHECK(outStr == "HelloVectorcall");
    CHECK(outBool == true);

    // Niepoprawna liczba argumentow (za malo lub za duzo)
    int32_t dummyInt = 0;
    CHECK_FALSE(Client::UI::PyFastCall::CheckArgs(args, nargsf, nullptr, dummyInt));

    // Niezgodnosc typow (np. oczekiwany int jako string)
    int32_t failInt = 0;
    int32_t failInt2 = 0;
    bool failBool = false;
    CHECK_FALSE(Client::UI::PyFastCall::CheckArgs(args, nargsf, nullptr, failInt, failInt2, failBool));

    Py_DECREF(pyInt);
    Py_DECREF(pyStr);
    Py_DECREF(pyBool);
}

int main(int argc, char** argv) {
    Py_NoSiteFlag = 1;
    Py_FrozenFlag = 1;
    Py_InitializeEx(0);

    doctest::Context context;
    context.applyCommandLine(argc, argv);
    int res = context.run();

    Client::UI::PyFastCall::PyStringCache::Clear();
    Client::UI::PyFastCall::EmptyTupleCache::Clear();

    Py_Finalize();
    return res;
}
