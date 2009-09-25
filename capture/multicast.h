#ifndef MULTICAST_H
#define MULTICAST_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syslog.h>
#include "parseFile.h"

#define ROW_BASE "<td>%s</td><td>%llx</td>"

#define TABLE_BASE "\
<table>\
<thead><tr><th>Direction</th><th>GUID</th><th>Last Image</th><th>Size</th><th>Average</th></tr></thead>\
<tbody>\
%s\
</tbody>\
</table>"

#define PACKET_BASE "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<group>\
<name>Cameras</name>\
<clock>%s</clock>\
<frequency>1</frequency>\
<health>%d</health>\
<status><![CDATA[<html>%s</html>]]></status>\
</group>"

/*struct to hold information for xml packet*/
typedef struct {
	int health;				//xml health field
	int numCams;			//number of cameras on bus
	int *lastSize;			//size of last fileset written
	float *runningAvg;		//running avg size for each camera
	char clock[21];			//xml clock field
	char *html;				//xml html field
	char **cam_row;			//html rows for each camera (created once)
	char **latest;			//latest filename for each camera
} status_t;

void multicast_status_init(status_t *status, camConf_t **camArray, int numCams);
void multicast_clean_up(status_t *status);
int multicast_send_status(status_t *status);
char *multicast_make_packet(status_t *status);
void multicast_send_packet(char *packet, char *group, int port);

#endif
