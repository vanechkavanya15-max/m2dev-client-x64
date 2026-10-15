#pragma once

#include <memory>
#include <string_view>
#include <expected>
#include <span>
#include <vector>
#include <typeindex>
#include <utility>
#include "EterBase/EventBus.h"

// Forward declaration of Python C-API structures
extern "C" {
    struct _object;
    typedef _object PyObject;
}

namespace Client::Bridge {

    /**
     * @brief Error types for Python Event Bridge operations
     */
    enum class PythonBridgeError {
        PythonNotInitialized,
        MethodNotFound,
        ExecutionFailed,
        InvalidArguments,
        MemoryError
    };

    /**
     * @brief Interface for routing C++ events to Python callbacks
     */
    class IPythonEventBridge {
    public:
        virtual ~IPythonEventBridge() = default;

        /**
         * @brief Bind Python object that contains callback methods
         * @param module Instance of Python class or module
         * @return std::expected<void, PythonBridgeError> Success or error
         */
        virtual std::expected<void, PythonBridgeError> BindModule(PyObject* module) noexcept = 0;

        /**
         * @brief Unbind current Python module safely
         */
        virtual void UnbindModule() noexcept = 0;

        /**
         * @brief Check if module is currently bound
         * @return true if a Python module is active
         */
        virtual bool IsBound() const noexcept = 0;
    };

    /**
     * @brief Core implementation mapping C++ Events to Python via fast vector calls
     */
    class PythonEventBridge final : public IPythonEventBridge {
    public:
        explicit PythonEventBridge(EterBase::EventBus& eventBus);
        ~PythonEventBridge() override;

        PythonEventBridge(const PythonEventBridge&) = delete;
        PythonEventBridge& operator=(const PythonEventBridge&) = delete;

        std::expected<void, PythonBridgeError> BindModule(PyObject* module) noexcept override;
        void UnbindModule() noexcept override;
        bool IsBound() const noexcept override;

    private:
        void RegisterEventHandlers();
        void UnregisterEventHandlers();
        
        void OnTargetBoardRefresh(const EterBase::TargetBoardRefreshEvent& event);
        void OnMountStateChanged(const EterBase::MountStateChangedEvent& event);
        void OnActorDead(const EterBase::ActorDeadEvent& event);
        void OnTextTailVisibilityChanged(const EterBase::TextTailVisibilityChangedEvent& event);

        EterBase::EventBus& m_eventBus;
        PyObject* m_pythonModule{nullptr};
        
        // Map containing event types and their specific subscription IDs
        struct Subscription {
            std::type_index eventType;
            uint32_t id;
            void (*unsubscribeFunc)(EterBase::EventBus&, uint32_t);
        };
        std::vector<Subscription> m_subscriptions;
        
        // Interned Python strings for optimal vector calls
        struct InternedStrings {
            PyObject* OnTargetBoardRefresh;
            PyObject* OnMountStateChanged;
            PyObject* OnActorDead;
            PyObject* OnTextTailVisibilityChanged;
        };
        InternedStrings m_internedStrings{};
        
        void InitializeInternedStrings();
        void FreeInternedStrings();
    };

} // namespace Client::Bridge
