#pragma once

#include "../../EterBase/Result.h"
#include <python/python.h>
#include <string_view>
#include <type_traits>
#include <concepts>
#include <cstdint>
#include <optional>
#include <span>

/**
 * @file PyArgumentPacker.h
 * @brief Szybka konwersja typow C++ na obiekty PyObject bez zbednych alokacji (Zero-Allocation).
 * 
 * Zastepuje powolne i alokujace Py_BuildValue w warstwie pomostowej C++ <-> Python.
 * W pelni zaimplementowane w C++20/C++23 przy uzyciu std::expected (Result) oraz concepts.
 */

namespace Client::Bridge {

class PyArgumentPacker {
public:
    // ============================================================================
    // Pack dla pojedynczych wartosci
    // ============================================================================

    template <typename T>
    [[nodiscard]] static auto Pack(T&& value) noexcept -> EterBase::Result<PyObject*, std::string_view> {
        using DecayT = std::decay_t<T>;
        PyObject* result = nullptr;

        if constexpr (std::is_same_v<DecayT, bool>) {
            result = PyBool_FromLong(value ? 1 : 0);
        }
        else if constexpr (std::integral<DecayT>) {
            if constexpr (std::is_unsigned_v<DecayT>) {
                if constexpr (sizeof(DecayT) > sizeof(unsigned long)) {
                    result = PyLong_FromUnsignedLongLong(static_cast<unsigned long long>(value));
                } else {
                    result = PyLong_FromUnsignedLong(static_cast<unsigned long>(value));
                }
            } else {
                if constexpr (sizeof(DecayT) > sizeof(long)) {
                    result = PyLong_FromLongLong(static_cast<long long>(value));
                } else {
                    result = PyLong_FromLong(static_cast<long>(value));
                }
            }
        }
        else if constexpr (std::floating_point<DecayT>) {
            result = PyFloat_FromDouble(static_cast<double>(value));
        }
        else if constexpr (std::is_convertible_v<T, std::string_view>) {
            std::string_view sv = value;
            result = PyUnicode_FromStringAndSize(sv.data(), sv.size());
        }
        else if constexpr (std::is_same_v<DecayT, std::nullptr_t>) {
            Py_INCREF(Py_None);
            result = Py_None;
        }
        else {
            // Compile-time weryfikacja (C++20/23 idiom for static_assert in branches)
            static_assert(std::is_same_v<T, void>, "Unsupported type for PyArgumentPacker::Pack");
        }

        if (!result) {
            return EterBase::MakeError("Failed to pack argument");
        }

        return result;
    }

    // ============================================================================
    // Packowanie do krotki (Tuple) z weryfikacja
    // ============================================================================
    
    template <typename... Args>
    [[nodiscard]] static auto PackTuple(Args&&... args) noexcept -> EterBase::Result<PyObject*, std::string_view> {
        constexpr std::size_t numArgs = sizeof...(Args);
        
        PyObject* tuple = PyTuple_New(numArgs);
        if (!tuple) {
            return EterBase::MakeError("Failed to create tuple");
        }

        if constexpr (numArgs > 0) {
            if (!PackIntoTuple<0>(tuple, std::forward<Args>(args)...)) {
                Py_DECREF(tuple);
                return EterBase::MakeError("Failed to pack tuple arguments");
            }
        }

        return tuple;
    }

private:
    template <std::size_t Index, typename T, typename... Rest>
    static bool PackIntoTuple(PyObject* tuple, T&& value, Rest&&... rest) noexcept {
        auto result = Pack(std::forward<T>(value));
        if (!result.has_value()) {
            return false;
        }

        // PyTuple_SetItem przejmuje referencje, wiec nie robimy Py_DECREF na sukcesie
        if (PyTuple_SetItem(tuple, Index, result.value()) != 0) {
            Py_DECREF(result.value());
            return false;
        }

        if constexpr (sizeof...(Rest) > 0) {
            return PackIntoTuple<Index + 1>(tuple, std::forward<Rest>(rest)...);
        } else {
            return true;
        }
    }
};

} // namespace Client::Bridge
