#pragma once

#include <expected>
#include <string_view>
#include <unordered_map>
#include <shared_mutex>
#include <string>
#include <optional>
#include <cstdint>
#include <limits>
#include "../../EterBase/PyBridge.h"

namespace Client::UI::PyFastCall
{
    struct StringHash {
        using is_transparent = void;
        size_t operator()(std::string_view sv) const {
            return std::hash<std::string_view>{}(sv);
        }
    };

    class PyStringCache
    {
    public:
        static PyBridge::PyRef<> Get(std::string_view str);
        static void Clear();

    private:
        static std::unordered_map<std::string, PyObject*, StringHash, std::equal_to<>> ms_cache;
        static std::shared_mutex ms_mutex;
    };

    class EmptyTupleCache
    {
    public:
        static PyBridge::PyRef<> Get();
        static void Clear();

    private:
        static PyObject* ms_emptyTuple;
        static std::shared_mutex ms_mutex;
    };

    // ============================================================================
    // Szablon GetArg<T> - bezpieczna ekstrakcja typow z PyObject*
    // ============================================================================

    template <typename T>
    std::optional<T> GetArg(PyObject* item);

    template <>
    inline std::optional<bool> GetArg<bool>(PyObject* item)
    {
        if (!item || !PyBool_Check(item))
        {
            return std::nullopt;
        }
        return (item == Py_True);
    }

    template <>
    inline std::optional<int32_t> GetArg<int32_t>(PyObject* item)
    {
        if (!item || PyBool_Check(item) || !PyLong_Check(item))
        {
            return std::nullopt;
        }
        int overflow = 0;
        long long val = PyLong_AsLongLongAndOverflow(item, &overflow);
        if (overflow != 0 || PyErr_Occurred())
        {
            PyErr_Clear();
            return std::nullopt;
        }
        if (val < static_cast<long long>(std::numeric_limits<int32_t>::min()) ||
            val > static_cast<long long>(std::numeric_limits<int32_t>::max()))
        {
            return std::nullopt;
        }
        return static_cast<int32_t>(val);
    }

    template <>
    inline std::optional<uint32_t> GetArg<uint32_t>(PyObject* item)
    {
        if (!item || PyBool_Check(item) || !PyLong_Check(item))
        {
            return std::nullopt;
        }
        int overflow = 0;
        long long val = PyLong_AsLongLongAndOverflow(item, &overflow);
        if (overflow != 0 || PyErr_Occurred())
        {
            PyErr_Clear();
            return std::nullopt;
        }
        if (val < 0 || val > static_cast<long long>(std::numeric_limits<uint32_t>::max()))
        {
            return std::nullopt;
        }
        return static_cast<uint32_t>(val);
    }

    template <>
    inline std::optional<int64_t> GetArg<int64_t>(PyObject* item)
    {
        if (!item || PyBool_Check(item) || !PyLong_Check(item))
        {
            return std::nullopt;
        }
        int overflow = 0;
        long long val = PyLong_AsLongLongAndOverflow(item, &overflow);
        if (overflow != 0 || PyErr_Occurred())
        {
            PyErr_Clear();
            return std::nullopt;
        }
        return static_cast<int64_t>(val);
    }

    template <>
    inline std::optional<uint64_t> GetArg<uint64_t>(PyObject* item)
    {
        if (!item || PyBool_Check(item) || !PyLong_Check(item))
        {
            return std::nullopt;
        }
        unsigned long long val = PyLong_AsUnsignedLongLong(item);
        if (PyErr_Occurred())
        {
            PyErr_Clear();
            return std::nullopt;
        }
        return static_cast<uint64_t>(val);
    }

#ifdef _MSC_VER
    template <>
    inline std::optional<long> GetArg<long>(PyObject* item)
    {
        auto val = GetArg<int32_t>(item);
        if (!val)
            return std::nullopt;
        return static_cast<long>(*val);
    }

    template <>
    inline std::optional<unsigned long> GetArg<unsigned long>(PyObject* item)
    {
        auto val = GetArg<uint32_t>(item);
        if (!val)
            return std::nullopt;
        return static_cast<unsigned long>(*val);
    }
#endif

    template <>
    inline std::optional<double> GetArg<double>(PyObject* item)
    {
        if (!item || PyBool_Check(item) || !PyFloat_Check(item))
        {
            return std::nullopt;
        }
        double val = PyFloat_AsDouble(item);
        if (PyErr_Occurred())
        {
            PyErr_Clear();
            return std::nullopt;
        }
        return val;
    }

    template <>
    inline std::optional<float> GetArg<float>(PyObject* item)
    {
        if (!item || PyBool_Check(item) || !PyFloat_Check(item))
        {
            return std::nullopt;
        }
        double d = PyFloat_AsDouble(item);
        if (PyErr_Occurred())
        {
            PyErr_Clear();
            return std::nullopt;
        }
        if (d > static_cast<double>(std::numeric_limits<float>::max()) ||
            d < static_cast<double>(-std::numeric_limits<float>::max()))
        {
            return std::nullopt;
        }
        return static_cast<float>(d);
    }

    template <typename T>
    inline std::optional<T> GetArg(PyObject* tuple, Py_ssize_t index)
    {
        if (!tuple || !PyTuple_Check(tuple))
            return std::nullopt;
        if (index < 0 || index >= PyTuple_Size(tuple))
            return std::nullopt;
        PyObject* item = PyTuple_GetItem(tuple, index);
        if (!item)
            return std::nullopt;
        return GetArg<T>(item);
    }

    template <>
    inline std::optional<std::string_view> GetArg<std::string_view>(PyObject* item)
    {
        if (!item || !PyUnicode_Check(item))
        {
            return std::nullopt;
        }
        Py_ssize_t size = 0;
        const char* str = PyUnicode_AsUTF8AndSize(item, &size);
        if (!str || PyErr_Occurred())
        {
            PyErr_Clear();
            return std::nullopt;
        }
        return std::string_view(str, static_cast<size_t>(size));
    }

    template <>
    inline std::optional<std::string> GetArg<std::string>(PyObject* item)
    {
        auto sv = GetArg<std::string_view>(item);
        if (!sv)
        {
            return std::nullopt;
        }
        return std::string(*sv);
    }

    template <>
    inline std::optional<const char*> GetArg<const char*>(PyObject* item)
    {
        if (!item || !PyUnicode_Check(item))
        {
            return std::nullopt;
        }
        const char* str = PyUnicode_AsUTF8(item);
        if (!str || PyErr_Occurred())
        {
            PyErr_Clear();
            return std::nullopt;
        }
        return str;
    }

    template <>
    inline std::optional<PyObject*> GetArg<PyObject*>(PyObject* item)
    {
        if (!item)
        {
            return std::nullopt;
        }
        return item; // Borrowed reference - bez wycieku
    }

    template <>
    inline std::optional<PyBridge::PyRef<>> GetArg<PyBridge::PyRef<>>(PyObject* item)
    {
        if (!item)
        {
            return std::nullopt;
        }
        return PyBridge::PyRef<>(item, true); // INCREF dla bezpieczenstwa RAII
    }

    // ============================================================================
    // CheckArgs - bezpieczna obsluga flagi PY_VECTORCALL_ARGUMENTS_OFFSET
    // ============================================================================

    inline size_t VectorcallArgCount(size_t nargsf) noexcept
    {
#if defined(PyVectorcall_NARGS)
        return static_cast<size_t>(PyVectorcall_NARGS(nargsf));
#elif defined(PY_VECTORCALL_ARGUMENTS_OFFSET)
        return static_cast<size_t>(nargsf & ~PY_VECTORCALL_ARGUMENTS_OFFSET);
#else
        return nargsf;
#endif
    }

    inline bool CheckArgs(size_t nargsf, size_t expectedCount, PyObject* kwnames = nullptr) noexcept
    {
        if (kwnames != nullptr && (!PyTuple_Check(kwnames) || PyTuple_GET_SIZE(kwnames) != 0))
        {
            return false;
        }
        return VectorcallArgCount(nargsf) == expectedCount;
    }

    inline bool CheckArgs(size_t nargsf, size_t minCount, size_t maxCount, PyObject* kwnames = nullptr) noexcept
    {
        if (kwnames != nullptr && (!PyTuple_Check(kwnames) || PyTuple_GET_SIZE(kwnames) != 0))
        {
            return false;
        }
        const size_t count = VectorcallArgCount(nargsf);
        return (count >= minCount && count <= maxCount);
    }

    template <typename... Args>
    inline bool CheckArgs(PyObject* const* args, size_t nargsf, PyObject* kwnames, Args&... outArgs)
    {
        constexpr size_t expectedCount = sizeof...(Args);
        if (!CheckArgs(nargsf, expectedCount, kwnames))
        {
            return false;
        }

        if constexpr (expectedCount == 0)
        {
            return true;
        }
        else
        {
            if (!args)
            {
                return false;
            }

            size_t idx = 0;
            bool success = true;
            auto extractOne = [&](auto& out) {
                if (!success)
                    return;
                auto val = GetArg<std::decay_t<decltype(out)>>(args[idx++]);
                if (val.has_value())
                {
                    out = *val;
                }
                else
                {
                    success = false;
                }
            };

            (extractOne(outArgs), ...);
            return success;
        }
    }

    std::expected<PyBridge::PyRef<>, std::string_view> FastCallMethod(PyObject* instance, std::string_view methodName);
}

namespace PyBridgeFastCall = Client::UI::PyFastCall;

