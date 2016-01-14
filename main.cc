#include "FileManager.h"
#include "FuseFS.h"

#include <iostream>
#include <fstream>

int main(int argc, char ** argv){
	int status = 0;
	std::string root_dir = "/Users/Mitchell/Pictures/Misc/rec";
	StorageManager* sm = new StorageManager(root_dir);
	sm->printStegPieces();
	FileManager *fm = new FileManager(sm);
	FuseHandler *fh = new FuseHandler(fm);
	status = fh->start(argc, argv);
    delete fh;
    return 0;

	unsigned char buf[sm->getStegSize()];
	char * bufPtr = (char*)buf;
	memset(bufPtr, 0, sm->getStegSize());
	std::cout << "Dumping data from files...\n";
	status = sm->read(buf, 0, sm->getStegSize());
	if(status != SUCCESS){
		std::cout << "Read failed!\n";
		return 0;
	}
	std::ofstream outfile ("/Users/Mitchell/SpiderOak Hive/sfs/extractedfs.bin", std::ofstream::binary);
	outfile.write (bufPtr, sm->getStegSize());
	outfile.close();
	
	return 0;
}