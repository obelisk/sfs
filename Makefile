CC = clang++

BASE_FLAGS = -g -std=c++11

# INCLUDE BASE DIRECTORY AND BOOST DIRECTORY FOR HEADERS
LDFLAGS = -I/usr/local/Cellar/cryptopp/5.6.2/include/cryptopp -I/usr/local/Cellar/boost/1.58.0/include -I/usr/local/include/osxfuse/fuse -I/usr/local/opt/jpeg-turbo/include -Iinclude -Istegs/include

# INCLUDE BASE DIRECTORY AND BOOST DIRECTORY FOR LIB FILES
LLIBFLAGS = -L/usr/local/Cellar/boost/1.58.0/lib -L/usr/local/lib

# SPECIFIY LINK OPTIONS
LINKFLAGS = -lcryptopp -lboost_filesystem -lboost_system -ljpeg -pthread -liconv -losxfuse -D_DARWIN_USE_64_BIT_INODE -D_FILE_OFFSET_BITS=64

# FINAL FLAGS -- TO BE USED THROUGHOUT
FLAGS = $(BASE_FLAGS) $(LDFLAGS) $(LLIBFLAGS) $(LINKFLAGS) 

JPEGFILES = stegs/Jpeg/Jpeg.cc stegs/Jpeg/Jpegutils.cc

STORAGE = storage/*

UTILS = utils/*

FILESYSTEM = filesystem/*

main: main.cc $(JPEGFILES) $(UTILS) $(STOREAGE) $(FILESYSTEM)
	echo "Compiling System, Please Stand By..."
	$(CC) $(FLAGS) -o sfs main.cc $(JPEGFILES) $(UTILS) $(STORAGE) $(FILESYSTEM)
