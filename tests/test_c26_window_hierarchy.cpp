#include <iostream>
#include <cassert>
#include <memory>
#include "../src/Client/UI/WindowHierarchy.h"

using namespace Client::UI;

class TestWindow : public Window {
public:
    TestWindow(std::string name) : Window(std::move(name)) {}

    bool OnMouseEvent(const MouseEvent& event) override {
        lastEvent = event.type;
        return eventHandled;
    }

    bool OnDragDrop(Window* source, const DragDropPayload& payload) override {
        lastDropPayload = payload;
        return true;
    }

    std::optional<MouseEventType> lastEvent;
    std::optional<DragDropPayload> lastDropPayload;
    bool eventHandled = true;
};

void TestPositioning() {
    auto root = std::make_shared<TestWindow>("Root");
    root->SetLocalPosition(0, 0);
    root->SetSize(800, 600);

    auto child1 = std::make_shared<TestWindow>("Child1");
    child1->SetLocalPosition(100, 50);
    child1->SetSize(200, 200);
    root->AddChild(child1);

    auto child2 = std::make_shared<TestWindow>("Child2");
    child2->SetLocalPosition(10, 20);
    child2->SetSize(50, 50);
    child1->AddChild(child2);

    Point globalPos = child2->GetGlobalPosition();
    assert(globalPos.x == 110);
    assert(globalPos.y == 70);

    Rect globalRect = child2->GetGlobalRect();
    assert(globalRect.x == 110);
    assert(globalRect.y == 70);
    assert(globalRect.width == 50);
    assert(globalRect.height == 50);

    std::cout << "TestPositioning passed.\n";
}

void TestZOrder() {
    auto root = std::make_shared<TestWindow>("Root");
    root->SetLocalPosition(0, 0);
    root->SetSize(800, 600);

    auto child1 = std::make_shared<TestWindow>("Child1");
    child1->SetLocalPosition(100, 100);
    child1->SetSize(200, 200);
    root->AddChild(child1);

    auto child2 = std::make_shared<TestWindow>("Child2");
    child2->SetLocalPosition(150, 150); // Overlaps child1
    child2->SetSize(200, 200);
    root->AddChild(child2);

    // Initial HitTest: child2 was added last, should be on top
    Window* hit = root->HitTest({160, 160});
    assert(hit == child2.get());

    // Bring child1 to front
    child1->BringToFront();
    
    // Now HitTest should return child1
    hit = root->HitTest({160, 160});
    assert(hit == child1.get());

    std::cout << "TestZOrder passed.\n";
}

void TestMouseEventsRouting() {
    WindowManager manager;
    auto root = std::make_shared<TestWindow>("Root");
    root->SetLocalPosition(0, 0);
    root->SetSize(800, 600);
    manager.SetRootWindow(root);

    auto child1 = std::make_shared<TestWindow>("Child1");
    child1->SetLocalPosition(100, 100);
    child1->SetSize(200, 200);
    root->AddChild(child1);

    // Click on child1
    MouseEvent clickEvent{MouseEventType::OnClick, {150, 150}};
    bool handled = manager.HandleMouseEvent(clickEvent);
    
    assert(handled == true);
    assert(child1->lastEvent.has_value());
    assert(child1->lastEvent.value() == MouseEventType::OnClick);
    assert(!root->lastEvent.has_value()); // child1 handled it

    // Click on root (outside child1)
    MouseEvent clickRootEvent{MouseEventType::OnClick, {50, 50}};
    handled = manager.HandleMouseEvent(clickRootEvent);

    assert(handled == true);
    assert(root->lastEvent.has_value());
    assert(root->lastEvent.value() == MouseEventType::OnClick);

    std::cout << "TestMouseEventsRouting passed.\n";
}

void TestModalBlocking() {
    WindowManager manager;
    auto root = std::make_shared<TestWindow>("Root");
    root->SetLocalPosition(0, 0);
    root->SetSize(800, 600);
    manager.SetRootWindow(root);

    auto backgroundChild = std::make_shared<TestWindow>("Background");
    backgroundChild->SetLocalPosition(100, 100);
    backgroundChild->SetSize(200, 200);
    root->AddChild(backgroundChild);

    auto modalWindow = std::make_shared<TestWindow>("Modal");
    modalWindow->SetLocalPosition(400, 300);
    modalWindow->SetSize(100, 100);
    modalWindow->SetModal(true);
    root->AddChild(modalWindow);

    manager.SetModalWindow(modalWindow.get());

    // Click on background child (should be blocked)
    MouseEvent clickBg{MouseEventType::OnClick, {150, 150}};
    bool handled = manager.HandleMouseEvent(clickBg);
    assert(handled == false);
    assert(!backgroundChild->lastEvent.has_value());

    // Click on modal window (should go through)
    MouseEvent clickModal{MouseEventType::OnClick, {450, 350}};
    handled = manager.HandleMouseEvent(clickModal);
    assert(handled == true);
    assert(modalWindow->lastEvent.has_value());
    assert(modalWindow->lastEvent.value() == MouseEventType::OnClick);

    manager.ClearModalWindow(modalWindow.get());
    
    // Now background child should receive events again
    handled = manager.HandleMouseEvent(clickBg);
    assert(handled == true);
    assert(backgroundChild->lastEvent.has_value());

    std::cout << "TestModalBlocking passed.\n";
}

void TestDragAndDrop() {
    WindowManager manager;
    auto root = std::make_shared<TestWindow>("Root");
    root->SetLocalPosition(0, 0);
    root->SetSize(800, 600);
    manager.SetRootWindow(root);

    auto source = std::make_shared<TestWindow>("Source");
    source->SetLocalPosition(100, 100);
    source->SetSize(50, 50);
    source->SetDraggable(true);
    source->SetDragPayload({"ITEM_SLOT", 42});
    root->AddChild(source);

    auto target = std::make_shared<TestWindow>("Target");
    target->SetLocalPosition(300, 100);
    target->SetSize(50, 50);
    root->AddChild(target);

    // 1. Mouse down on source
    manager.HandleMouseEvent({MouseEventType::ButtonDown, {125, 125}});
    
    // 2. Mouse move (start dragging)
    manager.HandleMouseEvent({MouseEventType::MouseMove, {150, 125}});
    
    // 3. Mouse move over target
    manager.HandleMouseEvent({MouseEventType::MouseMove, {325, 125}});
    
    // 4. Mouse up on target (drop)
    manager.HandleMouseEvent({MouseEventType::ButtonUp, {325, 125}});

    assert(target->lastDropPayload.has_value());
    assert(target->lastDropPayload.value().dataType == "ITEM_SLOT");
    assert(target->lastDropPayload.value().dataId == 42);

    std::cout << "TestDragAndDrop passed.\n";
}

int main() {
    TestPositioning();
    TestZOrder();
    TestMouseEventsRouting();
    TestModalBlocking();
    TestDragAndDrop();

    std::cout << "All WindowHierarchy tests passed successfully.\n";
    return 0;
}
