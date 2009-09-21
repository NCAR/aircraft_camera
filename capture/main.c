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

/* default settings - if not specified by user */
#define CONFIG_FILE "/etc/capture.conf"
#define FILE_PREFIX "./flight_number_"
#define DB_HOST "acserver"

void finishUp();
void getTime(char *, char *);
void parseInputLine(int, char **, char**, char**, char**, char**, int*);
char *defaults(char **arg, char *value);
void printArgsError();
char *trimWhiteSpace(char*);

/* global var for interrupt handling */
int keepGoing = 1;

int main(int argc, char *argv[])
{
	/* declare vars */
	int i, useDB = 1, getFNfromDB = 0, night=0;
	char image_file_name[100], directory[100], timeStr[20];

	status_t statMC;				//struct to hold cumulative status data
	camConf_t **camArray;			//array to hold settings from conf file
	PGconn *conn; 					//postgresql database connection
	pid_t cpid;						//PID for timing process
	dc1394_t * d;					//dc1394 base class (for scanning the bus)
	dc1394camera_t * resCam;		//dc1394 camera class, used here for reset
	dc1394camera_list_t * list; 	//dc1394 struct to hold list of cams on bus
	dc1394error_t err;				//dc1394 struct to hold err message,status
	
	char *conf, *prefix, *dbHost, *flNum;

	/* set up handler for SIGINT (ctrl-c) signal */
	signal(SIGINT, &finishUp);

	/* set up syslog */
	setlogmask(LOG_UPTO(LOG_DEBUG));
	openlog("capture:", LOG_CONS | LOG_PID, LOG_LOCAL1);

	/* get input args */
	parseInputLine(argc, argv, &conf, &prefix, &dbHost, &flNum, &getFNfromDB);
	//printf("conf: %s, prefix: %s, host: %s\n", conf, prefix, dbHost);
	syslog(LOG_NOTICE, "Started with conf: %s, prefix: %s, host: %s", conf, prefix, dbHost);

	/* set up libdc1394 opject */		
	d = dc1394_new ();
	if (!d)
		return 1;
	err=dc1394_camera_enumerate (d, &list);
	DC1394_ERR_RTN(err,"Failed to enumerate cameras");

	/* check to make sure there is at least one camera on the bus */
	if (list->num == 0) {
		dc1394_log_error("No cameras found");
		return 1;
	}

	/* relase any old bandwith allocations */
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

	if(getFNfromDB) getDbFlNum(conn, &flNum);
	syslog(LOG_NOTICE, "Flight number recieved: %s", flNum);

	/* fill structs with data from config file */
	for (i=0; i<list->num; i++){
		camArray[i] = malloc(sizeof(camConf_t)); //allocate config structures
		if (!getConf(conf, list->ids[i].guid, camArray[i])){
			syslog(LOG_WARNING,"no settings for camera: %llx in file: %s,using defaults.",
					list->ids[i].guid, conf);
		}
		strcpy(camArray[i]->flNum, flNum);
	}

	/* set up postgres db on sucessfull connection */
	if(useDB) initPostgres(conn, camArray, list->num);

	/* setup multicast status packet */
	multicast_status_init(&statMC, camArray, list->num);

	/* make image directory structure */
	sprintf(directory, "mkdir -m 777 %s%s &> /dev/null",prefix, flNum); 
	system(directory);
	for (i=0; i<list->num; i++){
		sprintf(directory, "mkdir -m 777 %s%s/%s &> /dev/null", prefix, 
				flNum, camArray[i]->direction); 
		system(directory);
	}

	/*				 %=== MAIN LOOP HERE ===% 
	 * keep getting pictures until SIGINT signal or night detection 
	 */
	statMC.health = 1;
	while(keepGoing && night<10){

		/* spawn child process that waits one second, then die */
		if( !(cpid = fork()) ) {
			sleep(1);
			exit(0);
		}

		/* main process continues here, where it will
		   get image(s) and then wait for child process to die */
		getTime(timeStr, statMC.clock);
		for (i=0; i<list->num; i++){
			sprintf(statMC.latest[i], "%s%s/%s/%s", prefix, flNum, 
					camArray[i]->direction, timeStr); 
			statMC.lastSize[i] = getIMG(statMC.latest[i], camArray[i], d, &night);
			if(useDB) updatePostgres(conn, timeStr, i);
		}

		/* send multicast status packet */
		multicast_send_status(&statMC);

		/* wait for timer (child) process to die */
		waitpid(cpid, NULL, 0);
	}

	/* Will get here after SIGINT signal, or night detection */
	if (night>10) syslog(LOG_NOTICE, "night detected, shutting down");
	else syslog(LOG_NOTICE, "sigint signal captured, exiting cleanly");

	/* clean up memory and exit*/
	for (i=0; i<list->num; i++) free(camArray[i]);

	free(camArray);
	dc1394_camera_free_list (list);
	if(useDB) cleanUpDB(conn, night);
	multicast_clean_up(&statMC);

	return 0;
}

void finishUp(){
/* This function will be set as the SIGINT signal handler
   It simply breaks the main loop so that we can exit cleanly */
	keepGoing = 0;
}

void getTime(char *s1, char *s2){
/* This function returns the a string formatted with the date and time
   in the style YYMMDD-HHMMSS */
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
		printArgsError();
	}

	/* set to NULL, so we can apply defaults if needed later */
	*flNum = *confFile = *filePrefix = *dbHost = NULL;
	*getFNfromDB = 0;

	while (i<argc) {
//		printf("arg[%d]: %s\n", i, argv[i]);
		if (*argv[i] == '-'){
			opt = *(argv[i]+1);
			i++;
//			printf("option found: %c\n", opt);
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
					printArgsError();
			}
		} else if (i>0) *flNum = argv[i];
		i++;
	}
	
	/* if the params were not set, use the default vals */
	if (*flNum == NULL && !(*getFNfromDB)) printArgsError();
	if (*confFile == NULL) defaults(confFile, CONFIG_FILE);
	if (*filePrefix == NULL) defaults(filePrefix, FILE_PREFIX);
	if (*dbHost == NULL) defaults(dbHost, DB_HOST);

//	printf("conf: %s, prefix: %s, host: %s, flNum: %s\n", confFile, filePrefix, dbHost, flNum);
}

char *defaults(char **arg, char *value){
	/* allocates memory for the value, and copies the string into the new location*/
//	printf("size of: '%s' is %d\n", value, sizeof(char)*strlen(value));
	*arg = malloc(sizeof(char)*strlen(value)+1);
	strcpy(*arg, value);
	return *arg;
}

void printArgsError(){
	/* this function is called when there is impropper input on the
	   command line. It displays some help for the user and _exits */
	printf("improper usage - use format:\n");
	printf("capture [-c <configFile>] [-f <file prefix>] [-d <db host>] <flightnumber>\n");
	printf("\tNOTE: you must specify a flight number or use -d to get from database\n\n");
	exit(1);

}
char *trimWhiteSpace(char *input) {
	/* this function sets the end of a string at the first instance of two spaces
	   it is useful for trimming whitespace at the end of a string:
		i.e 'hello Tom \n and Jerry' => 'hello Tom'
			'rf09	   '	 => 'rf09' 
	*/

	int i=0;
	char c, d;
	while (c = input[i]){
		if ((c == ' ' || c == '\n') && (d == ' ' || d == '\n')){
			input[i-1] = '\0';
			return input;
		}
		d = c;
		i++;
	}
	return input;
}

