#pragma once

#include <vector>
#include <memory>
#include <string>
#include <optional>
#include <functional>
#include <algorithm>

namespace Client::UI {

struct Point {
    int x = 0;
    int y = 0;
};

struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    [[nodiscard]] bool Contains(const Point& p) const {
        return p.x >= x && p.x < x + width &&
               p.y >= y && p.y < y + height;
    }
};

enum class MouseEventType {
    MouseMove,
    OnClick,
    OnDoubleClick,
    ButtonDown,
    ButtonUp
};

struct MouseEvent {
    MouseEventType type;
    Point position;
};

struct DragDropPayload {
    std::string dataType;
    int dataId = 0;
};

class Window {
public:
    Window(std::string name);
    virtual ~Window() = default;

    // Hierarchy
    void SetParent(Window* parent);
    Window* GetParent() const;
    void AddChild(std::shared_ptr<Window> child);
    void RemoveChild(Window* child);
    const std::vector<std::shared_ptr<Window>>& GetChildren() const;

    // Positioning and sizing
    void SetLocalPosition(int x, int y);
    Point GetLocalPosition() const;
    void SetSize(int width, int height);
    Rect GetLocalRect() const;
    Rect GetGlobalRect() const;
    Point GetGlobalPosition() const;

    // Z-Order
    void BringToFront();
    
    // Visibility and Modal
    void SetVisible(bool visible);
    bool IsVisible() const;
    void SetModal(bool modal);
    bool IsModal() const;

    // Properties
    const std::string& GetName() const { return m_name; }

    // Hit Testing
    Window* HitTest(const Point& globalPos);

    // Event Handlers
    virtual bool OnMouseEvent(const MouseEvent& event);
    virtual bool OnDragDrop(Window* source, const DragDropPayload& payload);

    // Drag and Drop source configuration
    void SetDraggable(bool draggable);
    bool IsDraggable() const;
    void SetDragPayload(const DragDropPayload& payload);
    const std::optional<DragDropPayload>& GetDragPayload() const;

private:
    std::string m_name;
    Window* m_parent = nullptr;
    std::vector<std::shared_ptr<Window>> m_children;

    int m_localX = 0;
    int m_localY = 0;
    int m_width = 0;
    int m_height = 0;

    bool m_isVisible = true;
    bool m_isModal = false;
    bool m_isDraggable = false;

    std::optional<DragDropPayload> m_dragPayload;
};

class WindowManager {
public:
    WindowManager();
    ~WindowManager() = default;

    void SetRootWindow(std::shared_ptr<Window> root);
    std::shared_ptr<Window> GetRootWindow() const;

    // Input Handling
    bool HandleMouseEvent(const MouseEvent& event);
    
    // Returns the current modal window, or nullptr if none
    Window* GetActiveModalWindow() const;

    // Sets active modal window manually (or automatically triggered by UI logic)
    void SetModalWindow(Window* window);
    void ClearModalWindow(Window* window);

private:
    std::shared_ptr<Window> m_root;
    std::vector<Window*> m_modalStack;

    // Drag and Drop state
    Window* m_dragSource = nullptr;
    bool m_isDragging = false;
};

} // namespace Client::UI
