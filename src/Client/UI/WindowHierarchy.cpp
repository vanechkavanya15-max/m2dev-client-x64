#include "WindowHierarchy.h"
#include <utility>

namespace Client::UI {

// ---------------------------------------------------------
// Window Implementation
// ---------------------------------------------------------

Window::Window(std::string name)
    : m_name(std::move(name))
{
}

void Window::SetParent(Window* parent)
{
    m_parent = parent;
}

Window* Window::GetParent() const
{
    return m_parent;
}

void Window::AddChild(std::shared_ptr<Window> child)
{
    if (!child) return;

    // Remove from old parent if any
    if (child->GetParent()) {
        child->GetParent()->RemoveChild(child.get());
    }

    child->SetParent(this);
    m_children.push_back(std::move(child));
}

void Window::RemoveChild(Window* child)
{
    if (!child) return;

    auto it = std::find_if(m_children.begin(), m_children.end(),
        [child](const std::shared_ptr<Window>& ptr) {
            return ptr.get() == child;
        });

    if (it != m_children.end()) {
        (*it)->SetParent(nullptr);
        m_children.erase(it);
    }
}

const std::vector<std::shared_ptr<Window>>& Window::GetChildren() const
{
    return m_children;
}

void Window::SetLocalPosition(int x, int y)
{
    m_localX = x;
    m_localY = y;
}

Point Window::GetLocalPosition() const
{
    return {m_localX, m_localY};
}

void Window::SetSize(int width, int height)
{
    m_width = width;
    m_height = height;
}

Rect Window::GetLocalRect() const
{
    return {m_localX, m_localY, m_width, m_height};
}

Point Window::GetGlobalPosition() const
{
    Point pos{m_localX, m_localY};
    Window* current = m_parent;
    while (current) {
        pos.x += current->GetLocalPosition().x;
        pos.y += current->GetLocalPosition().y;
        current = current->GetParent();
    }
    return pos;
}

Rect Window::GetGlobalRect() const
{
    Point globalPos = GetGlobalPosition();
    return {globalPos.x, globalPos.y, m_width, m_height};
}

void Window::BringToFront()
{
    if (!m_parent) return;

    auto& siblings = m_parent->m_children;
    auto it = std::find_if(siblings.begin(), siblings.end(),
        [this](const std::shared_ptr<Window>& ptr) {
            return ptr.get() == this;
        });

    if (it != siblings.end() && it != siblings.end() - 1) {
        // Move to the back of the vector (which represents top of Z-order)
        std::shared_ptr<Window> self = *it;
        siblings.erase(it);
        siblings.push_back(self);
    }
}

void Window::SetVisible(bool visible)
{
    m_isVisible = visible;
}

bool Window::IsVisible() const
{
    return m_isVisible;
}

void Window::SetModal(bool modal)
{
    m_isModal = modal;
}

bool Window::IsModal() const
{
    return m_isModal;
}

Window* Window::HitTest(const Point& globalPos)
{
    if (!m_isVisible) return nullptr;

    // Iterate in reverse for Z-order (topmost first)
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        Window* hit = (*it)->HitTest(globalPos);
        if (hit) {
            return hit;
        }
    }

    // Check self after children
    if (GetGlobalRect().Contains(globalPos)) {
        return this;
    }

    return nullptr;
}

bool Window::OnMouseEvent(const MouseEvent& /*event*/)
{
    // Base implementation just returns false (unhandled)
    return false;
}

bool Window::OnDragDrop(Window* /*source*/, const DragDropPayload& /*payload*/)
{
    // Base implementation
    return false;
}

void Window::SetDraggable(bool draggable)
{
    m_isDraggable = draggable;
}

bool Window::IsDraggable() const
{
    return m_isDraggable;
}

void Window::SetDragPayload(const DragDropPayload& payload)
{
    m_dragPayload = payload;
}

const std::optional<DragDropPayload>& Window::GetDragPayload() const
{
    return m_dragPayload;
}


// ---------------------------------------------------------
// WindowManager Implementation
// ---------------------------------------------------------

WindowManager::WindowManager() = default;

void WindowManager::SetRootWindow(std::shared_ptr<Window> root)
{
    m_root = std::move(root);
}

std::shared_ptr<Window> WindowManager::GetRootWindow() const
{
    return m_root;
}

Window* WindowManager::GetActiveModalWindow() const
{
    if (!m_modalStack.empty()) {
        return m_modalStack.back();
    }
    return nullptr;
}

void WindowManager::SetModalWindow(Window* window)
{
    if (!window) return;
    
    // Add to stack if not already at the top
    if (m_modalStack.empty() || m_modalStack.back() != window) {
        // If it's already in the stack somewhere else, we could theoretically remove it,
        // but for simplicity we just append.
        m_modalStack.push_back(window);
    }
}

void WindowManager::ClearModalWindow(Window* window)
{
    if (!window) return;

    auto it = std::find(m_modalStack.begin(), m_modalStack.end(), window);
    if (it != m_modalStack.end()) {
        m_modalStack.erase(it);
    }
}

bool WindowManager::HandleMouseEvent(const MouseEvent& event)
{
    if (!m_root) return false;

    Window* target = m_root->HitTest(event.position);
    Window* activeModal = GetActiveModalWindow();

    // Modal check
    if (activeModal) {
        // If there's an active modal window, only events targeting the modal window 
        // or its children are processed.
        bool isModalChild = false;
        Window* current = target;
        while (current) {
            if (current == activeModal) {
                isModalChild = true;
                break;
            }
            current = current->GetParent();
        }

        if (!isModalChild) {
            // Event is blocked by modal window
            return false;
        }
    }

    if (!target) {
        // If drag is active but we released over nothing, cancel drag
        if (m_isDragging && event.type == MouseEventType::ButtonUp) {
            m_isDragging = false;
            m_dragSource = nullptr;
        }
        return false;
    }

    bool handled = false;

    // Drag and Drop logic
    if (event.type == MouseEventType::ButtonDown) {
        if (target->IsDraggable() && target->GetDragPayload().has_value()) {
            m_dragSource = target;
        }
    }
    else if (event.type == MouseEventType::MouseMove) {
        if (m_dragSource) {
            m_isDragging = true;
        }
    }
    else if (event.type == MouseEventType::ButtonUp) {
        if (m_isDragging && m_dragSource && m_dragSource != target) {
            // Trigger drag-drop
            handled = target->OnDragDrop(m_dragSource, m_dragSource->GetDragPayload().value());
        }
        
        m_isDragging = false;
        m_dragSource = nullptr;
    }

    // Normal event routing
    if (!handled) {
        Window* current = target;
        while (current) {
            if (current->OnMouseEvent(event)) {
                handled = true;
                break;
            }
            current = current->GetParent();
        }
    }

    return handled;
}

} // namespace Client::UI
