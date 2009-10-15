#include <dc1394/dc1394.h>
#include <exiv2/image.hpp>
#include <exiv2/pngimage.hpp>
#include <exiv2/xmp.hpp>
#include <iostream>
#include <iomanip>
#include <string>
#include <cmath>
#include <cassert>
#include <syslog.h>
#include "getIMG.h"

extern "C" int addXMP(const char *fileName, camConf_t *camConf, dc1394camera_t *camera)
/* Writes out all the relivant data to XMP image metadata */

try {
	std::string file(fileName);
	char guid_s[17];
	sprintf(guid_s, "%llx", camConf->guid);
	
	/* array of mode strings for correct dc1394 mode */
	std::string dc_mode_array[] = {
		"160x120_YUV444",
		"320x240_YUV422",
		"640x480_YUV411",
		"640x480_YUV422",
		"640x480_RGB8",
		"640x480_MONO8",
		"640x480_MONO16",
		"800x600_YUV422",
		"800x600_RGB8",
		"800x600_MONO8",
		"1024x768_YUV422",
		"1024x768_RGB8",
		"1024x768_MONO8",
		"800x600_MONO16",
		"1024x768_MONO16",
		"1280x960_YUV422",
		"1280x960_RGB8",
		"1280x960_MONO8",
		"1600x1200_YUV422",
		"1600x1200_RGB8",
		"1600x1200_MONO8",
		"1280x960_MONO16",
		"1600x1200_MONO16",
		"EXIF",
		"FORMAT7_0",
		"FORMAT7_1",
		"FORMAT7_2",
		"FORMAT7_3",
		"FORMAT7_4",
		"FORMAT7_5",
		"FORMAT7_6",
		"FORMAT7_7" };
	
	/* array of bayer method strings */
	std::string dc_debayer_array[] = {
		"NEAREST",
		"SIMPLE",
		"BILINEAR",
		"HQLINEAR",
		"DOWNSAMPLE",
		"EDGESENSE",
		"VNG",
		"AHD" };

	/* Fill in XMP structure */
	Exiv2::XmpData xmpData;
	xmpData["Xmp.dc.make"] = camera->vendor; 
	xmpData["Xmp.dc.model"] = camera->model;   
	xmpData["Xmp.dc.guid"]  = guid_s;   
	xmpData["Xmp.dc.direction"]  = camConf->direction; 
	xmpData["Xmp.dc.dc1394_mode"]  = dc_mode_array[camConf->mode - 64];   
	xmpData["Xmp.dc.flight_number"]  = camConf->flNum;
	xmpData["Xmp.dc.iso"]  = 100*(1<<(camConf->iso));   
	xmpData["Xmp.dc.bayer_method"]  = dc_debayer_array[camConf->bayerMethod]; 
	xmpData["Xmp.dc.quality"]  = camConf->quality;	
	xmpData["Xmp.dc.whiteBal_blue"]  = camConf->whiteBalance_blue;	
	xmpData["Xmp.dc.whiteBal_red"]  = camConf->whiteBalance_red;	
	xmpData["Xmp.dc.UTC_timestamp"]  = file.substr(file.length()-17, 13);

	/* open the image file for writing */
	Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(file);
	assert(image.get() != 0);

	/* Write all the data to the image file */
	image->setXmpData(xmpData);
	image->writeMetadata();

	return 0;
}
catch (Exiv2::AnyError& e) {
//	std::cout << "Caught Exiv2 exception '" << e << "'\n";
	syslog(LOG_ERR, "Caught Exiv2 exception '%s'", e.what());
	return -1;
}

