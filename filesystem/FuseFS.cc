#include "FuseFS.h"

FileManager* FuseHandler::fm;

static int fs_getattr(const char *path, struct stat *stbuf){
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 2;
		return 0;
	}else if(FuseHandler::fm->getFileMap().count(std::string(path+1)) > 0){
		stbuf->st_mode = S_IFREG | 0777;
		stbuf->st_nlink = 1;
		stbuf->st_size = FuseHandler::fm->getFileInfo(std::string(path+1)).size;
		return 0;
	}else{
		std::cout << std::string(path+1) << " does not exist.\n";
		return -ENOENT;
	}
}
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	// Loop through file map here
	for(auto& element : FuseHandler::fm->getFileMap()){
		filler(buf, element.first.c_str(), NULL, 0);
	}
	return 0;
}
static int fs_open(const char *path, struct fuse_file_info *fi){
	// Does the file exist?
	if(FuseHandler::fm->getFileMap().count(std::string(path+1)) > 0){
		return 0;
	}
	std::cout << "Opening a file (" <<  std::string(path+1) << ") that doesn't exist!\n";
	return -ENOENT;
}
static int fs_close(const char *path, struct fuse_file_info *fi){
	if(FuseHandler::fm->getFileMap().count(std::string(path+1)) == 0){
		std::cout << "Closing a file (" <<  std::string(path+1) << ") that doesn't exist!\n";
		return -ENOENT;
	}
	FuseHandler::fm->flushFile(std::string(path+1));
	return 0;
}
static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	FuseHandler::fm->readFromFile(std::string(path+1), buf, offset, size);
	return size;
}
static int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	FuseHandler::fm->writeToFile(std::string(path+1), buf, offset, size);
	return size;
}

static int fs_mknod(const char *path, mode_t mode, dev_t dev){
	std::cout << "Creating a new file...\n";
	FuseHandler::fm->createNewFile(path+1);
	return 0;
}

static int fs_rename(const char *path, const char *npath){
	if(FuseHandler::fm->getFileMap().count(std::string(path+1)) == 0){
		std::cout << "Trying to move a file (" <<  std::string(path+1) << ") that doesn't exist!\n";
		return -ENOENT;
	}
	FuseHandler::fm->renameFile(std::string(path+1), std::string(npath+1));
	return 0;
}

static int fs_unlink(const char *path){
	std::cout << "Deleting a file....\n";
	FuseHandler::fm->deleteFile(path+1);
	return 0;
}

static int fs_link(const char *target, const char* name){
	std::cout << "Filesystem does not currently support links.\n";
	return 0;
}

static int fs_xattr(const char *path, const char *name, const char *value, size_t size, int flags, unsigned int what){
	std::cout << "Filesystem does not currently support eXtended ATTRibutes.\n";
	return 0;
}

static int fs_truncate(const char *path, off_t offset){
	std::cout << "Adjusting " << path << " to " << offset << " length\n";
	if(FuseHandler::fm->getFileMap().count(std::string(path+1)) == 0){
		return -ENOENT;
	}
	FuseHandler::fm->setFileLength(std::string(path+1), offset);
	return 0;
}

static int fs_chmod(const char *path, mode_t mode){
	std::cout << "Filesystem does not currently support file permissions.\n";
	return 0;
}

static int fs_chown(const char *path, uid_t uid, gid_t gid){
	std::cout << "Filesystem does not currenctly support file owners.\n";
	return 0;
}

static int fs_utime(const char *path, struct utimbuf* utim){
	std::cout << "Filesystem does not currently support file timestamps.\n";
	return 0;
}

static struct fuse_operations fuse_oper = {
	.getattr	= fs_getattr,
	.readdir	= fs_readdir,
	.open		= fs_open,
	.release	= fs_close,
	.read		= fs_read,
	.rename		= fs_rename,
	.write		= fs_write,
	.mknod		= fs_mknod,
	.unlink		= fs_unlink,
	.link		= fs_link,
	.setxattr 	= fs_xattr,
	.truncate 	= fs_truncate,
	.chmod 		= fs_chmod,
	.chown 		= fs_chown,
	.utime 		= fs_utime,
};

FuseHandler::FuseHandler(FileManager * rfm){
	fm = rfm;
}

int FuseHandler::start(int argc, char ** argv){
	return fuse_main(argc, argv, &fuse_oper, NULL);
}

FuseHandler::~FuseHandler(){
	delete fm;
}