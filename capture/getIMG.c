/*
 * Grab an image using libdc1394
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dc1394/dc1394.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include "getIMG.h"
#include "writejpeg.h"
#include "writepng.h"

/* this prototype allows calling the addXMP c++ function */
extern int addXMP(const char *, camConf_t*, dc1394camera_t *);

void cleanup_and_exit(dc1394camera_t *camera){
	/* error function: Releases the cameras and exits */

	dc1394_video_set_transmission(camera, DC1394_OFF);
	dc1394_capture_stop(camera);
	dc1394_camera_free(camera);
	exit(1);
}

void cleanup_cams(camConf_t **camArray, int numCams) {
	/* function called before program exit, 
		sends stop signal to cameras and
		releases all allocated DMA buffers 
	*/
	int i;
	for (i=0; i<numCams; i++) {
		dc1394_video_set_transmission(camArray[i]->cam, DC1394_OFF);
		dc1394_capture_stop(camArray[i]->cam);
		dc1394_camera_free(camArray[i]->cam);
		free(camArray[i]);
	}
}

int initCams(dc1394_t *d, camConf_t **camArray, int numCams) {
	int i;
	dc1394error_t err;

	for (i=0; i<numCams; i++){	
		camArray[i]->cam = dc1394_camera_new (d, camArray[i]->guid);
		if (!camArray[i]->cam) {
			dc1394_log_error("Failed to initialize camera with guid %llx",
							 camArray[i]->cam->guid);
			return 0;
		}
	
		/* get gain boundaries */
		err=dc1394_feature_get_boundaries(camArray[i]->cam, DC1394_FEATURE_GAIN, &(camArray[i]->minGain), &(camArray[i]->maxGain));
		DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camArray[i]->cam),"Failed to read gain boundaries\n");
	
		err=dc1394_video_set_framerate(camArray[i]->cam, DC1394_FRAMERATE_15);
		DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camArray[i]->cam),"Could not set framerate\n");

		/* setup capture using settings from config file */
		err=dc1394_video_set_mode(camArray[i]->cam, camArray[i]->mode);
		DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camArray[i]->cam),"Could not set video mode\n");
	
		err=dc1394_feature_whitebalance_set_value(camArray[i]->cam, camArray[i]->whiteBalance_blue, camArray[i]->whiteBalance_red);
		DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camArray[i]->cam),"Could not set white balance\n");
	
		err=dc1394_format7_set_color_coding(camArray[i]->cam, camArray[i]->mode, camArray[i]->coding);
		DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camArray[i]->cam),"Could not set color coding\n");
	
		err=dc1394_format7_set_roi(camArray[i]->cam, camArray[i]->mode,
					 DC1394_QUERY_FROM_CAMERA, DC1394_QUERY_FROM_CAMERA,
					 DC1394_QUERY_FROM_CAMERA, DC1394_QUERY_FROM_CAMERA,
					 DC1394_QUERY_FROM_CAMERA, DC1394_QUERY_FROM_CAMERA);
		DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camArray[i]->cam),"Could not set roi\n");
	 
		err=dc1394_capture_setup(camArray[i]->cam,1, (DC1394_CAPTURE_FLAGS_DEFAULT & !DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC));
		DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camArray[i]->cam),"Could not setup camera-\n \
				make sure that the video mode and framerate are\n \
				supported by your camera\n");

		/* set single shot to avoid extranious data-transfer */
		err=dc1394_video_set_one_shot(camArray[i]->cam, DC1394_ON);
		DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camArray[i]->cam),"Could not set one shot");
	
		/* have the camera start sending us data */
		err=dc1394_video_set_transmission(camArray[i]->cam, DC1394_ON);
		DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camArray[i]->cam),
			"Could not start camera iso transmission\n");
	}
	return 1;
}

int getIMG(const char *image_file_name, camConf_t *camConfig, dc1394_t *d, int *night){
	/*  Gets an image from the specified camera and 
		writes to the specified filename. 
		updates the night parameter passed in by reference.
		returns the total number of byte written to filesystem.
	*/
	
	FILE* imagefile;
	char full_file_name[256];
	unsigned char *reducedFrame;
	int numPix, i, totalSize=0;
	unsigned gain;
	struct stat st;
	dc1394video_frame_t *frame=NULL;
	dc1394error_t err;

	/* set single shot to avoid extranious data-transfer */
	err=dc1394_video_set_one_shot(camConfig->cam, DC1394_ON);
	DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camConfig->cam),"Could not set one shot");

	/* capture one frame */
	err=dc1394_capture_dequeue(camConfig->cam, DC1394_CAPTURE_POLICY_WAIT, &frame);
	DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camConfig->cam),"Could not capture a frame\n");
	
	/* get current gain to use for night-time auto shutdown */
	err=dc1394_feature_get_value(camConfig->cam, DC1394_FEATURE_GAIN, &gain);
	DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camConfig->cam),"Failed to read gain\n");
	if ((100*((float)gain - camConfig->minGain))/(camConfig->maxGain - camConfig->minGain) > (camConfig->nightThreshold)) night += 1;

	/* save raw binary data if requested */
	if(camConfig->raw){
		sprintf(full_file_name, "%s.raw", image_file_name);
		imagefile=fopen(full_file_name, "wb");
		if( imagefile == NULL) {
			perror( "Can't create ' image_file_name '");
			cleanup_and_exit(camConfig->cam);
		}
		fwrite((const char *)frame->image, 1, frame->total_bytes, imagefile);
		fclose(imagefile);
//		printf("wrote: %s\n", full_file_name);
		if (!stat(full_file_name, &st)) totalSize += st.st_size;
	}

	/* debayer the image for all other formats*/
	dc1394video_frame_t *new_frame;
   	new_frame=calloc(1,sizeof(dc1394video_frame_t));
	dc1394_debayer_frames(frame, new_frame, DC1394_BAYER_METHOD_BILINEAR);

	/* save image as uncompressed .ppm image, if requested */
	if(camConfig->ppm){	
		sprintf(full_file_name, "%s.ppm", image_file_name);
		imagefile=fopen(full_file_name, "wb");
		if( imagefile == NULL) {
			perror( "Can't create ' image_file_name '");
			cleanup_and_exit(camConfig->cam);
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
		addXMP(full_file_name, camConfig, camConfig->cam);
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

	/* clean up and return */
	if (frame)
		dc1394_capture_enqueue (camConfig->cam, frame);

	free(new_frame->image);
	free(new_frame);

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
