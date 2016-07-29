#pragma once

#include <vector>
#include <list>
#include <iostream>

#define STEG_INSUFFICIENT_SIZE -1

typedef struct{
	char * data;
	int length;
	int location;
} cached_write_t;

class StegFile {
  public:
  	std::string path;
  	std::string name;
  	std::list<cached_write_t> cached_writes;
  	virtual ~StegFile(){}
	virtual size_t getSize() = 0;
	virtual size_t getStegSize() = 0;
	virtual int read(char* data, int location, int length) = 0;
	virtual int write(const char* data, int location, int length) = 0;
	virtual int flush() = 0;
	virtual std::string getStegType() = 0;
};