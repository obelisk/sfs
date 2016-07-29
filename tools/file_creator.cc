#include "StorageManager.h"
#include "FileManager.h"

#include <iostream>
#include <fstream>

int main(int argc, char ** argv){
		std::cout << "File Creator Example Program.\n";
		std::string root_dir = "/tmp/sfs-files/";
		std::string password = "viveriuniversumvivusvici";
		std::string data = "This is example text in the file.\n";

		StorageManager* sm = new StorageManager(root_dir, password);
		FileManager *fm = new FileManager(sm, password);

		if(!fm->isReady()){
			std::cout << "Filesystem was not ready. Check output for errors.\n";
			delete fm;
			return 1;
		}
		fm->createNewFile("TestFile.txt");
		fm->writeToFile("TestFile.txt", data.c_str(), 0, data.size());
		fm->flushFile("TestFile.txt");
		delete fm;
    return 0;
}
