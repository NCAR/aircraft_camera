/*
 * The purpose of this program is to collect and archive images from 
 * IEEE1394 scientific cameras.
 */

#include <dc1394/dc1394.h>
#include <inttypes.h>
#include <libpq-fe.h>	//for postgres
#include <stdint.h>
#include <stdio.h>
#include <signal.h> 	//for catching kill signals
#include <stdlib.h>	
#include <string.h>
#include <sys/wait.h> 	//for waitpid
#include <syslog.h>
#include <time.h> 		//for timestamp
#include <unistd.h> 	//for sleep, fork

#include "getIMG.h"
#include "pgFuncs.h"
#include "multicast.h"

#define MAX_NIGHT_TRIP 5
#define MAX_NIGHT_VAL MAX_NIGHT_TRIP * 10

#define RESCAN_BUS_INTERVAL 10

/* default settings - if not specified by user */
#define CONFIG_FILE "/etc/capture.conf"
#define FILE_PREFIX "./flight_number_"
#define DB_HOST "acserver"

void wait_for_camera(dc1394_t *d);
int bus_count_changed(dc1394_t *d, int numCams);
int cleanup_d(camConf_t ***, int, status_t *);
int reinitialize_d(char *, dc1394_t *, camConf_t ***, status_t *, int *, PGconn *, char *, char *);
void finishUp();
void getTime(char *, char *);
void parseInputLine(int, char **, char**, char**, char**, char**, int*, int*);
char *defaults(char **arg, char *value);
void printArgsError(char *);

/* global variable for interrupt handling */
int keepGoing = 1;

int main(int argc, char *argv[])
{
	/* declare vars */
	int i, useDB = 1, getFNfromDB = 0, night=0, camCount=0, waitbus=0, rescanCount=0;
	char timeStr[20];
	status_t statMC;				//struct to hold cumulative status data
	camConf_t **camArray = NULL;	//array to hold settings from conf file
	PGconn *conn; 					//postgresql database connection
	pid_t cpid;						//PID for timing process
	dc1394_t * d;					//dc1394 base class (for scanning the bus)
	char *conf, *prefix, *dbHost, *flNum;	//ptrs for command line arguements

	/* set up handler for signal */
	signal(SIGINT, &finishUp);
	signal(SIGTERM, &finishUp);
	signal(SIGHUP, &finishUp);

	/* set up syslog */
	setlogmask(LOG_UPTO(LOG_DEBUG));
	openlog("capture:", LOG_CONS | LOG_PID, LOG_LOCAL1);


	/* get input args */
	parseInputLine(argc, argv, &conf, &prefix, &dbHost, &flNum, &getFNfromDB, &waitbus);

	/* connect to postgres db */
	useDB = connectDB(&conn, dbHost, getFNfromDB);

	/* if not specifed on command line, get flight number from db */
	if(getFNfromDB) getDbFlNum(conn, &flNum);

	/* set up libdc1394 object */		
	if ( !(d=dc1394_new()) ) return 1;

	/* if -w was specified, wait for a camera to show up on the bus */
	if (waitbus) wait_for_camera(d);

	/* allocate/set up camArray */
	camCount = reinitialize_d(flNum, d, &camArray, &statMC, &useDB, conn, conf, prefix);


	/*				 %=== MAIN LOOP HERE ===% 
	 * keep getting pictures until signal or night detection 
	 */
	while(keepGoing && night<MAX_NIGHT_VAL){

		/* spawn child (timer) process to wait one second, then die */
		if( !(cpid = fork()) ) {
			sleep(1);
			exit(0);
		}

		/* main process continues here*/
		getTime(timeStr, statMC.clock);
		for (i=0; i<camCount; i++){
			/* produce file name */
			sprintf(statMC.latest[i], "%s%s/%s/%s", prefix, flNum,
					camArray[i]->direction, timeStr); 

			/* capture & compress image, return total bytes written */
			statMC.lastSize[i] = getIMG(statMC.latest[i], camArray[i], d, &night);

			/* Update database if connected */
			if(useDB) updatePostgres(conn, timeStr, i);
		}
		/* slowly bring night back to 0, to help avoid false triggers */
		if(night>0) night--;

		/* cleanup and reinit if there are new cams on the bus */
		if ( rescanCount++ >= RESCAN_BUS_INTERVAL ) {
			rescanCount = 0;
			if (bus_count_changed(d, camCount)) {
				syslog(LOG_WARNING, "warning: bus count changed, reinitializing\n");
				cleanup_d(&camArray, camCount, &statMC);
				camCount = reinitialize_d(flNum, d, &camArray, 
					&statMC, &useDB, conn, conf, prefix);
			}
		}

		/* send multicast status packet */
		multicast_send_status(&statMC);

		/* wait for timer (child) process to die before looping again*/
		waitpid(cpid, NULL, 0);
	}

	/* log the reason for stopping the program */
	if (night>=MAX_NIGHT_VAL) syslog(LOG_NOTICE, "night detected, shutting down");
	else syslog(LOG_NOTICE, "signal captured, shutting down");

	/* clean up memory and exit */
	cleanup_d(&camArray, camCount, &statMC);
	if(useDB) cleanUpDB(conn, night);	//update DB and close connection
	dc1394_free(d);

	return 0;
}


int bus_count_changed(dc1394_t *d, int numCams) {
	
	int n;
	dc1394error_t err;	
	dc1394camera_list_t * list; 
	
	err=dc1394_camera_enumerate (d, &list);
	DC1394_ERR_RTN(err,"Failed to enumerate cameras");

	n=list->num;
	dc1394_camera_free_list(list);		//release camera list struct
	if (n != numCams) return 1;
	else return 0;

}

void wait_for_camera(dc1394_t *d) {
	int cam_on_bus = 0;
	syslog(LOG_NOTICE, "waiting for a camera to be detected on the bus");
	printf("waiting for a camera to be detected on the bus\n");

	while (!cam_on_bus && keepGoing){
		if (bus_count_changed(d, 0) == 0) {
			sleep(5);
		} else {
			syslog(LOG_NOTICE, "camera detected on bus");
			return;
		}
	}
}

int cleanup_d(camConf_t ***camArray_ptr, int camCount, status_t *statMC) {

	camConf_t **camArray = *camArray_ptr;
	int i;
	for (i=0; i<camCount; i++) free(camArray[i]);

	free(camArray);					//release array of pointers
	multicast_clean_up(statMC);		//send final packet and release structs

}

int reinitialize_d(char *flNum, dc1394_t *d, camConf_t ***camArray_ptr, status_t *statMC, int *useDB, PGconn *conn, char *conf, char *prefix) {

	int numCams=0, i;
	char directory[100];
	camConf_t **camArray;
	//dc1394camera_t * resCam;		//dc1394 camera class, used here for reset
	dc1394camera_list_t * list; 	//dc1394 struct to hold list of cams on bus
	dc1394error_t err;				//dc1394 struct to hold err message,status

	err=dc1394_camera_enumerate (d, &list);
	DC1394_ERR_RTN(err,"Failed to enumerate cameras");

	/* check to make sure there is at least one camera on the bus */
	numCams = list->num;
	if (numCams == 0) {
		printf("No cameras found\n");
		syslog(LOG_ERR, "No cameras found, exiting");
		exit(1);
	}

	/* allocate memory for struct pointers for each cam */
	camArray = malloc(sizeof(camConf_t*) * numCams); 
	*camArray_ptr = camArray;

	/* allocate and fill structs with data from config file */
	for (i=0; i<numCams; i++){
		camArray[i] = malloc(sizeof(camConf_t)); //allocate config structures
		if (!getConf(conf, list->ids[i].guid, camArray[i])){
			syslog(LOG_WARNING,"no settings for camera: %llx in file: %s,using defaults.",
				list->ids[i].guid, conf);
		}
		strcpy(camArray[i]->flNum, flNum);
	}

	/* create image directory structure */
	for (i=0; i<numCams; i++){
		sprintf(directory, "mkdir -p -m 777 %s%s/%s &> /dev/null", prefix, 
				flNum, camArray[i]->direction); 
		system(directory);
	}

	/* set up camera table in postgres db, if connected */
	if(*useDB != 0) initRow(conn, camArray, numCams);

	/* allocate and setup multicast status struct */
	multicast_status_init(statMC, camArray, numCams);

	/* apply config params to cameras, ensure valid connection */
	statMC->health = setup_cams(camArray, numCams, d);

	dc1394_camera_free_list(list);		//release camera list struct
	return numCams;
}

void finishUp(){
/* This function will be set as the signal handler
   It simply breaks the main loop so that we can exit cleanly */
	keepGoing = 0;
}

void getTime(char *s1, char *s2){
/* This function updates the strings passed in to the current date and time,
    formatted as follows:
    s1 (for image name): YYMMDD-HHMMSS 
    s2 (for status packet): YYYY-MM-DD HH:MM:SS
*/
	struct tm *t;
	time_t rawtime;

	time(&rawtime);
//	t = localtime(&rawtime);
	t = gmtime(&rawtime);

	/* make time string */
	strftime(s2, 20, "%Y-%m-%d %H:%M:%S", t);
	strftime(s1, 20, "%y%m%d-%H%M%S", t);

	return;
}

void parseInputLine(int argc, char **argv, char **confFile, char **filePrefix, char **dbHost, char **flNum, int *getFNfromDB, int *wait){

	int i=0;
	char opt; 
	if (argc <= 1) {
		printArgsError(argv[0]);
	}

	/* set to NULL, so we can apply defaults if needed later */
	*flNum = *confFile = *filePrefix = *dbHost = NULL;
	*getFNfromDB = *wait = 0;

	while (i<argc) {
		if (*argv[i] == '-'){
			opt = *(argv[i]+1);
			switch (opt) {
				case 'c':
					*confFile = argv[++i];
					break;
				case 'f':
					*filePrefix = argv[++i];
					break;
				case 'd':
					*dbHost = argv[++i];
					*getFNfromDB = 1;
					break;
				case 'w':
					*wait = 1;
					break;
				default:
					printArgsError(argv[0]);
			}
		} else if (i>0) *flNum = argv[i];
		i++;
	}
	
	/* if the params were not set, use the default vals */
	if (*flNum == NULL && !(*getFNfromDB)) printArgsError(argv[0]);
	if (*confFile == NULL) defaults(confFile, CONFIG_FILE);
	if (*filePrefix == NULL) defaults(filePrefix, FILE_PREFIX);
	if (*dbHost == NULL) defaults(dbHost, DB_HOST);

}

char *defaults(char **arg, char *value){
	/* allocates memory for the value, and copies the string into the new location*/
//	printf("size of: '%s' is %d\n", value, sizeof(char)*strlen(value));
	*arg = malloc(sizeof(char)*strlen(value)+1);
	strcpy(*arg, value);
	return *arg;
}

void printArgsError(char *cmd){
	/* this function is called when there is impropper input on the
	   command line. It displays some help for the user and _exits */
	printf("improper usage - use format:\n");
	printf("%s [-c <configFile>] [-f <file prefix>] [-d <db host>] <flightnumber>\n", cmd);
	printf("\tNOTE: you must specify a flight number or use -d to get from database\n\n");
	exit(1);

}

