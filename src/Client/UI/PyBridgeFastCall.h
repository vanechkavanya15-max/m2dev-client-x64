#pragma once

#include "../../UserInterface/StdAfx.h"
#include <expected>
#include <string_view>
#include <unordered_map>
#include <shared_mutex>
#include <string>
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

    std::expected<PyBridge::PyRef<>, std::string_view> FastCallMethod(PyObject* instance, std::string_view methodName);
}
