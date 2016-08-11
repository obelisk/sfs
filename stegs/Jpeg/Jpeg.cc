#include "Jpeg.h"

namespace fs = boost::filesystem;

Jpeg::Jpeg(std::string filepath){
	path = filepath;
	name = fs::path(filepath).filename().string();
	size = fs::file_size(filepath);
	cached_writes.clear();
	stegSizeCalc();
}

Jpeg::~Jpeg(){
	assert(cached_writes.size() == 0);
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

	// Count how many DCT CoEffs are above DCT_MIN_V.
	for (int ci = 0; ci < 3; ci++){
		compptr_one = cinfo.comp_info + ci;
		for (int by = 0; by < compptr_one->height_in_blocks; by++){
			buffer_one = (cinfo.mem->access_virt_barray)((j_common_ptr)&cinfo, coeffs_array[ci], by, (JDIMENSION)1, FALSE);
			for (int bx = 0; bx < compptr_one->width_in_blocks; bx++){
				blockptr_one = buffer_one[0][bx];
				for (int bi = 0; bi < 64; bi++){
					if(blockptr_one[bi] > DCT_MIN_V){
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
					if(blockptr_one[bi] > DCT_MIN_V){
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

int Jpeg::write(const char* data, int location, int length){
	// This modifies the list so it needs to be protected.
	list_mutex.lock();
	// Cache this call
	char* cdata = (char*)malloc(sizeof(char) * length);
	memcpy(cdata, data, length);
	cached_write_t cw = {cdata, length, location};
	cached_writes.push_back(cw);
	// We've finished with out modification, unlock.
	list_mutex.unlock();
	return 0;
}

int Jpeg::flush(){
	// Flushing needs to be protected to prevent double writes, frees, etc.
	list_mutex.lock();
	std::cout << "Flushing " << cached_writes.size() << " writes into '" << name << "'.\n";
	int ret = 0;
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
	int location, length;
	char * data;

	for (std::list<cached_write_t>::iterator i = cached_writes.begin(); i != cached_writes.end(); ++i){
		location = i->location;
		length = i->length;
		data = i->data;
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
						if(blockptr_one[bi] > DCT_MIN_V){
							if(bitloc > 0){
								bitloc--;
							}else if(bitloc == 0){
								// Start writing data.
								if(bits_written == 0){
									dataStart[0] = ci; dataStart[1] = by; dataStart[2] = bx; dataStart[3] = bi;
								}
								blockptr_one[bi] = blockptr_one[bi] & 0xFFFFFFFE; 	// Wipe LSB
								blockptr_one[bi] = blockptr_one[bi] | getBit(data, bits_written);	// Set LSB
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
		if(bitloc != 0 || done != 1){
			std::cout << "Data could not be written into this steg\n";
			ret =  STEG_INSUFFICIENT_SIZE;
			break;
		}
		free(i->data);
	}
	//write out new file
	write_jpeg_file_dct(path, cinfo, coeffs_array);
	jpeg_destroy_decompress(&cinfo);
	fclose(infile);
	cached_writes.clear();
	// All writes are done now, other threads can use this if they want.
	list_mutex.unlock();
	return ret;
}
