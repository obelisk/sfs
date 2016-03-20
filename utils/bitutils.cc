#include "bitutils.h"
#include <termios.h>
#include <unistd.h> 

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

void SetStdinEcho(bool enable){
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    if( !enable )
        tty.c_lflag &= ~ECHO;
    else
        tty.c_lflag |= ECHO;

    (void) tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}