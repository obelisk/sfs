#include "bitutils.h"

void reverseBytes(void * rdata, size_t len){
	int iters = len/2;
	char* data = (char*)rdata;
	unsigned char hold = '\0';
	for(int i=0; i < iters; i++){
		hold = data[len-iters-1];
		data[len-iters-1] = data[iters];
		data[iters] = hold;
	}
}

unsigned char getBit(const void* rdata, unsigned int bit_num){
	char* data = (char*)rdata;
	unsigned int byte_num = bit_num/8;
	unsigned int local_bit = bit_num%8;

	unsigned char mask = 0x1 << local_bit;
	return ((data[byte_num] & mask)>>local_bit);
}

// Incomplete
char* dataBytesToBits(void* rdata, size_t len){
	char * expanded = (char*)malloc(len*8);

	// Array loop
	for(int i=0; i < len; i++){
		// Byte loop
		for(int j=0; j < 8; j++){

		}
	}

	return expanded;
}