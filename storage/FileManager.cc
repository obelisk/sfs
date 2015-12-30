#include "FileManager.h"

#define STEG_KEY "STEG"

namespace fs = boost::filesystem;

FileManager::FileManager(StorageManager * rsm){
	sm = rsm;
	// Find out if there is a filesystem of this type already.
	unsigned char fsBlock[BLOCK_SIZE];
	unsigned char * fsBlockPtr = (unsigned char*)fsBlock;
	unsigned int filePtr = 0;

	sm->read((char*)fsBlock, 0, BLOCK_SIZE);
	if(strncmp(STEG_KEY, (char*)fsBlock, 4) != 0){
		// We couldn't find the key, so assume there is no filesystem here.
		std::cout << "Could not find a filesystem.\n";
		std::cout << "Create a new one?\n";
		char ans;
		std::cin >> ans;
		if(ans != 'y' && ans != 'Y'){
			std::cout << "Will not create filesystem. Exiting...\n";
			exit(1);
		}
		format();
		char key[5] = "STEG";
		sm->write(key, 0, 4);
		sm->write(&filePtr, 4, 4);

		// Keep block coherent
		sm->read((char*)fsBlock, 0, BLOCK_SIZE);
	}
	std::cout << "Found filesystem...\n";

	unsigned char fileBlock[BLOCK_SIZE];
	int success = 0;
	unsigned int nameLen = 0, dataLen;
	for(int i=0; i < (BLOCK_SIZE - SFS_HEAD_OFF)/4; ++i){
		memcpy(&filePtr, fsBlock+SFS_HEAD_OFF+i*4, 4);
		if(filePtr == 0){
			// There is no file here.
			continue;
		}
		std::cout << "File block is at: " << filePtr << "\n";
		success = sm->read(fileBlock, filePtr, BLOCK_SIZE);
		if(success != 0){
			std::cout << "Could not readfile...\n";
			continue;
		}
		char name[MAX_NAME_LEN+1];
		memcpy(name, fileBlock+FILE_NAME_OFF, MAX_NAME_LEN);
		std::string strName(name, strnlen(name, MAX_NAME_LEN));
		memcpy(&dataLen, fileBlock+FILE_SIZE_OFF, 4);
		std::cout << "File Length: " << dataLen << "\t File Name: " << name << "\n";
		FileInfo_t fi;
		fi.size = dataLen;
		fi.location = *(int*)(fsBlockPtr+i*4+SFS_HEAD_OFF);
		fileMap[strName] = fi;
	}
}

FileManager::~FileManager(){
	// Delete the StorageManager
	delete sm;
}

std::map<std::string, FileInfo_t> FileManager::getFileMap(){
	return fileMap;
}

FileInfo_t FileManager::getFileInfo(std::string path){
	return fileMap[path];
}

unsigned int FileManager::findOpenBlock(){
	unsigned int totalSpace = sm->getStegSize();
	unsigned int blocks = totalSpace/BLOCK_SIZE;
	char foundKey[4];

	for(int i=1; i < blocks; ++i){
		sm->read(foundKey, BLOCK_SIZE*i, 4);
		if(strncmp(USED_KEY, foundKey, 4) != 0){
			return i*BLOCK_SIZE;
		}
	}
	return 0;
}

unsigned int FileManager::findOpenFSPointer(){
	char fsBlock[BLOCK_SIZE];
	sm->read(fsBlock, 0, BLOCK_SIZE);
	unsigned int emptyPtr = 0;
	for(int i = SFS_HEAD_OFF; i < BLOCK_SIZE; i+=4){
		memcpy(&emptyPtr, fsBlock+i, 4);
		if(emptyPtr == 0){
			return i;
		}
	}
	return 0;
}

unsigned int FileManager::findUsedFSPointer(unsigned int ptr){
	char fsBlock[BLOCK_SIZE];
	sm->read(fsBlock, 0, BLOCK_SIZE);
	unsigned int emptyPtr = 0;
	for(int i = SFS_HEAD_OFF; i < BLOCK_SIZE; i+=4){
		memcpy(&emptyPtr, fsBlock+i, 4);
		if(emptyPtr == ptr){
			return i;
		}
	}
	return 0;
}

int FileManager::deleteFile(const char* path){
	std::cout << "Deleting file path: " << path << "\n";
	std::string spath(path+1);
	FileInfo_t fi = fileMap[spath];
	
	// How many blocks are there?
	int blocksOwned = fi.size/BLOCK_SIZE;
	blocksOwned += (fi.size%BLOCK_SIZE > 0); // If we have a partial last block, add 1
	std::cout << "This file owns: " << blocksOwned << " blocks.\n";
	// Remove ownership on blocks
	char fileBlock[BLOCK_SIZE];
	unsigned int blockPtr = 0;
	unsigned int zero = 0;
	sm->read(fileBlock, fi.location, BLOCK_SIZE);

	for(int i=0; i<blocksOwned; ++i){
		memcpy(&blockPtr, fileBlock+BLOCK_PTR_OFF+i*4, 4);
		std::cout << "This file owns block: " << blockPtr << "\n";
		sm->write(&zero, blockPtr, 4);
	}
	// Remove ownership on FSblock
	sm->write(&zero, fi.location, 4);
	// 0 out entry in fileblock
	sm->write(&zero, findUsedFSPointer(fi.location), 4);
	// Remove entry from filemap
	fileMap.erase(spath);
	return 0;
}

int FileManager::createNewFile(const char* path){
	// Find empty block
	unsigned int newBlockLoc = findOpenBlock();
	if(newBlockLoc == 0){
		return -ENOENT;
	}
	std::cout << "Writing a new file block at: " << newBlockLoc << "\n";
	// Add the file to the map
	FileInfo_t fi;
	fi.location = newBlockLoc;
	fi.size = 0;

	// Add file pointer
	// Minus 1 because the first file is 0 bytes from the end of SFS_HEAD_OFF
	unsigned int entryLoc = findOpenFSPointer();
	std::cout << "There is a 0 entry in the filemap at: " << entryLoc << "\n";
	sm->write(&newBlockLoc, entryLoc, 4);
	fileMap[std::string(path+1)] = fi;
	// Inc the file count
	unsigned int hold;
	sm->read(&hold, 4, 4);
	hold++;
	sm->write(&hold, 4, 4);
	hold = 0;
	// Own the block
	sm->write(USED_KEY, newBlockLoc, 4);
	// Zero out the file length
	sm->write(&hold, newBlockLoc+FILE_SIZE_OFF, 4);

	// Write the file name
	// Copy to new buffer because path might be too long or short
	char npath[MAX_NAME_LEN];
	memset(npath, 0, MAX_NAME_LEN);
	unsigned int pathLen = strnlen(path, MAX_NAME_LEN);
	strncpy(npath, path, pathLen);
	npath[MAX_NAME_LEN-1] = '\0';
	sm->write(npath+1, newBlockLoc+FILE_NAME_OFF, MAX_NAME_LEN);
	//std::cout << "Name written at: " << newBlockLoc+FILE_NAME_OFF << "\n";
	return 0;
}

void FileManager::format(){
	unsigned int totalSpace = sm->getStegSize();
	unsigned int blocks = totalSpace/BLOCK_SIZE;
	unsigned int zeros[BLOCK_SIZE];
	memset(zeros, 0, 4096);
	sm->write(zeros, 0, BLOCK_SIZE);
	for(int i=0; i < blocks; ++i){
		sm->write(zeros, BLOCK_SIZE*i, 4);
	}
	std::cout << "Format complete.\n";
}

// This is a two step process:
// Check the file length compared to the previously set value and allocate
// or free blocks accordingly. Then
// Set the new file length

int FileManager::setFileLength(std::string path, size_t newSize){
	int sizeChange = newSize - fileMap[path].size;
	FileInfo_t fi = getFileInfo(path);
	char zeros[BLOCK_SIZE];
	memset(zeros, 0, BLOCK_SIZE);
	unsigned int pointer_to_cur_block = fi.location;

	int blocksOwned = fi.size/BLOCK_SIZE;
	blocksOwned += (fi.size%BLOCK_SIZE > 0); // If we have a partial last block, add 1
	int blocksNeeded = newSize/BLOCK_SIZE;
	blocksNeeded += (newSize%BLOCK_SIZE > 0);

	if(sizeChange > 0){	
		// If we are just expanding the last block but not needing more, just zero out
		// that part of the last block
		if(blocksNeeded == blocksOwned){
			// Might need code here eventually
		}else{
			// We need to zero, own, then add new blocks
			for(int i=blocksOwned; i < blocksNeeded; ++i){
				unsigned int freshBlock = findOpenBlock();
				sm->write(zeros, freshBlock, BLOCK_SIZE);
				sm->write(USED_KEY, freshBlock, 4);
				sm->write(&freshBlock, fi.location+BLOCK_PTR_OFF+(i*4), 4);
			}
		}
	}else if(sizeChange < 0){
		// We just need to go through and free and blocks we no longer need
		for(int i=blocksOwned; i > blocksNeeded; --i){
			unsigned int unneededBlock = 0;
			sm->read(&unneededBlock, fi.location+BLOCK_PTR_OFF+((i-1)*4), 4);
		}
	}else{
		// No size change
	}
	// Write new size, return
	fileMap[path].size = newSize;
	return sm->write(&newSize, fileMap[path].location+FILE_SIZE_OFF, 4);
	return 0;
}

int FileManager::writeToFile(std::string path, const void* rdata, unsigned int placeInFile, unsigned int length){
	const unsigned char * data = (unsigned char*)rdata;
	FileInfo_t fi = fileMap[path];
	if(placeInFile + length > fi.size){
		fi.size = placeInFile + length;
		setFileLength(path, fi.size);
	}
	// Find starting block, find ending block, do all writes
	// Write first block
	// The number of bytes we can write is the blocksize - the header length - how far we go into that block
	unsigned int dataWritten = BLOCK_SIZE-DATA_BLOCK_H_SIZE - placeInFile%((unsigned int)DATA_BLOCK_SIZE);
	if(dataWritten > length){
		dataWritten = length;
	}
	unsigned int startingBlock = (placeInFile)/(DATA_BLOCK_SIZE);
	unsigned int endingBlock = (placeInFile+length)/(DATA_BLOCK_SIZE);
	// Where is the first block
	unsigned int blockLoc = 0;
	sm->read(&blockLoc, fi.location+BLOCK_PTR_OFF+startingBlock*4, 4);

	// Write the first block
	sm->write(rdata, blockLoc+DATA_BLOCK_H_SIZE+(placeInFile%((unsigned int)DATA_BLOCK_SIZE)), dataWritten);

	// All interim blocks are full writes
	for(unsigned int i=startingBlock+1; endingBlock != 0 && i < endingBlock-1; ++i){
		sm->read(&blockLoc, fi.location+BLOCK_PTR_OFF+i*4, 4);
		sm->write(data+dataWritten, blockLoc+DATA_BLOCK_H_SIZE, DATA_BLOCK_SIZE);
		dataWritten += DATA_BLOCK_SIZE;
	}

	//Write last block
	sm->read(&blockLoc, fi.location+BLOCK_PTR_OFF+endingBlock*4, 4);
	sm->write(data+dataWritten, blockLoc+DATA_BLOCK_H_SIZE,length-dataWritten);

	return 0;
}

int FileManager::readFromFile(std::string path, void* rdata, unsigned int placeInFile, unsigned int length){
	unsigned char * data = (unsigned char*)rdata;
	FileInfo_t fi = fileMap[path];
	if(placeInFile + length > fi.size){
		return -1;
	}
	// Find starting block, find ending block, do all reads
	// Read first block
	// The number of bytes we can write is the blocksize - the header length - how far we go into that block
	unsigned int dataRead = BLOCK_SIZE-DATA_BLOCK_H_SIZE - placeInFile%((unsigned int)DATA_BLOCK_SIZE);
	if(dataRead > length){
		dataRead = length;
	}
	unsigned int startingBlock = (placeInFile)/(DATA_BLOCK_SIZE);
	unsigned int endingBlock = (placeInFile+length)/(DATA_BLOCK_SIZE);
	// Where is the first block
	unsigned int blockLoc = 0;
	sm->read(&blockLoc, fi.location+BLOCK_PTR_OFF+startingBlock*4, 4);
	// Read the first block
	sm->read(data, blockLoc+DATA_BLOCK_H_SIZE+(placeInFile%((unsigned int)DATA_BLOCK_SIZE)), dataRead);
	// All interim blocks are full writes
	for(unsigned int i=startingBlock+1; endingBlock != 0 && i < endingBlock-1; ++i){
		std::cout << startingBlock << " " << endingBlock << "\n";
		sm->read(&blockLoc, fi.location+BLOCK_PTR_OFF+i*4, 4);
		sm->read(data+dataRead, blockLoc+DATA_BLOCK_H_SIZE, DATA_BLOCK_SIZE);
		dataRead += DATA_BLOCK_SIZE;
	}

	//Write last block
	sm->read(&blockLoc, fi.location+BLOCK_PTR_OFF+endingBlock*4, 4);
	sm->read(data+dataRead, blockLoc+DATA_BLOCK_H_SIZE,length-dataRead);
	return 0;
}

