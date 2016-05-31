#include "FileManager.h"
#include "FuseFS.h"

#include <iostream>
#include <fstream>

int main(int argc, char ** argv){
	int status = 0;
	std::string root_dir;
	std::string password;
	root_dir.clear();
	std::cout << "Where can I find my files: ";
	std::cin >> root_dir;
	std::cout << "Password: ";
	SetStdinEcho(false);
	std::cin >> password;
	SetStdinEcho(true);
	std::cout << "\n";
	StorageManager* sm = new StorageManager(root_dir, password);
	sm->printStegPieces();
	FileManager *fm = new FileManager(sm, password);
	if(fm->isReady()){
		FuseHandler *fh = new FuseHandler(fm);
		status = fh->start(argc, argv);
    		delete fh;
	}else{
		std::cout << "Filesystem was not ready. Check output for errors.\n";
		delete fm;
	}
    return 0;
}
