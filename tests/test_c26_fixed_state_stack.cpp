#include "../src/EterLib/Render/FixedStateStack.h"
#include <iostream>
#include <cassert>
#include <string>
#include <memory>
#include <vector>

// -----------------------------------------------------------------------------
// Test helper for tracking leaks
// -----------------------------------------------------------------------------
struct Trackable
{
    static int s_instanceCount;
    int m_value;

    Trackable(int value = 0) : m_value(value)
    {
        ++s_instanceCount;
    }

    Trackable(const Trackable& other) : m_value(other.m_value)
    {
        ++s_instanceCount;
    }

    Trackable(Trackable&& other) noexcept : m_value(other.m_value)
    {
        other.m_value = 0;
        ++s_instanceCount;
    }

    Trackable& operator=(const Trackable& other)
    {
        if (this != &other)
        {
            m_value = other.m_value;
        }
        return *this;
    }

    Trackable& operator=(Trackable&& other) noexcept
    {
        if (this != &other)
        {
            m_value = other.m_value;
            other.m_value = 0;
        }
        return *this;
    }

    ~Trackable()
    {
        --s_instanceCount;
    }

    bool operator==(const Trackable& other) const
    {
        return m_value == other.m_value;
    }
};

int Trackable::s_instanceCount = 0;

// -----------------------------------------------------------------------------
// Test Functions
// -----------------------------------------------------------------------------

void TestBasicLIFO()
{
    std::cout << "Running TestBasicLIFO..." << std::endl;
    EterLib::Render::FixedStateStack<int, 5> stack;
    
    assert(stack.IsEmpty());
    assert(stack.Depth() == 0);

    stack.Push(10);
    stack.Push(20);
    stack.Push(30);

    assert(!stack.IsEmpty());
    assert(stack.Depth() == 3);
    assert(stack.Top() == 30);

    assert(stack.Pop() == 30);
    assert(stack.Depth() == 2);
    assert(stack.Top() == 20);

    assert(stack.Pop() == 20);
    assert(stack.Depth() == 1);
    assert(stack.Top() == 10);

    assert(stack.Pop() == 10);
    assert(stack.IsEmpty());
    assert(stack.Depth() == 0);

    std::cout << "TestBasicLIFO passed." << std::endl;
}

void TestUnderflow()
{
    std::cout << "Running TestUnderflow..." << std::endl;
    EterLib::Render::FixedStateStack<int, 3> stack;
    
    // Popping an empty stack should return default value (0 for int)
    assert(stack.Pop() == 0);
    assert(stack.Depth() == 0);
    assert(stack.IsEmpty());

    // Top on empty stack should also return default value
    assert(stack.Top() == 0);
    
    stack.Push(42);
    assert(stack.Pop() == 42);
    
    // Underflow again
    assert(stack.Pop() == 0);
    assert(stack.Depth() == 0);

    std::cout << "TestUnderflow passed." << std::endl;
}

void TestOverflow()
{
    std::cout << "Running TestOverflow..." << std::endl;
    EterLib::Render::FixedStateStack<int, 3> stack;
    
    stack.Push(1);
    stack.Push(2);
    stack.Push(3);
    
    assert(stack.Depth() == 3);
    
    // This should be ignored
    stack.Push(4);
    assert(stack.Depth() == 3);
    assert(stack.Top() == 3); // Top is still 3

    // This should also be ignored
    stack.Push(5);
    assert(stack.Depth() == 3);
    assert(stack.Top() == 3);

    assert(stack.Pop() == 3);
    assert(stack.Pop() == 2);
    assert(stack.Pop() == 1);
    
    assert(stack.IsEmpty());

    std::cout << "TestOverflow passed." << std::endl;
}

void TestClear()
{
    std::cout << "Running TestClear..." << std::endl;
    EterLib::Render::FixedStateStack<std::string, 4> stack;

    stack.Push("A");
    stack.Push("B");
    stack.Push("C");

    assert(stack.Depth() == 3);
    
    stack.Clear();
    
    assert(stack.IsEmpty());
    assert(stack.Depth() == 0);
    assert(stack.Top() == "");
    
    stack.Push("D");
    assert(stack.Depth() == 1);
    assert(stack.Top() == "D");

    std::cout << "TestClear passed." << std::endl;
}

void TestNoLeaksAndMoveSemantics()
{
    std::cout << "Running TestNoLeaksAndMoveSemantics..." << std::endl;
    
    // Ensure starting with 0 instances
    assert(Trackable::s_instanceCount == 0);

    {
        EterLib::Render::FixedStateStack<Trackable, 3> stack;
        
        // Pushing creates copies/moves
        stack.Push(Trackable(1));
        stack.Push(Trackable(2));
        stack.Push(Trackable(3));
        
        // Stack should hold exactly 3 instances + standard default initializations in the array
        // actually std::array default initializes all 3 elements.
        // Wait, array has 3 elements, so s_instanceCount is 3 when stack is created!
        // Then we assign/move to them. So it remains 3!
        // Let's verify instance count is exactly 3 for the stack size.
        assert(Trackable::s_instanceCount == 3);

        Trackable t3 = stack.Pop();
        // t3 is local (1 instance), stack has 3 elements (defaulted or old data overridden)
        // actually s_instanceCount will be 4 (3 in array, 1 in t3)
        assert(t3.m_value == 3);
        assert(Trackable::s_instanceCount == 4);
    }
    
    // After stack and locals go out of scope, everything should be destroyed
    assert(Trackable::s_instanceCount == 0);

    std::cout << "TestNoLeaksAndMoveSemantics passed." << std::endl;
}

void TestWithSharedPtr()
{
    std::cout << "Running TestWithSharedPtr..." << std::endl;
    
    EterLib::Render::FixedStateStack<std::shared_ptr<int>, 2> stack;
    
    std::shared_ptr<int> p1 = std::make_shared<int>(100);
    std::shared_ptr<int> p2 = std::make_shared<int>(200);
    
    assert(p1.use_count() == 1);
    
    stack.Push(p1);
    // After push by value, argument is copied into function then moved into stack
    // So stack holds 1 reference, p1 holds 1 reference -> total 2
    assert(p1.use_count() == 2);
    
    stack.Push(p2);
    assert(p2.use_count() == 2);
    
    // Overflow push - should not increase ref count permanently in stack
    stack.Push(p1);
    assert(p1.use_count() == 2);
    
    std::shared_ptr<int> top_ptr = stack.Top();
    assert(*top_ptr == 200);
    assert(p2.use_count() == 3);
    top_ptr.reset();
    
    std::shared_ptr<int> popped = stack.Pop();
    assert(*popped == 200);
    // stack slot was reset to default, so stack no longer holds a reference!
    assert(p2.use_count() == 2);
    popped.reset();
    assert(p2.use_count() == 1);
    
    stack.Clear();
    assert(p1.use_count() == 1);

    std::cout << "TestWithSharedPtr passed." << std::endl;
}

// -----------------------------------------------------------------------------
// Main execution
// -----------------------------------------------------------------------------
int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << " Starting FixedStateStack Tests         " << std::endl;
    std::cout << "========================================" << std::endl;

    TestBasicLIFO();
    TestUnderflow();
    TestOverflow();
    TestClear();
    TestNoLeaksAndMoveSemantics();
    TestWithSharedPtr();

    std::cout << "========================================" << std::endl;
    std::cout << " All tests completed successfully!      " << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
