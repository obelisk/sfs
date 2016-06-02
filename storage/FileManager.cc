#include "FileManager.h"

namespace fs = boost::filesystem;

FileManager::FileManager(StorageManager * rsm, std::string key, FileManagerConf_t conf){
	fskey = calculateKey(key);
	sm = rsm;
	// Check the StorageManager has non zero space available
	// The smallest amount of conceivable space needed is three blocks:
	// MFT, File indirection, One data block
	if(rsm->getStegSize()/BLOCK_SIZE < 3){
		std::cout << "There is not enough space to create a filesystem.\n";
		ready = false;	
	}else{
		openFileSystem();
	}
}

std::string FileManager::calculateKey(std::string key){
	// Key may not have suitable entropy, take a SHA256 hash first
	std::string hkey;
	hkey.resize(CryptoPP::SHA256::DIGESTSIZE);
	CryptoPP::SHA256().CalculateDigest((byte*)&hkey[0], (byte*)key.c_str(), key.length());
	return hkey.substr(0,16);
}

void FileManager::openFileSystem(){
	// Find out if there is a filesystem of this type already.
	unsigned int filePtr = 0;
	std::string emft = readFileSystemBlock(0);
	const char* fsBlock = emft.c_str();
	// Our MFT is still encrypted so we need to decrypt it now
	// In our new format, the first 16 bytes are the initialization
	// vector for the (BLOCKSIZE - 16) bytes that make up the rest of
	// the MFT.
	memmft = decryptMFT(emft);

	// Decryption Failed
	// Wrong Password, Corrupted, etc
	if(memmft.length() == 0 || memmft.substr(0, 4) != "STEG"){
		// We should give the option to create a new filesystem here
		std::cout << "Could not find a filesystem. Create a new one?\n";
		char ans;
		std::cin >> ans;
		if(ans != 'y' && ans != 'Y'){
			std::cout << "Will not create filesystem. Exiting...\n";
			exit(1);
		}
		std::string emft = createEncMFT();
		sm->write(emft.c_str(), 0, BLOCK_SIZE);
		sm->flush();
		return openFileSystem();	// We've create an encrypted MFT so hopefully now we can restart the method and try again
	}

	unsigned char fileBlock[BLOCK_SIZE];
	int success = 0;
	unsigned int nameLen = 0, dataLen = 0, files = 0, block_number = 0, file_block_ptr = 0;

	std::cout << "Processing vDisk.\n";
	unsigned int num_blocks = sm->getStegSize()/BLOCK_SIZE;
	// Add the MFT block as used.
	used_blocks.push_back(0);

	for(int i = 4; i < memmft.length(); i += 4){
		memcpy(&filePtr, memmft.c_str()+i, 4);
		if(filePtr == 0){continue;}	// No File
		block_number = filePtr/BLOCK_SIZE;

		// Add the block number to the used blocks array
		used_blocks.push_back(block_number);

		// Go through this pointer block and add all those blocks as used
		std::string ptrBlock = decryptMFT(readFileSystemBlock(block_number));

		// Start past the header of the indirection pointer
		for(int j = BLOCK_PTR_OFF; j < ptrBlock.length(); j += 4){
			memcpy(&file_block_ptr, ptrBlock.c_str()+j, 4);
			if(file_block_ptr == 0){
				break;
			}else{
				block_number = file_block_ptr/BLOCK_SIZE;
				used_blocks.push_back(block_number);
			}
		}

		FileInfo_t fi;
		memcpy(&fi.size, ptrBlock.c_str()+FILE_SIZE_OFF, 4);
		fi.location = filePtr;
		fi.loaded = 0;
		fi.bptr = ptrBlock;
		fi.writes = 0;
		char name[MAX_NAME_LEN+1];
		memcpy(name, ptrBlock.c_str()+FILE_NAME_OFF, MAX_NAME_LEN);
		fileMap[std::string(name, strnlen(name, MAX_NAME_LEN))] = fi;
		files++;
	}

	std::cout << "Filesystem contains: " << files << " files.\n";
	for(auto const &file : fileMap) {
		std::cout << "\t" << file.second.size << "\t" << file.first << "\n";
	}

	used_blocks.sort();
	std::list<unsigned int>::iterator forward = used_blocks.begin();
	++forward;
	std::list<unsigned int>::iterator behind = used_blocks.begin();

	// Find all the free blocks in between other blocks
	for (; forward != used_blocks.end(); ++forward, ++behind){
		for(unsigned int i = 1; i < *forward-*behind; i++){
			free_blocks.push_back(*behind+i);
		}
	}

	// Find the free blocks at the end of the disk
	for(unsigned int i = used_blocks.back()+1; i < num_blocks; i++){
		free_blocks.push_back(i);
	}

	std::cout << used_blocks.size() << " blocks used, " << free_blocks.size() << " blocks available, " << num_blocks << " blocks total.\n";
	ready = true;
}

FileManager::~FileManager(){
	delete sm; // Delete the StorageManager
}

bool FileManager::isReady(){
	return ready;
}

std::map<std::string, FileInfo_t> FileManager::getFileMap(){
	return fileMap;
}

FileInfo_t FileManager::getFileInfo(std::string path){
	return fileMap[path];
}

std::string FileManager::readFileSystemBlock(unsigned int blockNumber){
	unsigned int location = blockNumber*BLOCK_SIZE;
	unsigned char fsBlock[BLOCK_SIZE];

	sm->read((char*)fsBlock, location, BLOCK_SIZE);
	return std::string((char*)fsBlock, BLOCK_SIZE);
}

std::string FileManager::decryptMFT(std::string mft){
	CBC_Mode< AES >::Decryption d;
	// This should set the IV
	d.SetKeyWithIV((byte *)fskey.c_str(), fskey.length(), (byte *)mft.c_str());

	// Remove the IV from the string to decrypt
	mft.erase(0, AES::BLOCKSIZE);
	std::string dmft;
	try{
		CryptoPP::StringSource ss( mft, true, new CryptoPP::StreamTransformationFilter( d, new CryptoPP::StringSink( dmft ), CryptoPP::AuthenticatedEncryptionFilter::NO_PADDING ));
	}catch(const CryptoPP::Exception& e){
		std::cout << "There may not be an encrypted filesystem here.\n";
		std::cerr << e.what() << "\n";
		return std::string();
	}
	return dmft;
}

std::string FileManager::createEncMFT(){
	CBC_Mode<AES>::Encryption e;
	std::string dmft, emft, encoded;
	unsigned char iv[AES::BLOCKSIZE];

	dmft.resize(BLOCK_SIZE - AES::BLOCKSIZE);
	dmft[0] = 'S'; dmft[1] = 'T'; dmft[2] = 'E'; dmft[3] = 'G';
	arc4random_stir();
	arc4random_buf(iv, AES::BLOCKSIZE);
	e.SetKeyWithIV((byte *)fskey.c_str(), fskey.length(), (byte*)iv);

	try{
		// Encryption
		CryptoPP::StringSource ss( dmft, true, new CryptoPP::StreamTransformationFilter(e, new CryptoPP::StringSink( emft ), CryptoPP::AuthenticatedEncryptionFilter::NO_PADDING ));
	}catch(const CryptoPP::Exception& e){
		std::cerr << e.what() << std::endl;
	}
	emft.insert(0, (char*)iv, 16);
	return emft;
}

unsigned int FileManager::flushMFT(){
	CBC_Mode<AES>::Encryption e;
	std::string eblock, iv, emft;
	iv.resize(AES::BLOCKSIZE);
	arc4random_stir();
	arc4random_buf((byte*)&iv[0], AES::BLOCKSIZE);
	e.SetKeyWithIV((byte *)fskey.c_str(), fskey.length(), (byte*)iv.c_str());

	try{
		// Encryption
		CryptoPP::StringSource ss( memmft, true, new CryptoPP::StreamTransformationFilter(e, new CryptoPP::StringSink( emft ), CryptoPP::AuthenticatedEncryptionFilter::NO_PADDING ));
	}catch(const CryptoPP::Exception& e){
		std::cerr << e.what() << std::endl;
	}
	emft.insert(0, iv);
	sm->write(emft.c_str(), 0, BLOCK_SIZE);
	sm->flush();
	return 0;
}

unsigned int FileManager::flushFileIndirection(FileInfo_t fi){
	std::string iv, eptr;
	iv.resize(AES::BLOCKSIZE);
	arc4random_stir();
	arc4random_buf((byte*)&iv[0], AES::BLOCKSIZE);
	eptr = encryptPtrBlock(fi.bptr, iv);
	eptr.insert(0, iv);
	sm->write(eptr.c_str(), fi.location, BLOCK_SIZE);
	return sm->flush();
}

unsigned int FileManager::flushFile(std::string path){
	// std::string iv = fileMap[path].bptr.substr(FILE_IV_OFF, FILE_IV_OFF+AES::BLOCKSIZE);
	std::string iv, eptr;
	iv.resize(AES::BLOCKSIZE);
	arc4random_stir();
	arc4random_buf((byte*)&iv[0], AES::BLOCKSIZE);
	memcpy((&fileMap[path].bptr[0])+FILE_IV_OFF, iv.c_str(), ENC_IV_LEN);

	if(fileMap[path].size == 0){
		std::cout << "No content flush for file of size 0, " << path << "\n";
		return 0;
	}
	if(fileMap[path].writes == 0){
		std::cout << "No changes to file data, will not flush.\n";
		return 0;
	}

	std::string paddedContent = fileMap[path].data;
	unsigned int requiredPad = (BLOCK_SIZE - (fileMap[path].size%BLOCK_SIZE))%BLOCK_SIZE;
	paddedContent.resize(fileMap[path].size+requiredPad, 0x0);

	CBC_Mode<AES>::Encryption e;
	std::string eblock;
	eblock.clear();
	e.SetKeyWithIV((byte *)fskey.c_str(), fskey.length(), (byte*)iv.c_str());

	try{
		// Encryption
		CryptoPP::StringSource ss(paddedContent, true, new CryptoPP::StreamTransformationFilter(e, new CryptoPP::StringSink( eblock ), CryptoPP::AuthenticatedEncryptionFilter::NO_PADDING));
	}catch(const CryptoPP::Exception& e){
		std::cerr << e.what() << std::endl;
	}
	unsigned int numBlocks = paddedContent.size()/BLOCK_SIZE, file_block_ptr = 0;
	for(int i = 0; i < numBlocks; ++i){
		// Find where the data block is
		memcpy(&file_block_ptr, fileMap[path].bptr.c_str()+BLOCK_PTR_OFF+(i*4), 4);
		assert(file_block_ptr != 0);
		// Write out that much of our encrypted file to that block
		sm->write(eblock.c_str()+i*BLOCK_SIZE, file_block_ptr, BLOCK_SIZE);
	}
	// sm->flush();
	std::cout << "File flushed to disk\n";
	fileMap[path].writes = 0;
	return flushFileIndirection(fileMap[path]);
}

std::string FileManager::encryptPtrBlock(std::string dblock, std::string iv){
	CBC_Mode<AES>::Encryption e;
	std::string eblock;
	e.SetKeyWithIV((byte *)fskey.c_str(), fskey.length(), (byte*)iv.c_str());

	try{
		// Encryption
		CryptoPP::StringSource ss( dblock, true, new CryptoPP::StreamTransformationFilter(e, new CryptoPP::StringSink( eblock ), CryptoPP::AuthenticatedEncryptionFilter::NO_PADDING ));
	}catch(const CryptoPP::Exception& e){
		std::cerr << e.what() << std::endl;
	}
	return eblock;
}

unsigned int FileManager::claimOpenBlock(){
	unsigned int block = free_blocks.front();
	free_blocks.pop_front();
	return block;
}

unsigned int FileManager::findOpenFSPointer(){
	unsigned int emptyPtr = 0;
	for(int i = SFS_HEAD_OFF; i < BLOCK_SIZE; i+=4){
		memcpy(&emptyPtr, memmft.c_str()+i, 4);
		if(emptyPtr == 0){
			return i;
		}
	}
	return 0;
}

unsigned int FileManager::findUsedFSPointer(unsigned int ptr){
	unsigned int emptyPtr = 0;
	for(int i = SFS_HEAD_OFF; i < BLOCK_SIZE; i+=4){
		memcpy(&emptyPtr, memmft.c_str()+i, 4);
		if(emptyPtr == ptr){
			return i;
		}
	}
	return 0;
}

// Currently, freed blocks are not returned to the system until restart.
int FileManager::deleteFile(const char* path){
	unsigned int pathLen = strnlen(path, MAX_NAME_LEN+1);
	if(pathLen > MAX_NAME_LEN){
		return -ENOENT;
	}
	std::string strpath(path, strnlen(path, MAX_NAME_LEN));
	if(fileMap.count(strpath) == 0){
		std::cout << "File doesn't exist\n";
		return -ENOENT;
	}
	FileInfo_t fi = fileMap[strpath];
	// Find the pointer to the indirection block
	unsigned int indirectionBlockLoc = findUsedFSPointer(fi.location);
	unsigned int zero = 0; 
	memcpy(&memmft[0]+indirectionBlockLoc, &zero, 4);
	fileMap.erase(strpath);
	return flushMFT();
}

int FileManager::createNewFile(const char* path){
	unsigned int pathLen = strnlen(path, MAX_NAME_LEN+1);
	if(pathLen > MAX_NAME_LEN){
		return -ENOENT;
	}
	std::string strpath(path, strnlen(path, MAX_NAME_LEN));
	if(fileMap.count(strpath) > 0){
		std::cout << "File already exists\n";
		return -ENOENT;
	}
	// Find empty block
	unsigned int newBlockLoc = claimOpenBlock()*BLOCK_SIZE;
	if(newBlockLoc == 0){
		return -ENOENT;
	}

	// Add the file to the map
	FileInfo_t fi;
	fi.location = newBlockLoc;
	fi.size = 0;
	fi.data = std::string();
	fi.bptr = std::string();
	fi.bptr.resize(BLOCK_SIZE - AES::BLOCKSIZE, 0);

	// Add file pointer to mft
	unsigned int entryLoc = findOpenFSPointer();
	memcpy(&memmft[entryLoc], &newBlockLoc, 4);	// This should always work because new c++ standards force contiguous string memory

	// Create an IV for the file (should be 16 bytes)
	//AutoSeededRandomPool prng;
	//prng.GenerateBlock((byte*)&fi.bptr[4], AES::BLOCKSIZE);
	arc4random_stir();
	arc4random_buf((byte*)&fi.bptr[4], AES::BLOCKSIZE);

	// Create an IV for the pointer block (should be 16 bytes)
	std::string iv;
	iv.resize(AES::BLOCKSIZE);
	//prng.GenerateBlock((byte*)&iv[0], AES::BLOCKSIZE);
	arc4random_stir();
	arc4random_buf((byte*)&iv[0], AES::BLOCKSIZE);

	// Copy the name into the buffer
	strncpy(&fi.bptr[20], path, pathLen);
	fileMap[strpath] = fi;
	std::string ePtr = encryptPtrBlock(fi.bptr, iv);
	ePtr.insert(0, iv);
	sm->write(ePtr.c_str(), newBlockLoc, BLOCK_SIZE);
	sm->flush();
	flushMFT();
	return 0;
}

// This is a two step process:
// Check the file length compared to the previously set value and allocate
// or free blocks accordingly. Then
// Set the new file length
int FileManager::setFileLength(std::string path, size_t newSize){
	int sizeChange = newSize - fileMap[path].size;
	FileInfo_t fi = getFileInfo(path);

	int blocksOwned = fi.size/BLOCK_SIZE;
	blocksOwned += (fi.size%BLOCK_SIZE > 0); // If we have a partial last block, add 1
	int blocksNeeded = newSize/BLOCK_SIZE;
	blocksNeeded += (newSize%BLOCK_SIZE > 0);

	if(blocksNeeded > free_blocks.size()){
		std::cout << "A file tried to be resized beyond the capacity of the disk.\n";
		return -ENOENT;
	}

	if(sizeChange > 0){	
		// If we are just expanding the last block but not needing more, just zero out
		// that part of the last block
		if(blocksNeeded == blocksOwned){
			// Might need code here eventually
		}else{
			// Add the new blocks sequentially into our ptr table.
			for(unsigned int i=blocksOwned; i < blocksNeeded; ++i){
				unsigned int freshBlock = claimOpenBlock()*BLOCK_SIZE;
				//std::cout << "Writing a block pointer(" << freshBlock << ") at: " << BLOCK_PTR_OFF+(i*4) << "\n";
				memcpy(&fi.bptr[0]+BLOCK_PTR_OFF+(i*4), &freshBlock, 4);
			}
		}
	}else if(sizeChange < 0){
		// We just need to go through and free and blocks we no longer need
		unsigned int unneededBlock = 0;
		for(int i=blocksOwned; i > blocksNeeded; --i){
			memcpy(&fi.bptr[BLOCK_PTR_OFF+((i-1)*4)], &unneededBlock, 4);
		}
	}else{
		// No size change
	}
	fileMap[path].data.resize(newSize);
	// Write new size, update global bptr, flush to disk
	fileMap[path].size = newSize;
	memcpy(&fi.bptr[0], &newSize, 4);
	fileMap[path].bptr = fi.bptr;
	std::cout << "Finishing setFileLength\n";
	return flushFileIndirection(fi);
}

int FileManager::renameFile(std::string path, std::string npath){
	fileMap[npath] = fileMap[path];
	fileMap.erase(path);
	char zeros[MAX_NAME_LEN];
	memset(zeros, MAX_NAME_LEN, 0);
	memcpy(&fileMap[npath].bptr[0]+FILE_NAME_OFF, zeros, MAX_NAME_LEN); // Zero out the name
	memcpy(&fileMap[npath].bptr[0]+FILE_NAME_OFF, npath.c_str(), MAX_NAME_LEN);
	flushFileIndirection(fileMap[npath]);
	return 0;
}

int FileManager::loadFile(std::string path){
	FileInfo_t fi = getFileInfo(path);
	unsigned int numBlocks = fi.size/BLOCK_SIZE;
	numBlocks += (fi.size%BLOCK_SIZE > 0);

	std::string efile;
	efile.resize(numBlocks*BLOCK_SIZE);
	unsigned int file_block_ptr = 0;

	for(int i = 0; i < numBlocks; ++i){
		memcpy(&file_block_ptr, &fi.bptr[0]+BLOCK_PTR_OFF+(i*4), 4);
		sm->read(&efile[0]+i*BLOCK_SIZE, file_block_ptr, BLOCK_SIZE);
	}

	CBC_Mode< AES >::Decryption d;
	d.SetKeyWithIV((byte *)fskey.c_str(), fskey.length(), (byte *)fi.bptr.c_str()+4);

	// Remove the IV from the string to decrypt
	try{
		CryptoPP::StringSource ss( efile, true, new CryptoPP::StreamTransformationFilter( d, new CryptoPP::StringSink( fileMap[path].data ), CryptoPP::AuthenticatedEncryptionFilter::NO_PADDING ));
	}catch(const CryptoPP::Exception& e){
		std::cout << "This file could not be loaded (decryption failed)\n";
		std::cerr << e.what() << "\n";
		return -1;
	}
	fileMap[path].data.resize(fi.size);
	fileMap[path].loaded++;
	std::cout << "File cached in memory\n";
	return 0;
}

int FileManager::writeToFile(std::string path, const void* rdata, unsigned int placeInFile, unsigned int length){
	FileInfo_t fi = getFileInfo(path);
	// We need to have the file loaded into memory first
	if(fi.loaded == 0){
		loadFile(path);
	}

	if(placeInFile+length > fi.size){
		setFileLength(path, placeInFile+length);
	}
	memcpy(&fileMap[path].data[0]+placeInFile, rdata, length);
	fileMap[path].writes++;
	return 0;
}

int FileManager::readFromFile(std::string path, void* rdata, unsigned int placeInFile, unsigned int length){
	FileInfo_t fi = getFileInfo(path);
	// We need to have the file loaded into memory first
	if(fi.loaded == 0){
		loadFile(path);
	}
	memcpy(rdata, &fileMap[path].data[0]+placeInFile, length);
	return 0;
}
