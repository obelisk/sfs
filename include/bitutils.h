#pragma once
#include <stddef.h>
#include <stdlib.h>

// Any functions for bit/byte manipulations have prototypes here

// Reverse the bytes of the given data
void reverseBytes(void * rdata, size_t len);

// Take an array of bytes, return the bit at the bit_numth position
unsigned char getBit(const void* rdata, unsigned int bit_num);

void SetStdinEcho(bool enable = true);