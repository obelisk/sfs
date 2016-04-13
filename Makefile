CC = clang++

BASE_FLAGS = -g -std=c++11

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
LDFLAGS =  -Ilibjpeg -I/usr/include/fuse/ -Iinclude -Istegs/include
LINKFLAGS = -lcryptopp -lboost_filesystem -lboost_system -ljpeg -lpthread -liconv -lfuse -D_FILE_OFFSET_BITS=64
else
LDFLAGS = -I/usr/local/include -I/usr/local/include/osxfuse -Iinclude -Istegs/include
LLIBFLAGS = -L/usr/local/lib
LINKFLAGS = -lcryptopp -lboost_filesystem -lboost_system -ljpeg -pthread -liconv -losxfuse -D_DARWIN_USE_64_BIT_INODE -D_FILE_OFFSET_BITS=64
endif

# FINAL FLAGS -- TO BE USED THROUGHOUT
FLAGS = $(BASE_FLAGS) $(LDFLAGS) $(LLIBFLAGS) $(LINKFLAGS) 

JPEGFILES = stegs/Jpeg/Jpeg.cc stegs/Jpeg/Jpegutils.cc

STORAGE = storage/*

UTILS = utils/*

FILESYSTEM = filesystem/*

main: main.cc $(JPEGFILES) $(UTILS) $(STOREAGE) $(FILESYSTEM)
	echo "Compiling System, Please Stand By..."
	$(CC) $(FLAGS) -o sfs main.cc $(JPEGFILES) $(UTILS) $(STORAGE) $(FILESYSTEM)
