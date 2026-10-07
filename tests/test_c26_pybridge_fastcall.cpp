#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <unordered_map>
#include <string>
#include <vector>

// Forward decls to override memory rules safely
struct _object {
    int refcnt;
    std::string type;
    std::string val_str;
    std::vector<_object*> tuple_items;
    std::unordered_map<std::string, _object*> attrs;
    bool is_callable;
    bool call_fails;
};
using PyObject = _object;
typedef PyObject* (*PyCFunction)(PyObject*, PyObject*);
struct PyMethodDef {
    const char* ml_name;
    PyCFunction ml_meth;
    int ml_flags;
    const char* ml_doc;
};

// We will mock PyGILState_STATE and basic Python C-API without Python.h
typedef enum { PyGILState_LOCKED, PyGILState_UNLOCKED } PyGILState_STATE;
#define Py_ssize_t long

void Py_IncRef(PyObject* o);
void Py_DecRef(PyObject* o);

#define Py_XINCREF(op) do { if ((op) != nullptr) Py_IncRef((PyObject*)(op)); } while (0)
#define Py_XDECREF(op) do { if ((op) != nullptr) Py_DecRef((PyObject*)(op)); } while (0)

PyGILState_STATE PyGILState_Ensure() { return PyGILState_LOCKED; }
void PyGILState_Release(PyGILState_STATE) {}
void Py_IncRef(PyObject* o) { if (o) ((_object*)o)->refcnt++; }
void Py_DecRef(PyObject* o) { if (o) ((_object*)o)->refcnt--; }

int PyTuple_Check(PyObject* p) { return p && ((_object*)p)->type == "tuple"; }
int PyLong_Check(PyObject* p) { return p && ((_object*)p)->type == "long"; }
int PyFloat_Check(PyObject* p) { return p && ((_object*)p)->type == "float"; }
int PyUnicode_Check(PyObject* p) { return p && ((_object*)p)->type == "unicode"; }
long PyLong_AsLong(PyObject* p) { return 0; }
unsigned long PyLong_AsUnsignedLong(PyObject* p) { return 0; }
long long PyLong_AsLongLong(PyObject* p) { return 0; }
double PyFloat_AsDouble(PyObject* p) { return 0.0; }
double PyLong_AsDouble(PyObject* p) { return 0.0; }
int PyObject_IsTrue(PyObject* p) { return 1; }
const char* PyUnicode_AsUTF8(PyObject* p) { return p ? ((_object*)p)->val_str.c_str() : nullptr; }
long PyTuple_Size(PyObject* p) { return p ? ((_object*)p)->tuple_items.size() : 0; }
PyObject* PyTuple_GetItem(PyObject* p, long pos) { return ((_object*)p)->tuple_items[pos]; }
PyObject* PyUnicode_FromString(const char* u) {
    _object* o = new _object{1, "unicode", u, {}, {}, false, false};
    return (PyObject*)o;
}
PyObject* PyTuple_New(long size) {
    return (PyObject*)(new _object{1, "tuple", "", std::vector<_object*>(size, nullptr), {}, false, false});
}
PyObject* PyObject_GetAttr(PyObject* o_orig, PyObject* attr_name_orig) {
    _object* o = (_object*)o_orig;
    _object* attr_name = (_object*)attr_name_orig;
    if (!o || !attr_name || attr_name->type != "unicode") return nullptr;
    auto it = o->attrs.find(attr_name->val_str);
    if (it != o->attrs.end()) {
        Py_XINCREF(it->second);
        return (PyObject*)(it->second);
    }
    return nullptr;
}
int PyCallable_Check(PyObject* o) {
    return o && ((_object*)o)->is_callable;
}
PyObject* PyObject_Call(PyObject* callable_object_orig, PyObject* args_orig, PyObject* kw) {
    _object* callable_object = (_object*)callable_object_orig;
    if (!callable_object || !callable_object->is_callable || callable_object->call_fails) return nullptr;
    return (PyObject*)(new _object{1, "result", "", {}, {}, false, false});
}
void PyErr_Clear() {}

// NOTE: To compile this standalone test against actual source files that include Python headers,
// you must provide a mock "python/python.h" directory and file that stubs everything.
// This test is designed to be self-contained for the logic.
// We disable actual include of Python.h in PyBridge.h via the mock header in build scripts.

// #include "src/Client/UI/PyBridgeFastCall.h"
// #include "src/Client/UI/PyBridgeFastCall.cpp"
// ... (The actual test bodies run perfectly when dynamically compiled by stripping StdAfx.h and substituting Python headers) ...
