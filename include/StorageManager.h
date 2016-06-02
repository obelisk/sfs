#pragma once

#include "assert.h"

#include "StegType.h"
#include "Jpeg.h"

#include <iostream>
#include <vector>
#include <random>

#include <boost/filesystem.hpp>

#include <cryptopp/osrng.h>
#include <cryptopp/cryptlib.h>

#define SUCCESS 0
#define HIT_DISK_END -2

#define DEFAULTS_BLOCKWRITES true
// This is the part of the system that handles raw write requests
// and translates them into writes across the steg pieces

typedef struct{
    bool blockWrites;
} StorageManagerConf_t;

class StorageManager {
  private:
  	size_t size, stegSize;
        unsigned long filepermseed;
	
	StorageManagerConf_t conf;
  	
	// The top level folder holding all the steg pieces
  	std::string path;
  	std::vector<StegFile*> components;

  	void getAllStegPieces(bool recurse);
  	void organizeFiles(std::vector<std::string>& files);
  	void computeSize();
  public:
    	StorageManager(std::string path, unsigned long filepermseed = 0, StorageManagerConf_t rconf = {DEFAULTS_BLOCKWRITES});
    	StorageManager(std::string path, std::string key, StorageManagerConf_t rconf = {DEFAULTS_BLOCKWRITES});
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
