#ifndef GETIMG_H
#define GETIMG_H

#include "parseFile.h"

void cleanup_and_exit(dc1394camera_t *);
void cleanup_cams(camConf_t **camArray, int numCams);
int initCams(dc1394_t *d, camConf_t **camArray, int numCams);
int getIMG(const char *, camConf_t *, dc1394_t *, int *);
void printCamConf(camConf_t *camArray );

#endif
