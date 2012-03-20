#include <sys/time.h>
#include <time.h>


/*
 * Grab an image using libdc1394
 */

#include "getIMG.h"

/* this prototype allows calling the addXMP c++ function */
extern int addXMP(const char *, camConf_t*, dc1394camera_t *);

void cleanup_and_exit(dc1394camera_t *camera){
	/*  Releases the cameras and exits */
	syslog(LOG_WARNING, "Encountered Camera Error\n");
	dc1394_video_set_transmission(camera, DC1394_OFF);
	dc1394_capture_stop(camera);
	dc1394_camera_free(camera);

//	exit(1);  // DC1393_ERR_CLN_RTN exits the program
}

int setup_cams(camConf_t **camArray, int nCams, dc1394_t *d){

	int i;
	camConf_t* camConfig;
	dc1394camera_t *camera;
	dc1394error_t err;	

	for (i=0; i<nCams; i++) {
		camConfig = camArray[i];	
		camera = dc1394_camera_new (d, camConfig->guid);
		if (!camera) {
			dc1394_log_error("Failed to initialize camera with guid %llx",
							 camConfig->guid);
			return 0;
		}

		/* setup capture using settings from config file */
		err=dc1394_video_set_iso_speed(camera, camConfig->iso);
		DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not set iso speed");
	
		err=dc1394_video_set_mode(camera, camConfig->mode);
		DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not set video mode\n");
	
		err=dc1394_feature_whitebalance_set_value(camera, camConfig->whiteBalance_blue, camConfig->whiteBalance_red);
		DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not set white balance\n");
	
		err=dc1394_format7_set_color_coding(camera, camConfig->mode, camConfig->coding);
		DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not set color coding\n");
	
		err=dc1394_format7_set_roi(camera, camConfig->mode,
					 DC1394_QUERY_FROM_CAMERA, DC1394_QUERY_FROM_CAMERA,
					 DC1394_QUERY_FROM_CAMERA, DC1394_QUERY_FROM_CAMERA,
					 DC1394_QUERY_FROM_CAMERA, DC1394_QUERY_FROM_CAMERA);
		DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not set roi\n");
	 
		err=dc1394_video_set_framerate(camera, DC1394_FRAMERATE_7_5);
		DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not set framerate\n");
	
		err=dc1394_feature_get_boundaries(camera, DC1394_FEATURE_GAIN, &camConfig->minGain, &camConfig->maxGain);
		DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Failed to read gain boundaries\n");

		dc1394_camera_free(camera);
	}
	
	return 1;
}

int captureIMG( camConf_t *camConfig, dc1394_t *d, int *night, dc1394video_frame_t *frame) {
	/*  Gets an image from the specified camera and 
		writes to the specified filename */
	unsigned gain;
	dc1394camera_t *camera;
	dc1394video_frame_t *new_frame=NULL;
	dc1394error_t err;	

#ifdef DEBUG
	long ms;
	int sec;
	struct timeval tv;
	struct timezone tz;
	struct tm nt;
	gettimeofday(&tv, &tz);
	localtime_r(&tv.tv_sec, &nt);
	syslog(LOG_WARNING, "%d:%02d:%02d %3d - captureIMG called for guid %11x\n", nt.tm_hour, nt.tm_min, nt.tm_sec, tv.tv_usec/1000, camConfig->guid);
	ms = tv.tv_usec/1000;
	sec = nt.tm_sec;
#endif

	camera = dc1394_camera_new (d, camConfig->guid);
	if (!camera) {
		dc1394_log_error("Failed to initialize camera with guid %llx",
						 camConfig->guid);
		return 1;
	}
//	printf("initialized camera GUID %"PRIx64"\n", camera->guid);


//	err=dc1394_capture_setup(camera,4, (DC1394_CAPTURE_FLAGS_DEFAULT & !DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC));
	err=dc1394_capture_setup(camera,1, (DC1394_CAPTURE_FLAGS_DEFAULT & !DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC));
	DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not setup camera-\n \
			make sure that the video mode and framerate are\n \
			supported by your camera\n");

	/* have the camera start sending us data */
	err=dc1394_video_set_transmission(camera, DC1394_ON);
	DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),
		"Could not start camera iso transmission\n");

	/* capture one frame */
	err=dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_WAIT, &new_frame);
	DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not capture a frame\n");
	
	/* stop data transmission */
	err=dc1394_video_set_transmission(camera,DC1394_OFF);
	DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Could not stop the camera?\n");

	/* get gain data - to tell if it's night */
	err=dc1394_feature_get_value(camera, DC1394_FEATURE_GAIN, &gain);
	DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"Failed to read gain\n");

	/* increment night by 10, if gain threshold is breached */
	if ((100*((float)gain - camConfig->minGain))/
		(camConfig->maxGain - camConfig->minGain) > 
		(camConfig->nightThreshold)) *night += 11;

	/* Make a copy of the new frame to use going forward - i.e. get it out of dc1394 memory space */
	copyFrame(frame, new_frame);

#ifdef DEBUG
	gettimeofday(&tv, &tz);
	localtime_r(&tv.tv_sec, &nt);
	ms = (tv.tv_usec/1000)-ms;
	if (nt.tm_sec != sec) ms=ms+1000;
	syslog(LOG_WARNING, "%d:%02d:%02d %3d - Image capture for guid %11x:%3d\n", nt.tm_hour, nt.tm_min, nt.tm_sec, tv.tv_usec/1000, camConfig->guid, ms);
	ms = tv.tv_usec/1000;
	sec = nt.tm_sec;
#endif

	dc1394_capture_stop(camera);
	dc1394_camera_free(camera);

	return 0;
}

/* Save the image to whatever files have been specified.  Return bytes writen. */
int saveIMG(const char *image_file_name, camConf_t *camConfig, dc1394video_frame_t *frame, dc1394_t *d) {

#ifdef DEBUG
	long ms;
	int sec;
	struct timeval tv;
	struct timezone tz;
	struct tm nt;
	gettimeofday(&tv, &tz);
	localtime_r(&tv.tv_sec, &nt);
	syslog(LOG_WARNING, "%d:%02d:%02d %3d - saveIMG called for guid %11x\n", nt.tm_hour, nt.tm_min, nt.tm_sec, tv.tv_usec/1000, camConfig->guid);
	ms = tv.tv_usec/1000;
	sec = nt.tm_sec;
#endif

	unsigned char *reducedFrame;
	int totalSize=0;
	int numPix, i; 
	char full_file_name[256];
	FILE* imagefile;
	dc1394camera_t *camera;
	struct stat st;

	camera = dc1394_camera_new (d, camConfig->guid);
	if (!camera) {
		dc1394_log_error("Failed to initialize camera with guid %llx",
						 camConfig->guid);
		return 1;
	}

	frame->camera = camera;

	/* save raw binary data if requested */
	if(camConfig->raw){
		sprintf(full_file_name, "%s.raw", image_file_name);
		imagefile=fopen(full_file_name, "wb");
		if( imagefile == NULL) {
			perror( "Can't create ' image_file_name '");
			cleanup_and_exit(camera);
		}
		fwrite((const char *)frame->image, 1, frame->total_bytes, imagefile);
		fclose(imagefile);
//		printf("wrote: %s\n", full_file_name);
		if (!stat(full_file_name, &st)) totalSize += st.st_size;
	}

	/* debayer the image for all other formats*/
	dc1394video_frame_t *new_frame;
   	new_frame=calloc(1,sizeof(dc1394video_frame_t));
	dc1394_debayer_frames(frame, new_frame, camConfig->bayerMethod);

#ifdef DEBUG
	gettimeofday(&tv, &tz);
	localtime_r(&tv.tv_sec,&nt);
	ms = (tv.tv_usec/1000)-ms;
	if (nt.tm_sec != sec) ms=ms+1000;
	syslog(LOG_WARNING, "%d:%02d:%02d %3d - Image debayer for guid %11x:%3d\n", nt.tm_hour, nt.tm_min, nt.tm_sec, tv.tv_usec/1000, camConfig->guid, ms);
	ms = tv.tv_usec/1000;
	sec = nt.tm_sec;
#endif

	/* save image as uncompressed .ppm image, if requested */
	if(camConfig->ppm){	
		sprintf(full_file_name, "%s.ppm", image_file_name);
		imagefile=fopen(full_file_name, "wb");
		if( imagefile == NULL) {
			perror( "Can't create ' image_file_name '");
			cleanup_and_exit(camera);
		}
	
		fprintf(imagefile,"P6\n%u %u\n", new_frame->size[0], new_frame->size[1]);
		fprintf(imagefile,"%ld\n", (long)((1<<new_frame->data_depth) - 1));
		fwrite((const char *)new_frame->image, 1, new_frame->total_bytes, imagefile);
		fclose(imagefile);
//		printf("wrote: %s\n", full_file_name);
		if (!stat(full_file_name, &st)) totalSize += st.st_size;
	}

	/* compress and save image as .jpg image, if requested */	
	if(camConfig->jpg){
		sprintf(full_file_name, "%s.jpg", image_file_name);
	
		/* if the image is 16bpp, then we need to reduce it before we convert to jpeg. */
		if(new_frame->data_depth == 16) {
			numPix = (new_frame->size[0])*(new_frame->size[1]);
			reducedFrame = malloc(sizeof(unsigned char)*(numPix*3));
			for (i=0; i<numPix*3; i++){
				reducedFrame[i] = new_frame->image[i*2];
			}
			write_JPEG_file(full_file_name, camConfig->quality, 
						new_frame->size[1], new_frame->size[0], reducedFrame);	
			free(reducedFrame);
		}
		else {
			write_JPEG_file(full_file_name, camConfig->quality, 
						new_frame->size[1], new_frame->size[0], new_frame->image);
		}
		addXMP(full_file_name, camConfig, camera);
//  		printf("wrote: %s, at %d%% quality\n", full_file_name, camConfig->quality);
		if (!stat(full_file_name, &st)) totalSize += st.st_size;
	}

	/* compress and save the image as a .png image, if requested */
	if(camConfig->png){
		sprintf(full_file_name, "%s.png", image_file_name);
		write_PNG_file(full_file_name, new_frame->data_depth, new_frame->size[1], 
					new_frame->size[0], new_frame->image);

/*the line below is commented out because it does not work with the verison of 
libexiv2 in that is available in the epel repos at this time - it does work with
the latest (svn) verison of libexiv2 */
//		addXMP(full_file_name, camConfig, camera); 
//  		printf("wrote: %s\n", full_file_name);
		if (!stat(full_file_name, &st)) totalSize += st.st_size;
	}

#ifdef DEBUG
	gettimeofday(&tv, &tz);
	localtime_r(&tv.tv_sec,&nt);
	ms = (tv.tv_usec/1000)-ms;
	if (nt.tm_sec != sec) ms=ms+1000;
	syslog(LOG_WARNING, "%d:%02d:%02d %3d - Image written for guid %11x:%3d\n", nt.tm_hour, nt.tm_min, nt.tm_sec, tv.tv_usec/1000, camConfig->guid, ms);
	ms = tv.tv_usec/1000;
	sec = nt.tm_sec;
#endif

	/* clean up and return */
	free(new_frame->image);
	free(new_frame);
	dc1394_capture_stop(camera);
	dc1394_camera_free(camera);

	return totalSize;
}

void printCamConf(camConf_t *camArray) {
/* prints out all of the options read in by the config
   file parser - useful for debugging */

		printf("guid: %llx\n", camArray->guid);
		printf("night threshold: %d\n", camArray->nightThreshold);
		printf("iso: %d\n", camArray->iso);
		printf("mode: %d\n", camArray->mode);
		printf("coding: %d\n", camArray->coding);
		printf("bayerMethod: %d\n", camArray->bayerMethod);
		printf("direction: %s\n", camArray->direction);

		printf("formats: ");
		if (camArray->jpg) printf(" jpg");
		if (camArray->raw) printf(" raw");
		if (camArray->ppm) printf(" ppm");
		if (camArray->png) printf(" png");
		printf("\n");

		printf("quality: %d\n", camArray->quality);
}

void copyFrame(dc1394video_frame_t *to_frame, dc1394video_frame_t *from_frame) {

	/* copy captured frame into returned frame using brute force 
  	   Note: this relies on the dc1394video_frame_t structure being unchanged.
  	   if the  dc1394 libraries are updated (current ver: 2.1.2)  */
	to_frame->allocated_image_bytes = from_frame->allocated_image_bytes;
        to_frame->camera = from_frame->camera;
        to_frame->color_coding = from_frame->color_coding;
        to_frame->color_filter = from_frame->color_filter;
	to_frame->data_depth = from_frame->data_depth;
	to_frame->data_in_padding = from_frame->data_in_padding;
	to_frame->frames_behind = from_frame->frames_behind;
	to_frame->id = from_frame->id;
	to_frame->image = calloc(1,from_frame->total_bytes);
        memcpy(to_frame->image, from_frame->image, from_frame->total_bytes);
	to_frame->image_bytes = from_frame->image_bytes;
	to_frame->little_endian = from_frame->little_endian;
	to_frame->packet_size = from_frame->packet_size;
	to_frame->packets_per_frame = from_frame->packets_per_frame;
	to_frame->padding_bytes = from_frame->padding_bytes;
	to_frame->position[0] = from_frame->position[0];
	to_frame->position[1] = from_frame->position[1];
	to_frame->size[0] = from_frame->size[0];
	to_frame->size[1] = from_frame->size[1];
	to_frame->stride = from_frame->stride;
	to_frame->timestamp = from_frame->timestamp;
	to_frame->total_bytes = from_frame->total_bytes;
	to_frame->video_mode = from_frame->video_mode;
	to_frame->yuv_byte_order = from_frame->yuv_byte_order;

	return;
}
