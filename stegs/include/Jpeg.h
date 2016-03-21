#pragma once

#include "StegType.h"
#include "bitutils.h"

#include <vector>
#include <iostream>
#include <boost/filesystem.hpp>
#include <jpeglib.h>
#include <jerror.h>
#include <setjmp.h>

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

int write_jpeg_file_dct(std::string outname,jpeg_decompress_struct in_cinfo, jvirt_barray_ptr *coeffs_array );

class Jpeg: public StegFile {
  private:
  	int stegSize, size;
  	size_t stegSizeCalc();
  public:
    Jpeg(std::string filepath);
    ~Jpeg();
	  size_t getSize();
	  size_t getStegSize();
	  std::string getName();
	  std::string getStegType();
    int read(char* data, int location, int length);
	  int write(const char* data, int location, int length);
	  int flush();
};