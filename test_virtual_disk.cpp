#include <iostream>
#include "VirtualDisk.h"
#include <filesystem>
#include <system_error>
#include <chrono>
int main() {
    bool allTestsPassed = true;
    const auto now =
    std::chrono::steady_clock::now();
    const auto timestamp =
    now.time_since_epoch().count();
    const std::string testDiskFileName =
    "test_disk_" + std::to_string(timestamp) + ".bin";
    const std::string invalidSizeDiskFileName =
    "test_invalid_size_" + std::to_string(timestamp) + ".bin";
    std::error_code invalidFileCheckError;

const bool invalidFileAlreadyExists =
    std::filesystem::exists(
        invalidSizeDiskFileName,
        invalidFileCheckError
    );

if (invalidFileCheckError || invalidFileAlreadyExists) {
    std::cerr << "Invalid-size test filename check: FAIL\n";
    return 1;
}
VirtualDisk disk(testDiskFileName);
 std::error_code ec;

const bool diskAlreadyExists =
   std::filesystem::exists(testDiskFileName, ec);
    if (ec) {
    std::cerr << "Disk existence check: FAIL - "
              << ec.message() << '\n';
    return 1;
}
 if (diskAlreadyExists) {
    std::cerr << "Disk creation: FAIL (filename collision)\n";
    return 1;
}
 else if (disk.create()) {
    std::cout << "Disk creation: PASS\n";
} else {
    std::cout << "Disk creation: FAIL\n";
    allTestsPassed = false;
    return 1;
}
        if (disk.open()) {
        std::cout << "Disk open: PASS\n";
    } else {
        std::cout << "Disk open: FAIL\n";
        allTestsPassed = false;
    }
        if (!disk.open()) {
        std::cout << "Double open protection: PASS\n";
    } else {
        std::cout << "Double open protection: FAIL\n";
        allTestsPassed = false;
    }
        std::array<char, VirtualDisk::BLOCK_SIZE> writeData{};
            writeData.fill('A');
    if (disk.writeBlock(5, writeData)) {
    std::cout << "Block write: PASS\n";
} else {
    std::cout << "Block write: FAIL\n";
    allTestsPassed = false;
}
        std::array<char, VirtualDisk::BLOCK_SIZE> readData{};
                const bool readSuccess = disk.readBlock(5, readData);

    if (readSuccess) {
        std::cout << "Block read: PASS\n";
    } else {
        std::cout << "Block read: FAIL\n";
        allTestsPassed = false;
    }
           if (readSuccess && writeData == readData) {
        std::cout << "Data verification: PASS\n";
    } else {
        std::cout << "Data verification: FAIL\n";
        allTestsPassed = false;
    }
        disk.close();
            if (disk.open()) {
        std::cout << "Disk reopen: PASS\n";
    } else {
        std::cout << "Disk reopen: FAIL\n";
        allTestsPassed = false;
    }
        std::array<char, VirtualDisk::BLOCK_SIZE> persistedData{};
            const bool persistenceReadSuccess = disk.readBlock(5, persistedData);
                if (persistenceReadSuccess && writeData == persistedData) {
        std::cout << "Data persistence: PASS\n";
    } else {
        std::cout << "Data persistence: FAIL\n";
        allTestsPassed = false;
    }    if (!disk.readBlock(256, readData)) {
        std::cout << "Invalid block read: PASS\n";
    } else {
        std::cout << "Invalid block read: FAIL\n";
        allTestsPassed = false;
    }
        if (!disk.writeBlock(256, writeData)) {
        std::cout << "Invalid block write: PASS\n";
    } else {
        std::cout << "Invalid block write: FAIL\n";
        allTestsPassed = false;
    }
        if (disk.readBlock(255, readData)) {
        std::cout << "Last valid block read: PASS\n";
    } else {
        std::cout << "Last valid block read: FAIL\n";
        allTestsPassed = false;
        }
        if (disk.readBlock(0, readData)) {
        std::cout << "First valid block read: PASS\n";
    } else {
        std::cout << "First valid block read: FAIL\n";
        allTestsPassed = false;
    }
    if (disk.writeBlock(0, writeData)) {
    std::cout << "First valid block write: PASS\n";
} else {
    std::cout << "First valid block write: FAIL\n";
    allTestsPassed = false;
}
    std::array<char, VirtualDisk::BLOCK_SIZE> firstBlockData{};

const bool firstBlockReadSuccess =
    disk.readBlock(0, firstBlockData);

if (firstBlockReadSuccess && firstBlockData == writeData) {
    std::cout << "First valid block data verification: PASS\n";
} else {
    std::cout << "First valid block data verification: FAIL\n";
    allTestsPassed = false;
}disk.close();

const bool firstBlockReopenSuccess = disk.open();

std::array<char, VirtualDisk::BLOCK_SIZE> firstBlockPersistedData{};

const bool firstBlockPersistenceSuccess =
    firstBlockReopenSuccess &&
    disk.readBlock(0, firstBlockPersistedData);

if (firstBlockPersistenceSuccess &&
    firstBlockPersistedData == writeData) {
    std::cout << "First valid block persistence: PASS\n";
} else {
    std::cout << "First valid block persistence: FAIL\n";
    allTestsPassed = false;
}
disk.close();

if (!disk.writeBlock(0, writeData)) {
    std::cout << "Closed disk write protection: PASS\n";
} else {
    std::cout << "Closed disk write protection: FAIL\n";
    allTestsPassed = false;
}
std::array<char, VirtualDisk::BLOCK_SIZE> closedReadData{};

if (!disk.readBlock(0, closedReadData)) {
    std::cout << "Closed disk read protection: PASS\n";
} else {
    std::cout << "Closed disk read protection: FAIL\n";
    allTestsPassed = false;
}
// Test: Closing an already closed disk
disk.close();

std::cout << "Double close: completed without exception\n";
if (disk.open()) {
    std::cout << "Reopen after double close: PASS\n";
} else {
    std::cout << "Reopen after double close: FAIL\n";
    allTestsPassed = false;
}
std::array<char, VirtualDisk::BLOCK_SIZE> reopenedReadData{};

const bool reopenedReadSuccess =
    disk.readBlock(0, reopenedReadData);

if (reopenedReadSuccess && reopenedReadData == writeData) {
    std::cout << "Read after double close and reopen: PASS\n";
} else {
    std::cout << "Read after double close and reopen: FAIL\n";
    allTestsPassed = false;
}
if (!disk.readBlock(257, readData)) {
    std::cout << "Invalid block 257 read: PASS\n";
} else {
    std::cout << "Invalid block 257 read: FAIL\n";
    allTestsPassed = false;
}
if (!disk.writeBlock(257, writeData)) {
    std::cout << "Invalid block 257 write: PASS\n";
} else {
    std::cout << "Invalid block 257 write: FAIL\n";
    allTestsPassed = false;
}
if (!disk.readBlock(1000000, readData)) {
    std::cout << "Large invalid block read: PASS\n";
} else {
    std::cout << "Large invalid block read: FAIL\n";
    allTestsPassed = false;
}
if (!disk.writeBlock(1000000, writeData)) {
    std::cout << "Large invalid block write: PASS\n";
} else {
    std::cout << "Large invalid block write: FAIL\n";
    allTestsPassed = false;
}
if (!disk.readBlock(static_cast<std::size_t>(-1), readData)) {
    std::cout << "Maximum invalid block read: PASS\n";
} else {
    std::cout << "Maximum invalid block read: FAIL\n";
    allTestsPassed = false;
}
if (!disk.writeBlock(static_cast<std::size_t>(-1), writeData)) {
    std::cout << "Maximum invalid block write: PASS\n";
} else {
    std::cout << "Maximum invalid block write: FAIL\n";
    allTestsPassed = false;
}
VirtualDisk missingDisk("missing_test_disk.bin");

if (!missingDisk.open()) {
    std::cout << "Missing disk open protection: PASS\n";
} else {
    std::cout << "Missing disk open protection: FAIL\n";
    allTestsPassed = false;
}
if (!missingDisk.writeBlock(0, writeData)) {
    std::cout << "Missing disk write protection: PASS\n";
} else {
    std::cout << "Missing disk write protection: FAIL\n";
    allTestsPassed = false;
}
std::array<char, VirtualDisk::BLOCK_SIZE> missingReadData{};

if (!missingDisk.readBlock(0, missingReadData)) {
    std::cout << "Missing disk read protection: PASS\n";
} else {
    std::cout << "Missing disk read protection: FAIL\n";
    allTestsPassed = false;
}
// Test: Reject a disk with an invalid file size.
VirtualDisk invalidDisk(invalidSizeDiskFileName);

if (!invalidDisk.create()) {
    std::cout << "Invalid size disk setup: FAIL\n";
    allTestsPassed = false;
} else {
    std::error_code resizeError;

    std::filesystem::resize_file(
        invalidSizeDiskFileName, 500, resizeError
    );

    if (resizeError) {
        std::cout << "Invalid size disk resize: FAIL - "
                  << resizeError.message() << '\n';
        allTestsPassed = false;
    } else if (!invalidDisk.open()) {
        std::cout << "Invalid disk size rejection: PASS\n";
    } else {
        std::cout << "Invalid disk size rejection: FAIL\n";
        allTestsPassed = false;
    }

    invalidDisk.close();

    std::error_code invalidCleanupError;
    const bool invalidCleanupSuccess =
        std::filesystem::remove(
            invalidSizeDiskFileName, invalidCleanupError
        );

    if (!invalidCleanupSuccess || invalidCleanupError) {
        std::cout << "Invalid disk cleanup: FAIL\n";
        allTestsPassed = false;
    } else {
        std::cout << "Invalid disk cleanup: PASS\n";
    }
}
// Cleanup: Remove only the temporary disk created by this test run.
disk.close();
std::error_code cleanupError;
const bool cleanupSuccess =
    std::filesystem::remove(testDiskFileName, cleanupError);
    if (!cleanupSuccess || cleanupError) {
    std::cerr << "Temporary disk cleanup: FAIL - "
              << cleanupError.message() << '\n';
    allTestsPassed = false;
} else {
    std::cout << "Temporary disk cleanup: PASS\n";
}
   return allTestsPassed ? 0 : 1;
}