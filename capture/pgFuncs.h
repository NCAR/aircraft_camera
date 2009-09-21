#ifndef PGFUNCS_H
#define PGFUNCS_H

#include <libpq-fe.h>	//for postgres
#include <dc1394/dc1394.h>
#include <stdlib.h>	
#include <syslog.h>	
#include "parseFile.h"

int connectDB(PGconn **conn, char* dbHost, int getFNfromDB);
void cleanUpDB(PGconn*, int);
int initPostgres(PGconn*, camConf_t**, int);
int updatePostgres(PGconn*, const char*, int); 
int getDbFlNum(PGconn*, char**);

#endif
