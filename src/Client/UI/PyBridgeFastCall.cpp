#include "PyBridgeFastCall.h"

namespace Client::UI::PyFastCall
{
    std::unordered_map<std::string, PyObject*, StringHash, std::equal_to<>> PyStringCache::ms_cache;
    std::shared_mutex PyStringCache::ms_mutex;

    PyBridge::PyRef<> PyStringCache::Get(std::string_view str)
    {
        {
            std::shared_lock<std::shared_mutex> lock(ms_mutex);
            if (auto it = ms_cache.find(str); it != ms_cache.end())
            {
                return PyBridge::PyRef<>(it->second, true); // Borrow reference to maintain valid PyRef behavior downstream
            }
        }

        std::unique_lock<std::shared_mutex> lock(ms_mutex);
        if (auto it = ms_cache.find(str); it != ms_cache.end())
        {
            return PyBridge::PyRef<>(it->second, true);
        }

        std::string s(str);
        PyObject* newStrObj = PyUnicode_FromString(s.c_str());
        if (newStrObj)
        {
            ms_cache[s] = newStrObj;
            return PyBridge::PyRef<>(newStrObj, true);
        }
        
        PyErr_Clear();
        return PyBridge::PyRef<>();
    }

    void PyStringCache::Clear()
    {
        std::unique_lock<std::shared_mutex> lock(ms_mutex);
        for (auto& pair : ms_cache)
        {
            Py_XDECREF(pair.second);
        }
        ms_cache.clear();
    }

    PyObject* EmptyTupleCache::ms_emptyTuple = nullptr;
    std::shared_mutex EmptyTupleCache::ms_mutex;

    PyBridge::PyRef<> EmptyTupleCache::Get()
    {
        {
            std::shared_lock<std::shared_mutex> lock(ms_mutex);
            if (ms_emptyTuple != nullptr)
            {
                return PyBridge::PyRef<>(ms_emptyTuple, true);
            }
        }

        std::unique_lock<std::shared_mutex> lock(ms_mutex);
        if (ms_emptyTuple == nullptr)
        {
            ms_emptyTuple = PyTuple_New(0);
        }
        return PyBridge::PyRef<>(ms_emptyTuple, true);
    }

    void EmptyTupleCache::Clear()
    {
        std::unique_lock<std::shared_mutex> lock(ms_mutex);
        if (ms_emptyTuple)
        {
            Py_XDECREF(ms_emptyTuple);
            ms_emptyTuple = nullptr;
        }
    }

    std::expected<PyBridge::PyRef<>, std::string_view> FastCallMethod(PyObject* instance, std::string_view methodName)
    {
        if (!instance)
        {
            return std::unexpected("Instance is null");
        }

        PyBridge::PyRef<> pyMethodName = PyStringCache::Get(methodName);
        if (!pyMethodName.IsValid())
        {
            return std::unexpected("Failed to create Python string for method name");
        }

        PyBridge::PyRef<> callable(PyObject_GetAttr(instance, pyMethodName.Get()));
        if (!callable.IsValid())
        {
            PyErr_Clear();
            return std::unexpected("Method not found on instance");
        }

        if (!PyCallable_Check(callable.Get()))
        {
            return std::unexpected("Attribute is not callable");
        }

        PyBridge::PyRef<> emptyTuple = EmptyTupleCache::Get();
        if (!emptyTuple.IsValid())
        {
            PyErr_Clear();
            return std::unexpected("Failed to create empty tuple");
        }

        PyBridge::PyRef<> result(PyObject_Call(callable.Get(), emptyTuple.Get(), nullptr));
        if (!result.IsValid())
        {
            PyErr_Clear();
            return std::unexpected("Method call failed");
        }

        return result;
    }
}
