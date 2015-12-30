#pragma once

#include "StorageManager.h"

#include <iostream>
#include <map>
#include <cstring>

#include <boost/filesystem.hpp>

#define MAX_NAME_LEN 128
#define BLOCK_SIZE 4096
#define SFS_HEAD_OFF 8
#define USED_KEY "MGCX"
#define USED_KEY_LEN 4
#define DATA_LENGTH_SIZE 4
#define DATA_BLOCK_PTR_SIZE 4
#define FILE_SIZE_OFF 4
#define FILE_NAME_OFF 8
#define BLOCK_PTR_OFF USED_KEY_LEN + DATA_LENGTH_SIZE + MAX_NAME_LEN
#define DATA_BLOCK_H_SIZE USED_KEY_LEN
#define DATA_BLOCK_SIZE BLOCK_SIZE - DATA_BLOCK_H_SIZE

// Simple FS implementation, probably should be replaced/improved later
// Block size 4KiB, first block defines filesystem and has the following
// format.
// Key (4 bytes) - Check to see if there is a sFS here at all
// Number of files (4 bytes) - Tells you the number of files in the system.

// If this number of files fills the whole block, then the last 4 bytes will
// be the location of the next data block. (To be implemented)

// Files in the system are located as such:
// There is a pointer to a block of memory in the filesystem. This is the indirection block
// This block has the following header of 136 bytes.
// USED_KEY         4 bytes
// File Length      4 bytes
// File name/path   128 bytes

// From here, every 4 bytes after that (up to Math.ceil(File Length/BLOCK_SIZE)) is a pointer
// to the next block that makes up the file. Given this setup, a single file can be
// no larger than around 4MB. However, given the purpose, and prospective size of the file
// system, for now I'm deeming this acceptable even if in practice it is not.

// The format of data blocks is as such:
// USED_KEY         4 bytes
// Data             BLOCK_SIZE-4

// Note: When a block is owned, it is zeroed out.

// This is the part of the system that builds a simple filesystem
// to provide to FUSE/other system

typedef struct{
  unsigned int size;
  unsigned int location;
} FileInfo_t;

class FileManager {
  private:
    StorageManager * sm;

    // A map of filenames (with paths eventually) to memory locations
    std::map<std::string, FileInfo_t> fileMap;
    unsigned int findOpenBlock();
    unsigned int findOpenFSPointer();
    unsigned int findUsedFSPointer(unsigned int ptr);
  public:
  	FileManager(StorageManager* path);
    std::map<std::string, FileInfo_t> getFileMap();
    FileInfo_t getFileInfo(std::string path);
  	~FileManager();

    void format();

    int deleteFile(const char* path);
    int createNewFile(const char* path);
    int setFileLength(std::string path, size_t newSize);
    int writeToFile(std::string path, const void* rdata, unsigned int placeInFile, unsigned int length);
    int readFromFile(std::string path, void* rdata, unsigned int placeInFile, unsigned int length);
};