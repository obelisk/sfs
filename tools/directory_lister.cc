#include "StorageManager.h"
#include "FileManager.h"

#include <iostream>
#include <fstream>

int main(int argc, char ** argv){
		std::cout << "Directory Lister Example Program.\n";
		std::string root_dir = "/tmp/sfs-files/";
		std::string password = "viveriuniversumvivusvici";

		StorageManager* sm = new StorageManager(root_dir, password);
		FileManager *fm = new FileManager(sm, password);

		if(!fm->isReady()){
			std::cout << "Filesystem was not ready. Check output for errors.\n";
			delete fm;
			return 1;
		}
		std::map<std::string, FileInfo_t> files = fm->getFileMap();
		for (auto const &file : files){
			std::cout << file.first << "\n";
		}
		delete fm;
    return 0;
}
