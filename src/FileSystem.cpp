#include "FileSystem.h"
#include <algorithm>
#include <cstring>
#include <limits>

namespace {
using Block = std::array<char, VirtualDisk::BLOCK_SIZE>;
constexpr std::uint16_t INVALID = FileSystem::INVALID;
void put16(Block& b, std::size_t p, std::uint16_t v) {
    b[p] = static_cast<char>(v & 255);
    b[p+1] = static_cast<char>((v >> 8) & 255);
}
std::uint16_t get16(const Block& b, std::size_t p) {
    return static_cast<std::uint8_t>(b[p]) |
           (static_cast<std::uint16_t>(static_cast<std::uint8_t>(b[p+1])) << 8);
}
void put32(Block& b, std::size_t p, std::uint32_t v) {
    for (int i=0; i<4; ++i) b[p+i] = static_cast<char>((v >> (8*i)) & 255);
}
std::uint32_t get32(const Block& b, std::size_t p) {
    std::uint32_t v=0;
    for (int i=0; i<4; ++i) v |= static_cast<std::uint32_t>(static_cast<std::uint8_t>(b[p+i])) << (8*i);
    return v;
}
}

FileSystem::FileSystem(VirtualDisk& disk) : disk_(disk) {}
bool FileSystem::readBlock(std::uint16_t n, Block& b) { return disk_.readBlock(n, b); }
bool FileSystem::writeBlock(std::uint16_t n, const Block& b) { return disk_.writeBlock(n, b); }

bool FileSystem::format() {
    if (mounted_) return false;
    Block initial{};
    if (!readBlock(0, initial)) return false;
    // Never format an existing filesystem, even when its version is unrecognized.
    for (char c : initial) if (c != 0) return false;
    inodeUsed_.fill(0); blockUsed_.fill(0);
    inodes_.fill(Inode{}); entries_.fill(Entry{});
    for (std::uint16_t i=0; i<DATA_START; ++i) blockUsed_[i]=1;
    inodeUsed_[0]=1;
    inodes_[0].kind=2;
    inodes_[0].size=0;
    inodes_[0].direct[0]=5;
    if (!saveMetadata()) return false;
    mounted_=true;
    return true;
}

bool FileSystem::saveMetadata() {
    Block super{};
    const char magic[8]={'V','A','U','L','T','F','S','3'};
    std::copy(magic, magic+8, super.begin());
    put32(super,8,1); // format version
    put32(super,12,VirtualDisk::BLOCK_SIZE);
    put32(super,16,VirtualDisk::TOTAL_BLOCKS);
    put16(super,20,INODE_COUNT);
    put16(super,22,DATA_START);
    put16(super,24,0); // root inode
    Block inodeBitmap{}; Block blockBitmap{};
    for (std::size_t i=0;i<INODE_COUNT;++i) inodeBitmap[i]=static_cast<char>(inodeUsed_[i]);
    for (std::size_t i=0;i<VirtualDisk::TOTAL_BLOCKS;++i) blockBitmap[i]=static_cast<char>(blockUsed_[i]);
    Block table0{}; Block table1{};
    for (std::size_t i=0;i<INODE_COUNT;++i) {
        Block& b = i<32 ? table0 : table1;
        const std::size_t p=(i%32)*128;
        b[p]=static_cast<char>(inodes_[i].kind);
        put32(b,p+4,inodes_[i].size);
        for (std::size_t j=0;j<DIRECT_COUNT;++j) put16(b,p+8+j*2,inodes_[i].direct[j]);
        put16(b,p+16,inodes_[i].indirect);
    }
    Block directory{};
    for (std::size_t i=0;i<entries_.size();++i) {
        std::size_t p=i*64;
        put16(directory,p,entries_[i].inode);
        if (entries_[i].inode!=INVALID) {
            if (entries_[i].name.empty() || entries_[i].name.size()>61) return false;
            std::copy(entries_[i].name.begin(),entries_[i].name.end(),directory.begin()+p+2);
        }
    }
    // Write superblock LAST so a newly formatted disk isn't accepted before metadata is written.
    return writeBlock(1,inodeBitmap) && writeBlock(2,blockBitmap) &&
           writeBlock(3,table0) && writeBlock(4,table1) &&
           writeBlock(5,directory) && writeBlock(0,super);
}

bool FileSystem::loadMetadata() {
    Block super{};
    if (!readBlock(0,super)) return false;
    if (std::string(super.data(),8)!="VAULTFS3" || get32(super,8)!=1 ||
        get32(super,12)!=VirtualDisk::BLOCK_SIZE ||
        get32(super,16)!=VirtualDisk::TOTAL_BLOCKS ||
        get16(super,20)!=INODE_COUNT || get16(super,22)!=DATA_START ||
        get16(super,24)!=0) return false;
    Block bitmapInode{}, bitmapBlock{}, table0{}, table1{}, directory{};
    if (!readBlock(1,bitmapInode) || !readBlock(2,bitmapBlock) ||
        !readBlock(3,table0) || !readBlock(4,table1) || !readBlock(5,directory)) return false;
    for (std::size_t i=0;i<INODE_COUNT;++i) {
        inodeUsed_[i]=static_cast<std::uint8_t>(bitmapInode[i]);
        if (inodeUsed_[i]>1) return false;
        const Block& b=i<32?table0:table1;
        std::size_t p=(i%32)*128;
        auto& n=inodes_[i];
        n.kind=static_cast<std::uint8_t>(b[p]);
        n.size=get32(b,p+4);
        for (std::size_t j=0;j<DIRECT_COUNT;++j) n.direct[j]=get16(b,p+8+j*2);
        n.indirect=get16(b,p+16);
        if ((inodeUsed_[i]==0 && n.kind!=0) || (inodeUsed_[i]==1 && (n.kind!=1 && n.kind!=2))) return false;
    }
    for (std::size_t i=0;i<VirtualDisk::TOTAL_BLOCKS;++i) {
        blockUsed_[i]=static_cast<std::uint8_t>(bitmapBlock[i]);
        if (blockUsed_[i]>1 || (i<DATA_START && blockUsed_[i]!=1)) return false;
    }
    if (inodeUsed_[0]!=1 || inodes_[0].kind!=2 || inodes_[0].direct[0]!=5) return false;
    std::array<bool, INODE_COUNT> seen{};
    for (std::size_t i=0;i<entries_.size();++i) {
        std::size_t p=i*64;
        std::uint16_t id=get16(directory,p);
        entries_[i]=Entry{};
        if (id==INVALID) continue;
        if (id==0 || id>=INODE_COUNT || !inodeUsed_[id] || inodes_[id].kind!=1 || seen[id]) return false;
        seen[id]=true;
        const char* start=directory.data()+p+2;
        const char* end=start+62;
        const char* zero=std::find(start,end,'\0');
        if (zero==start || zero==end) return false;
        entries_[i].inode=id;
        entries_[i].name.assign(start,zero);
        if (entries_[i].name=="." || entries_[i].name==".." ||
            entries_[i].name.find('/')!=std::string::npos || entries_[i].name.find('\\')!=std::string::npos) return false;
        for (std::size_t k=0;k<i;++k) if (entries_[k].inode!=INVALID && entries_[k].name==entries_[i].name) return false;
    }
    for (std::size_t i=1;i<INODE_COUNT;++i) if (inodeUsed_[i] && !seen[i]) return false;
    // Validate that allocated pointers are unique and consistent with the bitmap.
    std::array<bool, VirtualDisk::TOTAL_BLOCKS> seenBlocks{};
    for (std::uint16_t i=0;i<DATA_START;++i) seenBlocks[i]=true;
    for (std::size_t i=1;i<INODE_COUNT;++i) if (inodeUsed_[i]) {
        const Inode& n=inodes_[i];
        std::size_t required=(static_cast<std::uint64_t>(n.size)+VirtualDisk::BLOCK_SIZE-1)/VirtualDisk::BLOCK_SIZE;
        if (required>DIRECT_COUNT+2048) return false;
        std::vector<std::uint16_t> pointers;
        if (!dataPointers(n,pointers) || pointers.size()!=required) return false;
        if (n.indirect!=INVALID) {
            if (n.indirect<DATA_START || n.indirect>=VirtualDisk::TOTAL_BLOCKS || seenBlocks[n.indirect] || !blockUsed_[n.indirect]) return false;
            seenBlocks[n.indirect]=true;
        }
        for (auto id : pointers) {
            if (id<DATA_START || id>=VirtualDisk::TOTAL_BLOCKS || seenBlocks[id] || !blockUsed_[id]) return false;
            seenBlocks[id]=true;
        }
    }
    for (std::size_t i=0;i<VirtualDisk::TOTAL_BLOCKS;++i) if (seenBlocks[i]!=static_cast<bool>(blockUsed_[i])) return false;
    return true;
}

bool FileSystem::mount() {
    if (mounted_) return false;
    if (!loadMetadata()) return false;
    mounted_=true;
    return true;
}

std::uint16_t FileSystem::allocateBlock() {
    for (std::uint16_t i=DATA_START;i<VirtualDisk::TOTAL_BLOCKS;++i)
        if (!blockUsed_[i]) { blockUsed_[i]=1; return i; }
    return INVALID;
}
std::uint16_t FileSystem::allocateInode() {
    for (std::uint16_t i=1;i<INODE_COUNT;++i)
        if (!inodeUsed_[i]) { inodeUsed_[i]=1; inodes_[i]=Inode{}; return i; }
    return INVALID;
}
int FileSystem::findEntry(const std::string& name) const {
    for (std::size_t i=0;i<entries_.size();++i)
        if (entries_[i].inode!=INVALID && entries_[i].name==name) return static_cast<int>(i);
    return -1;
}
int FileSystem::findEmptyEntry() const {
    for (std::size_t i=0;i<entries_.size();++i)
        if (entries_[i].inode==INVALID) return static_cast<int>(i);
    return -1;
}
bool FileSystem::createFile(const std::string& name) {
    if (!mounted_ || name.empty() || name.size()>61 || name=="." || name==".." ||
        name.find('/')!=std::string::npos || name.find('\\')!=std::string::npos ||
        findEntry(name)>=0) return false;
    int slot=findEmptyEntry();
    if (slot<0) return false;
    std::uint16_t id=allocateInode();
    if (id==INVALID) return false;
    inodes_[id].kind=1;
    entries_[slot]={id,name};
    inodes_[0].size+=1;
    if (saveMetadata()) return true;
    entries_[slot]=Entry{}; inodeUsed_[id]=0; inodes_[id]=Inode{}; --inodes_[0].size;
    return false;
}

bool FileSystem::dataPointers(const Inode& n, std::vector<std::uint16_t>& result) {
    result.clear();
    const std::size_t needed=(static_cast<std::uint64_t>(n.size)+VirtualDisk::BLOCK_SIZE-1)/VirtualDisk::BLOCK_SIZE;
    if (needed>DIRECT_COUNT+2048) return false;
    for (std::size_t i=0;i<DIRECT_COUNT;++i) {
        if (i<needed) {
            if (n.direct[i]==INVALID) return false;
            result.push_back(n.direct[i]);
        } else if (n.direct[i]!=INVALID) return false;
    }
    if (needed<=DIRECT_COUNT) return n.indirect==INVALID;
    if (n.indirect==INVALID || n.indirect>=VirtualDisk::TOTAL_BLOCKS) return false;
    Block indirect{};
    if (!readBlock(n.indirect,indirect)) return false;
    for (std::size_t j=0;j<2048;++j) {
        auto id=get16(indirect,j*2);
        if (j<needed-DIRECT_COUNT) {
            if (id==INVALID) return false;
            result.push_back(id);
        } else if (id!=INVALID) return false;
    }
    return true;
}

void FileSystem::releaseBlocks(const Inode& inode) {
    std::vector<std::uint16_t> pointers;
    if (dataPointers(inode,pointers)) {
        for (auto id:pointers) if (id>=DATA_START && id<VirtualDisk::TOTAL_BLOCKS) blockUsed_[id]=0;
        if (inode.indirect!=INVALID && inode.indirect>=DATA_START && inode.indirect<VirtualDisk::TOTAL_BLOCKS)
            blockUsed_[inode.indirect]=0;
    }
}

bool FileSystem::writeFile(const std::string& name, const std::string& bytes) {
    if (!mounted_ || bytes.size()>MAX_FILE_BYTES || bytes.size()>std::numeric_limits<std::uint32_t>::max()) return false;
    int slot=findEntry(name); if (slot<0) return false;
    std::uint16_t id=entries_[slot].inode;
    Inode previous=inodes_[id];
    auto previousBitmap=blockUsed_;
    std::size_t needed=(bytes.size()+VirtualDisk::BLOCK_SIZE-1)/VirtualDisk::BLOCK_SIZE;
    // Copy-on-write block allocation: do not overwrite the previous file's blocks.
    std::vector<std::uint16_t> allocated;
    Inode next; next.kind=1; next.size=static_cast<std::uint32_t>(bytes.size());
    auto rollback=[&]() { blockUsed_=previousBitmap; inodes_[id]=previous; return false; };
    if (needed>DIRECT_COUNT) {
        next.indirect=allocateBlock();
        if (next.indirect==INVALID) return rollback();
    }
    for (std::size_t i=0;i<needed;++i) {
        auto block=allocateBlock(); if (block==INVALID) return rollback();
        allocated.push_back(block);
        if (i<DIRECT_COUNT) next.direct[i]=block;
    }
    Block indirect{}; indirect.fill(static_cast<char>(255));
    for (std::size_t i=DIRECT_COUNT;i<needed;++i) put16(indirect,(i-DIRECT_COUNT)*2,allocated[i]);
    for (std::size_t i=0;i<needed;++i) {
        Block block{};
        const std::size_t offset=i*VirtualDisk::BLOCK_SIZE;
        const std::size_t count=std::min<std::size_t>(VirtualDisk::BLOCK_SIZE,bytes.size()-offset);
        std::copy_n(bytes.data()+offset,count,block.begin());
        if (!writeBlock(allocated[i],block)) return rollback();
    }
    if (next.indirect!=INVALID && !writeBlock(next.indirect,indirect)) return rollback();
    // Write new inode/bitmap. Crash consistency across separate metadata blocks is not guaranteed.
    inodes_[id]=next;
    releaseBlocks(previous);
    if (saveMetadata()) return true;
    return rollback();
}

bool FileSystem::readFile(const std::string& name, std::string& bytes) {
    if (!mounted_) return false;
    int slot=findEntry(name); if (slot<0) return false;
    const auto& n=inodes_[entries_[slot].inode];
    std::vector<std::uint16_t> blocks;
    if (!dataPointers(n,blocks)) return false;
    std::string result;
    result.reserve(n.size);
    for (auto id:blocks) {
        if (id<DATA_START || id>=VirtualDisk::TOTAL_BLOCKS || !blockUsed_[id]) return false;
        Block block{}; if (!readBlock(id,block)) return false;
        result.append(block.data(),std::min<std::size_t>(VirtualDisk::BLOCK_SIZE,n.size-result.size()));
    }
    bytes.swap(result);
    return true;
}

bool FileSystem::deleteFile(const std::string& name) {
    if (!mounted_) return false;
    int slot=findEntry(name); if (slot<0) return false;
    std::uint16_t id=entries_[slot].inode;
    auto oldBitmap=blockUsed_;
    Inode oldInode=inodes_[id]; Entry oldEntry=entries_[slot];
    releaseBlocks(oldInode);
    inodes_[id]=Inode{}; inodeUsed_[id]=0; entries_[slot]=Entry{}; --inodes_[0].size;
    if (saveMetadata()) return true;
    blockUsed_=oldBitmap; inodes_[id]=oldInode; inodeUsed_[id]=1;
    entries_[slot]=oldEntry; ++inodes_[0].size;
    return false;
}
std::vector<std::string> FileSystem::listFiles() const {
    std::vector<std::string> list;
    if (!mounted_) return list;
    for (const auto& entry:entries_) if (entry.inode!=INVALID) list.push_back(entry.name);
    return list;
}
std::uint32_t FileSystem::fileSize(const std::string& name) const {
    int slot=findEntry(name);
    return mounted_ && slot>=0 ? inodes_[entries_[slot].inode].size : 0;
}
std::size_t FileSystem::freeBlockCount() const {
    return mounted_ ? std::count(blockUsed_.begin(),blockUsed_.end(),static_cast<std::uint8_t>(0)) : 0;
}
std::size_t FileSystem::freeInodeCount() const {
    return mounted_ ? std::count(inodeUsed_.begin(),inodeUsed_.end(),static_cast<std::uint8_t>(0)) : 0;
}
