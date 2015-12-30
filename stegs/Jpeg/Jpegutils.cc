#include "Jpeg.h"

int write_jpeg_file_dct(std::string outname,jpeg_decompress_struct in_cinfo, jvirt_barray_ptr *coeffs_array ){

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE * outfile;

    if ((outfile = fopen(outname.c_str(), "wb")) == NULL) {
      fprintf(stderr, "can't open %s\n", outname.c_str());
      return 0;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

    j_compress_ptr cinfo_ptr = &cinfo;
    jpeg_copy_critical_parameters((j_decompress_ptr)&in_cinfo,cinfo_ptr);
    jpeg_write_coefficients(cinfo_ptr, coeffs_array);

    jpeg_finish_compress( &cinfo );
    jpeg_destroy_compress( &cinfo );
    fclose( outfile );
    return 1;
}

jvirt_barray_ptr* getCoeffsArray(struct jpeg_decompress_struct& cinfo, struct my_error_mgr& jerr, std::string path){
	jvirt_barray_ptr* coeffs_array;
	FILE * infile;
	if ((infile = fopen(path.c_str(), "rb")) == NULL) {
    	fprintf(stderr, "can't open %s\n", path.c_str());
    	return 0;
  	}

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;

	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo); // Error Happened
		fclose(infile);
		return NULL;
	}
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	jpeg_read_header(&cinfo, TRUE);
	coeffs_array = jpeg_read_coefficients(&cinfo);
	fclose(infile);
	return coeffs_array;
}