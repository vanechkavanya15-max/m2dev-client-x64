#pragma once

// Wymaga zdefiniowania Python.h i PyGILState_STATE przed uzyciem.
// Zostalo to wyizolowane, aby zminimalizowac zaleznosci i umozliwic
// mockowanie w testach bez koniecznosci includowania calego pyconfig.h
#ifndef Py_PYTHON_H
struct PyThreadState;
typedef enum { PyGILState_LOCKED, PyGILState_UNLOCKED } PyGILState_STATE;
extern "C" PyGILState_STATE PyGILState_Ensure(void);
extern "C" void PyGILState_Release(PyGILState_STATE);
#endif

namespace Client::Bridge {

/**
 * @brief RAII dla Global Interpreter Lock (GIL) w srodowisku miedzywatkowym (C++20/C++23).
 *
 * Automatyczne przejmowanie i zwalnianie blokady GIL umozliwia bezpieczne wywolywanie 
 * API Pythona z dowolnego watku roboczego bez modyfikowania wspoldzielonych 
 * plikow bazowych (Zero-Conflict).
 *
 * Klasa jest w 100% self-contained i zabezpieczona przed kopiowaniem
 * oraz przenoszeniem, by zapobiec przypadkowemu wyciekowi blokady.
 */
class PyGILScopeGuard {
public:
    /**
     * @brief Przejmuje blokade GIL i zapisuje poprzedni stan.
     */
    PyGILScopeGuard() noexcept 
        : m_gstate(PyGILState_Ensure()) {
    }

    /**
     * @brief Zwalnia blokade GIL i przywraca stan zapisany w konstruktorze.
     */
    ~PyGILScopeGuard() noexcept {
        PyGILState_Release(m_gstate);
    }

    // Zakaz kopiowania (Memory Safety)
    PyGILScopeGuard(const PyGILScopeGuard&) = delete;
    PyGILScopeGuard& operator=(const PyGILScopeGuard&) = delete;

    // Zakaz przenoszenia (Semantyka RAII dla locka)
    PyGILScopeGuard(PyGILScopeGuard&&) = delete;
    PyGILScopeGuard& operator=(PyGILScopeGuard&&) = delete;

private:
    PyGILState_STATE m_gstate;
};

} // namespace Client::Bridge
