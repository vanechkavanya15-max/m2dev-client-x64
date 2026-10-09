#pragma once

#include "PyGILScopeGuard.h"
#include <Python.h>
#include <expected>
#include <string_view>
#include <memory>
#include <string>

namespace Client::Bridge {

// Typy bledow dla inwokatora
enum class PyCallError {
    ObjectDeleted,
    MethodNotFound,
    ExecutionFailed,
    WeakRefNotSupported
};

// RAII Deleter dla bezpiecznego zarzadzania pamiecia PyObject
struct PyObjectDeleter {
    void operator()(PyObject* obj) const noexcept {
        if (obj) {
            PyGILScopeGuard gil;
            Py_DECREF(obj);
        }
    }
};

using PyObjectPtr = std::unique_ptr<PyObject, PyObjectDeleter>;

/**
 * @brief Bezpieczny inwokator wywolan zwrotnych (callbacks) Pythona.
 * Wykorzystuje mechanizm slabych referencji (weakref) by zabezpieczyc 
 * C++ przed wolaniem funkcji na usunietym oknie (UI) Pythona.
 */
class PySafeCallbackInvoker {
public:
    /**
     * @brief Inicjuje bezpieczny uchwyt do obiektu Pythona.
     * @param targetObj Surowy wskaznik na obiekt, do ktorego utworzona zostanie slaba referencja.
     */
    explicit PySafeCallbackInvoker(PyObject* targetObj) noexcept : m_weakRef(nullptr) {
        if (targetObj) {
            PyGILScopeGuard gil;
            PyObject* weak = PyWeakref_NewRef(targetObj, nullptr);
            if (weak) {
                m_weakRef.reset(weak);
            } else {
                PyErr_Clear(); // Obiekt moze nie wspierac slabych referencji
            }
        }
    }

    ~PySafeCallbackInvoker() = default;

    PySafeCallbackInvoker(const PySafeCallbackInvoker&) = delete;
    PySafeCallbackInvoker& operator=(const PySafeCallbackInvoker&) = delete;

    PySafeCallbackInvoker(PySafeCallbackInvoker&&) noexcept = default;
    PySafeCallbackInvoker& operator=(PySafeCallbackInvoker&&) noexcept = default;

    /**
     * @brief Weryfikuje, czy docelowy obiekt Pythona wciaz istnieje w pamieci.
     */
    [[nodiscard]] bool IsValid() const noexcept {
        if (!m_weakRef) {
            return false;
        }
        PyGILScopeGuard gil;
        PyObject* obj = PyWeakref_GetObject(m_weakRef.get());
        return (obj != nullptr && obj != Py_None);
    }

    /**
     * @brief Wykonuje metode Pythona z weryfikacja czasu zycia obiektu.
     * @param methodName Nazwa metody do wywolania.
     * @param args Krotka (Tuple) z argumentami (borrowed / zarzadzana przez wolajacego).
     * @return Zarzadzany wskaznik na wynik lub enumerator bledu (PyCallError).
     */
    [[nodiscard]] std::expected<PyObjectPtr, PyCallError> Invoke(std::string_view methodName, PyObject* args = nullptr) const noexcept {
        PyGILScopeGuard gil;
        if (!m_weakRef) {
            return std::unexpected(PyCallError::WeakRefNotSupported);
        }

        // PyWeakref_GetObject returns a borrowed reference. 
        // We acquire a strong reference to keep the object alive during execution.
        PyObject* rawTarget = PyWeakref_GetObject(m_weakRef.get());
        if (!rawTarget || rawTarget == Py_None) {
            return std::unexpected(PyCallError::ObjectDeleted);
        }

        Py_INCREF(rawTarget);
        PyObjectPtr targetObj(rawTarget);

        std::string methodStr{methodName};
        PyObjectPtr method{PyObject_GetAttrString(targetObj.get(), methodStr.c_str())};
        
        if (!method) {
            PyErr_Clear();
            return std::unexpected(PyCallError::MethodNotFound);
        }

        if (!PyCallable_Check(method.get())) {
            return std::unexpected(PyCallError::MethodNotFound);
        }

        PyObject* actualArgs = args;
        PyObjectPtr emptyTuple;
        if (!actualArgs) {
            emptyTuple.reset(PyTuple_New(0));
            actualArgs = emptyTuple.get();
        }

        PyObjectPtr result{PyObject_CallObject(method.get(), actualArgs)};
        if (!result) {
            PyErr_Clear();
            return std::unexpected(PyCallError::ExecutionFailed);
        }

        return result;
    }

private:
    PyObjectPtr m_weakRef;
};

} // namespace Client::Bridge
