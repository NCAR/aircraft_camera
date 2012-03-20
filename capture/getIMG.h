#ifndef GETIMG_H
#define GETIMG_H

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dc1394/dc1394.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "writejpeg.h"
#include "writepng.h"
#include "parseFile.h"

void cleanup_and_exit(dc1394camera_t *);
int setup_cams(camConf_t **camArray, int nCams, dc1394_t *d);
int captureIMG( camConf_t *camConfig, dc1394_t *d, int *night, dc1394video_frame_t *frame); 
int saveIMG(const char *image_file_name, camConf_t *camConfig, dc1394video_frame_t *frame, dc1394_t *d); 
void printCamConf(camConf_t *camArray );
void copyFrame(dc1394video_frame_t *frame, dc1394video_frame_t *new_frame);

#endif
