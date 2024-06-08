#ifndef C_WIN_CPP
#define C_WIN_CPP

#include <windows.h>
#include <iostream>
#include <cstdint>

void testFile() {
    std::cout << "MMAP For Windows" << std::endl;
}

void* mmap_file(const char* filename, size_t* length) {
    HANDLE file_handle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening file: " << GetLastError() << std::endl;
        return nullptr;
    }

    DWORD file_size = GetFileSize(file_handle, NULL);
    if (file_size == INVALID_FILE_SIZE) {
        DWORD error_code = GetLastError();
        std::cerr << "Error getting file size: " << error_code << std::endl;
        CloseHandle(file_handle);
        return nullptr;
    }

    HANDLE file_mapping = CreateFileMapping(file_handle, NULL, PAGE_READONLY, 0, 0, NULL);
    if (file_mapping == NULL) {
        std::cerr << "Error creating file mapping: " << GetLastError() << std::endl;
        CloseHandle(file_handle);
        return nullptr;
    }

    void* mapped_addr = MapViewOfFile(file_mapping, FILE_MAP_READ, 0, 0, 0);
    if (mapped_addr == NULL) {
        std::cerr << "Error mapping file: " << GetLastError() << std::endl;
        CloseHandle(file_mapping);
        CloseHandle(file_handle);
        return nullptr;
    }

    *length = static_cast<size_t>(file_size);

    CloseHandle(file_mapping);
    CloseHandle(file_handle);

    return mapped_addr;
}

void munmap_file(void* addr) {
    if (!UnmapViewOfFile(addr)) {
        std::cerr << "Error unmapping file: " << GetLastError() << std::endl;
    }
}

#endif // C_WIN_CPP
