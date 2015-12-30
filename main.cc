#include "FileManager.h"
#include "FuseFS.h"

#include <iostream>
#include <fstream>

int main(int argc, char ** argv){
	int status = 0;
	std::string root_dir = "/Users/Mitchell/Pictures/Misc/rec";
	StorageManager* sm = new StorageManager(root_dir);
	sm->printStegPieces();
	int fresh = 0;
	char formatQ = '\0';
	std::cout << "Do you want to force a format?: ";
	std::cin >> formatQ;
	if(formatQ == 'y'){
		sm->write(&fresh, 0, 4); sm->write(&fresh, 4, 4);
	}
	FileManager *fm = new FileManager(sm);
	FuseHandler *fh = new FuseHandler(fm);
	status = fh->start(argc, argv);
    delete fh;
    return 0;/**/

	unsigned int fileSize = 81920;
	unsigned char buf[fileSize];
	char * bufPtr = (char*)buf;
	memset(bufPtr, 0, fileSize);
	std::cout << "Testing full system read...Hold on to your butts...again...\n";
	status = sm->read(buf, 0, fileSize);
	if(status != SUCCESS){
		std::cout << "Read failed!\n";
		return 0;
	}
	std::ofstream outfile ("/Users/Mitchell/SpiderOak Hive/sfs/extractedfs.bin", std::ofstream::binary);
	outfile.write (bufPtr, fileSize);
	outfile.close();/**/
	
	return 0;
}