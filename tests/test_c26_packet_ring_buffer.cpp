#include "../src/Client/Network/PacketRingBuffer.h"
#include <iostream>
#include <vector>
#include <cassert>

using namespace Client::Network;

void TestInitialization() {
    PacketRingBuffer buffer(100);
    assert(buffer.GetCapacity() == 100);
    assert(buffer.GetAvailableSize() == 0);
    assert(buffer.GetFreeSize() == 100);
    std::cout << "TestInitialization passed.\n";
}

void TestWriteAndRead() {
    PacketRingBuffer buffer(10);
    std::vector<uint8_t> dataToWrite = {1, 2, 3, 4, 5};
    auto writeResult = buffer.Write(dataToWrite);
    assert(writeResult.has_value());
    assert(buffer.GetAvailableSize() == 5);
    assert(buffer.GetFreeSize() == 5);

    std::vector<uint8_t> readData(5);
    auto readResult = buffer.Read(readData);
    assert(readResult.has_value());
    assert(readData == dataToWrite);
    assert(buffer.GetAvailableSize() == 0);
    assert(buffer.GetFreeSize() == 10);
    std::cout << "TestWriteAndRead passed.\n";
}

void TestWrapAround() {
    PacketRingBuffer buffer(10);
    
    std::vector<uint8_t> data1 = {1, 2, 3, 4, 5, 6, 7, 8};
    (void)buffer.Write(data1);

    std::vector<uint8_t> read1(5);
    (void)buffer.Read(read1);
    assert(buffer.GetAvailableSize() == 3);

    std::vector<uint8_t> data2 = {9, 10, 11, 12, 13, 14};
    auto writeResult = buffer.Write(data2);
    assert(writeResult.has_value());
    assert(buffer.GetAvailableSize() == 9);

    std::vector<uint8_t> read2(9);
    (void)buffer.Read(read2);
    std::vector<uint8_t> expected = {6, 7, 8, 9, 10, 11, 12, 13, 14};
    assert(read2 == expected);

    std::cout << "TestWrapAround passed.\n";
}

void TestOverflow() {
    PacketRingBuffer buffer(5);
    std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6}; 
    auto writeResult = buffer.Write(data);
    assert(!writeResult.has_value());
    assert(writeResult.error() == EterBase::PacketError::BufferUnderflow);
    std::cout << "TestOverflow passed.\n";
}

void TestUnderflow() {
    PacketRingBuffer buffer(5);
    std::vector<uint8_t> data = {1, 2, 3};
    (void)buffer.Write(data);

    std::vector<uint8_t> readData(4); 
    auto readResult = buffer.Read(readData);
    assert(!readResult.has_value());
    assert(readResult.error() == EterBase::PacketError::BufferUnderflow);
    std::cout << "TestUnderflow passed.\n";
}

void TestPeekAndSkip() {
    PacketRingBuffer buffer(10);
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    (void)buffer.Write(data);

    std::vector<uint8_t> peekData(3);
    auto peekResult = buffer.Peek(peekData);
    assert(peekResult.has_value());
    assert(peekData[0] == 1 && peekData[1] == 2 && peekData[2] == 3);
    assert(buffer.GetAvailableSize() == 5); 

    auto skipResult = buffer.Skip(3);
    assert(skipResult.has_value());
    assert(buffer.GetAvailableSize() == 2);

    std::vector<uint8_t> readData(2);
    (void)buffer.Read(readData);
    assert(readData[0] == 4 && readData[1] == 5);
    std::cout << "TestPeekAndSkip passed.\n";
}

void TestZeroCapacity() {
    PacketRingBuffer buffer(0);
    assert(buffer.GetCapacity() == 1); // Zabezpieczenie przed dzieleniem przez zero

    std::vector<uint8_t> data = {1, 2};
    auto writeResult = buffer.Write(data);
    assert(!writeResult.has_value()); // Przekracza wymuszona pojemnosc 1
    std::cout << "TestZeroCapacity passed.\n";
}

void TestPeekDynamicSize() {
    PacketRingBuffer buffer(10);
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    (void)buffer.Write(data);

    auto dynSizeResult1 = buffer.PeekDynamicSize(5);
    assert(dynSizeResult1.has_value());

    auto dynSizeResult2 = buffer.PeekDynamicSize(6);
    assert(!dynSizeResult2.has_value());
    assert(dynSizeResult2.error() == EterBase::PacketError::BufferUnderflow);

    std::cout << "TestPeekDynamicSize passed.\n";
}

int main() {
    TestInitialization();
    TestZeroCapacity();
    TestWriteAndRead();
    TestWrapAround();
    TestOverflow();
    TestUnderflow();
    TestPeekAndSkip();
    TestPeekDynamicSize();

    std::cout << "All tests passed successfully.\n";
    return 0;
}
