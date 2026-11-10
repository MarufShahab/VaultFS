#include "FileSystem.h"
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char** argv) {
    const std::string path = argc>=2 ? argv[1] : "disk1.bin";
    VirtualDisk disk(path);
    FileSystem fs(disk);
    if (!disk.open()) {
        // Only create when the file is absent; never replace an existing file.
        if (!disk.create() || !disk.open() || !fs.format()) {
            std::cerr << "Cannot open/create filesystem: " << path << '\n';
            return 1;
        }
        std::cout << "Created new Phase 3 filesystem on " << path << "\n";
    } else if (!fs.mount()) {
        std::cerr << "Existing disk has no valid Phase 3 filesystem. Refusing to format it.\n";
        return 1;
    }
    std::cout << "VaultFS Phase 3 | help for commands | exit to quit\n";
    std::string line;
    while (std::cout << "vaultfs> " && std::getline(std::cin,line)) {
        std::istringstream input(line);
        std::string cmd, name;
        input >> cmd;
        if (cmd=="exit" || cmd=="quit") break;
        if (cmd=="help") {
            std::cout << "help, ls, touch <name>, write <name> <text>, cat <name>, rm <name>, stat <name>, df, exit\n";
        } else if (cmd=="ls") {
            for (const auto& f:fs.listFiles()) std::cout << f << " (" << fs.fileSize(f) << " bytes)\n";
        } else if (cmd=="df") {
            std::cout << "Free data blocks: " << fs.freeBlockCount()
                      << ", free inodes: " << fs.freeInodeCount() << '\n';
        } else if (cmd=="touch") {
            input >> name;
            std::cout << (fs.createFile(name) ? "Created\n" : "Failed\n");
        } else if (cmd=="rm") {
            input >> name;
            std::cout << (fs.deleteFile(name) ? "Deleted\n" : "Failed\n");
        } else if (cmd=="stat") {
            input >> name;
            bool exists=false;
            for (const auto& f:fs.listFiles()) if (f==name) exists=true;
            if (exists) std::cout << name << ": " << fs.fileSize(name) << " bytes\n";
            else std::cout << "Not found\n";
        } else if (cmd=="write") {
            input >> name;
            std::string data;
            std::getline(input,data);
            if (!data.empty() && data[0]==' ') data.erase(0,1);
            std::cout << (fs.writeFile(name,data) ? "Written\n" : "Failed\n");
        } else if (cmd=="cat") {
            input >> name;
            std::string data;
            if (fs.readFile(name,data)) std::cout << data << '\n';
            else std::cout << "Failed\n";
        } else if (!cmd.empty()) {
            std::cout << "Unknown command. Try help.\n";
        }
    }
    disk.close();
}
