#include "Jpeg.h"

namespace fs = boost::filesystem;

Jpeg::Jpeg(std::string filepath){
	path = filepath;
	name = fs::path(filepath).filename().string();
	size = fs::file_size(filepath);
	stegSizeCalc();
}

std::string Jpeg::getName(){
	return name;
}

std::string Jpeg::getStegType(){
	return "JPG";
}

size_t Jpeg::getSize(){
	return size;
}

size_t Jpeg::stegSizeCalc(){
	stegSize = 0;
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
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
		return 0;
	}
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	jpeg_read_header(&cinfo, TRUE);
	coeffs_array = jpeg_read_coefficients(&cinfo);
	JBLOCKARRAY buffer_one;
	JCOEFPTR blockptr_one;
	jpeg_component_info* compptr_one;

	// Count how many DCT CoEffs are above 1. We cannot reliably store data in CoEffs that are 1
	for (int ci = 0; ci < 3; ci++){
		compptr_one = cinfo.comp_info + ci;
		for (int by = 0; by < compptr_one->height_in_blocks; by++){
			buffer_one = (cinfo.mem->access_virt_barray)((j_common_ptr)&cinfo, coeffs_array[ci], by, (JDIMENSION)1, FALSE);
			for (int bx = 0; bx < compptr_one->width_in_blocks; bx++){
				blockptr_one = buffer_one[0][bx];
				for (int bi = 0; bi < 64; bi++){
					if(blockptr_one[bi] > 1){
						stegSize++;
					}
				}                  
			}   
		} 
	}
	jpeg_destroy_decompress(&cinfo);
	fclose(infile);

	// We can only store 1 bit in each CoEff, so take the number and divide by 8
	// to get the number of available bytes
	stegSize /=8;
	return stegSize;
}

size_t Jpeg::getStegSize(){
	return stegSize;
}

int Jpeg::write(const char* data, int location, int length){
	//Get the coeffs array
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
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
		return 0;
	}
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	jpeg_read_header(&cinfo, TRUE);
	coeffs_array = jpeg_read_coefficients(&cinfo);

	JBLOCKARRAY buffer_one;
	JCOEFPTR blockptr_one;
	jpeg_component_info* compptr_one;

	//Find location
	int bitloc = location*8; // We only store 1 bit per coeff, so we need to go 8 times the coeffs
	int done = 0;
	// Color, height, width, coeffNumber
	int dataStart[4] = {0,0,0,0};	// Location is defined by 4 numbers, color DCT, then block index for height, width and finally coeff number
	unsigned int bits_written = 0, total_data_length = length*8;
	for (int ci = 0; ci < 3 && !done; ci++){
		compptr_one = cinfo.comp_info + ci;
		for (int by = 0; by < compptr_one->height_in_blocks && !done; by++){
			buffer_one = (cinfo.mem->access_virt_barray)((j_common_ptr)&cinfo, coeffs_array[ci], by, (JDIMENSION)1, FALSE);
			for (int bx = 0; bx < compptr_one->width_in_blocks && !done; bx++){
				blockptr_one = buffer_one[0][bx];
				for (int bi = 0; bi < 64 && !done; bi++){
					if(blockptr_one[bi] > 1){
						if(bitloc > 0){
							bitloc--;
						}else if(bitloc == 0){
							// Start writing data.
							if(bits_written == 0){
								dataStart[0] = ci; dataStart[1] = by; dataStart[2] = bx; dataStart[3] = bi;
							}
							//std::cout << blockptr_one[bi]<< "\t";
							blockptr_one[bi] = blockptr_one[bi] & 0xFFFFFFFE; 	// Wipe LSB
							//std::cout << blockptr_one[bi]<< "\t";
							blockptr_one[bi] = blockptr_one[bi] | getBit(data, bits_written);	// Set LSB
							//std::cout << blockptr_one[bi]<< "\n";
							bits_written++;
							if(bits_written == total_data_length){
								done = 1;
							}
						}
					}
				}
			}
		}
	}
	if(bitloc == 0 && done == 1){
		std::cout << "Data written at: " << dataStart[0] << " " << dataStart[1] << " " << dataStart[2] << " " << dataStart[3] << "\n";
	}else{
		std::cout << "Data could not be written into this steg\n";
		return STEG_INSUFFICIENT_SIZE;
		fclose(infile);
	}
	//write out new file
	write_jpeg_file_dct(path, cinfo, coeffs_array);
	jpeg_destroy_decompress(&cinfo);
	fclose(infile);
	return 0;
}

int Jpeg::read(char* data, int location, int length){
	//Get the coeffs array
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
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
		return 0;
	}
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	jpeg_read_header(&cinfo, TRUE);
	coeffs_array = jpeg_read_coefficients(&cinfo);

	JBLOCKARRAY buffer_one;
	JCOEFPTR blockptr_one;
	jpeg_component_info* compptr_one;

	//Find location
	int bitloc = location*8; // We only store 1 bit per coeff, so we need to go 8 times the coeffs
	int done = 0;
	// Color, height, width, coeffNumber
	int dataStart[4] = {0,0,0,0};	// Location is defined by 4 numbers, color DCT, then block index for height, width and finally coeff number
	unsigned int bits_read = 0, total_data_length = length*8;
	unsigned char bit = 0;
	for (int ci = 0; ci < 3 && !done; ci++){
		compptr_one = cinfo.comp_info + ci;
		for (int by = 0; by < compptr_one->height_in_blocks && !done; by++){
			buffer_one = (cinfo.mem->access_virt_barray)((j_common_ptr)&cinfo, coeffs_array[ci], by, (JDIMENSION)1, FALSE);
			for (int bx = 0; bx < compptr_one->width_in_blocks && !done; bx++){
				blockptr_one = buffer_one[0][bx];
				for (int bi = 0; bi < 64 && !done; bi++){
					if(blockptr_one[bi] > 1){
						if(bitloc > 0){
							bitloc--;
						}else if(bitloc == 0){
							// Start reading data.
							if(bits_read == 0){
								dataStart[0] = ci; dataStart[1] = by; dataStart[2] = bx; dataStart[3] = bi;
							}
							bit = blockptr_one[bi] & 0x1; 	// Read LSB
							data[bits_read/8] = data[bits_read/8] | (bit<<(bits_read%8));
							bits_read++;
							if(bits_read == total_data_length){
								done = 1;
							}
						}
					}
				}
			}
		}
	}
	fclose(infile);
	jpeg_destroy_decompress(&cinfo);
	return 0;
}
