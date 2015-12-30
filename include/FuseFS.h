#pragma once
#define FUSE_USE_VERSION 30

#include "FileManager.h"

#include <iostream>
#include <map>
#include <cstring>
#include <string>

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include <fuse.h>
#include <fuse_lowlevel.h>

const std::string info_file = "/README";

// Simple FUSE implementation. No folders, no attributes, just files. Just simple
// for now.

class FuseHandler {
  public:
	static FileManager * fm;
  	FuseHandler(FileManager* path);
  	//static int configure(FileManager * rfm);
  	static int start(int argc, char ** argv);
  	~FuseHandler();
};