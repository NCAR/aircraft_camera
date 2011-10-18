#ifndef PARSEFILE_H
#define PARSEFILE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <dc1394/dc1394.h>
#include <inttypes.h>
#include <ctype.h> //for tolower

#define MAX_FLNUM_LEN 32

/* struct to hold information from config file
 */
typedef struct {
	long long guid;			//64-bit GUID of the camera
	int iso;			//ISO setting
	int whiteBalance_red;		//White balance - red component
	int whiteBalance_blue;		//White balance - blue component
	int nightThreshold;		//Night detection threshold (100 = off)
	uint32_t mode;			//Camera mode - almost always mode7
	uint32_t coding;		//Color Coding - RAW8 or RAW16 for color cameras
	uint32_t bayerMethod;		//Method to 'debayer' the raw image (convert from raw to RGB)
	char direction[50]; 		//Direction the camera is pointing - for reference
	char raw;			//option for RAW binary output (not debayered)
	char jpg;			//option for lossy JPEG compressed output
	char png;			//option for lossless PNG compressed output
	char ppm;			//option for uncompressed PPM output
	int quality; 			//if format is 'jpg', specify the compression quality
	char flNum[MAX_FLNUM_LEN];	//number of the flight on which the image was taken
	int minGain, maxGain;		//min/max gain values used to calculate percentage
} camConf_t;

char *strLower(char* s);
int getConf(const char *filename, long long guid, camConf_t *camConf);
void fillConf(camConf_t *camConf, char* param, char* value);
int decoment (char* s);
void applyDefaults(camConf_t *camConf);


#endif
