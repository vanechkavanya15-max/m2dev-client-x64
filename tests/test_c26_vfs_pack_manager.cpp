#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "../src/Client/Platform/VFSManager.h"
#include "../src/Client/Platform/VFSManager.cpp" // Include cpp for direct testing

using namespace Client::Platform;

// Helper to create dummy loose file
void CreateLooseFile(const std::string& path, const std::string& content) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream out(path, std::ios::binary);
    out.write(content.data(), content.size());
}

// Helper to calculate CRC for testing
uint32_t CalculateCrc32Test(std::string_view data) {
    // We can just use the manager's private method by tricking it or replicating it
    // But actually, we need the EXACT same CRC32 function, so let's replicate the standard CRC32
    std::array<uint32_t, 256> table{};
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int j = 0; j < 8; ++j) {
            if (c & 1)
                c = 0xEDB88320 ^ (c >> 1);
            else
                c >>= 1;
        }
        table[i] = c;
    }
    
    uint32_t crc = 0xFFFFFFFF;
    for (char byte : data) {
        crc = table[(crc ^ static_cast<uint8_t>(byte)) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

// Helper to create dummy pack
void CreateDummyPack(const std::string& packPath, const std::vector<std::pair<std::string, std::string>>& entries) {
    std::ofstream packFile(packPath, std::ios::binary);
    std::ofstream idxFile(packPath + ".idx", std::ios::binary);

    uint32_t numEntries = entries.size();
    idxFile.write(reinterpret_cast<const char*>(&numEntries), sizeof(numEntries));

    uint32_t currentOffset = 0;
    for (const auto& entry : entries) {
        const std::string& vpath = entry.first;
        const std::string& data = entry.second;

        // Write to pack
        packFile.write(data.data(), data.size());

        // Write to idx
        uint16_t pathLen = vpath.size();
        idxFile.write(reinterpret_cast<const char*>(&pathLen), sizeof(pathLen));
        idxFile.write(vpath.data(), pathLen);
        
        idxFile.write(reinterpret_cast<const char*>(&currentOffset), sizeof(currentOffset));
        uint32_t size = data.size();
        idxFile.write(reinterpret_cast<const char*>(&size), sizeof(size));
        
        uint32_t crc = CalculateCrc32Test(data);
        idxFile.write(reinterpret_cast<const char*>(&crc), sizeof(crc));

        currentOffset += size;
    }
}

int main() {
    std::cout << "Starting VFSManager Tests..." << std::endl;

    std::filesystem::path testDir = std::filesystem::current_path() / "vfs_test_dir";
    std::filesystem::remove_all(testDir);
    std::filesystem::create_directories(testDir);
    std::filesystem::create_directories(testDir / "loose");

    auto& vfs = VFSManager::Instance();
    vfs.RegisterLooseFileDirectory((testDir / "loose").string());

    // Test Loose File
    std::string looseContent = "Hello Loose File";
    CreateLooseFile((testDir / "loose" / "test_loose.txt").string(), looseContent);

    assert(vfs.Exists("test_loose.txt") == true);
    auto readLoose = vfs.ReadFile("test_loose.txt");
    assert(readLoose.has_value());
    assert(std::string(readLoose.value().begin(), readLoose.value().end()) == looseContent);

    // Test Pack File
    std::string packPath = (testDir / "testpack").string();
    std::string packContent1 = "Pack Data 1";
    std::string packContent2 = "Pack Data 2";
    CreateDummyPack(packPath, {
        {"pack_file1.txt", packContent1},
        {"dir/pack_file2.txt", packContent2}
    });

    auto mountRes = vfs.MountPack(packPath);
    assert(mountRes.has_value());

    assert(vfs.Exists("pack_file1.txt") == true);
    auto readPack = vfs.ReadFile("dir/pack_file2.txt");
    assert(readPack.has_value());
    assert(std::string(readPack.value().begin(), readPack.value().end()) == packContent2);

    // Test Cache
    auto cacheRes1 = vfs.GetCachedFile("pack_file1.txt");
    assert(cacheRes1.has_value());
    assert(std::string(reinterpret_cast<const char*>(cacheRes1.value().data()), cacheRes1.value().size()) == packContent1);

    auto cacheRes2 = vfs.GetCachedFile("pack_file1.txt"); // Should hit cache
    assert(cacheRes2.has_value());
    assert(cacheRes1.value().data() == cacheRes2.value().data()); // Same memory address means cached

    // Test Not Found
    assert(vfs.Exists("non_existent.txt") == false);
    auto notFound = vfs.ReadFile("non_existent.txt");
    assert(!notFound.has_value());
    assert(notFound.error() == VFSError::FileNotFound);

    // Cleanup
    std::filesystem::remove_all(testDir);

    std::cout << "All VFSManager Tests Passed!" << std::endl;
    return 0;
}
