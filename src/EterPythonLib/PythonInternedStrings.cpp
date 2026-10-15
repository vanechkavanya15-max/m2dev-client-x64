#include "StdAfx.h"
#include "PythonInternedStrings.h"

namespace UI
{
    PyObject* PythonInternedStrings::OnUpdate = nullptr;
    PyObject* PythonInternedStrings::OnRender = nullptr;
    PyObject* PythonInternedStrings::OnSelectEmptySlot = nullptr;
    PyObject* PythonInternedStrings::OnSelectItemSlot = nullptr;
    PyObject* PythonInternedStrings::OnUnselectEmptySlot = nullptr;
    PyObject* PythonInternedStrings::OnUnselectItemSlot = nullptr;
    PyObject* PythonInternedStrings::OnUseSlot = nullptr;
    PyObject* PythonInternedStrings::OnOverInItem = nullptr;
    PyObject* PythonInternedStrings::OnOverOutItem = nullptr;
    PyObject* PythonInternedStrings::OnPressedSlotButton = nullptr;
    PyObject* PythonInternedStrings::OnMoveWindow = nullptr;
    PyObject* PythonInternedStrings::OnPressEscapeKey = nullptr;
    PyObject* PythonInternedStrings::SetItemData = nullptr;
    PyObject* PythonInternedStrings::RefreshStatus = nullptr;

    void PythonInternedStrings::Initialize()
    {
        if (!Py_IsInitialized())
        {
            return;
        }

        if (OnUpdate != nullptr)
        {
            return;
        }

        OnUpdate = PyUnicode_InternFromString("OnUpdate");
        OnRender = PyUnicode_InternFromString("OnRender");
        OnSelectEmptySlot = PyUnicode_InternFromString("OnSelectEmptySlot");
        OnSelectItemSlot = PyUnicode_InternFromString("OnSelectItemSlot");
        OnUnselectEmptySlot = PyUnicode_InternFromString("OnUnselectEmptySlot");
        OnUnselectItemSlot = PyUnicode_InternFromString("OnUnselectItemSlot");
        OnUseSlot = PyUnicode_InternFromString("OnUseSlot");
        OnOverInItem = PyUnicode_InternFromString("OnOverInItem");
        OnOverOutItem = PyUnicode_InternFromString("OnOverOutItem");
        OnPressedSlotButton = PyUnicode_InternFromString("OnPressedSlotButton");
        OnMoveWindow = PyUnicode_InternFromString("OnMoveWindow");
        OnPressEscapeKey = PyUnicode_InternFromString("OnPressEscapeKey");
        SetItemData = PyUnicode_InternFromString("SetItemData");
        RefreshStatus = PyUnicode_InternFromString("RefreshStatus");
    }

    void PythonInternedStrings::Finalize()
    {
        if (!Py_IsInitialized())
        {
            OnUpdate = nullptr;
            OnRender = nullptr;
            OnSelectEmptySlot = nullptr;
            OnSelectItemSlot = nullptr;
            OnUnselectEmptySlot = nullptr;
            OnUnselectItemSlot = nullptr;
            OnUseSlot = nullptr;
            OnOverInItem = nullptr;
            OnOverOutItem = nullptr;
            OnPressedSlotButton = nullptr;
            OnMoveWindow = nullptr;
            OnPressEscapeKey = nullptr;
            SetItemData = nullptr;
            RefreshStatus = nullptr;
            return;
        }

        Py_CLEAR(OnUpdate);
        Py_CLEAR(OnRender);
        Py_CLEAR(OnSelectEmptySlot);
        Py_CLEAR(OnSelectItemSlot);
        Py_CLEAR(OnUnselectEmptySlot);
        Py_CLEAR(OnUnselectItemSlot);
        Py_CLEAR(OnUseSlot);
        Py_CLEAR(OnOverInItem);
        Py_CLEAR(OnOverOutItem);
        Py_CLEAR(OnPressedSlotButton);
        Py_CLEAR(OnMoveWindow);
        Py_CLEAR(OnPressEscapeKey);
        Py_CLEAR(SetItemData);
        Py_CLEAR(RefreshStatus);
    }

    bool PythonInternedStrings::Call(PyObject* poInstance, PyObject* poMethodName, PyObject* const* args, size_t nargs, PyObject** ppoRet)
    {
        if (!poInstance || !poMethodName)
        {
            return false;
        }

        PyObject* poFunc = PyObject_GetAttr(poInstance, poMethodName);
        if (!poFunc)
        {
            PyErr_Clear();
            return false;
        }

        if (!PyCallable_Check(poFunc))
        {
            Py_DECREF(poFunc);
            return false;
        }

        // Bezpieczny Vectorcall: czysty nargs bez flagi PY_VECTORCALL_ARGUMENTS_OFFSET.
        // Zapobiega to corruptowaniu stosu przez modyfikacje pamieci args[-1].
        PyObject* poRet = PyObject_Vectorcall(poFunc, args, nargs, nullptr);
        Py_DECREF(poFunc);

        if (!poRet)
        {
            PyErr_Print();
            return false;
        }

        if (ppoRet)
        {
            *ppoRet = poRet;
        }
        else
        {
            Py_DECREF(poRet);
        }

        return true;
    }

    bool PythonInternedStrings::Call(PyObject* poInstance, PyObject* poMethodName, PyObject** ppoRet)
    {
        return Call(poInstance, poMethodName, nullptr, 0, ppoRet);
    }

    bool PythonInternedStrings::Call(PyObject* poInstance, PyObject* poMethodName, long arg0, PyObject** ppoRet)
    {
        PyObject* poArg = PyLong_FromLong(arg0);
        if (!poArg)
        {
            PyErr_Clear();
            return false;
        }

        PyObject* args[1] = { poArg };
        bool result = Call(poInstance, poMethodName, args, 1, ppoRet);
        Py_DECREF(poArg);
        return result;
    }

    bool PythonInternedStrings::Call(PyObject* poInstance, PyObject* poMethodName, unsigned long arg0, PyObject** ppoRet)
    {
        PyObject* poArg = PyLong_FromUnsignedLong(arg0);
        if (!poArg)
        {
            PyErr_Clear();
            return false;
        }

        PyObject* args[1] = { poArg };
        bool result = Call(poInstance, poMethodName, args, 1, ppoRet);
        Py_DECREF(poArg);
        return result;
    }

    bool PythonInternedStrings::Call(PyObject* poInstance, PyObject* poMethodName, long arg0, long arg1, PyObject** ppoRet)
    {
        PyObject* poArg0 = PyLong_FromLong(arg0);
        PyObject* poArg1 = PyLong_FromLong(arg1);
        if (!poArg0 || !poArg1)
        {
            Py_XDECREF(poArg0);
            Py_XDECREF(poArg1);
            PyErr_Clear();
            return false;
        }

        PyObject* args[2] = { poArg0, poArg1 };
        bool result = Call(poInstance, poMethodName, args, 2, ppoRet);
        Py_DECREF(poArg0);
        Py_DECREF(poArg1);
        return result;
    }

    bool PythonInternedStrings::CallWithReturn(PyObject* poInstance, PyObject* poMethodName, long* plRet)
    {
        PyObject* poRet = nullptr;
        if (!Call(poInstance, poMethodName, &poRet))
        {
            return false;
        }

        if (poRet)
        {
            if (plRet)
            {
                if (PyNumber_Check(poRet))
                {
                    *plRet = PyLong_AsLong(poRet);
                }
                else if (PyBool_Check(poRet))
                {
                    *plRet = (poRet == Py_True) ? 1 : 0;
                }
                else
                {
                    *plRet = 0;
                }
            }
            Py_DECREF(poRet);
            return true;
        }
        return false;
    }
}
