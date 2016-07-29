CC = clang++

BASE_FLAGS = -std=c++11

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
LDFLAGS =  -Ilibjpeg -I/usr/include/fuse/ -Iinclude
LINKFLAGS = -lcryptopp -lboost_filesystem -lboost_system -ljpeg -lpthread -lbsd -lfuse -D_FILE_OFFSET_BITS=64
else
LDFLAGS = -I/usr/local/include -I/usr/local/include/osxfuse -Iinclude -Istegs/include
LLIBFLAGS = -L/usr/local/lib
LINKFLAGS = -lcryptopp -lboost_filesystem -lboost_system -ljpeg -pthread -liconv -losxfuse -D_DARWIN_USE_64_BIT_INODE -D_FILE_OFFSET_BITS=64
endif

# FINAL FLAGS -- TO BE USED THROUGHOUT
FLAGS = $(BASE_FLAGS) $(LDFLAGS) $(LLIBFLAGS) $(LINKFLAGS)
LIBFLAGS = $(BASE_FLAGS) $(LDFLAGS) $(LLIBFLAGS) -D_FILE_OFFSET_BITS=64

JPEGFILES = stegs/Jpeg/Jpeg.cc stegs/Jpeg/Jpegutils.cc

STORAGE = storage/*

UTILS = utils/*

FILESYSTEM = filesystem/*

LIBRARY = build/sfs.a

main: main.cc $(JPEGFILES) $(UTILS) $(STOREAGE) $(FILESYSTEM)
	echo "Building System..."
	$(CC) $(FLAGS) -o build/sfs main.cc $(JPEGFILES) $(UTILS) $(STORAGE) $(FILESYSTEM)

library: $(JPEGFILES) $(UTILS) $(STOREAGE) $(FILESYSTEM)
	echo "Building Library..."
	$(CC) $(LIBFLAGS) -c $(JPEGFILES) $(UTILS) $(STORAGE) $(FILESYSTEM)
	ar rvs $(LIBRARY) *.o
	rm *.o

tools: tools/directory_lister.cc tools/file_creator.cc
	echo "Building Tools..."
	$(CC) $(FLAGS) -o build/directory_lister tools/directory_lister.cc $(LIBRARY)
	$(CC) $(FLAGS) -o build/file_creator tools/file_creator.cc $(LIBRARY)

clean: build/*
	echo "Cleaning..."
	rm -rf build/*
