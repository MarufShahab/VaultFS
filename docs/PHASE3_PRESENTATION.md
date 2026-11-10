# VaultFS Phase 3 — presentation notes

**Goal:** Transform fixed-size block I/O from Phase 2 into a persistent inode-based single-disk filesystem.

**Architecture:** CLI → FileSystem → VirtualDisk → `disk1.bin` (or a safely chosen demo `.bin` filename).

**Key design:** 1 MiB physical disk, 256 blocks of 4096 bytes; six reserved metadata blocks; 250 allocatable data/indirect blocks; 64 fixed 128-byte inodes. Root directory contains at most 64 entries but only 63 file inodes are available (inode 0 is root). Four direct pointers + one single-indirect pointer per inode. The root directory is stored in fixed block 5.

**Show:** `touch`, `write`, `cat`, `ls`, `df`, restart and `cat` again, `rm`. Run `ctest --test-dir build -V` to show existing Phase 2 and new Phase 3 tests.

**Trade-offs:** Fixed small structure is easy to reason about and serialize. One inode table is simpler but not scalable. One fixed root directory keeps the exercise focused. Copy-on-write data allocation reduces overwriting old data, but the multi-block metadata update remains vulnerable to interrupted writes. Single-disk only for now; intended data replication and CRC recovery belong to later phases.

**Accurate project status:** Phase 3 source is complete for the stated educational scope, but Windows build/test execution must still be demonstrated on the presenter's own machine before claiming it passed there. The overall replicated VaultFS project is not yet complete.
