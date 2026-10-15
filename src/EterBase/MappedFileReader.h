#ifndef ETERBASE_MAPPED_FILE_READER_H
#define ETERBASE_MAPPED_FILE_READER_H

#include <cstdint>
#include <string_view>
#include <span>
#include <string>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

/**
 * @class MappedFileReader
 * @brief Provides ultra-fast read-only access to files on disk using Win32 memory mapping.
 */
class MappedFileReader {
public:
    /**
     * @brief Default constructor. Initializes an empty reader.
     */
    MappedFileReader() noexcept = default;

    /**
     * @brief Destructor. Ensures all mapped resources are properly freed.
     */
    ~MappedFileReader() noexcept {
        Close();
    }

    // Prevent copying
    MappedFileReader(const MappedFileReader&) = delete;
    MappedFileReader& operator=(const MappedFileReader&) = delete;

    /**
     * @brief Move constructor. Takes ownership of mapped resources.
     * @param other The instance to move from.
     */
    MappedFileReader(MappedFileReader&& other) noexcept {
        MoveFrom(std::move(other));
    }

    /**
     * @brief Move assignment operator. Takes ownership of mapped resources.
     * @param other The instance to move from.
     * @return Reference to this instance.
     */
    MappedFileReader& operator=(MappedFileReader&& other) noexcept {
        if (this != &other) {
            Close();
            MoveFrom(std::move(other));
        }
        return *this;
    }

    /**
     * @brief Opens and maps a file into memory for reading.
     * @param file_path The path to the file to open.
     * @return true if the file was successfully mapped, false otherwise.
     */
    [[nodiscard]] bool Open(std::string_view file_path) {
        Close();

        std::string path(file_path);
        
        file_handle = ::CreateFileA(
            path.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (file_handle == INVALID_HANDLE_VALUE) {
            return false;
        }

        LARGE_INTEGER file_size_info;
        if (!::GetFileSizeEx(file_handle, &file_size_info)) {
            Close();
            return false;
        }

        size = static_cast<size_t>(file_size_info.QuadPart);
        if (size == 0) {
            return true;
        }

        mapping_handle = ::CreateFileMappingA(
            file_handle,
            nullptr,
            PAGE_READONLY,
            0,
            0,
            nullptr
        );

        if (!mapping_handle) {
            Close();
            return false;
        }

        mapped_data = ::MapViewOfFile(
            mapping_handle,
            FILE_MAP_READ,
            0,
            0,
            0
        );

        if (!mapped_data) {
            Close();
            return false;
        }

        return true;
    }

    /**
     * @brief Closes the file and unmaps it from memory.
     */
    void Close() noexcept {
        if (mapped_data) {
            ::UnmapViewOfFile(mapped_data);
            mapped_data = nullptr;
        }
        if (mapping_handle) {
            ::CloseHandle(mapping_handle);
            mapping_handle = nullptr;
        }
        if (file_handle != INVALID_HANDLE_VALUE) {
            ::CloseHandle(file_handle);
            file_handle = INVALID_HANDLE_VALUE;
        }
        size = 0;
    }

    /**
     * @brief Gets a view of the mapped file data.
     * @return A std::span covering the file's data.
     */
    [[nodiscard]] std::span<const uint8_t> GetData() const noexcept {
        if (!mapped_data || size == 0) {
            return {};
        }
        return std::span<const uint8_t>(static_cast<const uint8_t*>(mapped_data), size);
    }

    /**
     * @brief Gets the size of the mapped file.
     * @return The size of the file in bytes.
     */
    [[nodiscard]] size_t GetSize() const noexcept {
        return size;
    }

    /**
     * @brief Checks if a file is currently open and mapped.
     * @return true if a file is mapped, false otherwise.
     */
    [[nodiscard]] bool IsOpen() const noexcept {
        return file_handle != INVALID_HANDLE_VALUE;
    }

private:
    /**
     * @brief Helper to transfer resources from another instance.
     * @param other The instance to take resources from.
     */
    void MoveFrom(MappedFileReader&& other) noexcept {
        file_handle = other.file_handle;
        mapping_handle = other.mapping_handle;
        mapped_data = other.mapped_data;
        size = other.size;

        other.file_handle = INVALID_HANDLE_VALUE;
        other.mapping_handle = nullptr;
        other.mapped_data = nullptr;
        other.size = 0;
    }

    HANDLE file_handle{INVALID_HANDLE_VALUE};
    HANDLE mapping_handle{nullptr};
    void* mapped_data{nullptr};
    size_t size{0};
};

#endif // ETERBASE_MAPPED_FILE_READER_H
