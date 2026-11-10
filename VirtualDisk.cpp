
#include "VirtualDisk.h"
#include <filesystem>
#include <windows.h>
#include <iostream>


VirtualDisk::VirtualDisk(const std::string& filename)
    : fileName(filename) {
}
VirtualDisk::~VirtualDisk() {
    close(); 
}
bool VirtualDisk::open() {
    if (diskFile.is_open()) {
        return false;
    }
    diskFile.clear();
    diskFile.open(
        fileName,
        std::ios::in | std::ios::out | std::ios::binary
    );
    if (!diskFile) {
    diskFile.clear();
    close();
    return false;
}
    diskFile.seekg(0, std::ios::end);
    const auto fileSize = diskFile.tellg();
    if (fileSize == std::streampos(-1)) {
        diskFile.clear();
        close();
        return false;
    }
    const auto expectedSize = BLOCK_SIZE * TOTAL_BLOCKS;

    if (fileSize != static_cast<std::streamoff>(expectedSize)) {
        diskFile.clear();
        close();
        return false;
    }
    diskFile.seekg(0, std::ios::beg);
    if (!diskFile) {
        diskFile.clear();
        close();        
        return false;
    }
    return true;
}

bool VirtualDisk::create() {
    if (diskFile.is_open()) {
        return false;
    }

    std::error_code ec;

    if (std::filesystem::exists(fileName, ec) || ec) {
        return false;
    }

    
HANDLE handle = CreateFileA(
    fileName.c_str(),
    GENERIC_READ | GENERIC_WRITE,
    0,
    nullptr,
    CREATE_NEW,
    FILE_ATTRIBUTE_NORMAL,
    nullptr
);

if (handle == INVALID_HANDLE_VALUE) {
    return false;
}


LARGE_INTEGER diskSize;
diskSize.QuadPart = static_cast<LONGLONG>(
    BLOCK_SIZE * TOTAL_BLOCKS
);

if (!SetFilePointerEx(handle, diskSize, nullptr, FILE_BEGIN) ||
    !SetEndOfFile(handle)) {
    CloseHandle(handle);

    if (!DeleteFileA(fileName.c_str())) {
        std::cerr << "Failed to delete incomplete disk. Windows error: "
                  << GetLastError() << '\n';
    }

    return false;
}


LARGE_INTEGER startPosition{};

if (!SetFilePointerEx(handle, startPosition, nullptr, FILE_BEGIN)) {
    CloseHandle(handle);

    if (!DeleteFileA(fileName.c_str())) {
        std::cerr << "Failed to delete incomplete disk. Windows error: "
                  << GetLastError() << '\n';
    }

    return false;
}


std::array<char, BLOCK_SIZE> zeroBlock{};

for (std::size_t i = 0; i < TOTAL_BLOCKS; ++i) {
    DWORD bytesWritten = 0;

    
if (!WriteFile(handle, zeroBlock.data(),
               static_cast<DWORD>(BLOCK_SIZE),
               &bytesWritten, nullptr) ||
    bytesWritten != BLOCK_SIZE) {
    CloseHandle(handle);

    if (!DeleteFileA(fileName.c_str())) {
        std::cerr << "Failed to delete incomplete disk. Windows error: "
                  << GetLastError() << '\n';
    }

    return false;
    }
}

if (!FlushFileBuffers(handle)) {
    CloseHandle(handle);

    if (!DeleteFileA(fileName.c_str())) {
        std::cerr << "Failed to delete incomplete disk. Windows error: "
                  << GetLastError() << '\n';
    }

    return false;
}

if (!CloseHandle(handle)) {
    if (!DeleteFileA(fileName.c_str())) {
        std::cerr << "Failed to delete incomplete disk. Windows error: "
                  << GetLastError() << '\n';
    }
    return false;
}

return true;
}

void VirtualDisk::close() {
    if (diskFile.is_open()) {
        diskFile.close();
    }

    diskFile.clear();
}
bool VirtualDisk::writeBlock(
    std::size_t blockNumber,
    const std::array<char, BLOCK_SIZE>& data
) {
    if (blockNumber >= TOTAL_BLOCKS) {
        return false;
    }
        if (!diskFile.is_open()) {
        return false;
    }
        if (!diskFile) {
        return false;
    }
        const std::streamoff offset = static_cast<std::streamoff>(blockNumber * BLOCK_SIZE);
            diskFile.seekp(offset, std::ios::beg);
                if (!diskFile) {
        return false;
    }
        diskFile.write(data.data(), static_cast<std::streamsize>(BLOCK_SIZE));
            if (!diskFile) {
        return false;
    }
        diskFile.flush();
            if (!diskFile) {
        return false;
    }
        return true;
}
bool VirtualDisk::readBlock(
    std::size_t blockNumber,
    std::array<char, BLOCK_SIZE>& data
) {
        if (blockNumber >= TOTAL_BLOCKS) {
        return false;
    }
        if (!diskFile.is_open()) {
        return false;
    }
        if (!diskFile) {
        return false;
    }    
    const std::streamoff offset = static_cast<std::streamoff>(blockNumber * BLOCK_SIZE);
        diskFile.seekg(offset, std::ios::beg);
            if (!diskFile) {
        return false;
    }
        diskFile.read(data.data(), static_cast<std::streamsize>(BLOCK_SIZE));
            if (!diskFile ||
        diskFile.gcount() != static_cast<std::streamsize>(BLOCK_SIZE)) {
        return false;
    }
        return true;
}