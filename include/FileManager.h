#pragma once

#include "StorageManager.h"

#include <stdlib.h>
#if __linux__
#include <bsd/stdlib.h>
#endif

#include <iostream>
#include <map>
#include <algorithm>
#include <cstring>

#include <boost/filesystem.hpp>

#include <cryptopp/cryptlib.h>
using CryptoPP::Exception;

#include <cryptopp/hex.h>
using CryptoPP::HexEncoder;
using CryptoPP::HexDecoder;

#include <cryptopp/filters.h>
using CryptoPP::StringSink;
using CryptoPP::StringSource;
using CryptoPP::StreamTransformationFilter;

#include <cryptopp/aes.h>
using CryptoPP::AES;

#include <cryptopp/ccm.h>
using CryptoPP::CBC_Mode;

#define MAX_NAME_LEN 128
#define BLOCK_SIZE 4096
#define SFS_HEAD_OFF 4
#define ENC_IV_LEN 16
#define DATA_LENGTH_SIZE 4
#define DATA_BLOCK_PTR_SIZE 4
#define FILE_SIZE_OFF 0
#define FILE_NAME_OFF DATA_LENGTH_SIZE + ENC_IV_LEN 
#define FILE_IV_OFF DATA_LENGTH_SIZE
#define BLOCK_PTR_OFF FILE_NAME_OFF + MAX_NAME_LEN
#define DATA_BLOCK_SIZE BLOCK_SIZE - DATA_BLOCK_H_SIZE

// Simple FS implementation, probably should be replaced/improved later
// Block size 4KiB, first block defines filesystem and has the following
// format.
// IV (16 bytes) - The initialization vector for the MFT 
// Key (4 bytes) - Check to see if there is a sFS here at all

// Files in the system are located as such:
// There is a pointer to a block of memory in the filesystem. This is the indirection block
// This block has the following header of 132 bytes.
// IV               16 bytes      This is outside th encrypted container

// File Length      4 bytes
// File IV          16 bytes
// File name/path   128 bytes

// From here, every 4 bytes after that (up to Math.ceil(File Length/BLOCK_SIZE)) is a pointer
// to the next block that makes up the file. Given this setup, a single file can be
// no larger than around 4MB. However, given the purpose, and prospective size of the file
// system, for now I'm deeming this acceptable even if in practice it is not.

// The format of data blocks is as such:
// USED_KEY         4 bytes
// Data             BLOCK_SIZE-4

// This is the part of the system that builds a simple filesystem
// to provide to FUSE/other systems

typedef struct{
  unsigned int loaded;
  unsigned int size;
  unsigned int location;
  unsigned int writes;
  std::string data;
  std::string bptr;
} FileInfo_t;

class FileManager {
  private:
    StorageManager * sm;

    // A map of filenames (with paths eventually) to memory locations
    std::map<std::string, FileInfo_t> fileMap;
    std::list<unsigned int> free_blocks;
    std::list<unsigned int> used_blocks;
    std::string fskey;
    std::string memmft;

    unsigned int findOpenBlock();
    unsigned int claimOpenBlock();
    unsigned int findOpenFSPointer();
    unsigned int findUsedFSPointer(unsigned int ptr);
    unsigned int flushMFT();
    unsigned int flushFileIndirection(FileInfo_t fi);
    int loadFile(std::string path);
    std::string readFileSystemBlock(unsigned int blockNumber);
    std::string decryptMFT(std::string mft);
    std::string createEncMFT();
    std::string encryptPtrBlock(std::string dblock, std::string iv);
  public:
    FileManager(StorageManager* path, std::string key);
    std::map<std::string, FileInfo_t> getFileMap();
    FileInfo_t getFileInfo(std::string path);
  	~FileManager();

    void openFileSystem();
    unsigned int flushFile(std::string path);
    int deleteFile(const char* path);
    int createNewFile(const char* path);
    int renameFile(std::string path, std::string npath);
    int setFileLength(std::string path, size_t newSize);
    int writeToFile(std::string path, const void* rdata, unsigned int placeInFile, unsigned int length);
    int readFromFile(std::string path, void* rdata, unsigned int placeInFile, unsigned int length);
};
