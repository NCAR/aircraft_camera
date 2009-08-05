#include <dc1394/dc1394.h>
#include <exiv2/image.hpp>
#include <exiv2/pngimage.hpp>
#include <exiv2/xmp.hpp>
#include <iostream>
#include <iomanip>
#include <string>
#include <cmath>
#include <cassert>
#include "getIMG.h"

extern "C" int addXMP(const char *fileName, camConf_t *camConf, dc1394camera_t *camera)
/* Writes out all the relivant data to XMP image metadata */

try {
    std::string file(fileName);
	char guid_s[17];
	sprintf(guid_s, "%llx", camConf->guid);
	
	/* array of mode strings for correct dc1394 mode */
	std::string dc_mode_array[] = {
		"DC1394_VIDEO_MODE_160x120_YUV444",
		"DC1394_VIDEO_MODE_320x240_YUV422",
		"DC1394_VIDEO_MODE_640x480_YUV411",
		"DC1394_VIDEO_MODE_640x480_YUV422",
		"DC1394_VIDEO_MODE_640x480_RGB8",
		"DC1394_VIDEO_MODE_640x480_MONO8",
		"DC1394_VIDEO_MODE_640x480_MONO16",
		"DC1394_VIDEO_MODE_800x600_YUV422",
		"DC1394_VIDEO_MODE_800x600_RGB8",
		"DC1394_VIDEO_MODE_800x600_MONO8",
		"DC1394_VIDEO_MODE_1024x768_YUV422",
		"DC1394_VIDEO_MODE_1024x768_RGB8",
		"DC1394_VIDEO_MODE_1024x768_MONO8",
		"DC1394_VIDEO_MODE_800x600_MONO16",
		"DC1394_VIDEO_MODE_1024x768_MONO16",
		"DC1394_VIDEO_MODE_1280x960_YUV422",
		"DC1394_VIDEO_MODE_1280x960_RGB8",
		"DC1394_VIDEO_MODE_1280x960_MONO8",
		"DC1394_VIDEO_MODE_1600x1200_YUV422",
		"DC1394_VIDEO_MODE_1600x1200_RGB8",
		"DC1394_VIDEO_MODE_1600x1200_MONO8",
		"DC1394_VIDEO_MODE_1280x960_MONO16",
		"DC1394_VIDEO_MODE_1600x1200_MONO16",
		"DC1394_VIDEO_MODE_EXIF",
		"DC1394_VIDEO_MODE_FORMAT7_0",
		"DC1394_VIDEO_MODE_FORMAT7_1",
		"DC1394_VIDEO_MODE_FORMAT7_2",
		"DC1394_VIDEO_MODE_FORMAT7_3",
		"DC1394_VIDEO_MODE_FORMAT7_4",
		"DC1394_VIDEO_MODE_FORMAT7_5",
		"DC1394_VIDEO_MODE_FORMAT7_6",
		"DC1394_VIDEO_MODE_FORMAT7_7" };
	
	/* array of bayer method strings */
	std::string dc_debayer_array[] = {
		"DC1394_BAYER_METHOD_NEAREST",
		"DC1394_BAYER_METHOD_SIMPLE",
		"DC1394_BAYER_METHOD_BILINEAR",
		"DC1394_BAYER_METHOD_HQLINEAR",
		"DC1394_BAYER_METHOD_DOWNSAMPLE",
		"DC1394_BAYER_METHOD_EDGESENSE",
		"DC1394_BAYER_METHOD_VNG",
		"DC1394_BAYER_METHOD_AHD" };

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
    std::cout << "Caught Exiv2 exception '" << e << "'\n";
    return -1;
}

