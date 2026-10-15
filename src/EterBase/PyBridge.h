#pragma once

/**
 * @file PyBridge.h
 * @brief Nowoczesny, bezpieczny most C++23 RAII dla Pythona 3.14.
 * 
 * Zapewnia:
 * 1. PyRef<T> - inteligentny wskaznik z automatycznym licznikiem referencji INCREF/DECREF.
 * 2. PyGILScope - RAII blokada GIL (Python Global Interpreter Lock) dla watkow roboczych.
 * 3. Type-safe ekstrakcje i budowanie krotek bez zawodnych makr C-API.
 */

#ifdef _DEBUG
	#undef _DEBUG
	#include <python/python.h>
	#define _DEBUG
#else
	#include <python/python.h>
#endif

#ifdef BYTE
#undef BYTE
#endif

#include <string>
#include <string_view>
#include <concepts>
#include <type_traits>
#include <utility>
#include <cstdint>

namespace PyBridge
{
    /**
     * @brief Blokada GIL z automatycznym zwalnianiem w destruktorze (RAII).
     */
    class PyGILScope
    {
    public:
        PyGILScope() noexcept
            : m_state(PyGILState_Ensure())
        {
        }

        ~PyGILScope() noexcept
        {
            PyGILState_Release(m_state);
        }

        PyGILScope(const PyGILScope&) = delete;
        PyGILScope& operator=(const PyGILScope&) = delete;
        PyGILScope(PyGILScope&&) = delete;
        PyGILScope& operator=(PyGILScope&&) = delete;

    private:
        PyGILState_STATE m_state;
    };

    /**
     * @brief Inteligentny wskaznik RAII zarzadzajacy czasem zycia obiektow PyObject*.
     */
    template <typename T = PyObject>
    class PyRef
    {
    public:
        constexpr PyRef() noexcept : m_ptr(nullptr) {}

        explicit PyRef(T* ptr, bool borrow = false) noexcept : m_ptr(ptr)
        {
            if (borrow && m_ptr)
            {
                Py_XINCREF(reinterpret_cast<PyObject*>(m_ptr));
            }
        }

        PyRef(const PyRef& other) noexcept : m_ptr(other.m_ptr)
        {
            if (m_ptr)
            {
                Py_XINCREF(reinterpret_cast<PyObject*>(m_ptr));
            }
        }

        PyRef(PyRef&& other) noexcept : m_ptr(other.m_ptr)
        {
            other.m_ptr = nullptr;
        }

        ~PyRef() noexcept
        {
            Reset();
        }

        PyRef& operator=(const PyRef& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                m_ptr = other.m_ptr;
                if (m_ptr)
                {
                    Py_XINCREF(reinterpret_cast<PyObject*>(m_ptr));
                }
            }
            return *this;
        }

        PyRef& operator=(PyRef&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                m_ptr = other.m_ptr;
                other.m_ptr = nullptr;
            }
            return *this;
        }

        void Reset(T* newPtr = nullptr) noexcept
        {
            if (m_ptr)
            {
                Py_XDECREF(reinterpret_cast<PyObject*>(m_ptr));
            }
            m_ptr = newPtr;
        }

        [[nodiscard]] T* Release() noexcept
        {
            T* temp = m_ptr;
            m_ptr = nullptr;
            return temp;
        }

        [[nodiscard]] T* Get() const noexcept { return m_ptr; }
        [[nodiscard]] bool IsValid() const noexcept { return m_ptr != nullptr; }

        explicit operator bool() const noexcept { return m_ptr != nullptr; }
        T* operator->() const noexcept { return m_ptr; }
        T& operator*() const noexcept { return *m_ptr; }

    private:
        T* m_ptr;
    };

    /**
     * @brief Bezpieczna ekstrakcja wartosci ze wskazanego indeksu krotki Pythona.
     */
    inline bool GetTupleItem(PyObject* poTuple, Py_ssize_t index, int32_t& outVal) noexcept
    {
        if (!poTuple || !PyTuple_Check(poTuple) || index >= PyTuple_Size(poTuple)) return false;
        PyObject* item = PyTuple_GetItem(poTuple, index);
        if (!item || !PyLong_Check(item)) return false;
        outVal = static_cast<int32_t>(PyLong_AsLong(item));
        return true;
    }

    inline bool GetTupleItem(PyObject* poTuple, Py_ssize_t index, uint32_t& outVal) noexcept
    {
        if (!poTuple || !PyTuple_Check(poTuple) || index >= PyTuple_Size(poTuple)) return false;
        PyObject* item = PyTuple_GetItem(poTuple, index);
        if (!item || !PyLong_Check(item)) return false;
        outVal = static_cast<uint32_t>(PyLong_AsUnsignedLong(item));
        return true;
    }

    inline bool GetTupleItem(PyObject* poTuple, Py_ssize_t index, int64_t& outVal) noexcept
    {
        if (!poTuple || !PyTuple_Check(poTuple) || index >= PyTuple_Size(poTuple)) return false;
        PyObject* item = PyTuple_GetItem(poTuple, index);
        if (!item || !PyLong_Check(item)) return false;
        outVal = PyLong_AsLongLong(item);
        return true;
    }

    inline bool GetTupleItem(PyObject* poTuple, Py_ssize_t index, float& outVal) noexcept
    {
        if (!poTuple || !PyTuple_Check(poTuple) || index >= PyTuple_Size(poTuple)) return false;
        PyObject* item = PyTuple_GetItem(poTuple, index);
        if (!item) return false;
        if (PyFloat_Check(item)) {
            outVal = static_cast<float>(PyFloat_AsDouble(item));
            return true;
        }
        if (PyLong_Check(item)) {
            outVal = static_cast<float>(PyLong_AsDouble(item));
            return true;
        }
        return false;
    }

    inline bool GetTupleItem(PyObject* poTuple, Py_ssize_t index, bool& outVal) noexcept
    {
        if (!poTuple || !PyTuple_Check(poTuple) || index >= PyTuple_Size(poTuple)) return false;
        PyObject* item = PyTuple_GetItem(poTuple, index);
        if (!item) return false;
        outVal = PyObject_IsTrue(item) != 0;
        return true;
    }

    inline bool GetTupleItem(PyObject* poTuple, Py_ssize_t index, std::string& outVal) noexcept
    {
        if (!poTuple || !PyTuple_Check(poTuple) || index >= PyTuple_Size(poTuple)) return false;
        PyObject* item = PyTuple_GetItem(poTuple, index);
        if (!item || !PyUnicode_Check(item)) return false;
        const char* str = PyUnicode_AsUTF8(item);
        if (!str) return false;
        outVal = str;
        return true;
    }

    inline bool GetTupleItem(PyObject* poTuple, Py_ssize_t index, const char*& outVal) noexcept
    {
        if (!poTuple || !PyTuple_Check(poTuple) || index >= PyTuple_Size(poTuple)) return false;
        PyObject* item = PyTuple_GetItem(poTuple, index);
        if (!item || !PyUnicode_Check(item)) return false;
        outVal = PyUnicode_AsUTF8(item);
        return outVal != nullptr;
    }

    inline bool GetTupleItem(PyObject* poTuple, Py_ssize_t index, PyObject*& outVal) noexcept
    {
        if (!poTuple || !PyTuple_Check(poTuple) || index >= PyTuple_Size(poTuple)) return false;
        outVal = PyTuple_GetItem(poTuple, index);
        return outVal != nullptr;
    }

    /**
     * @brief Type-safe ekstrakcja wielu argumentow naraz z krotki Pythona.
     */
    template <typename... Args>
    [[nodiscard]] inline bool ExtractArgs(PyObject* poArgs, Args&... outArgs) noexcept
    {
        if (!poArgs || !PyTuple_Check(poArgs))
            return false;

        const Py_ssize_t expectedCount = sizeof...(Args);
        if (PyTuple_Size(poArgs) != expectedCount)
            return false;

        Py_ssize_t idx = 0;
        return (GetTupleItem(poArgs, idx++, outArgs) && ...);
    }
}
