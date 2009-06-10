#ifndef PARSEFILE_H
#define PARSEFILE_H

/*struct to hold information from config file*/
typedef struct {
	long long guid;			//64-bit GUID of the camera
	int iso;				//ISO setting
	int nightThreshold;		//Night detection threshold (100 = off)
	uint32_t mode;			//Camera mode - almost always mode7
	uint32_t coding;		//Color Coding - RAW8 or RAW16 for color cameras
	uint32_t bayerMethod;	//Method to 'debayer' the raw image (convert from raw to RGB)
	char direction[10]; 	//Direction the camera is pointing - for reference
	char raw;				//option for RAW binary output (not debayered)
	char jpg;				//option for lossy JPEG compressed output
	char png;				//option for lossless PNG compressed output
	char ppm;				//option for uncompressed PPM output
	int quality; 			//if format is 'jpg', specify the compression quality
	char flNum[10];			//number of the flight on which the image was taken
} camConf_t;

char *strLower(char* s);
int getConf(const char *filename, long long guid, camConf_t *camConf);
void fillConf(camConf_t *camConf, char* param, char* value);
int decoment (char* s);
int setFP(const char *filename, long long guid, camConf_t *camConf);
void applyDefaults(camConf_t *camConf);


#endif
