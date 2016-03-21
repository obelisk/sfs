#pragma once

#include "osrng.h"
using CryptoPP::AutoSeededRandomPool;

#include "cryptlib.h"
using CryptoPP::Exception;

#include "hex.h"
using CryptoPP::HexEncoder;
using CryptoPP::HexDecoder;

#include "filters.h"
using CryptoPP::StringSink;
using CryptoPP::StringSource;
using CryptoPP::StreamTransformationFilter;

#include "aes.h"
using CryptoPP::AES;

#include "ccm.h"
using CryptoPP::CBC_Mode;

#include "assert.h"

#include "StegType.h"
#include "Jpeg.h"

#include <iostream>
#include <vector>
#include <random>
#include <boost/filesystem.hpp>

#define SUCCESS 0
#define HIT_DISK_END -2

// This is the part of the system that handles raw write requests
// and translates them into writes across the steg pieces

class StorageManager {
  private:
  	// The top level folder holding all the steg pieces
  	std::string path;
  	std::vector<StegFile*> components;
  	size_t size, stegSize;
    unsigned long filepermseed;

  	void getAllStegPieces(bool recurse);
  	void organizeFiles(std::vector<std::string>& files);
  	void computeSize();
  public:
  	StorageManager(std::string path);
    StorageManager(std::string path, unsigned long filepermseed);
    StorageManager(std::string path, std::string key);
  	~StorageManager();

  	// Start the FUSEFS filesystem
  	void startFS();

  	// Get the size that all the files are taking on disk
	  size_t getApparentSize();

	  // Get the size of the virtual steg disk
	  size_t getStegSize();

	  // Output the files that make up the steg disk
	  void printStegPieces();

    // Read data, of length length, from location in the steg disk
    int read(void* rdata, int location, int length);

	  // Write data, of length length, to location in the steg disk
	  int write(const void* rdata, int location, int length);

    // Writes will not have been sent to disk yet, we need to flush the data to make that happen
    int flush();

};