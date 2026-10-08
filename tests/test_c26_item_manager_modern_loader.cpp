#include <iostream>
#include <vector>
#include <string>
#include <format>
#include <zstd.h>
#include <cstring>
#include <cassert>

// Mock CItemData
#include "GameLib/ItemData.h"

// Mock PackManager
#include "EterBase/Singleton.h"
#include "PackLib/PackManager.h"
CPackManager::CPackManager() {}
CPackManager::~CPackManager() {}
bool CPackManager::AddPack(const std::string& path) { return false; }
bool CPackManager::GetFile(std::string_view path, TPackFile& result) { return false; }
bool CPackManager::GetFileWithPool(std::string_view path, TPackFile& result, CBufferPool* pPool) { return false; }
bool CPackManager::IsExist(std::string_view path) const { return false; }
void CPackManager::NormalizePath(std::string_view in, std::string& out) const {}

// Mock Logging
namespace EterBase {
    template<typename... Args>
    void TraceError(std::format_string<Args...> fmt, Args&&... args) {
        std::cerr << "TraceError: " << std::format(fmt, std::forward<Args>(args)...) << std::endl;
    }
    template<typename... Args>
    void Trace(std::format_string<Args...> fmt, Args&&... args) {
        std::cout << "Trace: " << std::format(fmt, std::forward<Args>(args)...) << std::endl;
    }
}

void TraceErrorFmt(...) {}
void TraceFmt(...) {}
void LogFmt(...) {}

// Directly include the implementation for testing
#include "src/Client/Data/ItemManagerModernLoader.cpp"

int main()
{
    std::cout << "Running test_c26_item_manager_modern_loader..." << std::endl;

    // Create dummy data
    std::vector<CItemData::TItemTable> dummyData(3);
    for (size_t i = 0; i < dummyData.size(); ++i)
    {
        std::memset(&dummyData[i], 0, sizeof(CItemData::TItemTable));
        dummyData[i].dwVnum = 1000 + i;
        std::string name = "TestItem_" + std::to_string(i);
        std::strncpy(dummyData[i].szName, name.c_str(), sizeof(dummyData[i].szName) - 1);
    }

    // Compress dummy data
    size_t const bufferSize = dummyData.size() * sizeof(CItemData::TItemTable);
    size_t const bound = ZSTD_compressBound(bufferSize);
    std::vector<uint8_t> compressedBuffer(bound);

    size_t const compressedSize = ZSTD_compress(
        compressedBuffer.data(), compressedBuffer.size(),
        dummyData.data(), bufferSize,
        1 // compressionLevel
    );

    if (ZSTD_isError(compressedSize))
    {
        std::cerr << "ZSTD compression failed in test setup: " << ZSTD_getErrorName(compressedSize) << std::endl;
        return 1;
    }

    compressedBuffer.resize(compressedSize);

    // Test ItemManagerModernLoader
    int registeredCount = 0;
    auto registrar = [&](const CItemData::TItemTable& item) {
        if (registeredCount < dummyData.size())
        {
            assert(item.dwVnum == dummyData[registeredCount].dwVnum);
            assert(std::string(item.szName) == std::string(dummyData[registeredCount].szName));
        }
        registeredCount++;
    };

    auto result = ItemManagerModernLoader::LoadFromModernBlob(compressedBuffer, registrar);

    if (!result.has_value())
    {
        std::cerr << "Test failed: LoadFromModernBlob returned error: " << result.error() << std::endl;
        return 1;
    }

    if (registeredCount != dummyData.size())
    {
        std::cerr << "Test failed: Registered items count mismatch. Expected " << dummyData.size() << ", got " << registeredCount << std::endl;
        return 1;
    }

    std::cout << "Test passed successfully." << std::endl;
    return 0;
}
