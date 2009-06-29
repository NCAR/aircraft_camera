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

	/* Easy method - fill in XMP structure */
    Exiv2::XmpData xmpData;
    xmpData["Xmp.dc.make"] = camera->vendor; 
	xmpData["Xmp.dc.model"] = camera->model;   
	xmpData["Xmp.dc.guid"]  = guid_s;   
	xmpData["Xmp.dc.direction"]  = camConf->direction; 
	xmpData["Xmp.dc.dc1394_mode"]  = camConf->mode;   
	xmpData["Xmp.dc.flight_number"]  = camConf->flNum;
	xmpData["Xmp.dc.iso"]  = camConf->iso;   
	xmpData["Xmp.dc.bayer_method"]  = camConf->bayerMethod; 
	xmpData["Xmp.dc.quality"]  = camConf->quality;    

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

