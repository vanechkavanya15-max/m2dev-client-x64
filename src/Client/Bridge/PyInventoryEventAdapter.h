#pragma once

#include <cstdint>
#include <string_view>
#include <expected>
#include "../../EterBase/PyBridge.h"
#include "../../EterBase/EventBus.h"
#include "../UI/PyBridgeFastCall.h"
#include "../Core/DomainEvents.h"
#include "../Core/EventBus.h"

namespace Client::Bridge
{
    /**
     * @struct InventoryItemSetEvent
     * @brief Zdarzenie aktualizacji lub dodania przedmiotu w konkretnym slocie ekwipunku.
     */
    struct InventoryItemSetEvent : public EterBase::IEvent
    {
        uint8_t windowType{0};
        uint16_t cell{0};
        uint32_t vnum{0};
        uint8_t count{0};
        uint32_t flags{0};

        InventoryItemSetEvent(uint8_t window = 0, uint16_t c = 0, uint32_t v = 0, uint8_t cnt = 1, uint32_t f = 0) noexcept
            : windowType(window), cell(c), vnum(v), count(cnt), flags(f) {}
    };

    /**
     * @struct InventoryItemDelEvent
     * @brief Zdarzenie usuniecia przedmiotu ze wskazanego slotu ekwipunku.
     */
    struct InventoryItemDelEvent : public EterBase::IEvent
    {
        uint8_t windowType{0};
        uint16_t cell{0};

        InventoryItemDelEvent(uint8_t window = 0, uint16_t c = 0) noexcept
            : windowType(window), cell(c) {}
    };

    /**
     * @struct InventoryItemUseEvent
     * @brief Zdarzenie uzycia przedmiotu z ekwipunku.
     */
    struct InventoryItemUseEvent : public EterBase::IEvent
    {
        uint8_t windowType{0};
        uint16_t cell{0};

        InventoryItemUseEvent(uint8_t window = 0, uint16_t c = 0) noexcept
            : windowType(window), cell(c) {}
    };

    /**
     * @struct InventoryItemMoveEvent
     * @brief Zdarzenie przeniesienia przedmiotu miedzy slotami lub oknami.
     */
    struct InventoryItemMoveEvent : public EterBase::IEvent
    {
        uint8_t srcWindow{0};
        uint16_t srcCell{0};
        uint8_t dstWindow{0};
        uint16_t dstCell{0};
        uint8_t count{0};

        InventoryItemMoveEvent(uint8_t sw = 0, uint16_t sc = 0, uint8_t dw = 0, uint16_t dc = 0, uint8_t cnt = 1) noexcept
            : srcWindow(sw), srcCell(sc), dstWindow(dw), dstCell(dc), count(cnt) {}
    };

    /**
     * @enum InventoryAdapterError
     * @brief Kody bledow translacji mostka Pythona dla ekwipunku.
     */
    enum class InventoryAdapterError : uint8_t
    {
        None = 0,
        InvalidHandler,
        InvalidSlot,
        CallFailed,
        NotInitialized
    };

    constexpr std::string_view ToString(InventoryAdapterError err) noexcept
    {
        switch (err)
        {
            case InventoryAdapterError::None: return "None";
            case InventoryAdapterError::InvalidHandler: return "InvalidHandler";
            case InventoryAdapterError::InvalidSlot: return "InvalidSlot";
            case InventoryAdapterError::CallFailed: return "CallFailed";
            case InventoryAdapterError::NotInitialized: return "NotInitialized";
        }
        return "UnknownInventoryAdapterError";
    }

    /**
     * @class PyInventoryEventAdapter
     * @brief Adapter mostka przekazujacy zdarzenia ekwipunku (InventoryEvents) do warstwy Pythona (uiInventory.py).
     * 
     * Eliminuje archaiczny string-coupling i wywolywanie funkcji przez surowe ciagi znakow.
     * Zapewnia ochrone GIL, RAII dla referencji PyObject oraz zgodnosc z C++23 std::expected.
     */
    class PyInventoryEventAdapter
    {
    public:
        /**
         * @brief Konstruktor przyjmujacy obiekt handlera okna ekwipunku w Pythonie.
         */
        explicit PyInventoryEventAdapter(PyObject* pyHandler = nullptr) noexcept
            : m_pyHandler(pyHandler, true)
        {
        }

        ~PyInventoryEventAdapter() noexcept = default;

        // Disallow copy, allow move
        PyInventoryEventAdapter(const PyInventoryEventAdapter&) = delete;
        PyInventoryEventAdapter& operator=(const PyInventoryEventAdapter&) = delete;
        PyInventoryEventAdapter(PyInventoryEventAdapter&&) noexcept = default;
        PyInventoryEventAdapter& operator=(PyInventoryEventAdapter&&) noexcept = default;

        void SetHandler(PyObject* pyHandler) noexcept
        {
            m_pyHandler = PyBridge::PyRef<>(pyHandler, true);
        }

        [[nodiscard]] bool HasValidHandler() const noexcept
        {
            return m_pyHandler.IsValid();
        }

        /**
         * @brief Przekazuje zdarzenie ustawienia przedmiotu do okna Pythona.
         */
        void OnItemSet(const InventoryItemSetEvent& event) noexcept
        {
            if (!m_pyHandler.IsValid()) return;

            PyBridge::PyGILScope gilScope;
            // Wywolanie metody "RefreshItemSlot" lub "OnItemSet" w Pythonie
            auto result = Client::UI::PyFastCall::FastCallMethod(m_pyHandler.Get(), "RefreshItemSlot");
            if (!result.has_value())
            {
                // Fallback na generyczne odswiezenie
                (void)Client::UI::PyFastCall::FastCallMethod(m_pyHandler.Get(), "RefreshStatus");
            }
        }

        /**
         * @brief Przekazuje zdarzenie usuniecia przedmiotu do okna Pythona.
         */
        void OnItemDel(const InventoryItemDelEvent& event) noexcept
        {
            if (!m_pyHandler.IsValid()) return;

            PyBridge::PyGILScope gilScope;
            auto result = Client::UI::PyFastCall::FastCallMethod(m_pyHandler.Get(), "RefreshItemSlot");
            if (!result.has_value())
            {
                (void)Client::UI::PyFastCall::FastCallMethod(m_pyHandler.Get(), "RefreshStatus");
            }
        }

        /**
         * @brief Przekazuje zdarzenie uzycia przedmiotu do okna Pythona.
         */
        void OnItemUse(const InventoryItemUseEvent& event) noexcept
        {
            if (!m_pyHandler.IsValid()) return;

            PyBridge::PyGILScope gilScope;
            (void)Client::UI::PyFastCall::FastCallMethod(m_pyHandler.Get(), "OnUseItem");
        }

        /**
         * @brief Przekazuje zdarzenie przemieszczenia przedmiotu do okna Pythona.
         */
        void OnItemMove(const InventoryItemMoveEvent& event) noexcept
        {
            if (!m_pyHandler.IsValid()) return;

            PyBridge::PyGILScope gilScope;
            auto result = Client::UI::PyFastCall::FastCallMethod(m_pyHandler.Get(), "RefreshItemSlot");
            if (!result.has_value())
            {
                (void)Client::UI::PyFastCall::FastCallMethod(m_pyHandler.Get(), "RefreshStatus");
            }
        }

    private:
        PyBridge::PyRef<> m_pyHandler;
    };
}
