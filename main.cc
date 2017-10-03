#include "StorageManager.h"
#include "FileManager.h"
#include "FuseFS.h"

#include <iostream>
#include <fstream>

/// Get the path that contains the files for the hidden filesystem
std::string getStegPath(int argc, char ** argv) {
	for (int i = 0; i < argc; ++i) {
		if (strncmp(argv[i], "-f", 2) == 0 && i != argc) {
			return std::string(argv[i+1]);
		}
	}
	std::cout << "Couldn't find steg path\n";
	return "";
}

/// Get the path for mounting the filesystem
std::string getMountPath(int argc, char ** argv) {
	for (int i = 0; i < argc; ++i) {
		if (strncmp(argv[i], "-m", 2) == 0 && i != argc) {
			return std::string(argv[i+1]);
		}
	}
	std::cout << "Couldn't find mount path\n";
	return "";
}

/// See if the user wants to skip flag verification
bool getNoVerify(int argc, char ** argv) {
	for (int i = 0; i < argc; ++i) {
		if (strncmp(argv[i], "-n\0", 3) == 0) {
			return true;
		}
	}
	return false;
}

/// See if the user wants to run in verbose mode
bool getVerbose(int argc, char ** argv) {
	for (int i = 0; i < argc; ++i) {
		if (strncmp(argv[i], "-v\0", 3) == 0) {
			return true;
		}
	}
	return false;
}

/// See if the user wants to order files lexographically
bool getLexical(int argc, char ** argv) {
	for (int i = 0; i < argc; ++i) {
		if (strncmp(argv[i], "-l\0", 3) == 0) {
			return true;
		}
	}
	return false;
}

int main(int argc, char ** argv){
	int status = 0;
	char answer = 0x00;
	std::string mountDir = getMountPath(argc, argv);
	std::string rootDir = getStegPath(argc, argv);
	bool verbose_mode = getVerbose(argc, argv);
	const char* fuseFlags[3];
	fuseFlags[0] = argv[0];
	fuseFlags[1] = "-d";
	fuseFlags[2] = mountDir.c_str();
	std::string password{};

	std::cout << "Password: ";
	SetStdinEcho(false);
	std::cin >> password;
	SetStdinEcho(true);
	std::cout << "\n";
	if (!getNoVerify(argc, argv)) {
		std::cout << "These are the settings that will be used:\n";
		std::cout << "\tBlock Writes:\t" << DEFAULTS_BLOCKWRITES << "\n";
		std::cout << "Continue? (y/n): ";
		std::cin >> answer;
		if(answer != 'y'){
			return 0;
		}
	}
	StorageManager* sm = new StorageManager(rootDir, password, getLexical(argc, argv));
	if (verbose_mode) {
		sm->printStegPieces();
	}
	FileManager *fm = new FileManager(sm, password);
	if(fm->isReady()){
		FuseHandler *fh = new FuseHandler(fm);
		status = fh->start(3, (char**)fuseFlags);
    		delete fh;
	}else{
		std::cout << "Filesystem was not ready. Check output for errors.\n";
		delete fm;
	}
    return 0;
}
