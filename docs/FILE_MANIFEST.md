# File manifest

## Preserved, byte-for-byte from supplied Phase 2 files
- `include/VirtualDisk.h` ← `VirtualDisk(1).h`
- `src/VirtualDisk.cpp` ← `VirtualDisk(1).cpp`
- `tests/test_virtual_disk.cpp` ← `test_virtual_disk(2).cpp`

## New for Phase 3
- `include/FileSystem.h` — filesystem API, inode metadata types
- `src/FileSystem.cpp` — formatting, mounting, allocation, serialization, directory, and inode block pointers
- `src/main.cpp` — sample interactive CLI (the prior `main.cpp` was not uploaded)
- `tests/test_file_system.cpp` — Phase 3 integration test
- `docs/PHASE3_PRESENTATION.md` — short presentation speaking notes

## Updated
- `CMakeLists.txt` — adds core library, new source files, and Phase 3 CTest
- `.gitignore` — generated files excluded
- `README.md` — build, demonstrations and limitations

## Verification performed

Filesystem source and integration test compiled with g++ on Linux and passed against a temporary portable implementation of the *same VirtualDisk API*, used strictly for verification. CLI create/read/reopen/delete also passed through this adapter. The submitted `src/VirtualDisk.cpp` remains the original **Windows-specific code**; full compilation and CTest on Windows / MinGW are **not verified in this environment**.
