/*
 * The purpose of this program is to collect and archive images from 
 * IEEE1394 scientific cameras.
 */

#include <libpq-fe.h>	//for postgres
#include <stdio.h>
#include <time.h> 		//for timestamp
#include <stdint.h>
#include <unistd.h> 	//for sleep, fork
#include <sys/wait.h> 	//for waitpid
#include <signal.h> 	//for catching kill signals
#include <string.h>
#include <dc1394/dc1394.h>
#include <stdlib.h>	
#include <inttypes.h>
#include "getIMG.h"

/* default settings - if not specified by user */
#define CONFIG_FILE "/etc/capture.conf"
#define FILE_PREFIX "/scr/rafcam/flight_number_"
#define DB_HOST "acserver"

void finishUp();
void cleanUpDB(PGconn *, int);
void getTime(char*);
int initPostgres(PGconn *, camConf_t **, int);
int updatePostgres(PGconn *, const char *, int); 
int getDbFlNum(PGconn *conn, char **flNum);
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
    char image_file_name[100], directory[100], timeStr[20], dbConnectString[100];

	PGconn *conn; 					//postgresql database connection
	pid_t cpid;						//PID for timing process
	dc1394_t * d;					//dc1394 base class (for scanning the bus)
	dc1394camera_t * resCam;		//dc1394 camera class, used here for reset
    dc1394camera_list_t * list; 	
    dc1394error_t err;			
	
	char *conf, *prefix, *dbHost, *flNum;

	/* get input args */
	parseInputLine(argc, argv, &conf, &prefix, &dbHost, &flNum, &getFNfromDB);
	printf("conf: %s, prefix: %s, host: %s\n", conf, prefix, dbHost);

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
	camConf_t **camArray;
	camArray = malloc(sizeof(camConf_t*)*list->num); 

	/* connect to postgres db */
	sprintf(dbConnectString, "host=%s dbname=real-time user=ads", dbHost);
	conn = PQconnectdb(dbConnectString);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s",
				 PQerrorMessage(conn));
    	PQfinish(conn);
		if(getFNfromDB){
			printf("could not connect to specified DB.\n Must specifiy either flight number or valid real-time db host.\n");
			exit(1);
		} else {
			printf("continuing without DB updates\n");
		    useDB = 0;
		}
    } else printf("connected to db\n");

	if(getFNfromDB) getDbFlNum(conn, &flNum);
	printf("flnum: '%s'\n", flNum);

	/* fill structs with data from config file */
	for (i=0; i<list->num; i++){
		camArray[i] = malloc(sizeof(camConf_t)); //allocate config structures
		if (!getConf(conf, list->ids[i].guid, camArray[i])){
			printf("Warning: no settings for camera: %llx in file: %s,using defaults.\n",
					list->ids[i].guid, conf); 
		}
		strcpy(camArray[i]->flNum, flNum); 
	}

	/* set up postgres db */
	if(useDB) initPostgres(conn, camArray, list->num);

	/* make image directory structure */
	sprintf(directory, "mkdir %s%s &> /dev/null",prefix, flNum); 
	system(directory);
	for (i=0; i<list->num; i++){
		sprintf(directory, "mkdir %s%s/%s &> /dev/null", prefix, 
				flNum, camArray[i]->direction); 
		system(directory);
	}

	/* set up handler for SIGINT (ctrl-c) signal */
	signal(SIGINT, &finishUp);

	/* keep getting pictures until SIGINT signal or night detection */
	while(keepGoing && night<10){

		/* spawn child process that waits one second, then die */
		if( !(cpid = fork()) ) {
			sleep(1);
			exit(0);
		}

		/* main process continues here, where it will
		   get image(s) and then wait for child process to die */
		getTime(timeStr);
		for (i=0; i<list->num; i++){
			sprintf(image_file_name, "%s%s/%s/%s", prefix, flNum, 
					camArray[i]->direction, timeStr); 
			night += getIMG(image_file_name, camArray[i], d);
			if(useDB) updatePostgres(conn, timeStr, i);
		}
//		printf("night: %d\n",  night);
		waitpid(cpid, NULL, 0);
	}

	/* Will get here after SIGINT signal, or night detection
	   clean up memory and exit*/
	if (night>10) printf ("night detected, shutting down\n");
	else printf("sigINT signal captured, exiting cleanly\n");

	for (i=0; i<list->num; i++)
		free(camArray[i]);
	free(camArray);
	dc1394_camera_free_list (list);
	if(useDB) cleanUpDB(conn, night);

    return 0;
}

void finishUp(){
/* This function will be set as the SIGINT signal handler
   It simply breaks the main loop so that we can exit cleanly */
	keepGoing = 0;
}

void cleanUpDB(PGconn *conn, int night){

	PGresult *res;

	res = PQexec(conn, "UPDATE camera SET status=0, message='Recording Stopped'");
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "update table failed: %s", PQerrorMessage(conn));
	} //else printf("table updated\n"); 

	if (night >= 10) {
		res = PQexec(conn,
		"UPDATE camera SET status = 0, message = 'Night Detected, Not Recording'");
		if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    	    fprintf(stderr, "update table failed: %s", PQerrorMessage(conn));
        	PQclear(res);
		}
	}

	PQclear(res);
	PQfinish(conn);
}

void getTime(char* s){
/* This function returns the a string formatted with the date and time
   in the style YYMMDD-HHMMSS */
	struct tm *t;
	time_t rawtime;

	time(&rawtime);
//	t = localtime(&rawtime);
	t = gmtime(&rawtime);

	/* make time string; note that tm_mon is 0-11, not 1-12 */
	sprintf(s, "%2.2d%2.2d%2.2d-%2.2d%2.2d%2.2d", t->tm_year%100, t->tm_mon+1,
					 t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

	return;
}

int initPostgres(PGconn * conn, camConf_t **camArray, int numCams){
/* This function clears the old camera table and sets up a new one */

	int i;
	char command[500]; 
	char hostname[100];
	PGresult *res;
	
	/* get the local host name */
	gethostname(hostname, 100);

	/* drop existing camera table */
	res = PQexec(conn, "DROP TABLE camera");
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "drop table failed: %s", PQerrorMessage(conn));
        PQclear(res);
	} else printf("old table dropped\n");

	/* create new camera table */
	res = PQexec(conn,
	"CREATE TABLE camera ( cam_host text, status integer, message text, guid text[], direction text[], latest text[])");
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "create table failed: %s", PQerrorMessage(conn));
        PQclear(res);
	} else printf("new table created\n");

	/* create single row, cameras will be added as array elements */
	sprintf(command, 
		"INSERT INTO camera VALUES ('%s', 1, 'Recording Images', '{0}', '{0}', '{0}')",
		hostname);
	res = PQexec(conn, command);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "create row failed: %s", PQerrorMessage(conn));
        PQclear(res);
	} else printf("new row added\n");

	/* fill in array with camera data */
	for (i=0; i<numCams; i++) {
		sprintf(command, "UPDATE camera SET guid[%d]='%llx', direction[%d]='%s'"
			, i+1, camArray[i]->guid, i+1, camArray[i]->direction);
		res = PQexec(conn, command);
		if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    	    fprintf(stderr, "add row failed: %s", PQerrorMessage(conn));
        	PQclear(res);
		} else printf("added camera: %llx to db\n", camArray[i]->guid);
	}

    PQclear(res);
	return 1;
}

int updatePostgres(PGconn *conn, const char * img_name, int camNum) {
/* This function updates the database with the newest image, etc */

	char command[100];
	PGresult *res;

	sprintf(command, "UPDATE camera SET latest[%d] ='%s'",
			 camNum+1, img_name);
	res = PQexec(conn, command);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "update table failed: %s", PQerrorMessage(conn));
        PQclear(res);
	} //else printf("table updated\n"); 
	
    PQclear(res);
	return 1;
}

int getDbFlNum(PGconn *conn, char **flNum) {
	PGresult *res;
	res = PQexec(conn, "SELECT value FROM global_attributes WHERE key='FlightNumber'");
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "could not get flight number from DB: %s", PQerrorMessage(conn));
		exit(1);
	} 
	else defaults(flNum,trimWhiteSpace(PQgetvalue(res, 0, 0)));
	return 1;
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
            'rf09       '     => 'rf09' 
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

