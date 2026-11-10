#include "FileSystem.h"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

#define CHECK(expr) do { if (!(expr)) { std::cerr << "FAIL " << __LINE__ << ": " #expr "\n"; return 1; } } while (0)

int main() {
    const auto tick=std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path="phase3_test_"+std::to_string(tick)+".bin";
    CHECK(!std::filesystem::exists(path));
    {
        VirtualDisk disk(path);
        CHECK(disk.create());
        CHECK(disk.open());
        FileSystem fs(disk);
        CHECK(fs.format());
        CHECK(!fs.format());
        CHECK(fs.freeBlockCount()==250);
        CHECK(fs.freeInodeCount()==63);
        CHECK(fs.createFile("demo.txt"));
        CHECK(!fs.createFile("demo.txt"));
        CHECK(!fs.createFile("../bad"));
        CHECK(fs.writeFile("demo.txt","hello"));
        std::string read;
        CHECK(fs.readFile("demo.txt",read) && read=="hello");
        CHECK(fs.freeBlockCount()==249);
        CHECK(fs.createFile("large.bin"));
        const std::string many(5*VirtualDisk::BLOCK_SIZE+17,'Q');
        CHECK(fs.writeFile("large.bin",many));
        CHECK(fs.freeBlockCount()==242); // 6 file blocks + 1 indirect pointer block
        CHECK(fs.readFile("large.bin",read) && read==many);
        CHECK(fs.writeFile("demo.txt",""));
        CHECK(fs.fileSize("demo.txt")==0);
        CHECK(fs.freeBlockCount()==243);
        disk.close();
    }
    {
        VirtualDisk disk(path);
        CHECK(disk.open());
        FileSystem fs(disk);
        CHECK(fs.mount());
        CHECK(!fs.mount());
        CHECK(fs.listFiles().size()==2);
        std::string read;
        CHECK(fs.readFile("large.bin",read));
        CHECK(read==std::string(5*VirtualDisk::BLOCK_SIZE+17,'Q'));
        CHECK(fs.deleteFile("large.bin"));
        CHECK(fs.freeBlockCount()==250);
        CHECK(fs.deleteFile("demo.txt"));
        CHECK(fs.listFiles().empty());
        CHECK(!fs.deleteFile("demo.txt"));
        CHECK(fs.freeInodeCount()==63);
        disk.close();
    }
    {
        VirtualDisk disk(path); CHECK(disk.open());
        FileSystem fs(disk); CHECK(fs.mount()); CHECK(fs.listFiles().empty());
        disk.close();
    }
    CHECK(std::filesystem::remove(path));
    std::cout << "Phase 3 format, allocation, direct/indirect, persistence, deletion: PASS\n";
    return 0;
}
