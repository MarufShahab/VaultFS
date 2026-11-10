#ifndef VAULTFS_FILESYSTEM_H
#define VAULTFS_FILESYSTEM_H

#include "VirtualDisk.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

// Phase 3: single-disk persistent metadata and inode-backed files.
// Replica management and CRC recovery are deliberately deferred.
class FileSystem {
public:
    static constexpr std::uint16_t INVALID = 0xFFFF;
    static constexpr std::uint16_t INODE_COUNT = 64;
    static constexpr std::uint16_t DIRECT_COUNT = 4;
    static constexpr std::uint16_t DATA_START = 6;
    static constexpr std::size_t MAX_FILE_BYTES = (DIRECT_COUNT + 2048) * VirtualDisk::BLOCK_SIZE;

    explicit FileSystem(VirtualDisk& disk);
    bool format();             // Only on an already opened, newly created disk.
    bool mount();              // Read and validate existing persistent metadata.
    bool createFile(const std::string& name);
    bool deleteFile(const std::string& name);
    bool writeFile(const std::string& name, const std::string& bytes);
    bool readFile(const std::string& name, std::string& bytes);
    std::vector<std::string> listFiles() const;
    std::uint32_t fileSize(const std::string& name) const;
    std::size_t freeBlockCount() const;
    std::size_t freeInodeCount() const;
    bool isMounted() const { return mounted_; }

private:
    struct Inode {
        std::uint8_t kind = 0; // 0=free, 1=file, 2=root directory
        std::uint32_t size = 0;
        std::array<std::uint16_t, DIRECT_COUNT> direct{};
        std::uint16_t indirect = INVALID;
        Inode() { direct.fill(INVALID); }
    };
    struct Entry { std::uint16_t inode = INVALID; std::string name; };

    VirtualDisk& disk_;
    bool mounted_ = false;
    std::array<std::uint8_t, INODE_COUNT> inodeUsed_{};
    std::array<std::uint8_t, VirtualDisk::TOTAL_BLOCKS> blockUsed_{};
    std::array<Inode, INODE_COUNT> inodes_{};
    std::array<Entry, 64> entries_{};

    bool saveMetadata();
    bool loadMetadata();
    bool readBlock(std::uint16_t index, std::array<char, VirtualDisk::BLOCK_SIZE>& bytes);
    bool writeBlock(std::uint16_t index, const std::array<char, VirtualDisk::BLOCK_SIZE>& bytes);
    std::uint16_t allocateBlock();
    std::uint16_t allocateInode();
    int findEntry(const std::string& name) const;
    int findEmptyEntry() const;
    void releaseBlocks(const Inode& inode);
    bool dataPointers(const Inode& inode, std::vector<std::uint16_t>& result);
};
#endif
