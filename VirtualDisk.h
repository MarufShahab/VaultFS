
#ifndef VIRTUAL_DISK_H
#define VIRTUAL_DISK_H

#include <cstddef>
#include <fstream>
#include <string>
#include <array>

class VirtualDisk {
public:
    static constexpr std::size_t BLOCK_SIZE = 4096;
    static constexpr std::size_t TOTAL_BLOCKS = 256;

    
    explicit VirtualDisk(const std::string& filename);    
    ~VirtualDisk();
    bool create();
    bool open();

    void close();

    
    bool writeBlock(std::size_t blockNumber,
        const std::array<char, BLOCK_SIZE>& data);

    bool readBlock(std::size_t blockNumber,
               std::array<char, BLOCK_SIZE>& data);




private:
    std::string fileName;
    std::fstream diskFile;
};

#endif // VIRTUAL_DISK_H
