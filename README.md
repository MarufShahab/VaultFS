# VaultFS

VaultFS is an educational inode-based replicated mini filesystem project implemented in C++17.

## Project Objectives

- Virtual disk storage using binary files
- Inode-based file management
- Persistent filesystem metadata
- File and directory operations
- Three-copy file-data replication
- CRC32 corruption detection
- Automatic recovery from valid replicas

## Technologies

- C++17
- CMake
- GCC / G++
- Git
- GDB
- Windows API (VirtualDisk creation)

## Build Instructions

On Windows with MinGW and CMake:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

## Run

```powershell
.\build\vaultfs.exe
```

The main executable currently provides an initial welcome and startup message. Full filesystem operations are not yet integrated.

## Phase 2: VirtualDisk Layer

The VirtualDisk module provides fixed-size virtual disk storage backed by binary files.

### Disk Configuration

| Property | Value |
|---|---|
| Block size | 4096 bytes |
| Blocks per virtual disk | 256 |
| Valid block indices | 0–255 |
| Capacity per virtual disk | 1,048,576 bytes (1 MiB) |

The planned architecture uses three virtual disks: `disk1.bin`, `disk2.bin`, and `disk3.bin`. The present module provides the foundation for managing these disks; the three-disk replication layer is not yet implemented.

### Implemented Operations

- `create()` — Creates a new virtual disk, initializes its blocks and prevents existing-file overwrites.
- `open()` — Opens an existing virtual disk and validates its file size.
- `close()` — Closes the disk stream and resets its state.
- `writeBlock()` — Writes one 4096-byte block at a valid block index.
- `readBlock()` — Reads one 4096-byte block at a valid block index.

### Error Handling

The module includes block-boundary validation, file-open state checks, read/write error detection, disk-size verification and creation-failure cleanup attempts.

## Automated Testing

The VirtualDisk test executable is registered with CTest as `VirtualDiskTest`.

To build the test executable:

```powershell
cmake --build build --target test_virtual_disk
```

To run the automated test:

```powershell
ctest --test-dir build --output-on-failure
```

For detailed test output:

```powershell
ctest --test-dir build -V
```

The tests cover:

- Disk creation, opening and double-open protection
- Block reading, writing and data verification
- Persistence across disk close and reopen
- First and last valid block access
- Invalid block index rejection
- Closed disk read/write protection
- Double-close and reopen behavior
- Missing disk protection
- Incorrect disk-size rejection
- Temporary test-disk cleanup

Tests generate timestamp-based temporary disk filenames and perform cleanup during normal test completion. Existing data disks are not intentionally overwritten.

## Current Status

**Phase 1:** Initial project setup, C++17 configuration, CMake integration, GitHub repositories and project documentation completed.

**Phase 2:** VirtualDisk implementation and automated testing completed. The virtual disk supports fixed-size block storage, safe disk creation and opening, block read/write operations, error handling and persistence across restarts.

**Phase 3:** Filesystem metadata and inode-based file management implemented and tested. The current implementation includes:

- Superblock and persistent filesystem metadata
- Inode table and allocation bitmaps
- Direct and single-indirect block addressing
- File creation, reading, writing and deletion
- Filesystem persistence across application restarts
- Command-line interface for filesystem operations

**Verification (10 November 2026):**

- Windows CMake build: PASSED
- VirtualDiskTest: PASSED
- FileSystemTest: PASSED
- Automated tests: 2/2 PASSED
- CLI file creation, writing and reading: PASSED
- File persistence after application restart: PASSED

**Future Development:** Three-copy file-data replication, CRC32 integrity checking, corruption simulation and automatic recovery from a valid replica remain planned.

## Project Scope

VaultFS is an educational software engineering project and is not intended to replace a production filesystem.
