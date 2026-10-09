#pragma once

#ifdef _DEBUG
    #undef _DEBUG
    #include <python/python.h>
    #define _DEBUG
#else
    #include <python/python.h>
#endif

namespace UI
{
    class PythonInternedStrings
    {
    public:
        static void Initialize();
        static void Finalize();

        // Zinternowane wskazniki PyObject* dla kluczowych eventow i metod UI
        static PyObject* OnUpdate;
        static PyObject* OnRender;
        static PyObject* OnSelectEmptySlot;
        static PyObject* OnSelectItemSlot;
        static PyObject* OnUnselectEmptySlot;
        static PyObject* OnUnselectItemSlot;
        static PyObject* OnUseSlot;
        static PyObject* OnOverInItem;
        static PyObject* OnOverOutItem;
        static PyObject* OnPressedSlotButton;
        static PyObject* OnMoveWindow;
        static PyObject* OnPressEscapeKey;
        static PyObject* SetItemData;
        static PyObject* RefreshStatus;

        // Bezpieczne metody pomocnicze Vectorcall
        static bool Call(PyObject* poInstance, PyObject* poMethodName, PyObject** ppoRet = nullptr);
        static bool Call(PyObject* poInstance, PyObject* poMethodName, long arg0, PyObject** ppoRet = nullptr);
        static bool Call(PyObject* poInstance, PyObject* poMethodName, unsigned long arg0, PyObject** ppoRet = nullptr);
        static bool Call(PyObject* poInstance, PyObject* poMethodName, long arg0, long arg1, PyObject** ppoRet = nullptr);
        static bool Call(PyObject* poInstance, PyObject* poMethodName, PyObject* const* args, size_t nargs, PyObject** ppoRet = nullptr);
        static bool CallWithReturn(PyObject* poInstance, PyObject* poMethodName, long* plRet);
    };
}
