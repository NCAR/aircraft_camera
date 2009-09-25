/*
 *This program is largely based off of the example.c included with
 * libpng. It is used here to compress IEEE1394 camera images
 * to the lossess PNG format
 */

#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
 
#define PNG_DEBUG 3
#include <png.h>

 
int write_PNG_file (const char *filename, int bit_depth, int image_height, int image_width, unsigned char *image_buffer )
{
	int i;
	png_structp png_ptr;
	png_infop info_ptr;
	FILE *fp = fopen(filename, "wb");
	if (!fp) {
		syslog(LOG_ERR,"[write_png_file] error'd: unable to open file");
		return 1;
	}
	
	/* create png structure */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		syslog(LOG_ERR,"[write_png_file] png_create_write_struct failed");
		return 1;
	}
	
	/* create info structure */
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		syslog(LOG_ERR,"[write_png_file] png_create_info_struct failed");
		return 1;
	}
	
	/* set error callback */
	if (setjmp(png_jmpbuf(png_ptr))) {
		syslog(LOG_ERR,"[write_png_file] error'd: writing error");
		return 1;
	}
	
	/* set png compressor settings */
	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr, image_width, image_height,
		bit_depth, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	
	/* Fastest compression possible = 1, set to 0 for no compression */
	png_set_compression_level(png_ptr, 1);
	png_write_info(png_ptr, info_ptr);
	
	/* compress one line at a time */
	for (i=0;i<image_height;i++) {
		png_write_row(png_ptr, image_buffer+(i*3*image_width));
	}
	
	/* finish and clean up */
	png_write_end(png_ptr, NULL);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
	return 0;
}
