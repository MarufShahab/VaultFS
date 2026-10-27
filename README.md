
# VaultFS

VaultFS is an educational inode-based mini filesystem
implemented in C++.

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

## Build Instructions

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

## Run

```powershell
.\build\vaultfs.exe
```

## Current Status

Phase 1: Initial project setup.

Filesystem features are planned but not yet implemented.
