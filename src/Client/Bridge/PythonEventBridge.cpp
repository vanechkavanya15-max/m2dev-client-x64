#include "PythonEventBridge.h"
#include <iostream>

// Minimal Mock definitions required for compilation if python.h is empty
// In the real system these are provided by the actual Python C-API headers
#ifndef _MSC_VER
extern "C" {
    typedef ptrdiff_t Py_ssize_t;
    // We assume PyObject is defined in the header or here.
    
    // Function prototypes matching Python C API
    PyObject* PyUnicode_InternFromString(const char* v);
    PyObject* PyObject_VectorcallMethod(PyObject* name, PyObject* const* args, size_t nargsf, PyObject* kwnames);
    void Py_DecRef(PyObject* o);
    PyObject* PyLong_FromUnsignedLong(unsigned long v);
    PyObject* PyBool_FromLong(long v);
}
#endif

namespace Client::Bridge {

    PythonEventBridge::PythonEventBridge(EterBase::EventBus& eventBus) 
        : m_eventBus(eventBus) 
    {
        InitializeInternedStrings();
        RegisterEventHandlers();
    }

    PythonEventBridge::~PythonEventBridge() 
    {
        UnregisterEventHandlers();
        UnbindModule();
        FreeInternedStrings();
    }

    void PythonEventBridge::InitializeInternedStrings() 
    {
        m_internedStrings.OnTargetBoardRefresh = PyUnicode_InternFromString("OnTargetBoardRefresh");
        m_internedStrings.OnMountStateChanged = PyUnicode_InternFromString("OnMountStateChanged");
        m_internedStrings.OnActorDead = PyUnicode_InternFromString("OnActorDead");
        m_internedStrings.OnTextTailVisibilityChanged = PyUnicode_InternFromString("OnTextTailVisibilityChanged");
    }

    void PythonEventBridge::FreeInternedStrings() 
    {
        if (m_internedStrings.OnTargetBoardRefresh) Py_DecRef(m_internedStrings.OnTargetBoardRefresh);
        if (m_internedStrings.OnMountStateChanged) Py_DecRef(m_internedStrings.OnMountStateChanged);
        if (m_internedStrings.OnActorDead) Py_DecRef(m_internedStrings.OnActorDead);
        if (m_internedStrings.OnTextTailVisibilityChanged) Py_DecRef(m_internedStrings.OnTextTailVisibilityChanged);
    }

    std::expected<void, PythonBridgeError> PythonEventBridge::BindModule(PyObject* module) noexcept 
    {
        if (!module) {
            return std::unexpected(PythonBridgeError::InvalidArguments);
        }
        
        m_pythonModule = module;
        return {};
    }

    void PythonEventBridge::UnbindModule() noexcept 
    {
        m_pythonModule = nullptr;
    }

    bool PythonEventBridge::IsBound() const noexcept 
    {
        return m_pythonModule != nullptr;
    }

    void PythonEventBridge::RegisterEventHandlers() 
    {
        auto id1 = m_eventBus.Subscribe<EterBase::TargetBoardRefreshEvent>(
            [this](const EterBase::TargetBoardRefreshEvent& e) { this->OnTargetBoardRefresh(e); }
        );
        m_subscriptions.push_back({
            typeid(EterBase::TargetBoardRefreshEvent), id1,
            [](EterBase::EventBus& bus, uint32_t id) { bus.Unsubscribe<EterBase::TargetBoardRefreshEvent>(id); }
        });
        
        auto id2 = m_eventBus.Subscribe<EterBase::MountStateChangedEvent>(
            [this](const EterBase::MountStateChangedEvent& e) { this->OnMountStateChanged(e); }
        );
        m_subscriptions.push_back({
            typeid(EterBase::MountStateChangedEvent), id2,
            [](EterBase::EventBus& bus, uint32_t id) { bus.Unsubscribe<EterBase::MountStateChangedEvent>(id); }
        });
        
        auto id3 = m_eventBus.Subscribe<EterBase::ActorDeadEvent>(
            [this](const EterBase::ActorDeadEvent& e) { this->OnActorDead(e); }
        );
        m_subscriptions.push_back({
            typeid(EterBase::ActorDeadEvent), id3,
            [](EterBase::EventBus& bus, uint32_t id) { bus.Unsubscribe<EterBase::ActorDeadEvent>(id); }
        });

        auto id4 = m_eventBus.Subscribe<EterBase::TextTailVisibilityChangedEvent>(
            [this](const EterBase::TextTailVisibilityChangedEvent& e) { this->OnTextTailVisibilityChanged(e); }
        );
        m_subscriptions.push_back({
            typeid(EterBase::TextTailVisibilityChangedEvent), id4,
            [](EterBase::EventBus& bus, uint32_t id) { bus.Unsubscribe<EterBase::TextTailVisibilityChangedEvent>(id); }
        });
    }

    void PythonEventBridge::UnregisterEventHandlers() 
    {
        for (const auto& sub : m_subscriptions) {
            sub.unsubscribeFunc(m_eventBus, sub.id);
        }
        m_subscriptions.clear();
    }

    void PythonEventBridge::OnTargetBoardRefresh(const EterBase::TargetBoardRefreshEvent& event) 
    {
        if (!IsBound()) return;
        
        PyObject* args[2];
        args[0] = m_pythonModule;
        args[1] = PyLong_FromUnsignedLong(event.targetId);
        
        PyObject* result = PyObject_VectorcallMethod(m_internedStrings.OnTargetBoardRefresh, args, 2, nullptr);
        
        if (result) {
            Py_DecRef(result);
        }
        if (args[1]) Py_DecRef(args[1]);
    }

    void PythonEventBridge::OnMountStateChanged(const EterBase::MountStateChangedEvent& event) 
    {
        if (!IsBound()) return;

        PyObject* args[4];
        args[0] = m_pythonModule;
        args[1] = PyLong_FromUnsignedLong(event.charId);
        args[2] = PyLong_FromUnsignedLong(event.mountVnum);
        args[3] = PyLong_FromUnsignedLong(event.pos);
        
        PyObject* result = PyObject_VectorcallMethod(m_internedStrings.OnMountStateChanged, args, 4, nullptr);
        
        if (result) {
            Py_DecRef(result);
        }
        if (args[1]) Py_DecRef(args[1]);
        if (args[2]) Py_DecRef(args[2]);
        if (args[3]) Py_DecRef(args[3]);
    }

    void PythonEventBridge::OnActorDead(const EterBase::ActorDeadEvent& event) 
    {
        if (!IsBound()) return;

        PyObject* args[3];
        args[0] = m_pythonModule;
        args[1] = PyLong_FromUnsignedLong(event.entityId);
        args[2] = PyLong_FromUnsignedLong(event.vid);

        PyObject* result = PyObject_VectorcallMethod(m_internedStrings.OnActorDead, args, 3, nullptr);
        
        if (result) {
            Py_DecRef(result);
        }
        if (args[1]) Py_DecRef(args[1]);
        if (args[2]) Py_DecRef(args[2]);
    }

    void PythonEventBridge::OnTextTailVisibilityChanged(const EterBase::TextTailVisibilityChangedEvent& event) 
    {
        if (!IsBound()) return;

        PyObject* args[3];
        args[0] = m_pythonModule;
        args[1] = PyLong_FromUnsignedLong(event.entityId);
        args[2] = PyBool_FromLong(event.isVisible ? 1 : 0);

        PyObject* result = PyObject_VectorcallMethod(m_internedStrings.OnTextTailVisibilityChanged, args, 3, nullptr);
        
        if (result) {
            Py_DecRef(result);
        }
        if (args[1]) Py_DecRef(args[1]);
        if (args[2]) Py_DecRef(args[2]);
    }

} // namespace Client::Bridge
