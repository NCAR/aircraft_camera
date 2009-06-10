#ifndef GETIMG_H
#define GETIMG_H

#include "parseFile.h"

void cleanup_and_exit(dc1394camera_t *);
int getIMG(const char *, camConf_t *, dc1394_t *);
void printCamConf(camConf_t *camArray );

#endif
