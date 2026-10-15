#include <iostream>
#include <cassert>
#include <vector>

// Minimal dummy for StdAfx.h to satisfy compiling the test
// Using a macro so that we don't need a real StdAfx.h in the test environment if not fully mocked, 
// wait, the bash compile command will include it if we create a mock, or we can just mock it here.
// But we included "StdAfx.h" in the cpp file, so the compiler needs to find it.
// I will create a temporary include directory in the bash script, but for this file, just standard testing stuff.

#include "Client/Mimic/CrcPieceCalculator.h"

int main()
{
    std::cout << "Starting CrcPieceCalculator test..." << std::endl;

    // Fixed dummy CRC values for deterministic testing
    uint32_t dummyProcCrc = 0x12345678; // Bytes: 78 56 34 12
    uint32_t dummyFileCrc = 0x9ABCDEF0; // Bytes: F0 DE BC 9A

    // Expected magic cube mapping:
    // cube[0] = 0x78 (120)
    // cube[1] = 0xF0 (240)
    // cube[2] = 0x56 (86)
    // cube[3] = 0xDE (222)
    // cube[4] = 0x34 (52)
    // cube[5] = 0xBC (188)
    // cube[6] = 0x12 (18)
    // cube[7] = 0x9A (154)

    // XOR_TABLE: { 102, 30, 188, 44, 39, 201, 43, 5 }

    // Expected output pieces (cube[i] ^ XOR_TABLE[i]):
    // piece 0 = 120 ^ 102 = 0x78 ^ 0x66 = 0x1E = 30
    // piece 1 = 240 ^ 30  = 0xF0 ^ 0x1E = 0xEE = 238
    // piece 2 = 86  ^ 188 = 0x56 ^ 0xBC = 0xEA = 234
    // piece 3 = 222 ^ 44  = 0xDE ^ 0x2C = 0xF2 = 242
    // piece 4 = 52  ^ 39  = 0x34 ^ 0x27 = 0x13 = 19
    // piece 5 = 188 ^ 201 = 0xBC ^ 0xC9 = 0x75 = 117
    // piece 6 = 18  ^ 43  = 0x12 ^ 0x2B = 0x39 = 57
    // piece 7 = 154 ^ 5   = 0x9A ^ 0x05 = 0x9F = 159

    std::vector<uint8_t> expectedPieces = { 30, 238, 234, 242, 19, 117, 57, 159 };

    Client::Mimic::CrcPieceCalculator calc(dummyProcCrc, dummyFileCrc);

    for (size_t i = 0; i < 16; ++i)
    {
        uint8_t piece = calc.GetNextPiece();
        uint8_t expected = expectedPieces[i % 8];

        if (piece != expected)
        {
            std::cerr << "Test Failed at index " << i 
                      << ". Expected " << (int)expected 
                      << ", got " << (int)piece << std::endl;
            return 1;
        }
    }

    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
