#include "pgFuncs.h"	
extern int interrupted;

int connectDB(PGconn **conn, char* dbHost, int getFNfromDB) {
/* function takes a pointer to posgres connection pointer and creates the connection
	using PQconnect functions */

	char dbConnectString[100];
	int useDB = 1;
	sprintf(dbConnectString, "host=%s dbname=real-time user=ads", dbHost);
	*conn = PQconnectdb(dbConnectString);
	if (PQstatus(*conn) != CONNECTION_OK) {
		syslog(LOG_ERR, "Connection to database failed: %s",
				 PQerrorMessage(*conn));
		PQfinish(*conn);
		if(getFNfromDB){
			printf("could not connect to specified DB.\n Must specifiy either flight number or valid real-time db host.\n");
			syslog(LOG_ERR, "could not connect to database host: %s", dbHost);
			exit(1);
		} else {
			syslog(LOG_NOTICE, "no database connection, continuing without database updates");
			useDB = 0;
		}
	} else syslog(LOG_NOTICE, "connected to database");

	if (useDB) initTable(*conn);
	return useDB;
}


void cleanUpDB(PGconn *conn){
/* This function ends the postgres connection after updating the status one last time */

	PGresult *res;

	if (interrupted) {
		res = PQexec(conn, "UPDATE camera SET status=0, message='Recording Stopped'");
		if (PQresultStatus(res) != PGRES_COMMAND_OK) {
			syslog(LOG_ERR, "update table failed: %s", PQerrorMessage(conn));
		} 

	} else {
		res = PQexec(conn,
		"UPDATE camera SET status = 0, message = 'Night Detected, Recording Stopped'");
		if (PQresultStatus(res) != PGRES_COMMAND_OK) {
			syslog(LOG_ERR, "update table failed: %s", PQerrorMessage(conn));
		}
	}

	PQclear(res);
	PQfinish(conn);
}

int initTable(PGconn * conn){
/* This function clears the old camera table and sets up a new one */

	char command[500]; 
	PGresult *res;
	
	/* drop existing camera table */
	res = PQexec(conn, "DROP TABLE camera");
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		syslog(LOG_ERR, "drop table failed: %s", PQerrorMessage(conn));
		PQclear(res);
	} //else syslog(LOG_NOTICE, "old camera table dropped"); 

	/* create new camera table */
	res = PQexec(conn,
	"CREATE TABLE camera ( last_update timestamp with time zone, status integer, message text, guid text[], direction text[], latest text[])");
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		syslog(LOG_ERR, "create table failed: %s", PQerrorMessage(conn));
		PQclear(res);
	} //else syslog(LOG_NOTICE, "new camera table created");

	/* create single row, cameras will be added as array elements */
	sprintf(command, 
		"INSERT INTO camera VALUES (current_timestamp, 0, 'Waiting for Camera', '{0}', '{0}', '{0}')"
		);
	res = PQexec(conn, command);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		syslog(LOG_ERR, "create row failed: %s", PQerrorMessage(conn));
		PQclear(res);
	} //else syslog(LOG_NOTICE, "added row to table");
	return 1;
}

int initRow(PGconn * conn, camConf_t **camArray, int numCams){
/* this function sets the values of the camera table */
	int i;
	char command[500]; 
	PGresult *res;

	sprintf(command, 
		"UPDATE camera SET status=1,message='Recording'"
		);
	res = PQexec(conn, command);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		syslog(LOG_ERR, "update status failed: %s", PQerrorMessage(conn));
		PQclear(res);
	} 

	/* fill in array with camera data */
	for (i=0; i<numCams; i++) {
		sprintf(command, "UPDATE camera SET guid[%d]='%llx', direction[%d]='%s'"
			, i+1, camArray[i]->guid, i+1, camArray[i]->direction);
		res = PQexec(conn, command);
		if (PQresultStatus(res) != PGRES_COMMAND_OK) {
			syslog(LOG_ERR, "update camera failed: %s", PQerrorMessage(conn));
			PQclear(res);
		} else syslog(LOG_NOTICE, "added camera: %llx to db", camArray[i]->guid); 
	}

	PQclear(res);
	return 1;
}

/* This function updates the database with the newest image, etc
 */
int updatePostgres(PGconn *conn, const char * img_name, int camNum)
{

	char command[100];
	PGresult *res;

	sprintf(command, "UPDATE camera SET latest[%d] ='%s',last_update=current_timestamp",
			 camNum+1, img_name);

	while ( (res = PQgetResult(conn)) )
	{
		if (PQresultStatus(res) != PGRES_COMMAND_OK) {
			syslog(LOG_ERR, "update table failed: %s", PQerrorMessage(conn));
		}
		PQclear(res);
	}

	res = PQsendQuery(conn, command);
	
	while ( (res = PQgetResult(conn)) )
	{
		if (PQresultStatus(res) != PGRES_COMMAND_OK) {
			syslog(LOG_ERR, "update table failed: %s", PQerrorMessage(conn));
		}
		PQclear(res);
	}

	return 1;
}

int getDbFlNum(PGconn *conn, char **flNum) {
/* This function copies the flight number into the string that **flNum points to */

	PGresult *res;
	res = PQexec(conn, "SELECT value FROM global_attributes WHERE key='FlightNumber'");
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		syslog(LOG_ERR, "could not get flight number from DB: %s", PQerrorMessage(conn));
		exit(1);
	} 
	else defaults(flNum,trimWhiteSpace(PQgetvalue(res, 0, 0)));
	return 1;
}

char *trimWhiteSpace(char *input) {
	/* this function sets the end of a string at the first instance of two spaces
	   it is useful for trimming whitespace at the end of a string:
		i.e 'hello Tom \n and Jerry' => 'hello Tom'
			'rf09	   '	 => 'rf09' 
	*/

	int i=0;
	char c, d;
	while ( (c = input[i]) )
	{
		if ((c == ' ' || c == '\n') && (d == ' ' || d == '\n')){
			input[i-1] = '\0';
			return input;
		}
		d = c;
		i++;
	}
	return input;
}

