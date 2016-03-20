#include "FileManager.h"
#include "FuseFS.h"

#include <iostream>
#include <fstream>

int main(int argc, char ** argv){
	int status = 0;
	std::string root_dir = "/Users/Mitchell/Pictures/Misc/rec/";
	std::string password;
	//root_dir.clear();
	//std::cout << "Where can I find my files: ";
	//std::cin >> root_dir;
	std::cout << "Password: ";
	SetStdinEcho(false);
	std::cin >> password;
	SetStdinEcho(true);
	StorageManager* sm = new StorageManager(root_dir, password);
	sm->printStegPieces();
	FileManager *fm = new FileManager(sm);
	FuseHandler *fh = new FuseHandler(fm);
	status = fh->start(argc, argv);
    delete fh;
    return 0;
}