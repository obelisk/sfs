#pragma once

#include <vector>
#include <iostream>

#define STEG_INSUFFICIENT_SIZE -1

class StegFile {
  public:
  	std::string path;
  	std::string name;
  	virtual ~StegFile(){}
	virtual size_t getSize() = 0;
	virtual size_t getStegSize() = 0;
	virtual int write(const char* data, int location, int length) = 0;
	virtual int read(char* data, int location, int length) = 0;
	virtual std::string getStegType() = 0;
};