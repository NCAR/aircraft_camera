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

/* default settings - if not specified by user */
#define CONFIG_FILE "/etc/capture.conf"
#define FILE_PREFIX "./flight_number_"
#define DB_HOST "acserver"

void finishUp();
void getTime(char *, char *);
void parseInputLine(int, char **, char**, char**, char**, char**, int*);
char *defaults(char **arg, char *value);
void printArgsError(char *);

/* global variable for interrupt handling */
int keepGoing = 1;

int main(int argc, char *argv[])
{
	/* declare vars */
	int i, useDB = 1, getFNfromDB = 0, night=0;
	char directory[100], timeStr[20];

	status_t statMC;				//struct to hold cumulative status data
	camConf_t **camArray;			//array to hold settings from conf file
	PGconn *conn; 					//postgresql database connection
	pid_t cpid;						//PID for timing process
	dc1394_t * d;					//dc1394 base class (for scanning the bus)
	dc1394camera_t * resCam;		//dc1394 camera class, used here for reset
	dc1394camera_list_t * list; 	//dc1394 struct to hold list of cams on bus
	dc1394error_t err;				//dc1394 struct to hold err message,status
	
	char *conf, *prefix, *dbHost, *flNum;	//ptrs for command line arguements

	/* set up handler for SIGINT (ctrl-c) signal */
	signal(SIGINT, &finishUp);

	/* set up syslog */
	setlogmask(LOG_UPTO(LOG_DEBUG));
	openlog("capture:", LOG_CONS | LOG_PID, LOG_LOCAL1);

	/* get input args */
	parseInputLine(argc, argv, &conf, &prefix, &dbHost, &flNum, &getFNfromDB);

	/* set up libdc1394 object */		
	d = dc1394_new ();
	if (!d) return 1;
	err=dc1394_camera_enumerate (d, &list);
	DC1394_ERR_RTN(err,"Failed to enumerate cameras");

	/* check to make sure there is at least one camera on the bus */
	if (list->num == 0) {
		dc1394_log_error("No cameras found");
		return 1;
	}

	/* relase old bandwith allocations (if prev. proc. did not end nicely) */
	resCam = dc1394_camera_new (d, list->ids[0].guid);
	if (!resCam) {
		dc1394_log_error("Failed to initialize first camera [with guid %llx]",
						 list->ids[0].guid);
		return 1;
	}
	dc1394_iso_release_all(resCam);
	dc1394_camera_free(resCam); 

	/* allocate memory for struct pointers for each cam */
	camArray = malloc(sizeof(camConf_t*)*list->num); 

	/* connect to postgres db */
	useDB = connectDB(&conn, dbHost, getFNfromDB);

	/* if not specifed on command line, get flight number from db */
	if(getFNfromDB) getDbFlNum(conn, &flNum);

	/* allocate and fill structs with data from config file */
	for (i=0; i<list->num; i++){
		camArray[i] = malloc(sizeof(camConf_t)); //allocate config structures
		if (!getConf(conf, list->ids[i].guid, camArray[i])){
			syslog(LOG_WARNING,"no settings for camera: %llx in file: %s,using defaults.",
					list->ids[i].guid, conf);
		}
		strcpy(camArray[i]->flNum, flNum);
	}

	/* set up camera table in postgres db, if connected */
	if(useDB) initPostgres(conn, camArray, list->num);

	/* allocate and setup multicast status struct */
	multicast_status_init(&statMC, camArray, list->num);

	/* create image directory structure */
	sprintf(directory, "mkdir -p -m 777 %s%s &> /dev/null",prefix, flNum); 
	system(directory);
	for (i=0; i<list->num; i++){
		sprintf(directory, "mkdir -p -m 777 %s%s/%s &> /dev/null", prefix, 
				flNum, camArray[i]->direction); 
		system(directory);
	}

	/* assume progam is healthy at start */
	statMC.health = setup_cams(camArray, list->num, d);

	/*				 %=== MAIN LOOP HERE ===% 
	 * keep getting pictures until SIGINT signal or night detection 
	 */
	while(keepGoing && night<MAX_NIGHT_VAL){

		/* spawn child (timer) process to wait one second, then die */
		if( !(cpid = fork()) ) {
			sleep(1);
			exit(0);
		}

		/* main process continues here*/
		getTime(timeStr, statMC.clock);

		for (i=0; i<list->num; i++){
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

		/* send multicast status packet */
		multicast_send_status(&statMC);

		/* wait for timer (child) process to die before looping again*/
		waitpid(cpid, NULL, 0);
	}

	/* log the reason for stopping the program */
	if (night>=MAX_NIGHT_VAL) syslog(LOG_NOTICE, "night detected, shutting down");
	else syslog(LOG_NOTICE, "sigint signal captured, shutting down");

	/* clean up memory and exit */
	for (i=0; i<list->num; i++) free(camArray[i]);

	free(camArray);						//release array of pointers
	dc1394_camera_free_list (list);		//release camera list struct
	if(useDB) cleanUpDB(conn, night);	//update DB and close connection
	multicast_clean_up(&statMC);		//send final packet and release structs

	return 0;
}

void finishUp(){
/* This function will be set as the SIGINT signal handler
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

void parseInputLine(int argc, char **argv, char **confFile, char **filePrefix, char **dbHost, char **flNum, int *getFNfromDB){

	int i=0;
	char opt; 
	if (argc <= 1) {
		printArgsError(argv[0]);
	}

	/* set to NULL, so we can apply defaults if needed later */
	*flNum = *confFile = *filePrefix = *dbHost = NULL;
	*getFNfromDB = 0;

	while (i<argc) {
		if (*argv[i] == '-'){
			opt = *(argv[i]+1);
			i++;
			switch (opt) {
				case 'c':
					*confFile = argv[i];
					break;
				case 'f':
					*filePrefix = argv[i];
					break;
				case 'd':
					*dbHost = argv[i];
					*getFNfromDB = 1;
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

