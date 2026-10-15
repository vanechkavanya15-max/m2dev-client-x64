#include <iostream>
#include <cassert>
#include <vector>
#include <random>
#include <chrono>
#include <span>
#include <cstring>

#include "Network/ZeroCopyPacketBuffer.h"

#pragma pack(push, 1)
struct TestPacketHeader {
    uint8_t header;
    uint32_t sequence;
};

struct TestPacketChat {
    uint8_t header;
    uint32_t senderId;
    char message[64];
};

struct TestPacketMovement {
    uint8_t header;
    uint32_t entityId;
    int32_t x;
    int32_t y;
    float rotation;
};
#pragma pack(pop)

static void TestBasicBufferOperations() {
    std::cout << "[TEST] 1. Podstawowe operacje ZeroCopyPacketBuffer..." << std::endl;
    using namespace Client::Network;

    ZeroCopyPacketBuffer ring(1024);
    assert(ring.Capacity() == 1024);
    assert(ring.ReadableBytes() == 0);
    assert(ring.WritableBytes() == 1024);
    assert(ring.IsEmpty());

    std::vector<uint8_t> payload = {1, 2, 3, 4, 5, 6, 7, 8};
    bool writeOk = ring.Write(payload);
    assert(writeOk);
    assert(ring.ReadableBytes() == 8);
    assert(ring.WritableBytes() == 1024 - 8);
    assert(!ring.IsEmpty());

    std::vector<uint8_t> outBuffer(8, 0);
    bool readOk = ring.Read(outBuffer);
    assert(readOk);
    assert(outBuffer == payload);
    assert(ring.ReadableBytes() == 0);
    assert(ring.IsEmpty());

    std::cout << "[PASS] Podstawowe operacje bufora zaliczone." << std::endl;
}

static void TestZeroCopyPacketAccess() {
    std::cout << "[TEST] 2. Bezposredni odczyt pakietow Zero-Copy (PeekPacket & RecvPacket)..." << std::endl;
    using namespace Client::Network;

    ZeroCopyPacketBuffer ring(4096);

    TestPacketChat sentChat{};
    sentChat.header = 0xAA;
    sentChat.senderId = 12345;
    std::strncpy(sentChat.message, "Pozdrowienia z Zero-Copy Ring Buffer 2026", sizeof(sentChat.message));

    // Symulacja bezposredniego zapisu z socketu: GetWritableSpan -> memcpy -> CommitWrite
    auto writeSpan = ring.GetWritableSpan(sizeof(TestPacketChat));
    assert(writeSpan.size() >= sizeof(TestPacketChat));
    std::memcpy(writeSpan.data(), &sentChat, sizeof(TestPacketChat));
    ring.CommitWrite(sizeof(TestPacketChat));

    assert(ring.ReadableBytes() == sizeof(TestPacketChat));

    // Zero-Copy Peek
    const TestPacketChat* peeked = ring.PeekPacket<TestPacketChat>();
    assert(peeked != nullptr);
    assert(peeked->header == 0xAA);
    assert(peeked->senderId == 12345);
    assert(std::strcmp(peeked->message, "Pozdrowienia z Zero-Copy Ring Buffer 2026") == 0);
    assert(ring.ReadableBytes() == sizeof(TestPacketChat)); // Peek nie zjada bajtow

    // Zero-Copy Recv
    const TestPacketChat* received = ring.RecvPacket<TestPacketChat>();
    assert(received != nullptr);
    assert(received->header == 0xAA);
    assert(received->senderId == 12345);
    assert(std::strcmp(received->message, "Pozdrowienia z Zero-Copy Ring Buffer 2026") == 0);
    assert(ring.ReadableBytes() == 0); // Recv commituje odczyt

    std::cout << "[PASS] Bezposredni odczyt pakietow zaliczony. Hits=" << ring.GetZeroCopyHits() << std::endl;
}

static void TestWrapAroundBoundary() {
    std::cout << "[TEST] 3. Obliczenia graniczne Wrap-Around na koncu pierscienia..." << std::endl;
    using namespace Client::Network;

    // Maly bufor 128 bajtow
    ZeroCopyPacketBuffer ring(128);

    // Przesun readPos i writePos pod koniec bufora (np. do pozycji 118)
    std::vector<uint8_t> dummy(118, 0xFF);
    ring.Write(dummy);
    std::vector<uint8_t> dummyOut(118, 0);
    ring.Read(dummyOut);
    assert(ring.ReadableBytes() == 0);

    // Teraz readPos = 118, writePos = 118.
    // Pakiet TestPacketMovement ma np. 17 bajtow (sizeof = 1 + 4 + 4 + 4 + 4 = 17 bajtow).
    // Poniewaz 118 + 17 = 135 > 128, pakiet przetnie koniec bufora (10 bajtow na koncu, 7 na poczatku).
    TestPacketMovement sentMove{};
    sentMove.header = 0xBB;
    sentMove.entityId = 9999;
    sentMove.x = 100500;
    sentMove.y = 200700;
    sentMove.rotation = 1.57f;

    std::span<const uint8_t> moveBytes(reinterpret_cast<const uint8_t*>(&sentMove), sizeof(TestPacketMovement));
    bool writeOk = ring.Write(moveBytes);
    assert(writeOk);
    assert(ring.ReadableBytes() == sizeof(TestPacketMovement));

    // Odczyt przez RecvPacket powinien bezblednie zwrocic spojny pakiet ze scratchpada
    const TestPacketMovement* recvMove = ring.RecvPacket<TestPacketMovement>();
    assert(recvMove != nullptr);
    assert(recvMove->header == 0xBB);
    assert(recvMove->entityId == 9999);
    assert(recvMove->x == 100500);
    assert(recvMove->y == 200700);
    assert(recvMove->rotation == 1.57f);

    assert(ring.GetWrapAroundFallbacks() == 1);
    assert(ring.ReadableBytes() == 0);

    std::cout << "[PASS] Granica Wrap-Around prawidlowo obsluzona (Scratchpad Fallback Hits=1)." << std::endl;
}

static void TestStressHighThroughput() {
    std::cout << "[TEST] 4. Test obciazeniowy: 100 000 pakietow w petli sieciowej..." << std::endl;
    using namespace Client::Network;

    constexpr size_t BUFFER_SIZE = 512 * 1024; // 512 KB bufor
    ZeroCopyPacketBuffer ring(BUFFER_SIZE);

    constexpr uint32_t TOTAL_PACKETS = 100000;
    auto startTime = std::chrono::high_resolution_clock::now();

    for (uint32_t seq = 0; seq < TOTAL_PACKETS; ++seq) {
        TestPacketHeader pkt{};
        pkt.header = 0xF1;
        pkt.sequence = seq;

        // Wpisz do bufora
        std::span<const uint8_t> pktSpan(reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt));
        bool written = ring.Write(pktSpan);
        assert(written);

        // Odbierz natychmiast
        const TestPacketHeader* received = ring.RecvPacket<TestPacketHeader>();
        assert(received != nullptr);
        assert(received->header == 0xF1);
        assert(received->sequence == seq);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    std::cout << "[PASS] 100 000 pakietow przetworzonych w " << durationMs << " ms." << std::endl;
    std::cout << "       Zero-Copy Hits: " << ring.GetZeroCopyHits() 
              << ", Wrap-Around Hits: " << ring.GetWrapAroundFallbacks()
              << ", Wspolczynnik Zero-Copy: " << ring.GetZeroCopyRatio() << "%" << std::endl;
    assert(ring.GetZeroCopyRatio() > 95.0);
    assert(ring.ReadableBytes() == 0);
}

int main() {
    std::cout << "=== URUCHAMIANIE TESTOW JEDNOSTKOWYCH: ZeroCopyPacketBuffer ===" << std::endl;

    TestBasicBufferOperations();
    TestZeroCopyPacketAccess();
    TestWrapAroundBoundary();
    TestStressHighThroughput();

    std::cout << "=== WSZYSTKIE TESTY ZEROKOPIOWEGO BUFORA PAKIETOW ZAKONCZONE SUKCESEM (100% PASS) ===" << std::endl;
    return 0;
}
