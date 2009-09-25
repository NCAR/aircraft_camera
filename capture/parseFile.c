/*
 * Parses a camera config file, format specified in sample config file
 */

#include "parseFile.h"

int getConf(const char *filename, long long guidIN, camConf_t *camConf){ 
/*fills in the camConf with all available data from the conf file (specified
  by "filename". The rest of the settings will get default values */

	int foundID = 0;
	FILE *file;
	char param[100], value[100], line[200];
	file = fopen(filename, "r");
	if (!file) {
		syslog(LOG_ERR, "unable to open config file");
		perror("unable to open config file");
		exit(1);
	}

	long long guidOUT;

	/* Start with default values, config file values will overwrite them */
	applyDefaults(camConf);
	camConf->guid = guidIN;

	while(!feof(file)){
		/* get one line (if line is longer than 200 chars, this might break)
		   we want to find all of the lines between the specified GUID and 
		   the next GUID in the file. Then we want to parse those lines */
		fgets(line, 200, file);
		
		/* if line is not a comment, parse it */
		if (decoment(line) && strcmp(line, "")){
			sscanf(line, "%s %s", param, value);
		
			/* find 'camID' */
			if (!strcmp(param, "camID")){

				/* this is the next camID, we are done! */
				if(foundID){
					foundID = 0;
					fclose(file);
					return 1; 
				}

				/* check to see if this is the camID we want */
				else{
					sscanf(value, "%llx", &guidOUT);
					if (guidOUT == guidIN){
						foundID = 1;
					}
				}
			} 

			/* parse this data */
			else if (foundID) {
				fillConf(camConf, param, value);
			}
			
			
		}
	}	

	fclose(file);
	return foundID;
}

int decoment (char* s){
/*removes coments from a string (anything starting with #
  if the whole line is a comment, it returns 0*/
	int i;

	if (s[0] == '#') return 0; //the whole string is a comment
	
	for (i=0; s[i]; i++){
		if (s[i] == '#' || s[i] == '\n') {
			s[i] = '\0';
			return 1;
		}
	}
	return 1;

}

void fillConf(camConf_t *camConf, char* param, char* value){
/*interprets the param and places the value into the correct 
	part of the camConf struct*/
	strLower(param);
	strLower(value);

//	printf("%s = %s\n", param, value);

	if (!strcmp(param, "iso")){
		if(!strcmp(strLower(value), "100"))
			camConf->iso = DC1394_ISO_SPEED_100;
		else if(!strcmp(strLower(value), "200"))
			camConf->iso = DC1394_ISO_SPEED_200; 
		else if(!strcmp(strLower(value), "400"))
			camConf->iso = DC1394_ISO_SPEED_400;
		else if(!strcmp(strLower(value), "800"))
			camConf->iso = DC1394_ISO_SPEED_800;
		else if(!strcmp(strLower(value), "1600"))
			camConf->iso = DC1394_ISO_SPEED_1600;
		else if(!strcmp(strLower(value), "3200"))
			camConf->iso = DC1394_ISO_SPEED_3200;
	}else if (!strcmp(param, "mode")){
		if(!strcmp(value, "format7_mode0"))
			camConf->mode = DC1394_VIDEO_MODE_FORMAT7_0;
		/* add more options? */
	}else if (!strcmp(param, "coding")){
		if(!strcmp(strLower(value), "raw8"))
			camConf->coding = DC1394_COLOR_CODING_RAW8;
		else if(!strcmp(strLower(value), "raw16"))
			camConf->coding = DC1394_COLOR_CODING_RAW16;
		else if(!strcmp(strLower(value), "mono8"))
			camConf->coding = DC1394_COLOR_CODING_MONO8;
		else if(!strcmp(strLower(value), "mono16"))
			camConf->coding = DC1394_COLOR_CODING_MONO16;
	}else if (!strcmp(param, "direction")){
		strcpy(camConf->direction, value);
	}else if (!strcmp(param, "bayermethod")){
		if(!strcmp(strLower(value), "nearest"))
			camConf->bayerMethod = DC1394_BAYER_METHOD_NEAREST;
		else if(!strcmp(strLower(value), "simple"))
			camConf->bayerMethod = DC1394_BAYER_METHOD_SIMPLE;
		else if(!strcmp(strLower(value), "bilinear"))
			camConf->bayerMethod = DC1394_BAYER_METHOD_BILINEAR;
		else if(!strcmp(strLower(value), "hqlinear"))
			camConf->bayerMethod = DC1394_BAYER_METHOD_HQLINEAR;
		else if(!strcmp(strLower(value), "downsample"))
			camConf->bayerMethod = DC1394_BAYER_METHOD_DOWNSAMPLE;
		else if(!strcmp(strLower(value), "edgesense"))
			camConf->bayerMethod = DC1394_BAYER_METHOD_EDGESENSE;
		else if(!strcmp(strLower(value), "vng"))
			camConf->bayerMethod = DC1394_BAYER_METHOD_VNG;
		else if(!strcmp(strLower(value), "ahd"))
			camConf->bayerMethod = DC1394_BAYER_METHOD_AHD;
	}else if (!strcmp(param, "png")){
		if (value[0] == 'y') camConf->png = 1;
		else camConf->png = 0;
	}else if (!strcmp(param, "jpg")){
		if (value[0] == 'y') camConf->jpg = 1;
		else camConf->jpg = 0;
	}else if (!strcmp(param, "ppm")){
		if (value[0] == 'y') camConf->ppm = 1;
		else camConf->ppm = 0;
	}else if (!strcmp(param, "raw")){
		if (value[0] == 'y') camConf->raw = 1;
		else camConf->raw = 0;
	}else if (!strcmp(param, "whitebalance_blue")){
		camConf->whiteBalance_blue = atoi(value);
	}else if (!strcmp(param, "whitebalance_red")){
		camConf->whiteBalance_red = atoi(value);
	}else if (!strcmp(param, "quality")){
		camConf->quality = atoi(value);
	}else if (!strcmp(param, "nightthreshold")){
		camConf->nightThreshold = atoi(value);
	}else {
		if(param[0] != '\0' && param[0] != '\n'){
			syslog(LOG_WARNING, 
			"Warning: Invalid Parameter: \"%s\" in config file\n", param);
		}
	}
	return;
}

void applyDefaults(camConf_t *camConf){
/* sets all of the settings in camConf to these default values:*/

	camConf->guid = 0;
	strcpy(camConf->direction,"none");
	camConf->iso = DC1394_ISO_SPEED_400;
	camConf->whiteBalance_red = 75;
	camConf->whiteBalance_blue = 75;
	camConf->nightThreshold = 85;
	camConf->mode = DC1394_VIDEO_MODE_FORMAT7_0; 
	camConf->coding = DC1394_COLOR_CODING_RAW8;
	camConf->bayerMethod = DC1394_BAYER_METHOD_BILINEAR;
	camConf->raw = 0;
	camConf->jpg = 1;
	camConf->ppm = 0;
	camConf->png = 0;
	camConf->quality = 80;
	strcpy(camConf->flNum,"none");
}

char *strLower(char *s){
/*converts the string passed in to all lower case*/

	int i;
	for (i=0; s[i]; i++)
		s[i] = tolower(s[i]);

	return s;
}


